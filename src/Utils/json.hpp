#pragma once

#include <algorithm>
#include <charconv> // For efficient number parsing
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Json {

struct Value;

using Null = std::monostate;
using Bool = bool;
using Number = double;
using String = std::string;
using Array = std::vector<Value>;
using Object = std::map<std::string, Value>;

struct Value {
    std::variant<Null, Bool, Number, String, Array, Object> data;

    Value() : data(Null{}) {}
    Value(bool b) : data(b) {}
    Value(double d) : data(d) {}
    Value(int i) : data(static_cast<double>(i)) {}
    Value(const std::string &s) : data(s) {}
    Value(const char *s) : data(String(s)) {}
    Value(const Array &a) : data(a) {}
    Value(const Object &o) : data(o) {}

    bool is_null() const { return std::holds_alternative<Null>(data); }
    bool is_bool() const { return std::holds_alternative<Bool>(data); }
    bool is_number() const { return std::holds_alternative<Number>(data); }
    bool is_string() const { return std::holds_alternative<String>(data); }
    bool is_array() const { return std::holds_alternative<Array>(data); }
    bool is_object() const { return std::holds_alternative<Object>(data); }

    Value &operator[](const std::string &key) {
        if (std::holds_alternative<Null>(data)) {
            data = Object{};
        }

        if (!std::holds_alternative<Object>(data)) {
            throw std::runtime_error("Type is not an Object, cannot access key: " + key);
        }

        return std::get<Object>(data)[key];
    }

    std::string dump() const {
        return std::visit(
            [](auto &&arg) -> std::string {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Null>)
                    return "null";
                else if constexpr (std::is_same_v<T, Bool>)
                    return arg ? "true" : "false";
                else if constexpr (std::is_same_v<T, Number>)
                    return std::to_string(arg);
                else if constexpr (std::is_same_v<T, String>)
                    return "\"" + arg + "\"";
                else if constexpr (std::is_same_v<T, Array>) {
                    std::string s = "[";
                    for (size_t i = 0; i < arg.size(); ++i) {
                        s += arg[i].dump();
                        if (i < arg.size() - 1)
                            s += ", ";
                    }
                    s += "]";
                    return s;
                } else if constexpr (std::is_same_v<T, Object>) {
                    std::string s = "{";
                    auto it = arg.begin();
                    while (it != arg.end()) {
                        s += "\"" + it->first + "\": " + it->second.dump();
                        if (++it != arg.end())
                            s += ", ";
                    }
                    s += "}";
                    return s;
                }
                return "";
            },
            data);
    }
};

enum class TokenType {
    String,
    Number,
    True,
    False,
    Null,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Colon,
    Comma,
    EndOfFile,
    Error
};

struct Token {
    TokenType type;
    std::string_view value;
};

class Scanner {
    std::string_view input;
    size_t cursor = 0;

public:
    Scanner(std::string_view in) : input(in) {}

    Token next() {
        skipWhitespace();
        if (cursor >= input.size())
            return {TokenType::EndOfFile, ""};

        char c = input[cursor];

        if (c == '{')
            return consume(TokenType::LBrace, 1);
        if (c == '}')
            return consume(TokenType::RBrace, 1);
        if (c == '[')
            return consume(TokenType::LBracket, 1);
        if (c == ']')
            return consume(TokenType::RBracket, 1);
        if (c == ':')
            return consume(TokenType::Colon, 1);
        if (c == ',')
            return consume(TokenType::Comma, 1);

        if (c == '"')
            return scanString();

        if (isdigit(c) || c == '-')
            return scanNumber();

        if (c == 't')
            return scanLiteral("true", TokenType::True);
        if (c == 'f')
            return scanLiteral("false", TokenType::False);
        if (c == 'n')
            return scanLiteral("null", TokenType::Null);

        return {TokenType::Error, ""};
    }

private:
    Token consume(TokenType type, size_t len) {
        std::string_view val = input.substr(cursor, len);
        cursor += len;
        return {type, val};
    }

    void skipWhitespace() {
        while (cursor < input.size() && isspace(input[cursor]))
            cursor++;
    }

    Token scanString() {
        size_t start = cursor;
        cursor++; // Skip open quote
        while (cursor < input.size()) {
            if (input[cursor] == '"' && input[cursor - 1] != '\\') {
                cursor++; // Skip close quote
                return {TokenType::String, input.substr(start, cursor - start)};
            }
            cursor++;
        }
        return {TokenType::Error, "Unterminated String"};
    }

    Token scanNumber() {
        size_t start = cursor;
        if (cursor < input.size() && input[cursor] == '-')
            cursor++;
        while (cursor < input.size() && isdigit(input[cursor]))
            cursor++;
        if (cursor < input.size() && input[cursor] == '.') {
            cursor++;
            while (cursor < input.size() && isdigit(input[cursor]))
                cursor++;
        }
        if (cursor < input.size() && (input[cursor] == 'e' || input[cursor] == 'E')) {
            cursor++;
            if (cursor < input.size() && (input[cursor] == '+' || input[cursor] == '-'))
                cursor++;
            while (cursor < input.size() && isdigit(input[cursor]))
                cursor++;
        }
        return {TokenType::Number, input.substr(start, cursor - start)};
    }

    Token scanLiteral(const char *lit, TokenType type) {
        size_t len = std::string_view(lit).size();
        if (input.substr(cursor, len) == lit)
            return consume(type, len);
        return {TokenType::Error, "Unknown Literal"};
    }
};

class Parser {
    Scanner scanner;
    Token currentToken;

public:
    Parser(std::string_view json) : scanner(json) {
        currentToken = scanner.next(); // Prime the pump
    }

    static Value parse(std::string_view json) {
        Parser p(json);
        return p.parseValue();
    }

private:
    void advance() { currentToken = scanner.next(); }

    void expect(TokenType type) {
        if (currentToken.type != type) {
            throw std::runtime_error("Unexpected token: " + std::string(currentToken.value));
        }
        advance();
    }

    Value parseValue() {
        switch (currentToken.type) {
        case TokenType::Null:
            advance();
            return Value(); // Null
        case TokenType::True:
            advance();
            return Value(true);
        case TokenType::False:
            advance();
            return Value(false);
        case TokenType::Number: {
            std::string numStr(currentToken.value);
            advance();
            return std::stod(numStr);
        }
        case TokenType::String: {
            std::string raw = std::string(currentToken.value);
            advance();
            return raw.substr(1, raw.size() - 2);
        }
        case TokenType::LBracket:
            return parseArray();
        case TokenType::LBrace:
            return parseObject();
        default:
            throw std::runtime_error("Invalid Value Token");
        }
    }

    Value parseArray() {
        Array arr;
        advance();

        if (currentToken.type == TokenType::RBracket) {
            advance();
            return arr;
        }

        while (true) {
            arr.push_back(parseValue());

            if (currentToken.type == TokenType::RBracket) {
                advance();
                break;
            }
            expect(TokenType::Comma);
        }
        return arr;
    }

    Value parseObject() {
        Object obj;
        advance();

        if (currentToken.type == TokenType::RBrace) {
            advance();
            return obj;
        }

        while (true) {
            if (currentToken.type != TokenType::String)
                throw std::runtime_error("Key must be string");

            std::string keyRaw = std::string(currentToken.value);
            std::string key = keyRaw.substr(1, keyRaw.size() - 2);
            advance();

            expect(TokenType::Colon);

            obj[key] = parseValue();

            if (currentToken.type == TokenType::RBrace) {
                advance();
                break;
            }
            expect(TokenType::Comma);
        }
        return obj;
    }
};

} // namespace Json
