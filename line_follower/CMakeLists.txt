# Generated Cmake Pico project file
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(line_follower C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable with C++ files
add_executable(line_follower 
    main.cpp
    pixy2.cpp
)

pico_set_program_name(line_follower "line_follower")
pico_set_program_version(line_follower "0.1")

# Enable USB output for debugging
pico_enable_stdio_uart(line_follower 0)
pico_enable_stdio_usb(line_follower 1)

# Add the standard library to the build
target_link_libraries(line_follower
    pico_stdlib
)

# Add the standard include files to the build
target_include_directories(line_follower PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add hardware libraries we need
target_link_libraries(line_follower
    hardware_pwm
    hardware_gpio
    hardware_i2c
)

pico_add_extra_outputs(line_follower)