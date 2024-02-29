#!/usr/bin/env python
"""
Description: A python program that siumaltes the interaction of a client with diferent sensors and sends it to a server.
             It uses diferent modules located in the methods folder to simplify the readability of this main program.
Author: Eric Bitria Ribes
Version: 0.1
Last Modified: 2024-2-29
"""
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
from methods import config


def recv_data(sock_udp, stop_flag):
    while not stop_flag.is_set():
        try:
            data_available, _, _ = select.select([sock_udp], [], [], 0)
        except (select.error, OSError, IOError) as e:
            logs.error(f'Unexpected error when checking for available data on socket {sock_udp}: {e}')
            return

        if data_available:
            try:
                data, addr = sock_udp.recvfrom(103)
            except Exception as e:
                logs.error(f'An error has occurred when receiving data from socket {sock_udp}: {e}')
            # Process Packet
            print(vars(pdu_udp.from_bytes(data)))
            stop_subs_flag.set()

    # Close socket
    sock_udp.close()

def stop_receiving(stop_flag):
    stop_flag.set()

######################
# Subscription Phase #
######################

def send_subscription_packet(client_conf):
    # Create SUBS_REQ packet
    packet_type = pdu_udp.packet_type['SUBS_REQ']
    # Set client MAC
    mac = client_conf['MAC']
    num = '00000000'
    # Get client data
    data = client_conf['Name']+ ',' + client_conf['Situation']
    # Create PDU_UDP packet
    
    packet = pdu_udp.to_bytes(pdu_udp.Packet(packet_type,mac,num,data))

    # Send packet
    pdu_udp.send(sock_udp, packet, client_conf)

def subs_request(client_conf):
    # Set client status
    config.set_status('WAIT_ACK_SUBS')

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
    global sock_udp, sock_tcp

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
    config.init_client(args.c)

    # Create sockets
    sock_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # UDP
    # TCP

def main():
    ## Flags
    global stop_subs_flag
    stop_subs_flag = threading.Event()
    # Start packet reception thread
    recv_thread = threading.Thread(target=recv_data, args=(sock_udp, stop_subs_flag))
    recv_thread.daemon = True
    recv_thread.start()

    # Start Subscription Request
    subs_request(config.client)

# Init call
if __name__ == "__main__":
    _init()
    main()
