cmake_minimum_required(VERSION 3.13)
project(roiserver)

set(CMAKE_CXX_STANDARD 14)

include_directories(lib/roi_handler)
include_directories(lib/roi_handler/serial_handler)
include_directories(lib/roi_handler/serial_handler/RS-232)
include_directories(lib/server_socket_handler)

add_executable(roiserver
        lib/roi_handler/serial_handler/RS-232/rs232.c
        lib/roi_handler/serial_handler/RS-232/rs232.h
        lib/roi_handler/serial_handler/serial_handler.cpp
        lib/roi_handler/serial_handler/serial_handler.h
        lib/roi_handler/roi_opcodes.h
        lib/roi_handler/roomba_core.cpp
        lib/roi_handler/roomba_core.h
        lib/server_socket_handler/server_socket_handler.cpp
        lib/server_socket_handler/server_socket_handler.h
        lib/server_socket_handler/utilities.cpp
        lib/server_socket_handler/utilities.h
        roiserver.cpp)
