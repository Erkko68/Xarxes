#!/usr/bin/env python3

# utilities_p/logs.py

"""
Module Methods: Subfolder that contains auxiliar methods for the main client.py program.
Description: Used to contain all PDU_TCP packet-related methods for the main client.py program.
Author: Eric Bitria Ribes
Version: 0.1
Last Modified: 2024-3-17
"""

import socket

# Import codification module
from utilities_p import encode
from utilities_p import logs


packet_type = {
    'SEND_DATA': 0x20,
    'SET_DATA': 0x21,
    'GET_DATA': 0x22,
    'DATA_ACK': 0x23,
    'DATA_NACK': 0x24,
    'DATA_REJ': 0x25
}
"""
Define a dictionary with all possible packet types:
    'SEND_DATA': 0x20,
    'SET_DATA': 0x21,
    'GET_DATA': 0x22,
    'DATA_ACK': 0x23,
    'DATA_NACK': 0x24,
    'DATA_REJ': 0x25
"""

class Packet:
    def __init__(self,ptype: int,mac: str,rnd: str,device: str,val: str, info: str):
        self.ptype = ptype
        self.mac = mac
        self.rnd = rnd
        self.device = device
        self.val = val
        self.info = info


def to_bytes(packet: Packet) -> bytes:
    """
    Function that converts a pdu_tcp packet to a byte array

    Parameters:
    - param (packet): The PDU_TCP Packet object containing all its attributes.
    Returns:
    - A byte array
    """
    uchar = bytes([packet.packet_type])
    mac_bytes = encode.string_to_bytes(packet.mac, 13)
    rand = encode.string_to_bytes(packet.rnd, 9)
    device = encode.string_to_bytes(packet.device,8)
    val = encode.string_to_bytes(packet.val,7)
    info = encode.string_to_bytes(packet.info, 80)

    # Pack the data into a byte array
    return uchar + mac_bytes + rand + device + val + info


def from_bytes(buffer: bytes) -> Packet:
    """
    Function that converts a byte array to a pdu_tcp packet.

    Parameters:
    - param (buffer): A byte array representing the packet.
    Returns:
    - A new PDU_TCP Packet Object
    """
    # Unpack the data from the byte array
    # Get type
    packet_type = buffer[0]
    mac = encode.bytes_to_string(buffer[1:], 13)
    rand = encode.bytes_to_string(buffer[14:], 9)
    device = encode.bytes_to_string(buffer[23:], 8)
    value = encode.bytes_to_string(buffer[31:], 6)
    info = encode.bytes_to_string(buffer[37:], 80)

    return Packet(packet_type, mac, rand, device, value, info)