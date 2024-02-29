#!/usr/bin/env python

# methods/config.py

"""
Module Methods: Subfolder that contains auxiliar methods for the main client.py program.
Description: Used to contain client-related configurations, and methods to initialize and modify this data.
Author: Eric Bitria Ribes
Version: 0.1
Last Modified: 2024-2-29
"""

import re
from methods import logs

'''Define client possible status''' 
status = {
    'DISCONNECTED': 0xa0,
    'NOT_SUBSCRIBED': 0Xa1,
    'WAIT_ACK_SUBS': 0Xa2,
    'WAIT_INFO': 0Xa3,
    'WAIT_ACK_INFO': 0Xa4,
    'SUBSCRIBED': 0Xa5,
    'SEND_HELLO': 0Xa6
}

'''Define client configurations dictionary'''
client = {
    'status': status['DISCONNECTED'],
    'file_path': "client.cfg",
    'Name': None,
    'Situation': None,
    'MAC': None,
    'Local_TCP': None,
    'Srv_UDP': None,
    'Server': None,
    'Elements': []
}

"""
Function to initialize the client dictionary.

Parameters:
- param (file_path): The name of the file to read the client configuration.
"""
def init_client(file_path):
    global client
    try:
        with open(file_path, 'r') as file:
            for line in file:
                # Sanitize lines
                line = line.strip()
                line = line.replace(' ', '')
                key, value = line.split('=') 
                if key == 'Name':
                    if not len(value) == 8: logs.warning("Name must be 8 alphanumeric characters long.",True)
                elif key == 'Situation':
                    if not re.match(r'^B\d{2}L\d{2}R\d{2}A\d{2}$', value): logs.warning("Invalid Situation Format.",True)
                elif key == 'Elements':
                    value = _process_elements(value)
                elif key == 'MAC':
                    if not re.match(r'^([0-9a-fA-F]{12})$', value): logs.error("Invalid MAC address.",True)
                elif key in ['Local-TCP', 'Srv-UDP']:
                    key = key.replace('-','')
                    # Cast server port to int
                    value = int(value)
                    if not 0 <= int(value) <= 65535: logs.error(f"Invalid {key} Port, must be between 0 and 65535.",True)
                elif key == 'Server':
                    if not re.match(r'^(?:25[0-5]|2[0-4]\d|[01]?\d{1,2})(?:\.(?:25[0-5]|2[0-4]\d|[01]?\d{1,2})){3}$', value): logs.error("Invalid Server IP.",True)
                else:
                    continue # Ignore any other configuration found
                client[key] = value
    except Exception as e:
        logs.error(f'Unexpected error while reading file {file_path}: {e}')

"""
Auxiliar function to proces the elements of the client.

Parameters:
- param (elements): The string containing the elements.
"""
def _process_elements(elements):
    devices = elements.split(';')
    if len(devices) > 10:
        logs.warning("More than 10 devices detected, only the first 10 will be used.")
        devices = devices[:10]
    return devices

"""
Function to set the client status

Parameters:
- param (name): The name of the client status to set.
"""
def set_status(name):
    client['status'] = status[name]

