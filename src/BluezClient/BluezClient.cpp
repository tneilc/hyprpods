#include "BluezClient.h"
#include "../Config/Config.h"
#include "../Decoder/Decoder.h"
#include "../Utils/Utils.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>

// DBus Constants
static const sdbus::ServiceName BLUEZ_SERVICE{"org.bluez"};
static const sdbus::InterfaceName ADAPTER_IFACE{"org.bluez.Adapter1"};
static const sdbus::InterfaceName DEVICE_IFACE{"org.bluez.Device1"};
static const sdbus::InterfaceName PROP_IFACE{"org.freedesktop.DBus.Properties"};
static const sdbus::InterfaceName MGR_IFACE{"org.freedesktop.DBus.ObjectManager"};

// Helper to reduce boilerplate and repeated object construction
static std::unique_ptr<sdbus::IProxy> createBluezProxy(sdbus::IConnection &conn,
                                                       const std::string &path) {
    return sdbus::createProxy(conn, BLUEZ_SERVICE, sdbus::ObjectPath(path));
}

BluezClient::BluezClient() {}

BluezClient::~BluezClient() {
    // Attempt to stop discovery on exit
    if (connection && !adapter_path.empty()) {
        try {
            auto proxy = createBluezProxy(*connection, adapter_path);
            proxy->callMethod("StopDiscovery").onInterface(ADAPTER_IFACE);
        } catch (...) {
        }
    }
}

void BluezClient::run() {
    // Initial JSON output to prevent Waybar error
    state.print_json(true);

    init_connection();
    find_adapter();

    if (adapter_path.empty()) {
        std::cerr << "Error: No Bluetooth Adapter found." << std::endl;
        return;
    }

    state.set_adapter_powered(true);
    setup_signal_handler();
    start_scanning();

    // Enters the blocking event loop provided by sdbus-c++
    connection->enterEventLoop();
}

void BluezClient::init_connection() { connection = sdbus::createSystemBusConnection(); }

void BluezClient::find_adapter() {
    // Try to find the adapter up to i times
    for (int i = 0; i < 5; i++) {
        try {
            auto proxy = createBluezProxy(*connection, "/");

            // Map: ObjectPath -> InterfaceName -> PropertyName -> Variant
            std::map<sdbus::ObjectPath,
                     std::map<std::string, std::map<std::string, sdbus::Variant>>>
                objects;

            proxy->callMethod("GetManagedObjects").onInterface(MGR_IFACE).storeResultsTo(objects);

            for (const auto &[path, interfaces] : objects) {
                if (interfaces.count(ADAPTER_IFACE)) {
                    adapter_path = path;
                    if (Config::DEBUG_MODE)
                        std::cerr << "DEBUG: Found Adapter at " << adapter_path << std::endl;
                    return;
                }
            }
        } catch (const sdbus::Error &e) {
            if (Config::DEBUG_MODE)
                std::cerr << "DBus Error finding adapter: " << e.getMessage() << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void BluezClient::start_scanning() {
    if (adapter_path.empty())
        return;

    try {
        auto proxy = createBluezProxy(*connection, adapter_path);

        // Build Discovery Filter
        std::map<std::string, sdbus::Variant> filter;

        filter["Transport"] = sdbus::Variant(std::string("le"));
        filter["DuplicateData"] = sdbus::Variant(true);

        proxy->callMethod("SetDiscoveryFilter").onInterface(ADAPTER_IFACE).withArguments(filter);
        proxy->callMethod("StartDiscovery").onInterface(ADAPTER_IFACE);

    } catch (const sdbus::Error &e) {
        std::cerr << "Failed to start scanning: " << e.getMessage() << std::endl;
    }
}

void BluezClient::setup_signal_handler() {
    const std::string matchRule = "type='signal',interface='" + PROP_IFACE + "'";

    property_match_slot = connection->addMatch(
        matchRule,
        [this](sdbus::Message msg) {
            std::string iface;
            std::map<std::string, sdbus::Variant> changed;
            std::vector<std::string> invalidated;

            std::string obj_path = msg.getPath();
            try {
                msg >> iface >> changed >> invalidated;
            } catch (const sdbus::Error &e) {
                if (Config::DEBUG_MODE)
                    std::cerr << "Error parsing signal: " << e.getMessage() << std::endl;
                return;
            }

            if (iface != DEVICE_IFACE)
                return;

            // 1. Connection State
            if (auto it = changed.find("Connected"); it != changed.end()) {
                bool is_conn = it->second.get<bool>();
                state.set_connected(is_conn);
                if (Config::DEBUG_MODE)
                    std::cerr << "DEBUG: Connection State Changed: " << is_conn << std::endl;
                state.print_json();
            }

            // 2. Manufacturer Data (BLE Packets)
            if (auto it = changed.find("ManufacturerData"); it != changed.end()) {
                auto mfg_data = it->second.get<std::map<uint16_t, sdbus::Variant>>();

                if (auto mfg_it = mfg_data.find(Config::APPLE_CID); mfg_it != mfg_data.end()) {
                    auto bytes = Utils::to_byte_vector(mfg_it->second);
                    auto result = Decoder::parse(bytes);

                    if (result) {
                        if (state.update_from_packet(*result, obj_path)) {
                            state.print_json();
                        }
                    }
                }
            }
        },
        sdbus::return_slot);
}

void BluezClient::trigger_pairing() {
    auto conn = sdbus::createSystemBusConnection();

    std::ifstream in(Config::MAC_FILE);
    std::string mac;
    if (!std::getline(in, mac) || mac.empty()) {
        std::cerr << "No pending pairing device found in " << Config::MAC_FILE << std::endl;
        return;
    }

    std::cout << "Starting action for " << mac << "..." << std::endl;

    std::string device_path;
    bool is_paired = false;

    // 1. Find Device by MAC Address
    try {
        auto proxy = createBluezProxy(*conn, "/");
        std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>>
            objects;

        proxy->callMethod("GetManagedObjects").onInterface(MGR_IFACE).storeResultsTo(objects);

        for (const auto &[path, interfaces] : objects) {
            if (auto it = interfaces.find(DEVICE_IFACE); it != interfaces.end()) {
                const auto &props = it->second;

                if (auto addr_it = props.find("Address"); addr_it != props.end()) {
                    if (addr_it->second.get<std::string>() == mac) {
                        device_path = path;
                        if (auto paired_it = props.find("Paired"); paired_it != props.end()) {
                            is_paired = paired_it->second.get<bool>();
                        }
                        break;
                    }
                }
            }
        }
    } catch (const sdbus::Error &e) {
        std::cerr << "Error finding device: " << e.getMessage() << std::endl;
        return;
    }

    if (device_path.empty()) {
        std::cerr << "Device " << mac << " not found." << std::endl;
        return;
    }

    // 2. Pair (if needed) and Connect
    try {
        auto device = createBluezProxy(*conn, device_path);

        if (!is_paired) {
            std::cout << "Setting Trust..." << std::endl;
            device->callMethod("Set")
                .onInterface(PROP_IFACE)
                .withArguments(DEVICE_IFACE, "Trusted", sdbus::Variant(true));

            std::cout << "Pairing..." << std::endl;
            device->callMethod("Pair").onInterface(DEVICE_IFACE);
        } else {
            std::cout << "Already paired." << std::endl;
        }

        std::cout << "Connecting..." << std::endl;
        device->callMethod("Connect").onInterface(DEVICE_IFACE);

    } catch (const sdbus::Error &e) {
        std::cerr << "Operation failed: " << e.getMessage() << std::endl;
    }
}
