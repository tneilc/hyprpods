#pragma once

#include "../State/DeviceState.h"
#include <memory>
#include <sdbus-c++/sdbus-c++.h>
#include <string>

class BluezClient {
public:
    BluezClient();
    ~BluezClient();

    void run();

    static void trigger_pairing();

private:
    void init_connection();
    void find_adapter();
    void start_scanning();
    void setup_signal_handler();

    std::unique_ptr<sdbus::IConnection> connection;

    sdbus::Slot property_match_slot;

    DeviceState state;
    std::string adapter_path;
};
