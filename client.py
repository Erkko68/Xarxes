#!/usr/bin/env python
# Args
import argparse, re
# Sockets
import socket, select
# Time
import time

class Log:
    COLORS = {'red':'\033[91m','yellow':'\033[93m','blue':'\033[94m','end':'\033[0m'}

    # Set boolean in case the user uses -d argument
    debug = False

    @staticmethod
    def _colored_message(text, color):
        return f"{Log.COLORS[color]}{text}{Log.COLORS['end']}"

    @staticmethod
    def warning(message, override=False):
        msg = Log._colored_message("[WARNING]", 'yellow') + " " + message
        if Log.debug or override:
            print(msg)

    @staticmethod
    def info(message, override=False):
        msg = Log._colored_message("[INFO]", 'blue') + " " + message
        if Log.debug or override:
            print(msg)

    @staticmethod
    def error(message, override=False):
        msg = Log._colored_message("[ERROR]", 'red') + " " + message
        if Log.debug or override:
            print(msg)
        exit(-1)

    def get_key(val,dict):
        for key, value in dict.items():
            if val == value:
                return key

# Function to get user arguments
def args_parser():
    # Initialise parser
    parser = argparse.ArgumentParser(usage="client.py [-h] [-c client_config.cfg] [-d]")
    # Define Optional arguments
    parser.add_argument('-c', type=str, help="Path to config file", default="client.cfg",metavar="config_name")
    parser.add_argument('-d', action='store_true', help="Enable debugging mode")
    # Return user args object
    return parser.parse_args()

# Client Configuration Class
class Client:
    # Define client status
    status = {
        'DISCONNECTED'  : 0xa0,
        'NOT_SUBSCRIBED' : 0Xa1,
        'WAIT_ACK_SUBS' : 0Xa2,
        'WAIT_INFO' : 0Xa3,
        'WAIT_ACK_INFO' : 0Xa4,
        'SUBCRIBED' : 0Xa5,
        'SEND_HELLO' : 0Xa6
    }

    # Status setter
    def set_status(self, status):
        self.status = Client.status[status]

    def __init__(self, file_path="client.cfg"):
        try:
             # Open file descriptor and process each config value
            with open(file_path, 'r') as file:
                for line in file:
                    line = line.strip() # Sanitice line
                    # Ignore newline
                    if not line: 
                        continue
                    # Split into key-value
                    key, value = line.split('=') 

                    if key == 'Name':
                        if not len(value) == 8: Log.warning("Name must be 8 alphanumeric characters long.",True)
                    elif key == 'Situation':
                        if not re.match(r'^B\d{2}L\d{2}R\d{2}A\d{2}$', value): Log.warning("Invalid Situation Format.",True)
                    elif key == 'Elements':
                        value = self._process_elements(value)
                    elif key == 'MAC':
                        if not re.match(r'^([0-9a-fA-F]{12})$', value): Log.error("Invalid MAC address.",True)
                    elif key in ['Local-TCP', 'Srv-UDP']:
                        key = key.replace('-','')
                        # Cast server port to int
                        value = int(value)
                        if not 0 <= int(value) <= 65535: Log.error(f"Invalid {key} Port, must be between 0 and 65535.",True)
                    elif key == 'Server':
                        if not re.match(r'^(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$', value): Log.error("Invalid Server IP.",True)
                    else:
                        continue # Ignore any other configuration found

                    # Set attribute
                    setattr(self, key, value)
        # Throw exception in case of error
        except Exception as e:
            Log.error(f'Unexpected error while reading file {file_path}: {e}')
        
        # Set default status
        self.status = Client.status['NOT_SUBSCRIBED']

    def _process_elements(self, elements):
        devices = elements.split(';')
        if len(devices) > 10:
            Log.warning("More than 10 devices detected, only the first 10 will be used.")
            devices = devices[:10] # Take the first 10 devices
        for device in devices:
            if not re.match(r'^[A-Z]{3}-\d{1}-[IO]$', device): Log.warning(f"Invalid Device Format: {device}")
        return devices

# Class to encode client data
class Encode:
    # Ensure the desired lenght, then encode to bytes
    @staticmethod
    def _string_to_bytes(s, length):     
        return s[:length].ljust(length, '\0').encode('utf-8')
    
    # Convert bytes to string
    @staticmethod
    def _bytes_to_string(bytes, offset, length):
        return bytes[offset:offset+length].decode('utf-8').rstrip('\0')

