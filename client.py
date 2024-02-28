#!/usr/bin/env python
# Args
import argparse
# Sockets
import socket, select
# Time
import time


class Log:
    COLORS = {'red': '\033[91m', 'yellow': '\033[93m', 'blue': '\033[94m', 'end': '\033[0m'}

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

    @staticmethod
    def get_key(val, dictionary):
        for key, value in dictionary.items():
            if val == value:
                return key


# Function to get user arguments
def args_parser():
    # Initialise parser
    parser = argparse.ArgumentParser(usage="client.py [-h] [-c client_config.cfg] [-d]")
    # Define Optional arguments
    parser.add_argument('-c', type=str, help="Path to config file", default="client.cfg", metavar="config_name")
    parser.add_argument('-d', action='store_true', help="Enable debugging mode")
    # Return user args object
    return parser.parse_args()


# Client Configuration Class
class Client:
    # Define client status
    status = {
        'DISCONNECTED': 0xa0,
        'NOT_SUBSCRIBED': 0Xa1,
        'WAIT_ACK_SUBS': 0Xa2,
        'WAIT_INFO': 0Xa3,
        'WAIT_ACK_INFO': 0Xa4,
        'SUBSCRIBED': 0Xa5,
        'SEND_HELLO': 0Xa6
    }

    # Status setter
    def set_status(self, status):
        self.status = Client.status[status]

    def __init__(self, file_path="client.cfg"):
        try:
            # Open file descriptor and process each config value
            with open(file_path, 'r') as file:
                for line in file:
                    line = line.strip()  # Sanitize line
                    line = line.replace(' ','')
                    # Ignore newline
                    if not line:
                        continue
                    # Split into key-value
                    key, value = line.split('=')

                    if key not in ['Name', 'Situation', 'MAC', 'Local-TCP', 'Srv-UDP', 'Server']:
                        continue  # Ignore any other configuration found

                    if key == 'Elements':
                        value = self._process_elements(value)
                    elif key in ['Local-TCP', 'Srv-UDP']:
                        key = key.replace('-', '')
                        # Cast server port to int
                        value = int(value)

                    # Set attribute
                    setattr(self, key, value)

        except Exception as e:
            Log.error(f'Unexpected error while reading file {file_path}: {e}')

        # Set default status
        self.status = Client.status['NOT_SUBSCRIBED']

    @staticmethod
    def _process_elements(elements):
        devices = elements.split(';')
        if len(devices) > 10:
            Log.warning("More than 10 devices detected, only the first 10 will be used.")
            devices = devices[:10]  # Take the first 10 devices
        return devices


# Class to encode client data
class Encode:
    # Ensure the desired length, then encode to bytes
    @staticmethod
    def string_to_bytes(s, length):
        return s[:length].ljust(length, '\0').encode('utf-8')

    # Convert bytes to string
    @staticmethod
    def bytes_to_string(byte, offset, length):
        return byte[offset:offset + length].decode('utf-8').rstrip('\0')


# Class used to contain all PDU_UDP protocol methods
class PDU_UDP:
    # Define packet types
    packet_type = {
        'SUBS_REQ': 0x00,
        'SUBS_ACK': 0X01,
        'SUBS_REJ': 0X02,
        'SUBS_INFO': 0X03,
        'INFO_ACK': 0X04,
        'SUBS_NACK': 0X05
    }

    # A function that converts strings values to PDU_UDP format as a byte array
    @staticmethod
    def to_bytes(uchar, mac, rand, data):
        uchar = bytes([uchar])
        # Convert the MAC address to bytes
        mac_bytes = Encode.string_to_bytes(mac, 13)
        # Convert the 9 bytes long number to bytes
        rand = Encode.string_to_bytes(rand, 9)
        # Convert the data to bytes
        data = Encode.string_to_bytes(data, 80)

        # Pack the data into a byte array
        return uchar + mac_bytes + rand + data

    @staticmethod
    def from_bytes(buffer):
        # Unpack the data from the byte array
        # Get type
        uchar = buffer[0]
        # Convert the MAC address bytes to a MAC address string
        mac = Encode.bytes_to_string(buffer, 1, 13)
        # Ensure the rand number and data string are the correct length
        rand = Encode.bytes_to_string(buffer, 14, 9)
        data = Encode.bytes_to_string(buffer, 23, 80)

        return uchar, mac, rand, data

    @staticmethod
    def send(packet, client_conf):
        try:
            byte_count = 0
            while byte_count < len(packet[byte_count:]):
                byte_count = sock_udp.sendto(packet, (client_conf.Server, client_conf.SrvUDP))
                # Print information
                Log.info(
                    f'Client {client_conf.Name} sent {byte_count} bytes during {Log.get_key(packet[0], PDU_UDP.packet_type)}.')
                # Check if all bytes were sent
                if byte_count < len(packet[byte_count:]):
                    # If not all bytes were sent truncate the packet and send remaining information.
                    Log.warning(
                        f'Client {client_conf.Name}: Not all bytes were sent during {Log.get_key(packet[0], PDU_UDP.packet_type)}!, sending remaining information...')
                    packet = packet[byte_count:]

            Log.info(f'Client {client_conf.Name} sent {Log.get_key(packet[0], PDU_UDP.packet_type)} successfully.')

        except Exception as e:
            Log.error(
                f'Unexpected error when sending {Log.get_key(packet[0], PDU_UDP.packet_type)} by client: {client_conf.Name}: {e}',
                True)


########################
# Subscription request #
########################

def send_subscription_packet(client_conf):
    # Get client data
    data = client_conf.Name + ',' + client_conf.Situation
    # Create PDU_UDP packet
    packet_type = PDU_UDP.packet_type['SUBS_REQ']
    num = '00000000'
    packet = PDU_UDP.to_bytes(packet_type, client_conf.MAC, num, data)

    # Send packet
    PDU_UDP.send(packet, client_conf)


def subs_request(client_conf):
    # Set client status
    client.set_status('WAIT_ACK_SUBS')

    for _ in range(3):
        # Print subscription request
        Log.info(f'Starting subscription request for client: {client_conf.Name} [{_ + 1}/3]', True)
        sleep = 1
        for packets_sent in range(7):
            # Send packet
            send_subscription_packet(client_conf)

            try:
                # Check if there is avaiable data
                ready_to_read, _, _ = select.select([sock_udp], [], [], 0)
            except (select.error, OSError, IOError) as e:
                Log.error(f'Unexpected error when checking for available data on socket {sock_udp}: {e}')

            if ready_to_read:  # Data is available to be read
                data, addr = sock_udp.recvfrom(103)
                print(PDU_UDP.from_bytes(data))
        
                break # Exit subscription phase if data is received
            else:  # No data available
                # Increase sleep time
                if sleep < 3 * 1 and packets_sent >= 3:
                    sleep += 1
                time.sleep(sleep)
        else:  # If we don't receive data, (break not executed) wait and continue new subscription request.
            time.sleep(2)
            continue
        break  # Exit subscription phase if data is received
    else:
        # If loop ended print error message
        Log.info(f"Could not establish connection with server {client_conf.Server}", True)


# Initialization function
def _init():
    global sock_udp, sock_tcp, client  # Create global variables
    args = args_parser()  # Get User Initial Arguments
    Log.debug = args.d  # Set Log class to debug mode
    # Create sockets
    sock_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # UDP
    # TCP

    client = Client(args.c)  # Read and create client config


def main():
    # Start Subscription Request
    subs_request(client)

    # Close sockets
    sock_udp.close()


# Init call
if __name__ == "__main__":
    _init()
    main()
