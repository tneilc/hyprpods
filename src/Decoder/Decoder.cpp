#include "Decoder.h"

// Known Headers for AirPods Battery Packets
constexpr std::uint8_t HEADER_FLIP = 0x07; // Standard (Gen 1/2)
constexpr std::uint8_t HEADER_PRO = 0x19;  // Pro / Gen 3

std::optional<BatteryData> Decoder::parse(const std::vector<std::uint8_t> &data) {
    if (data.size() < 8)
        return std::nullopt;

    std::uint8_t header = data[0];
    if (header != HEADER_FLIP && header != HEADER_PRO) {
        return std::nullopt;
    }

    BatteryData out;
    out.in_pairing_mode = false;

    // --- PAIRING MODE DETECTION ---
    if (data.size() > 2) {
        if (data[2] == 0x07) {
            out.in_pairing_mode = true;
        }
    }
    // std::uint8_t status_byte = data[5];
    // int primary_pod = (status_byte >> 5) & 1; //1 = Left, 0 = Right

    // This decodes bytes as packed 4-bit values (0-10 -> 0-100%).
    // [6] = Left (high nibble) | Right (low nibble)
    // [7] = Case (low nibble) | Charging (high nibble)

    std::uint8_t byte_lr = data[6];
    std::uint8_t byte_case = data[7];

    int raw_left = (byte_lr >> 4) & 0x0F;
    int raw_right = byte_lr & 0x0F;
    int raw_case = byte_case & 0x0F;
    int raw_chg = (byte_case >> 4) & 0x0F;

    auto map_val = [](int raw) -> int {
        if (raw <= 10)
            return raw * 10;
        return -1;
    };

    out.left = map_val(raw_left);
    out.right = map_val(raw_right);
    out.case_val = map_val(raw_case);

    // Charging logic for legacy: bitmask on the high nibble of byte 7
    // Bit 0 = Right, Bit 1 = Left, Bit 2 = Case
    out.charging = (raw_chg & 0b0100) != 0;

    if (out.left == -1 && out.right == -1 && out.case_val == -1) {
        // If pairing mode is detected but no valid battery data,
        // return the struct so UI can show "Click to Pair"
        if (out.in_pairing_mode)
            return out;
        return std::nullopt;
    }

    return out;
}
