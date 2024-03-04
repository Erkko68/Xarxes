#!/usr/bin/env python

# methods/encode.py

"""
Module Methods: Subfolder that contains auxiliar methods for the main client.py program.
Description: Basic methods to convert strings to bytes and bytes to strings.
Author: Eric Bitria Ribes
Version: 0.1
Last Modified: 2024-2-29
"""

def string_to_bytes(s, length):
    """
    A function to encode a string into a byte array.

    Parameters:
    - param (s): The string to encode.
    - param (lenght): The lenght of the string/final byte array

    Returns:
    - byte array: The string encoded in a byte array.
    """
    return (s[:length-1]).ljust(length,'\0').encode('utf-8')

def bytes_to_string(buffer, length):
    """
    A function to decode a byte array into a string until the null character is encountered.

    Parameters:
    - param (buffer): The byte array.
    - param (length): The length of the byte array.

    Returns:
    - String: The string decoded from the byte array until the null character.
    """
    null_index = buffer.find(b'\x00')  # Find the index of the first null character
    if null_index != -1 and null_index < length:
        length = null_index  # Adjust length to decode until the null character
    return buffer[:length].decode('utf-8')

