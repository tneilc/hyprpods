#pragma once
#include <cstdint>
#include <sdbus-c++/sdbus-c++.h>
#include <vector>

namespace Utils {
// Helper to safely extract a byte vector from a generic sdbus Variant.
// Manufacturer data is often wrapped in a Variant containing an array of bytes.
inline std::vector<uint8_t> to_byte_vector(const sdbus::Variant &variant) {
    // sdbus::Variant doesn't have isValid(). We use isEmpty() or just rely on the try-catch.
    if (variant.isEmpty())
        return {};

    try {
        return variant.get<std::vector<uint8_t>>();
    } catch (...) {
        // Return empty if type mismatch occurs
        return {};
    }
}
} // namespace Utils
