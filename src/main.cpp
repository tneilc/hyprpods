#include "BluezClient/BluezClient.h"
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>

int main(int argc, char **argv) {
    setbuf(stdout, NULL);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);

    try {
        BluezClient app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
