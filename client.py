#!/usr/bin/env python
# Args
import argparse
# Sockets
import socket, select
# Time
import time
# Threads
import threading

## Import OWN auxiliar modules
from methods import logs
from methods import pdu_udp


# Configuration Class
class Config:
    # Define client status
    status = {
        'DISCONNECTED': 0xa0,
        'NOT_SUBSCRIBED': 0Xa1,
        'WAIT_ACK_SUBS': 0Xa2,
        'WAIT_INFO': 0Xa3,
        'WAIT_ACK_INFO': 0Xa4,
        'SUBSCRIBED': 0Xa5,
        'SEND_HELLO': 0Xa6
    }

    # Status setter
    def set_status(self, status):
        self.status = Config.status[status]

    # Class constructor
    def __init__(self, file_path="client.cfg"):
        try:
            # Open file descriptor and process each config value
            with open(file_path, 'r') as file:
                for line in file:
                    line = line.strip()  # Sanitize line
                    line = line.replace(' ','')
                    # Ignore newline
                    if not line:
                        continue
                    # Split into key-value
                    key, value = line.split('=')

                    if key not in ['Name', 'Situation', 'MAC', 'Local-TCP', 'Srv-UDP', 'Server']:
                        continue  # Ignore any other configuration found

                    if key == 'Elements':
                        value = self._process_elements(value)
                    elif key in ['Local-TCP', 'Srv-UDP']:
                        key = key.replace('-', '')
                        # Cast server port to int
                        value = int(value)

                    # Set attribute
                    setattr(self, key, value)

        except Exception as e:
            logs.error(f'Unexpected error while reading file {file_path}: {e}')

        # Set default status
        self.status = Config.status['NOT_SUBSCRIBED']

    @staticmethod
    def _process_elements(elements):
        devices = elements.split(';')
        if len(devices) > 10:
            logs.warning("More than 10 devices detected, only the first 10 will be used.")
            devices = devices[:10]  # Take the first 10 devices
        return devices

def process_packet():
    stop_subs_flag.set()

# Class to create UDP and TCP packet reception threads and methods
class RecvThread(threading.Thread):

    def __init__(self, sock_udp):
        super().__init__()
        self.sock_udp = sock_udp
        self.stop_flag = threading.Event()  # Event for signaling the thread to stop
        self.daemon = True

    def run(self):
        while not self.stop_flag.is_set():
            try:
                data_available, _, _ = select.select([self.sock_udp], [], [], 0)
            except (select.error, OSError, IOError) as e:
                logs.error(f'Unexpected error when checking for available data on socket {self.sock_udp}: {e}')
                return
            
            if data_available:
                try:
                    data, addr = self.sock_udp.recvfrom(103)
                except Exception as e:
                    logs.error(f'An error has occurred when receiving data from socket {self.sock_udp}: {e}')
                # Process Packet
                decoded_packet = pdu_udp.Packet(pdu_udp.from_bytes(data))
                process_packet()

        # Close socket
        self.sock_udp.close()

    def stop(self):
        self.stop_flag.set()

######################
# Subscription Phase #
######################

def send_subscription_packet(client_conf):
    # Get client data
    data = client_conf.Name + ',' + client_conf.Situation
    # Create PDU_UDP packet
    packet_type = pdu_udp.packet_type['SUBS_REQ']
    num = '00000000'
    packet = pdu_udp.to_bytes(packet_type, client_conf.MAC, num, data)

    # Send packet
    pdu_udp.send(sock_udp, packet, client_conf)

def subs_request(client_conf):
    # Set client status
    client.set_status('WAIT_ACK_SUBS')

    for _ in range(3):
        # Print subscription request
        logs.info(f'Starting subscription request. [{_ + 1}/3]', True)
        sleep = 1
        for packets_sent in range(7):
            if stop_subs_flag.is_set():  # SUBS_ACK Received
                break # Exit subscription phase if SUBS_ACK is received
            else:
                # Send packet
                send_subscription_packet(client_conf)
                # Increase sleep time
                if sleep < 3 * 1 and packets_sent >= 3:
                    sleep += 1
                time.sleep(sleep)
        else:  # If we don't receive response from the server, (break not executed) wait and continue new subscription request.
            time.sleep(2)
            continue
        break  # Exit subscription phase if data is received
    else:
        # If subscription attempts ended print error message
        logs.error(f"Could not establish connection with server {client_conf.Server}", True)

# Initialization function
def _init():
    # Create global variables
    global sock_udp, sock_tcp, client

    #Get User Arguments
    parser = argparse.ArgumentParser(usage="client.py [-h] [-c client_config.cfg] [-d]")
    # Define Optional arguments
    parser.add_argument('-c', type=str, help="Path to config file", default="client.cfg", metavar="config file")
    parser.add_argument('-d', action='store_true', help="Enable debugging mode")
    # Return user args object
    args = parser.parse_args()

    # Set Log class to debug mode
    logs.debug = args.d  
    # Read and create client config
    client = Config(args.c)  

    # Create sockets
    sock_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # UDP
    # TCP

    ## Flags
    global stop_subs_flag
    stop_subs_flag = threading.Event()

def main():
    # Start packet reception thread
    packet_thread = RecvThread(sock_udp)
    packet_thread.start()

    # Start Subscription Request
    subs_request(client)

# Init call
if __name__ == "__main__":
    _init()
    main()
