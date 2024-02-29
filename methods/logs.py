# methods/logs.py

"""
Module: methods: Subfolder that contains auxiliar methods for the main client.py program
Description: Basic methods to print info, warning, and error messages for the main client.py program
Author: Eric Bitria Ribes
Version: 0.1
Last Modified: 2024-2-29
"""

COLORS = {'red': '\033[91m', 'yellow': '\033[93m', 'blue': '\033[94m', 'end': '\033[0m'}

# Set boolean in case the user uses -d argument
debug = False

"""
General: Functions to print colored prefix messages

Parameters:
- param (text): The string to print.
- param (override): IF set to True it will always display the message
                    regardles of the debug mode being set.
"""


def _colored_message(text, color):
    return f"{COLORS[color]}{text}{COLORS['end']}"


def warning(message, override=False):
    msg = _colored_message("[WARNING]", 'yellow') + " " + message
    if debug or override:
        print(msg)


def info(message, override=False):
    msg = _colored_message("[INFO]", 'blue') + " " + message
    if debug or override:
        print(msg)


def error(message, override=False):
    msg = _colored_message("[ERROR]", 'red') + " " + message
    if debug or override:
        print(msg)
    exit(-1)

# Only used to obtain the name of the type of a dictionary, to show in the message
def get_key(val, dictionary):
    for key, value in dictionary.items():
        if val == value:
            return key