#!/usr/bin/env python3

# utilities_p/config.py

"""
Module utilities_p: Subfolder that contains auxiliar utilties for the main client.py program.
Description: Used to contain client-related configurations, and utilities to initialize and modify this data.
Author: Eric Bitria Ribes
Version: 0.2
Last Modified: 2024-2-29
"""

import re
from utilities_p import logs


status = {
    'DISCONNECTED': 0xa0,
    'NOT_SUBSCRIBED': 0xa1,
    'WAIT_ACK_SUBS': 0xa2,
    'WAIT_INFO': 0xa3,
    'WAIT_ACK_INFO': 0xa4,
    'SUBSCRIBED': 0xa5,
    'SEND_HELLO': 0xa6
}
"""
Define client possible status
    'DISCONNECTED': 0xa0,
    'NOT_SUBSCRIBED': 0xa1,
    'WAIT_ACK_SUBS': 0xa2,
    'WAIT_INFO': 0xa3,
    'WAIT_ACK_INFO': 0xa4,
    'SUBSCRIBED': 0xa5,
    'SEND_HELLO': 0xa6
""" 

client = {
    'status': status['DISCONNECTED'],
    'Name': None,
    'Situation': None,
    'MAC': None,
    'Local_TCP': None,
    'Srv_UDP': None,
    'Server': None,
    'Elements': {},
    'Server_Config':{}
}
"""
Define client configurations dictionary
    'status': status['DISCONNECTED'],
    'Name': None,
    'Situation': None,
    'MAC': None,
    'Local_TCP': None,
    'Srv_UDP': None,
    'Server': None,
    'Elements': [],
    'Server_Config':[]
"""

def init_client(file_path):
    """
    Function to initialize the client dictionary.

    Parameters:
    - param (file_path): The name of the file to read the client configuration.

    Raises:
    Invalid formats for the configuration errors.
    """
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
                    key = key.replace('-','_')
                    # Cast server port to int
                    value = int(value)
                    if not 0 <= int(value) <= 65535: logs.error(f"Invalid {key} Port, must be between 0 and 65535.",True)
                elif key == 'Server':
                    pass
                else:
                    continue # Ignore any other configuration found
                client[key] = value
    except Exception as e:
        logs.error(f'Unexpected error while reading file {file_path}: {e}')



def _process_elements(elements: str) -> dict:
    """
    Auxiliar function to process the elements of the client.

    Parameters:
    - param (elements): The string containing the elements.

    Returns:
    - dict: A dictionary where each device has a name and a default value associated with None.
    """
    devices = elements.split(';')
    if len(devices) > 10:
        logs.warning("More than 10 devices detected, only the first 10 will be used.")
        devices = devices[:10]
        for device in devices:
            if not re.match(r'^[A-Z]{3}-\d{1}-[IO]$', device):
                logs.warning(f"Invalid Device Format: {device}")
    
    device_dict = {}
    for device in devices:
        device_dict[device] = "NONE"
    
    return device_dict


def set_status(name: str,show=True) -> None:
    """
    Function to set the client status:
        'DISCONNECTED': 0xa0,
        'NOT_SUBSCRIBED': 0xa1,
        'WAIT_ACK_SUBS': 0xa2,
        'WAIT_INFO': 0xa3,
        'WAIT_ACK_INFO': 0xa4,
        'SUBSCRIBED': 0xa5,
        'SEND_HELLO': 0xa6

    Parameters:
    - param (name): The name of the client status to set.
    """
    if client['status'] != status[name]:  # Check if the status is already set to the desired value
        logs.info(f"Controller Status: {name}", show)
        client['status'] = status[name]

