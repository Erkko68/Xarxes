#!/usr/bin/env python

# methods/logs.py

"""
Module Methods: Subfolder that contains auxiliar methods for the main client.py program.
Description: Used to contain all PDU_UDP packet-related methods for the main client.py program.
Author: Eric Bitria Ribes
Version: 0.2
Last Modified: 2024-2-29
"""

# Import codification module
from methods import encode
from methods import logs

"""
Define a list with all possible packet types.
"""
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
            logs.error('Invalid PDU_UDP packet initalitation.')

"""
Function that converts a pdu_udp packet to a byte array

Parameters:
- param (packet): The UDP_PDU Packet object containing all its attributes.
Returns:
- A byte array
"""
def to_bytes(packet):
    uchar = bytes([packet.packet_type])
    # Convert the MAC address to bytes
    mac_bytes = encode.string_to_bytes(packet.mac, 13)
    # Convert the 9 bytes long number to bytes
    rand = encode.string_to_bytes(packet.rnd, 9)
    # Convert the data to bytes
    data = encode.string_to_bytes(packet.data, 80)

    # Pack the data into a byte array
    return uchar + mac_bytes + rand + data

"""
Function that converts a pdu_udp packet to a byte array

Parameters:
- param (buffer): A byte array representing the packet.
Returns:
- A new PDU_UDP Packet Object
"""
def from_bytes(buffer):
    # Unpack the data from the byte array
    # Get type
    packet_type = buffer[0]
    # Convert the MAC address bytes to a MAC address string
    mac = encode.bytes_to_string(buffer, 1, 13)
    # Ensure the rand number and data string are the correct length
    rand = encode.bytes_to_string(buffer, 14, 9)
    data = encode.bytes_to_string(buffer, 23, 80)

    return Packet(packet_type,mac,rand,data)

"""
Function to send PDU_UDP packet to the specified socket. If the packet hasn't 
been sent completly in the first try, sends the remaining information.

Parameters:
- param (udp_sockfd): The socket where to send the packet
- param (packet): The packet to send
- param (client_conf): THe client object containing the necessary information of the client
Throws:
- Exception if the sendto function fails
"""
def send(udp_sockfd, packet, client_conf):
    try:
        byte_count = 0
        while byte_count < len(packet[byte_count:]):
            byte_count = udp_sockfd.sendto(packet, (client_conf['Server'], client_conf['SrvUDP']))
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