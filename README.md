
# Hyprpods

**Hyprpods** is a lightweight, modern C++ utility designed to monitor Apple AirPods battery levels on Linux. It is specifically optimized for integration with **Waybar** (on Hyprland, Sway, etc.) by outputting real-time JSON data.

Unlike standard Bluetooth battery reporting which can be unreliable for dual-earbud devices, Hyprpods listens for the specific BLE (Bluetooth Low Energy) Manufacturer Data packets broadcast by Apple devices to report precise battery levels for the Left Pod, Right Pod, and Case.

## Features

-   **Real-time Monitoring:** Decodes Apple BLE manufacturer packets to get exact battery percentages.
    
-   **Waybar Integration:** Outputs JSON formatted specifically for Waybar's custom module.
    
-   **Modern C++:** Built using `sdbus-c++` for a robust, type-safe DBus connection.
    
-   **Pairing Helper:** Includes a utility to facilitate connecting/pairing to specific devices via MAC address.
    
-   **Low Resources:** Efficiently sits on the system bus waiting for signals, rather than polling aggressively.
    

## Prerequisites

To build Hyprpods, you need a C++17 compiler and the `sdbus-c++` library.

### Dependencies

-   **CMake** (>= 3.17)
    
-   **Clang** (supporting C++17)
    
-   **BlueZ** (Linux Bluetooth stack)
    
-   **sdbus-c++** (v2.0+ recommended)
    

#### Installing Dependencies

**Arch Linux:**

```
sudo pacman -S cmake clang sdbus-c++

```

**Ubuntu/Debian:**

```
sudo apt install cmake clang make libsdbus-c++-dev
```

## Build & Install

1.  **Clone the repository:**
    
    ```
    git clone https://github.com/tneilc/hyprpods.git
    cd hyprpods
    ```
    
2.  **Create a build directory:**
    
    ```
    mkdir build && cd build
    ```
    
3.  **Compile:**
    
    ```
    cmake ..
    make
    ```
    
4.  Install (Required):
    
    This step installs the binary to /usr/local/bin/hyprpods. This is required for the Waybar configuration below to work without specifying an absolute path.
    
    ```
    sudo make install
    ```
    

## Configuration

### 1. Set Target Device

Before building, ensure your `src/Config/Config.h` points to the location where you intend to store your AirPods' MAC address, or edit it to your preference.

By default, the application looks for a file containing the MAC address to use features like `--pair`.

### 2. Waybar Configuration

Add the following module to your `waybar` config (usually `~/.config/waybar/config`):

```
"custom/airpods": {
    "format": "{}",
    "exec": "hyprpods",
    "return-type": "json",
    "on-click": "pkill -SIGUSR1 hyprpods",
    "signal": 10
}
```

**Note:** The output format is dynamic. You can style it in `style.css` using the classes usually applied to custom modules.

## Usage

### Monitoring (Default)

Simply run the program. It will start listening for BLE packets from your AirPods (even if they are not currently "connected" to audio, as long as the case lid is open or they are in your ears).

```
hyprpods
```

_Output Example:_

```
{"text": "L: 100% R: 100% Case: 85%", "tooltip": "Connected", "class": "connected"}
```

### Pairing Helper

If you have your AirPods' MAC address saved in the configuration file referenced in `Config.h`, you can trigger a connection attempt:

```
hyprpods --pair
```

This attempts to trust, pair, and connect to the device.

## Troubleshooting

**No data showing up?**

1.  Ensure your Bluetooth controller is powered on: `bluetoothctl power on`.
    
2.  Open the AirPods case lid near the computer.
    
3.  Ensure your user has permissions to access DBus.
    
4.  Run `hyprpods` in a terminal to see if any errors (like "Adapter not found") appear.
    

Build fails on createProxy?

Ensure you have a recent version of sdbus-c++. The API for createProxy changed in v2.0. This project is updated to support v2.0+.

## License

[MIT License](https://www.google.com/search?q=LICENSE "null")
