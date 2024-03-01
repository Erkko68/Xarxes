#!/usr/bin/env python

# methods/logs.py

"""
Module Methods: Subfolder that contains auxiliar methods for the main client.py program.
Description: Basic methods to print info, warning, and error messages for the main client.py program.
Author: Eric Bitria Ribes
Version: 0.2
Last Modified: 2024-2-29
"""

COLORS = {'red': '\033[91m', 'yellow': '\033[93m', 'blue': '\033[94m', 'end': '\033[0m'}

# Set boolean in case the user uses -d argument
debug = False


def _colored_message(text, color):
    """
    Function to print colored prefix and messages

    Parameters:
    - param (text): The string to print.
    - param (color): The color to print
                        regardles of the debug mode being set.
    """
    return f"{COLORS[color]}{text}{COLORS['end']}"

def warning(message, override=False):
    """
    Function to print warning messages

    Parameters:
    - param (text): The string to print.
    - param (override): If set to True it will always display the message
                        regardles of the debug mode being set.
    """
    msg = _colored_message("[WARNING]", 'yellow') + " " + message
    if debug or override:
        print(msg)


def info(message, override=False):
    """
    Function to print info messages

    Parameters:
    - param (text): The string to print.
    - param (override): If set to True it will always display the message
                        regardles of the debug mode being set.
    """
    msg = _colored_message("[INFO]", 'blue') + " " + message
    if debug or override:
        print(msg)

def error(message, override=False):
    """
    Function to print warning messages

    Parameters:
    - param (text): The string to print.
    - param (override): If set to True it will always display the message
                        regardles of the debug mode being set.
    """
    msg = _colored_message("[ERROR]", 'red') + " " + message
    if debug or override:
        print(msg)
    exit(-1)

def get_key(val, dictionary):
    """
    Function to obtain the name of the type of a dictionary, to show in the message

    Parameters:
    - param (val): The value of the dictionary param.
    - param (dictionary): THe dictinoary where to obtain the name of the value.
    """
    for key, value in dictionary.items():
        if val == value:
            return key