#include "DeviceState.h"
#include "../Config/Config.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

DeviceState::DeviceState() {}

void DeviceState::save_mac(const std::string &path) {
    size_t pos = path.find("dev_");
    if (pos == std::string::npos)
        return;

    std::string suffix = path.substr(pos + 4);
    std::replace(suffix.begin(), suffix.end(), '_', ':');

    std::ofstream out(Config::MAC_FILE);
    if (out.good())
        out << suffix;
}

bool DeviceState::update_from_packet(const BatteryData &data, const std::string &path) {
    // If connected, are using the device.
    // Suppress "Click to Pair" to prevent UI annoyance, just show battery.
    if (connected) {
        bat = data;
        bat.in_pairing_mode = false;
        last_seen = std::chrono::steady_clock::now();
        return true;
    }

    // If NOT connected, and the packet indicates Pairing Mode (Byte 2 == 0x01),
    // prioritize showing the Pairing prompt.
    if (data.in_pairing_mode) {
        set_pairing_available(true, path);
        return true;
    }

    // Otherwise, just show passive battery stats (Case Open)
    pairing_available = false;
    bat = data;
    last_seen = std::chrono::steady_clock::now();
    return true;
}

void DeviceState::set_connected(bool is_connected) {
    if (connected != is_connected) {
        connected = is_connected;
        if (connected) {
            pairing_available = false;
            last_seen = std::chrono::steady_clock::now();
        }
    }
}

void DeviceState::set_adapter_powered(bool is_on) { adapter_powered = is_on; }

void DeviceState::set_pairing_available(bool available, const std::string &path) {
    if (available && !path.empty()) {
        save_mac(path);
    }
    pairing_available = available;
    last_seen = std::chrono::steady_clock::now();
}

bool DeviceState::is_stale() const {
    if (!adapter_powered)
        return true;
    if (connected)
        return false;

    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - last_seen).count();
    return diff > Config::TIMEOUT_SECONDS;
}

void DeviceState::print_json() {
    if (is_stale()) {
        if (was_visible) {
            std::cout << "{\"text\": \"\"}" << std::endl;
            was_visible = false;
        }
        return;
    }

    was_visible = true;
    std::stringstream ss;
    ss << "{";

    if (pairing_available && !connected) {
        ss << "\"text\": \" Click to Pair\",";
        ss << "\"class\": \"pairing\",";
        ss << "\"tooltip\": \"AirPods in pairing mode detected.\\nClick to connect.\"";
    } else {
        ss << "\"text\": \"";
        if (bat.charging)
            ss << " ";
        else
            ss << " ";

        ss << "L:" << (bat.left >= 0 ? std::to_string(bat.left) + "%" : "--") << " ";
        ss << "R:" << (bat.right >= 0 ? std::to_string(bat.right) + "%" : "--") << " ";
        if (bat.case_val >= 0)
            ss << "C:" << bat.case_val << "%";
        ss << "\",";

        ss << "\"tooltip\": \"";
        ss << "Left: " << (bat.left >= 0 ? std::to_string(bat.left) + "%" : "--") << "\\n";
        ss << "Right: " << (bat.right >= 0 ? std::to_string(bat.right) + "%" : "--") << "\\n";
        ss << "Case: " << (bat.case_val >= 0 ? std::to_string(bat.case_val) + "%" : "--");
        ss << "\",";

        ss << "\"class\": \"" << (connected ? "connected" : "discovered") << "\"";
    }

    ss << "}";
    std::cout << ss.str() << std::endl;
}
