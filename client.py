#!/usr/bin/env python3

###########
# Imports #
###########

# Args
import argparse
# Sockets
import socket
import select
# Byte conversion
import struct

##################
# Initialization #
##################

# Client Logs Class
class Log:
    # Color enumerate
    COLORS = {
        'red': '\033[91m',
        'green': '\033[92m',
        'yellow': '\033[93m',
        'blue': '\033[94m',
        'end': '\033[0m'
    }
    # Constructor
    def __init__(self, debug=False):
        self.debug = debug

    # Colored message function
    def _colored_message(self, text, color):
        return f"{self.COLORS[color]}{text}{self.COLORS['end']}"

    def warning(self, message):
        if self.debug:
            print(self._colored_message("[WARNING]", 'yellow') + " " + message)

    def info(self, message):
        if self.debug:
            print(self._colored_message("[INFO]", 'blue') + " " + message)

    def error(self, message):
        if self.debug:
            print(self._colored_message("[ERROR]", 'red') + " " + message)

# Client Configuration Class
class Config:
    def __init__(self, file_path="client.cfg"):
        # Open file descriptor
        with open(file_path, 'r') as file:
            for line in file:
                 # Skip empty lines
                if not line.strip():
                    continue
                # Split line based on '='
                key, value = line.strip().split('=')
                # Split Devices
                if key == 'Elements':
                    # Call method to handle elements
                    value = self.process_elements(value)
                # Set attribute to object
                setattr(self, key, value)

    def process_elements(self, elements):
        # Split the string on ';'
        devices = elements.split(';')
        
        # Limit the size of the array to 10 devices
        if len(devices) > 10:
            Log.warning("More than 10 devices detected, only the first 10 will be used.")
            devices = devices[:10] # Only take first 10 devices
        
        return devices
    
    # Function to validate that the object has all the correct configs
    required_params = ['Name', 'Situation', 'Elements', 'MAC', 'Local-TCP', 'Server', 'Srv-UDP']
    def validate(self):
        for param in self.required_params:
            if not hasattr(self, param) or not getattr(self, param):
                return False
        return True

# Function to get user arguments
def args_parser():
    # Initialise parser
    parser = argparse.ArgumentParser(usage="client.py [-h] [-c client_config.cfg] [-d]")
    # Define Optional arguments
    parser.add_argument('-c', type=str, help="Path to config file", default="client.cfg",metavar="config_name")
    parser.add_argument('-d', action='store_true', help="Enable debugging mode")
    # Return user args
    return parser.parse_args()

# Class for binary conversion
class Bytes:
    # MAC adress string to MAC byte format
    def MAC_to_bytes(MAC_string):
        # Split the MAC address string into components
        parts = MAC_string.split(':')
        
        # Convert each hexadecimal part into bytes, then join them in a binary array
        MAC_bytes = b''.join([struct.pack('B', int(part, 16)) for part in parts])
        
        return MAC_bytes

class PDU_UDP:
    # Create constructor that returns a byte array (packet)
    # Create constructor that receives byte array and returns object
    def __init__(self,type,MAC,rnd,data):
        self.type=type
        self.MAC=Bytes.MAC_to_bytes(MAC)
        self.rnd=rnd
        self.data=data
        

def main():
    # Get Arguments
    args = args_parser()
    # Initialize logs only when debug mode is enabled
    log = Log(args.d)
    # Create config object
    config = Config(args.c)

    if not config.validate():
        log.error("Found error at client configuration values.")

    for key in config.__dict__:
        print(f"{key}: {getattr(config, key)}")

    Bytes.MAC_to_bytes("00:B0:D0:63:C2:26")

# Init call
if __name__ == "__main__":
    main()