# Class used to contain all PDU_UDP protocol methods
class PDU_UDP:
    # Define packet types
    packet_type = {
        'SUBS_REQ'  : 0x00,
        'SUBS_ACK'  : 0X01,
        'SUBS_REJ'  : 0X02,
        'SUBS_INFO' : 0X03,
        'INFO_ACK'  : 0X04,
        'SUBS_NACK' : 0X05
    }

    # A function that converts strings values to PDU_UDP format as a byte array
    @staticmethod
    def to_bytes(uchar, mac, rand, data):
        uchar = bytes([uchar])
        # Convert the MAC address to bytes
        mac_bytes = Encode._string_to_bytes(mac,13)
        # Convert the 9 bytes long number to bytes
        rand = Encode._string_to_bytes(rand,9)
        # Convert the data to bytes
        data = Encode._string_to_bytes(data,80)

        # Pack the data into a byte array
        return uchar + mac_bytes + rand + data

    @staticmethod
    def from_bytes(buffer):
        # Unpack the data from the byte array
        # Get type
        uchar = buffer[0]
        # Convert the MAC address bytes to a MAC address string
        mac = Encode._bytes_to_string(buffer,1,13)
        # Ensure the rand number and data string are the correct length
        rand = Encode._bytes_to_string(buffer,14,9)
        data = Encode._bytes_to_string(buffer,23,80)

        return uchar, mac, rand, data
    
    def send(packet,client):
        try:
            bytecount = 0
            while bytecount < len(packet[bytecount:]):
                bytecount = sock_udp.sendto(packet, (client.Server, client.SrvUDP))
                # Print information
                Log.info(f'Client {client.Name} sent {bytecount} bytes during {Log.get_key(packet[0],PDU_UDP.packet_type)}.')
                # Check if all bytes were sent
                if bytecount < len(packet[bytecount:]):
                    # If not all bytes were sent truncate the packet and send remaining information.
                    Log.warning(f'Client {client.Name}: Not all bytes were sent during {Log.get_key(packet[0],PDU_UDP.packet_type)}!, sending remaining information...')
                    packet = packet[bytecount:]
            
            Log.info(f'Client {client.Name} sent {Log.get_key(packet[0],PDU_UDP.packet_type)} successfully.')

        except Exception as e:
            Log.error(f'Unexpected error when sending {Log.get_key(packet[0],PDU_UDP.packet_type)} by client: {client.Name}: {e}',True)

########################
# Subscription request #
########################
    
def send_subscription_packet(client):
    # Get client data
    data = client.Name + ',' + client.Situation
    # Create PDU_UDP packet
    packet_type = PDU_UDP.packet_type['SUBS_REQ']
    num = '00000000'
    packet = PDU_UDP.to_bytes(packet_type,client.MAC,num,data)

    # Send packet
    PDU_UDP.send(packet,client)
    
    # Set client status
    client.set_status('WAIT_ACK_SUBS')

def subs_request(client):
    for _ in range(3):
        # Print subscription request
        Log.info(f'Starting subscription request for client: {client.Name} [{_+1}/3]',True)
        sleep = 1
        for packets_sent in range(7):
            # Send packet

            send_subscription_packet(client)
            # Read data
            try:
                ready_to_read, _, _ = select.select([sock_udp], [], [], 0)
            except select.error as e:
                Log.error(f'Unexpected error when checking for available data on socket {sock_udp}: {e}')

            if ready_to_read:  # Data is available to be read
                data, addr = sock_udp.recvfrom(103)
                print(data)
                break
            else: # No data available
                # Increase sleep time
                if sleep < 3*1 and packets_sent >= 3:
                    sleep += 1
                time.sleep(sleep)
        else:  # If we don't receive data, (break not executed).
            # Pause subscription request and continue.
            time.sleep(2)
            continue 
        break  # Exit subscription phase if data is received
    # Print error message
    Log.info(f"Could not stablish connection with server {client.Server}",True)

# Initialization function
def _init():
    global sock_udp, sock_tcp, client # Create global variables
    args = args_parser() # Get User Initial Arguments
    Log.debug = args.d # Set Log class to debug mode
    # Create sockets
    sock_udp = socket.socket(socket.AF_INET,socket.SOCK_DGRAM) # UDP
    # TCP
    client = Client(args.c) # Read and create client config

def main():
    # Start Subscription Request
    subs_request(client)
            
    # Close sockets
    sock_udp.close()

# Init call
if __name__ == "__main__":
    _init()
    main()
