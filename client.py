#!/usr/bin/env python
# Args
import argparse, re, os
# Sockets
import socket, select
# Time
import time, datetime

class Log:
    COLORS = {'red':'\033[91m','yellow':'\033[93m','blue':'\033[94m','end':'\033[0m'}

    @staticmethod
    def _colored_message(text, color):
        return f"{Log.COLORS[color]}{text}{Log.COLORS['end']}"
    
    # Set boolean in case the user uses -d argument
    debug = False

    # Create a log file at the start of the program
    log_dir = "logs"
    os.makedirs(log_dir, exist_ok=True)
    log_file = open(os.path.join(log_dir, datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + ".log"), 'a')

    @staticmethod   
    def _write_to_file(message):
        # Remove color codes from the message before writing to the file
        for color_code in Log.COLORS.values():
            message = message.replace(color_code, '')
        message_with_time = f"{datetime.datetime.now().strftime('[%H:%M:%S]')} {message}"
        Log.log_file.write(message_with_time + "\n")

    @staticmethod
    def warning(message, override=False):
        msg = Log._colored_message("[WARNING]", 'yellow') + " " + message
        if Log.debug or override:
            print(msg)
        Log._write_to_file(msg)

    @staticmethod
    def info(message, override=False):
        msg = Log._colored_message("[INFO]", 'blue') + " " + message
        if Log.debug or override:
            print(msg)
        Log._write_to_file(msg)

    @staticmethod
    def error(message, override=False):
        msg = Log._colored_message("[ERROR]", 'red') + " " + message
        if Log.debug or override:
            print(msg)
        Log._write_to_file(msg)
        exit(-1)

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
                        if not re.match(r'^([0-9a-fA-F]{2}[:-]){5}([0-9a-fA-F]{2})$', value): Log.error("Invalid MAC address.",True)
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
    type = {
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
    
    def send(packet,type,client):
        try:
            bytecount = 0
            while bytecount < len(packet[bytecount:]):
                bytecount = sock_udp.sendto(packet, (client.Server, client.SrvUDP))
                # Print information
                Log.info(f'Client {client.Name} sent {bytecount} bytes during {type}.')
                # Check if all bytes were sent
                if bytecount < len(packet[bytecount:]):
                    # If not all bytes were sent truncate the packet and send remaining information.
                    Log.warning(f'Client {client.Name}: Not all bytes were sent!, sending remaining information...')
                    packet = packet[bytecount:]
            
            Log.info(f'Client {client.Name} sent all {type} bytes successfully.',True)

        except Exception as e:
            Log.error(f'Unexpected error when sending {type} packet by client: {client.Name}: {e}',True)

########################
# Subscription request #
########################
    
def subscription_request(client):
    # Get client data
    data = client.Name + ',' + client.Situation
    # Create PDU_UDP packet
    packet_type = PDU_UDP.type['SUBS_REQ']
    num = '00000000'
    packet = PDU_UDP.to_bytes(packet_type,client.MAC,num,data)

    # Send packet
    PDU_UDP.send(packet,'SUBS_REQ',client)
    
    # Set client status
    client.set_status('WAIT_ACK_SUBS')
    

def main():
    # Get Arguments
    args = args_parser()
    Log.debug = args.d

    # Create sockets
    # udp
    global sock_udp, sock_tcp
    sock_udp = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    # tcp

    # Create client object
    client = Client(args.c)
    print(vars(client))

    '''
    bytes = PDU_UDP.to_bytes(PDU_UDP.type['SUBS_REQ'],client.MAC,'129344123','lorem ipsum')
    print(bytes)
    decoded = PDU_UDP.from_bytes(bytes)
    print(decoded)
    '''

    subscription_request(client)

    # Close sockets
    sock_udp.close()

# Init call
if __name__ == "__main__":
    main()
