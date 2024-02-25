cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(PICO_BOARD pico_w)

project(SerialMirror CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


pico_sdk_init()

add_executable(serialMirror main.c)

target_link_libraries(serialMirror pico_stdlib)

pico_enable_stdio_usb(serialMirror 1)
pico_enable_stdio_uart(serialMirror 0)
pico_add_extra_outputs(serialMirror)
