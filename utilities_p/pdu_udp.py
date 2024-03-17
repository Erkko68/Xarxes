#!/usr/bin/env python3

# utilities_p/pdu_udp.py

"""
Module Methods: Subfolder that contains auxiliar methods for the main client.py program.
Description: Used to contain all PDU_UDP packet-related methods for the main client.py program.
Author: Eric Bitria Ribes
Version: 0.3
Last Modified: 2024-3-1
"""
import socket

# Import codification module
from utilities_p import encode
from utilities_p import logs


packet_type = {
    'SUBS_REQ': 0x00,
    'SUBS_ACK': 0x01,
    'SUBS_REJ': 0x02,
    'SUBS_INFO': 0x03,
    'INFO_ACK': 0x04,
    'SUBS_NACK': 0x05,
    'HELLO': 0x10,
    'HELLO_REJ': 0x11
}
"""
Define a dictionary with all possible packet types:
    'SUBS_REQ': 0x00,
    'SUBS_ACK': 0x01,
    'SUBS_REJ': 0x02,
    'SUBS_INFO': 0x03,
    'INFO_ACK': 0x04,
    'SUBS_NACK': 0x05
"""

class Packet:
    """
    A class to create a packet object

    Parameters:
    - param (*args): It can receive an list of 4 elements, or 4 elements, 
                    this way with a single list we can obtain a new 
                    packet object from the funciton from_bytes

    Attributes:
    - Packet Type: The type of the packet defined in the dictionary packet_type
    - MAC: THe MAC adress.
    - RND: a random number.
    - DATA: The data to be transimted by the packet.

    Returns:
    - A new Packet object
    """
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


def to_bytes(packet: Packet) -> bytes:
    """
    Function that converts a pdu_udp packet to a byte array

    Parameters:
    - param (packet): The UDP_PDU Packet object containing all its attributes.
    Returns:
    - A byte array
    """
    uchar = bytes([packet.packet_type])
    # Convert the MAC address to bytes
    mac_bytes = encode.string_to_bytes(packet.mac, 13)
    # Convert the 9 bytes long number to bytes
    rand = encode.string_to_bytes(packet.rnd, 9)
    # Convert the data to bytes
    data = encode.string_to_bytes(packet.data, 80)

    # Pack the data into a byte array
    return uchar + mac_bytes + rand + data


def from_bytes(buffer: bytes) -> Packet:
    """
    Function that converts a byte array to a pdu_udp packet.

    Parameters:
    - param (buffer): A byte array representing the packet.
    Returns:
    - A new PDU_UDP Packet Object
    """
    # Unpack the data from the byte array
    # Get type
    packet_type = buffer[0]
    # Convert the MAC address bytes to a MAC address string
    mac = encode.bytes_to_string(buffer[1:], 13)
    # Ensure the rand number and data string are the correct length
    rand = encode.bytes_to_string(buffer[14:], 9)
    data = encode.bytes_to_string(buffer[23:], 80)

    return Packet(packet_type, mac, rand, data)


def send(udp_sockfd: socket.socket, packet: bytes, server: str, port: int) -> None:
    """
    Function to send PDU_UDP packet to the specified socket. If the packet hasn't 
    been sent completely in the first try, sends the remaining information.

    Parameters:
    - param (udp_sockfd): The socket where to send the packet.
    - param (packet): The packet to send.
    - param (server): The server IP address.
    - param (port): The port to send the packet.

    Raises:
    - Exception if the sendto function fails.
    """
    try:
        byte_count = 0
        while byte_count < len(packet):
            byte_count = udp_sockfd.sendto(packet, (server, port))
            # Print information
            # logs.info(f'Sent {byte_count} bytes during {logs.get_key(packet[0], packet_type)}.')
            # Check if all bytes were sent
            if byte_count < len(packet):
                # If not all bytes were sent truncate the packet and send remaining information.
                logs.warning(f'Not all bytes were sent during {logs.get_key(packet[0], packet_type)}!, sending remaining information...')
                packet = packet[byte_count:]

        #logs.info(f'{logs.get_key(packet[0], packet_type)} packet sent successfully.')

    except Exception as e:
        logs.error(f'Unexpected error when sending {logs.get_key(packet[0], packet_type)}: {e}', True)


def recvUDP(sock_udp: socket.socket) -> tuple[bytes, tuple[str,str]]:
    """
    Receive data from a UDP socket.

    Parameters:
    - param (sock_udp): The UDP socket from which to receive data.

    Returns:
    - A tuple containing:
        - Either a Packet object if data is received successfully, or None if there's a timeout.
        - Either a tuple containing the received data and the address from which it was received, 
          or None if there's a timeout or an error occurs.
    """
    try:
        data, addr = sock_udp.recvfrom(103)

    except socket.timeout:
        return None, None
    except Exception as e:
        logs.error(f'An error has occurred when receiving data from socket {sock_udp}: {e}')
    
    return from_bytes(data), addr
