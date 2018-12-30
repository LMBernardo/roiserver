//--------------------------
// Roomba Open Interface Server
//--------------------------
// Description:
// Provides TCP/IP socket accessable wrapper to ROI serial interface over Pi3 B+ GPIO UART pins
// Designed specifically for Roomba 405, may not provide full functionality for more advanced models.
//--------------------------
// Author: Layne Bernardo
// Email: retnuh66@gmail.com
//
// Created July 20th, 2018
// Modified: October 8th, 2018
// Version 0.3
//--------------------------

// Uncomment to skip GPIO stuff (for runtime testing on non-rpi devices)
// #define _ROISERVER_TEST_MODE_

//-----------------------------
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>

#include <iterator>

// For seeding rand()
#include <time.h>

// wiringPi GPIO library
#ifndef _ROISERVER_TEST_MODE_
  #include <wiringPi.h>
#endif

// Socket library
#include "lib/server_socket_handler/server_socket_handler.h"

// Roomba library
#include "lib/roi_handler/roomba_core.h"

//-----------------------------

void print_usage(){
        std::cout << "Usage: roiserver (port)\n\n";
        std::cout << "(port) is an optional integer in the range 0 to 65534, if not provided a random port will be chosen.\n";
}

void err_quit(int errsv){
        std::cout << "Errno: " << errsv << std::endl;
        std::cout << "Exiting.." << std::endl;
        std::exit(-1);
}

void init_gpio(){
        #ifndef _ROISERVER_TEST_MODE_
        // Setup wiringPi and GPIO pin 4 for DD
        wiringPiSetup();
        digitalWrite(4, HIGH);
        pinMode(4, OUTPUT);
        #endif
}

// Wake roomba via GPIO23 (pin 4 in wiringPI)
void wake_roomba(){
        #ifndef _ROISERVER_TEST_MODE_
        std::cout << "Waking roomba..." << std::endl;
        digitalWrite(4, LOW); delay (500);
        digitalWrite(4, HIGH);
        #endif
}

// Handle commands
// TODO: Don't use a ton of if statements. Maybe use an enum and switch

//  0 = Success
// -1 = Send error (serial_handler)
// -2 = Malformed request (not enough parameters)
// -3 = Command not found

int process_cmd(roomba_core* s_roomba_p, std::vector<std::string> split_data){
        const int data_offset = 1;
        enum function {wake, init_safe, leds, power_led, add_song, play_song, drive, drive_wheels, power_off, raw};

        unsigned int num_bytes = split_data.size();
        if (num_bytes < data_offset+1) {
                return -2;
        }

        if (split_data[data_offset] == "wake") {
                wake_roomba();
                return 0;
        }

        if (split_data[data_offset] == "init_safe") {
                if ( s_roomba_p->init_roi() < 0) return -1;
                if ( s_roomba_p->safe_mode() < 0) return -1;
                return 0;
        }

        if (split_data[data_offset] == "leds") {
                if ( s_roomba_p->leds( split_data[data_offset+1] ) < 0) return -1;
                return 0;
        }

        if (split_data[data_offset] == "power_led") {
                if ( s_roomba_p->power_led( split_data[data_offset+1] ) < 0) return -1;
                return 0;
        }

        if (split_data[data_offset] == "add_song") {
                s_roomba_p->add_song( split_data[data_offset+1] );
                return 0;
        }

        if (split_data[data_offset] == "play_song") {
                s_roomba_p->play_song( split_data[data_offset+1] );
                return 0;
        }

        if (split_data[data_offset] == "drive") {
                s_roomba_p->drive( split_data[data_offset+1] );
                return 0;
        }

        if (split_data[data_offset] == "motors") {
                s_roomba_p->motors( split_data[data_offset+1] );
                return 0;
        }

        if (split_data[data_offset] == "drive_wheels") {
                s_roomba_p->drive_wheels ( split_data[data_offset+1] );
                return 0;
        }

        if (split_data[data_offset] == "power_off") {
                s_roomba_p->power_off();
                return 0;
        }

        if (split_data[data_offset] == "raw") {
                if (num_bytes < 3) {
                        return -2;
                }
                std::cout << "Sending raw data: " << split_data[data_offset+1] << std::endl;
                if ( s_roomba_p->s_serial.send_byte_array(split_data[data_offset+1]) < 0 ) {
                        return -1;
                } else {
                        return 0;
                }
        }

        // Command not found
        return -3;
}

int main( int argc, char* argv[] ){

        // Seed rand()
        srand( time(NULL) );

        init_gpio();

        // Create default socket handler
        server_socket_handler s_socket;

        // Create roomba_core
        roomba_core s_roomba;

        // Get port number
        int port_input;
        if ( argc < 2 ) {
                std::cout << "No port provided, using random port" << std::endl;
                port_input = rand() % 60535 + 5000;
        } else {
                try {
                        port_input = std::stoi( argv[1] );
                } catch (...) {
                        std::cout << "\nInvalid port!\n\n";
                        print_usage();
                        exit(-1);
                }
        }

        std::cout << "\nStarting server...\n";

        // Set port
        s_socket.s_port = port_input;

        // Initialize socket handler
        #ifndef _ROISERVER_TEST_MODE_
        int init_status = s_socket.s_init();
        if ( init_status != 0 ) {
                err_quit(init_status);
        }
  #endif

        // Connect to Roomba over GPIO UART
  #ifndef _ROISERVER_TEST_MODE_
        int roomba_s_status = s_roomba.connect();
        if ( roomba_s_status != 0 ) {
                err_quit(roomba_s_status);
        }
        #endif

        bool run = true;
        // Wait for client connection. If it drops, wait again.
        while ( run ) {
                std::cout << std::endl << "Waiting for client..." << std::endl;
                // Attempt to accept client connection
                if ( s_socket.s_accept() < 0 ) {
                        int errsv = errno;
                        std::cout << "Failed to accept client connection" << std::endl;
                        std::cout << "Errno: " << errsv << std::endl;
                } else {
                        std::cout << "Accepted new client connection" << std::endl;
                }

                // Wait for data. If connection is lost, go back to client waiting loop
                int num_bytes = -1;
                while ( num_bytes != 0 ) {
                        std::cout << "Waiting for data from client...\n\n";
                        num_bytes = s_socket.socket_read(0);

                        std::vector<std::string> split_data;
                        int start = 0;
                        std::cout << "\nDigesting messages...\n";
                        while (num_bytes > start) {
                                // Changes start to new value
                                std::cout << "Bytes left: " << (num_bytes - start) << std::endl;
                                split_data = s_socket.splitBuffer(start);

                                if (split_data[0] == "sendcmd") {
                                        int status = process_cmd(&s_roomba, split_data);
                                        if ( status == -2 ) {
                                                std::cout << "Malformed request (not enough parameters)" << std::endl;
                                        }
                                        if ( status == -3 ) {
                                                std::cout << "Invalid command: " << split_data[1] << std::endl;
                                        }
                                } else {
                                        std::cout << "Unknown operation: " << split_data[0] << std::endl;
                                }

                        }
                }

                s_socket.client_list.pop_back();
                std::cout << "Client disconnected" << std::endl;
        }

        return 0;
}
