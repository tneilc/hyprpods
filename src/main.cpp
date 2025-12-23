#include "BluezClient/BluezClient.h"
#include <cstdio>
#include <cstring>
#include <iostream>

int main(int argc, char **argv) {
    // Unbuffered output for Waybar
    setbuf(stdout, NULL);

    // Check for arguments
    if (argc > 1 && std::strcmp(argv[1], "--pair") == 0) {
        BluezClient::trigger_pairing();
        return 0;
    }

    // Default mode: Monitoring Loop
    try {
        BluezClient app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
