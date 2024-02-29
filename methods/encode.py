# methods/encode.py

"""
Module: methods: Subfolder that contains auxiliar methods for the main client.py program
Description: Basic methods to convert strings to bytes and bytes to strings.
Author: Eric Bitria Ribes
Version: 0.1
Last Modified: 2024-2-29
"""

"""
A function to encode a string into a byte array.

Parameters:
- param (s): The string to encode.
- param (lenght): The lenght of the string/final byte array

Returns:
- byte array: The string encoded in a byte array.
"""
def string_to_bytes(s, length):
    return s[:length].ljust(length, '\0').encode('utf-8')

"""
A function to decode a byte array into a string.

Parameters:
- param (byte): The byte array.
- param (offset): Number of positions moved from the start of the array.
- param (lenght): The lenght of the byte array.

Returns:
- String: The string decoded from the byte array.
"""
# 
def bytes_to_string(byte, offset, length):
    return byte[offset:offset + length].decode('utf-8').rstrip('\0')