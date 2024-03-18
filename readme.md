
---
# Server Program for Controller Communication

## Overview

This program facilitates communication between a server and multiple controllers using TCP and UDP protocols. It handles server configuration, socket creation, controller connections, and data transmission/reception.

## Author

- **Author:** Eric Bitria Ribes
- **Version:** 1.1
- **Date:** 2024-3-18

## Scope

- **Server Configuration Management:** Reads and initializes server settings from configuration files.
- **Socket Initialization:** Creates TCP and UDP socket file descriptors for communication with controllers.
- **Controller Management:**
  - Loads a list of allowed controllers into memory.
  - Monitors and validates incoming connections from controllers.
  - Handles subscription requests and manages connection status updates.
  - Detects and handles disconnections and inactive controller detection.
- **Concurrency Control:** Implements mutexes to manage concurrent access to shared resources, ensuring data integrity.
- **Communication Handling:**
  - Receives and processes UDP packets from controllers.
  - Validates packet content and controller status.
  - Responds to controllers based on their status and request type.
  - Detects and handles TCP connections for data transmission between the server and controllers.
- **User Interaction:**
  - Accepts commands from the user via standard input for server management.
  - Supports commands for listing controllers, setting device values, getting device data, and quitting the server.

## Organization

The program is organized into several modules and files:

- `utilities/commons.h`: A common file across all modules to maintain library organization.
- `utilities/pdu/udp.c`: Contains functions for UDP packet handling.
- `utilities/pdu/tcp.c`: Contains functions for TCP packet handling.
- `utilities/logs.c`: Provides logging functionality for the program.
- `utilities/server/controllers.c`: Manages controller loading and data.
- `utilities/server/conf.c`: Handles server configuration.
- `utilities/server/subs.c`: Manages controller subscription requests and periodic communication.
- `utilities/server/commands.c`: Executes server management commands.
- `utilities/server/data.c`: Handles data transmission, request, and storage.

## Encoding

- All packet data is encoded and sent in UTF-8 format.

## Installation

To run the server program, follow these steps:

1. Clone the repository to your local machine.
2. Navigate to the directory containing the server program.
3. Compile the program using your preferred compiler.
4. Run the compiled executable.

## Usage

Once the server program is running, you can interact with it using the following commands:

- `list controllers`: Displays a list of connected controllers.
- `set device_value`: Sets the value of a specific device.
- `get device_data`: Retrieves data from a specific device.
- `quit`: Exits the server program.

---

# Client Program for Sensor Interaction and Server Communication

## Description
This Python program serves as a client application that simulates the interaction of a client with different sensors and communicates with a server. It utilizes various modules and protocols to handle subscription, data transmission, and user commands effectively.

## Features
- **Subscription Phase**: 
  - Sends subscription requests to the server using UDP.
  - Handles acknowledgment and various server responses.

- **Hello Process**:
  - Periodically sends HELLO packets to maintain connection.
  - Handles incoming HELLO packets from the server.

- **Data Process**:
  - Processes SET_DATA and GET_DATA requests received via TCP.
  - Sends data of specified devices to the server.
  - Handles various responses related to data communication.

- **Commands**:
  - Provides a command-line interface for user interaction.
  - Supports commands to check status, set device values, send device data, and quit the program.

## Usage
1. **Installation**:
   - Ensure Python 3 is installed on your system.
   - Clone this repository to your local machine.

2. **Configuration**:
   - Modify the `client.cfg` file to specify client configurations such as server address, ports, and device details.

3. **Execution**:
   - Run the `client.py` file using Python 3.
   - Follow the command-line prompts to interact with the client application.

Once the server program is running, you can interact with it using the following commands:

- stat: Displays controller and devices information
- set <device-name> <value>: Sets the value of a device
- send <device-name>: Sends the value of a device to the server
- quit: Exits the program

---
## Dependencies
- Python 3
- GCC Compiler
- [utilities](./utilities) (custom utility modules for handling sockets, logs, and configuration for the server)
- [utilities_p](./utilities_p) (custom utility modules for handling sockets, logs, and configuration for the client)

## License
This project is licensed under the [MIT License](./LICENSE).

## Author
- [Eric Bitria Ribes](https://github.com/Erkko68)
## Version
- 0.4