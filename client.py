#!/usr/bin/env python
"""
Description: A python program that siumaltes the interaction of a client with diferent sensors and sends it to a server.
             It uses diferent modules located in the methods folder to simplify the readability of this main program.
Author: Eric Bitria Ribes
Version: 0.1
Last Modified: 2024-3-1
"""
# Args
import argparse
# Sockets
import socket, select
# Time
import time
# Threads
import threading

## Import auxiliar modules
from methods import logs
from methods import pdu_udp
from methods import config


def stop_receiving(stop_flag):
    stop_flag.set()

def udp_recv(sock_udp, stop_flag):
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
            process_packet(pdu_udp.from_bytes(data),addr)

    # Close socket
    sock_udp.close()

def process_packet(packet,addr):
    # Get packet type
    ptype = packet.packet_type

    # Excute process depending on the packet received
    if ptype == 0x00: # CLIENT SHOULD NOT RECEIVE THIS ONE
        logs.info("Received unexpected [SUBS_REQ] packet. Ignoring...")
    elif ptype == 0x01:
        process_subs_ack(packet,addr)
    elif ptype == 0x02:
        print("2")

    # Set Flags
    stop_subs_flag.set()

######################
# Subscription Phase #
######################
    
def process_subs_ack(packet,addr):
    """
    Processes a [SUBS_ACK] packet and if the client status is correct 
    it will send a new [SUBS_INFO] packet to the server

    PACKET CONTENTS:

    | [SUBS_ACK] | MAC | RAND_NUM | DATA: (NEW UDP PORT) |

    Parameters:
    - param (packet): The [SUBS_ACK] packet to analyze
    - param (addr): The addres to send the response packet [SUBS_INFO]

    Raises:
    Warning if it receives this packet when the client is not in WAIT_ACK_SUBS status
    """
    if config.client['status'] != 0xa2:
        logs.warning('Shouldn\'t have received [SUBS_ACK] packet, Ignoring.. ')
    else:
        # Store Server Information, MAC, Server IP, RAND
        server_config = [packet.mac,addr[0],packet.rnd]
        config.client['Server_Config'] = server_config

        # GET UDP Port From Packet
        udp = 11001

        # Send [SUBS_INFO] packet
        packet_type = pdu_udp.packet_type['SUBS_INFO']
        mac = config.client['MAC']
        num = config.client['Server_Config'][2]
        # Create Packet data: convert TCP port to string, add "," separator, join each Element into a string (CHANGE-IT IN THE FUTURE)
        data = str(config.client['Local_TCP']) + ',' + ';'.join(config.client['Elements'])

        # Create and encode packet
        spacket = pdu_udp.to_bytes(pdu_udp.Packet(packet_type,mac,num,data))

        pdu_udp.send(sock_udp,spacket,config.client['Server'],udp)

def send_subscription_packet():
    # Create SUBS_REQ packet
    packet_type = pdu_udp.packet_type['SUBS_REQ']
    # Set client MAC
    mac = config.client['MAC']
    num = '00000000'
    # Get client data
    data = config.client['Name']+ ',' + config.client['Situation']
    # Create PDU_UDP packet
    
    packet = pdu_udp.to_bytes(pdu_udp.Packet(packet_type,mac,num,data))

    # Send packet
    pdu_udp.send(sock_udp, packet, config.client['Server'], config.client['Srv_UDP'])

def subs_request():
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
                send_subscription_packet()
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
        logs.error(f"Could not establish connection with server {config.client['Server']}", True)

# Initialization function
def _init():
    # Create global variables for sockets
    global sock_udp, sock_tcp

    ## Get User Arguments
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
    recv_thread = threading.Thread(target=udp_recv, args=(sock_udp, stop_subs_flag))
    recv_thread.daemon = True
    recv_thread.start()

    # Start Subscription Request
    subs_request()

# Init call
if __name__ == "__main__":
    _init()
    main()
