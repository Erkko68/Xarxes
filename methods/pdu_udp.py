# methods/logs.py

"""
Module: methods: Subfolder that contains auxiliar methods for the main client.py program
Description: Used to contain all PDU_UDP packet related methods for the main client.py program
Author: Eric Bitria Ribes
Version: 0.1
Last Modified: 2024-2-29
"""

# Import codification module
from methods import encode
from methods import logs

# Define a list with all possible packet types

packet_type = {
    'SUBS_REQ': 0x00,
    'SUBS_ACK': 0X01,
    'SUBS_REJ': 0X02,
    'SUBS_INFO': 0X03,
    'INFO_ACK': 0X04,
    'SUBS_NACK': 0X05
}

# 
class Packet:
    # This allows passing an array or each param individually
    def __init__(self, *args):
        if len(args) == 1:
            packet = args[0]
            self.packet_type = packet[0]
            self.mac = packet[1]
            self.rnd = packet[2]
            self.data = packet[3]
        elif len(args) == 4:
            self.packet_type = args[0]
            self.mac = args[1]
            self.rnd = args[2]
            self.data = args[3]
        else:
            logs.error('INvalid PDU_UDP packet initalitation.')

# A function that converts strings values to PDU_UDP format as a byte array

def to_bytes(uchar, mac, rand, data):
    uchar = bytes([uchar])
    # Convert the MAC address to bytes
    mac_bytes = encode.string_to_bytes(mac, 13)
    # Convert the 9 bytes long number to bytes
    rand = encode.string_to_bytes(rand, 9)
    # Convert the data to bytes
    data = encode.string_to_bytes(data, 80)

    # Pack the data into a byte array
    return uchar + mac_bytes + rand + data


def from_bytes(buffer):
    # Unpack the data from the byte array
    # Get type
    uchar = buffer[0]
    # Convert the MAC address bytes to a MAC address string
    mac = encode.bytes_to_string(buffer, 1, 13)
    # Ensure the rand number and data string are the correct length
    rand = encode.bytes_to_string(buffer, 14, 9)
    data = encode.bytes_to_string(buffer, 23, 80)

    return uchar, mac, rand, data


def send(udp_sockfd, packet, client_conf):
    try:
        byte_count = 0
        while byte_count < len(packet[byte_count:]):
            byte_count = udp_sockfd.sendto(packet, (client_conf.Server, client_conf.SrvUDP))
            # Print information
            logs.info(f'Sent {byte_count} bytes during {logs.get_key(packet[0], packet_type)}.')
            # Check if all bytes were sent
            if byte_count < len(packet[byte_count:]):
                # If not all bytes were sent truncate the packet and send remaining information.
                logs.warning(f'Not all bytes were sent during {logs.get_key(packet[0], packet_type)}!, sending remaining information...')
                packet = packet[byte_count:]

        logs.info(f'{logs.get_key(packet[0], packet_type)} packet sent successfully.')

    except Exception as e:
        logs.error(f'Unexpected error when sending {logs.get_key(packet[0], packet_type)}: {e}',True)