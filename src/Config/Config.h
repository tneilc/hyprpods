#pragma once
#include <cstdint>
#include <string>

namespace Config {
// Apple identifier (Bluetooth SIG)
constexpr std::uint16_t APPLE_CID = 76; // 0x004C

// File to save the recognized device MAC address
const std::string MAC_FILE = "/tmp/hyprpods_last_mac";

// How long (seconds) to keep the widget shown after closing lid
constexpr int TIMEOUT_SECONDS = 2;

// Set to TRUE to see raw packet logs in terminal
constexpr bool DEBUG_MODE = true;
} // namespace Config
