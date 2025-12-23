cmake_minimum_required(VERSION 3.17)
project(hyprpods)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -O2)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUSCPP REQUIRED sdbus-c++)

include_directories(src)

# Source Files
add_executable(hyprpods 
    src/main.cpp
    src/BluezClient/BluezClient.cpp
    src/State/DeviceState.cpp
    src/Decoder/Decoder.cpp
)

target_link_libraries(hyprpods ${SDBUSCPP_LIBRARIES})
install(TARGETS hyprpods DESTINATION /usr/local/bin)
