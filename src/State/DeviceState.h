#pragma once
#include "../Decoder/Decoder.h"
#include <chrono>
#include <string>

class DeviceState {
public:
    DeviceState();

    // Core Updates
    // Returns true if UI needs update
    bool update_from_packet(const BatteryData &data, const std::string &path);
    void set_connected(bool is_connected);
    void set_adapter_powered(bool is_on);

    // Pairing Logic
    void set_pairing_available(bool available, const std::string &mac = "");

    // Output
    void print_json();
    bool is_connected() const { return connected; }

private:
    void save_mac(const std::string &path_suffix);
    std::string load_mac();
    bool is_stale() const;

    // Data
    BatteryData bat;
    bool connected = false;
    bool adapter_powered = false;

    // Pairing State
    bool pairing_available = false;
    std::string pairing_mac; // MAC of the device wanting to pair

    // Timing
    std::chrono::steady_clock::time_point last_seen;
    bool was_visible = false;
};
