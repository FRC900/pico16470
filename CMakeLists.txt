# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Include symbols for gdb
set(CMAKE_BUILD_TYPE Debug)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set($ENV{PICO_SDK_PATH})

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico16470 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(pico16470
        src/main.c
        src/imu.c
	src/reg.c
	src/script.c
)

target_include_directories(
        pico16470
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

pico_set_program_name(pico16470 "pico16470")
pico_set_program_version(pico16470 "0.1")

#For not debugging:
#pico_enable_stdio_uart(pico16470 0)
#pico_enable_stdio_usb(pico16470 1)

#For debugging:
pico_enable_stdio_uart(pico16470 1)
pico_enable_stdio_usb(pico16470 0)

# Add the standard library to the build
target_link_libraries(pico16470 pico_stdlib)

# Add any user requested libraries
target_link_libraries(pico16470
        hardware_spi
        hardware_dma
        hardware_pio
        hardware_timer
        hardware_watchdog
        hardware_clocks
        )

pico_add_extra_outputs(pico16470)
