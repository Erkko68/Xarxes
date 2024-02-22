#!/usr/bin/env python3
import argparse
import socket
import select
import struct
import re

class Log:
    COLORS = {
        'red': '\033[91m',
        'green': '\033[92m',
        'yellow': '\033[93m',
        'blue': '\033[94m',
        'end': '\033[0m'
    }
    debug = False
    @staticmethod
    def _colored_message(text, color):
        return f"{Log.COLORS[color]}{text}{Log.COLORS['end']}"
    
    @staticmethod
    def warning(message):
        if Log.debug:
            print(Log._colored_message("[WARNING]", 'yellow') + " " + message)

    @staticmethod
    def info(message):
        if Log.debug:
            print(Log._colored_message("[INFO]", 'blue') + " " + message)

    @staticmethod
    def error(message):
        if Log.debug:
            print(Log._colored_message("[ERROR]", 'red') + " " + message)

# Function to get user arguments
def args_parser():
    # Initialise parser
    parser = argparse.ArgumentParser(usage="client.py [-h] [-c client_config.cfg] [-d]")
    # Define Optional arguments
    parser.add_argument('-c', type=str, help="Path to config file", default="client.cfg",metavar="config_name")
    parser.add_argument('-d', action='store_true', help="Enable debugging mode")
    # Return user args
    return parser.parse_args()

# Client Configuration Class
class Client:
    def __init__(self, file_path="client.cfg"):
        self.status = 0x00 # Set default status
        with open(file_path, 'r') as file: # Open file descriptor and process each config value
            for line in file:
                line = line.strip() # Sanitice line
                if not line: # Ignore newline
                    continue
                key, value = line.split('=')  # Split into key-value
                if key == 'Elements':
                    value = self.process_elements(value)
                elif key == 'MAC':
                    assert re.match(r'^[0-9a-fA-F]{12}$', value), "Invalid MAC Address"
                elif key in ['Local-TCP', 'Srv-UDP']:
                    assert 0 <= int(value) <= 65535, "Invalid Port Number"
                elif key == 'Name':
                    assert len(value) == 8, "Name must be 8 characters long"
                elif key == 'Situation':
                    assert re.match(r'^B\d{2}L\d{2}R\d{2}A\d{2}$', value), "Invalid Situation Format"
                else:
                    continue # Ignore any other configuration found
                setattr(self, key, value) # Set attribute

    def set_status(self, status):
        self.status = status

    def process_elements(self, elements):
        devices = elements.split(';')
        if len(devices) > 10:
            Log.warning("More than 10 devices detected, only the first 10 will be used.")
            devices = devices[:10] # Take the first 10 devices
        for device in devices:
            if not re.match(r'^[A-Z]{3}-\d{1}-[IO]$', device):
                Log.warning(f"Invalid Device Format: {device}")
        return devices


class PDU_UDP:
    @staticmethod
    def MAC_to_bytes(MAC):
        return bytes(int(b, 16) for b in MAC.split(':'))
    
    @staticmethod
    def bytes_to_MAC(MAC_bytes):
        return ':'.join('{:02X}'.format(b) for b in MAC_bytes)
    
    # A function that converts strings values to PDU_UDP format as a byte array
    @staticmethod
    def to_bytes(p_type, MAC, rnd, data):
        # Convert MAC address to bytes
        mac_bytes = PDU_UDP.MAC_to_bytes(MAC)
        
        # Convert random number to bytes
        rnd_bytes = rnd.to_bytes(9, byteorder='big')

        # Encode data into bytes
        data_bytes = data.encode('utf-8')

        # Ensure each part has the correct length
        p_type_byte = struct.pack('B', p_type.value)
        mac_bytes = struct.pack('12s', mac_bytes)
        rnd_bytes = struct.pack('9s', rnd_bytes)
        data_bytes = struct.pack('80s', data_bytes[:80])

        return p_type_byte + mac_bytes + rnd_bytes + data_bytes

    # Method to decode from bytes to strings
    @staticmethod
    def from_bytes(pdu_bytes):
        # Extract type value (1 byte)
        type = pdu_bytes[0]

        # Extract MAC address (12 bytes)
        mac_address = PDU_UDP.bytes_to_MAC(pdu_bytes[1:13])

        # Extract random number (9 bytes)
        rnd = int.from_bytes(pdu_bytes[13:22], byteorder='big')

        # Extract data (80 bytes)
        data = pdu_bytes[22:]

        return type, mac_address, rnd, pdu_bytes

def main():
    # Get Arguments
    args = args_parser()
    Log.debug = args.d

    # Create client object
    client = Client(args.c)
    print(vars(client))
# Init call
if __name__ == "__main__":
    main()
