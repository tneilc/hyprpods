#pragma once
#include <cstdint>
#include <optional>
#include <vector>

struct BatteryData {
    int left = -1;
    int right = -1;
    int case_val = -1;
    bool charging = false;
    bool in_pairing_mode = false;
};

class Decoder {
public:
    // Returns std::nullopt if the packet is not a valid status packet.
    static std::optional<BatteryData> parse(const std::vector<std::uint8_t> &data);
};
