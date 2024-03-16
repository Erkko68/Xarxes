#!/usr/bin/env python3

"""
Description: A python program that simulates the interaction of a client with diferent sensors and sends it to a server.
             It uses diferent modules located in the utilities_p folder to simplify the readability of this main program.
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
from utilities_p import logs
from utilities_p import pdu_udp
from utilities_p import config

######################
# Subscription Phase #
######################
        
def send_subs_req(server,port):
    """
    Sends a [SUBS_REQ] to the server and port with UDP protocol.

    PACKET CONTENTS:

    | [SUBS_REQ] | MAC | 00000000 | DATA: CLIENT NAME + SITUATION |

    Parameters:
    - param (server): The server adress to send the packet.
    - param (port): The UDP Port of the server.
    """
    # Create SUBS_REQ packet
    packet_type = pdu_udp.packet_type['SUBS_REQ']
    # Set client MAC
    mac = config.client['MAC']
    num = '00000000'
    # Get client data
    data = config.client['Name']+ ',' + config.client['Situation']

    # Create and encode PDU_UDP packet
    packet = pdu_udp.to_bytes(pdu_udp.Packet(packet_type,mac,num,data))

    # Send packet
    pdu_udp.send(sock_udp, packet, server, port)

def send_subs_info(packet: pdu_udp.Packet) -> None:
    """
    Sends [SUBS_INFO] packet.

    Parameters:
    - param (packet): The SUBS_ACK packet containing the new UDP port

    Returns:
    - None
    """
    # Extract packet type, MAC, and random number from configuration
    packet_type: int = pdu_udp.packet_type['SUBS_INFO']
    mac: str = config.client['MAC']
    rnd: str = config.client['Server_Config']['rnd']
    
    # Create Packet data: convert TCP port to string, add "," separator, join each Element into a string (CHANGE-IT IN THE FUTURE)
    data: str = str(config.client['Local_TCP']) + ',' + ';'.join(config.client['Elements'])

    # Create and encode packet
    spacket: bytes = pdu_udp.to_bytes(pdu_udp.Packet(packet_type, mac, rnd, data))

    # Get UDP Port From Packet
    udp: int = int(packet.data)

    # Send packet
    pdu_udp.send(sock_udp, spacket, config.client['Server_Config']['IP'], udp)

def subs_process(subs_attempts: int) -> bool:
    """
    Process for handling subscription requests.

    Parameters:
    - param (subs_attempts): The number of subscription attempts made.

    Returns:
    - bool: True if subscription is successful, False otherwise.
    """
    t = 1; u = 2; n = 7; o = 3; p = 3; q = 3
    wait = t
    if subs_attempts < o:
        subs_attempts += 1
        # Print subscription request msg
        logs.info(f'Starting subscription request. [{subs_attempts}/{o}]', True)
        for packets_sent in range(n):
            # Set timeout
            sock_udp.settimeout(wait)
            # Send packet
            send_subs_req(config.client['Server'], config.client['Srv_UDP'])
            # Update client status
            config.set_status('WAIT_ACK_SUBS')
            # Wait "t" seconds or until a SUBS_ACK packet is received
            subs_ack, address = pdu_udp.recvUDP(sock_udp)
            # No packet received
            if subs_ack == None:
                # Increase wait time if a packet is not received
                if wait <= q * t and packets_sent+1 >= p:
                    wait += t

            elif subs_ack.packet_type == pdu_udp.packet_type['SUBS_ACK']:
                # Store server info
                config.client['Server_Config']['MAC'] = subs_ack.mac
                config.client['Server_Config']['IP'] = address[0]
                config.client['Server_Config']['rnd'] = subs_ack.rnd
                # Send SUBS_INFO
                send_subs_info(subs_ack)
                config.set_status('WAIT_ACK_INFO')
                # Exit loop and start WAIT_ACK_INFO process (below)
                break

            elif subs_ack.packet_type == pdu_udp.packet_type['SUBS_NACK']:
                config.set_status('NOT_SUBSCRIBED')
                continue

            elif subs_ack.packet_type == pdu_udp.packet_type['SUBS_REJ']:
                logs.info("Received SUBS_REJ")
                config.set_status('NOT_SUBSCRIBED')
                # Start new subscription process
                subs_process(subs_attempts)
                break

        else:  # If we don't receive response from the server, wait and continue new subscription request.
            time.sleep(u)
            # Set client status
            config.set_status('NOT_SUBSCRIBED')
            # Call new subs_process
            subs_process(subs_attempts)
    else:
        config.set_status('DISCONNECTED')
        # If subscription attempts ended exit function
        logs.error(f"Could not establish connection with server {config.client['Server']}", True)
    
    # WAIT_ACK_INFO process
    if config.client['status'] == config.status['WAIT_ACK_INFO']:
        info_ack, address = pdu_udp.recvUDP(sock_udp)
        # Check server information
        if (info_ack.packet_type == pdu_udp.packet_type['INFO_ACK'] and
            info_ack.mac == config.client['Server_Config']['MAC'] and 
            info_ack.rnd == config.client['Server_Config']['rnd']):
            # Store server TCP Port
            config.client['Server_Config']['TCP'] = info_ack.data
            config.set_status('SUBSCRIBED')
            return True
        config.set_status('NOT_SUBSCRIBED')
        subs_process(subs_attempts)

##########################
# Subscription Phase END #
##########################
        
#################
# Hello Process #
#################
        
def hello_send_thread():
    # Create HELLO packet
    hello_packet = pdu_udp.Packet(pdu_udp.packet_type['HELLO'], 
                                  config.client['MAC'], 
                                  config.client['Server_Config']['rnd'], 
                                  config.client['Name'] + ',' + config.client['Situation']
                                  )
    
    while not disconnected.is_set():
        pdu_udp.send(sock_udp, pdu_udp.to_bytes(hello_packet), config.client['Server_Config']['IP'], int(config.client['Srv_UDP']))
        time.sleep(2)
    
    # Exit Hello status
    return False


def hello_recv_thread():
    # Set max wait time
    sock_udp.settimeout(2)
    # Set maximum packets missed
    missed: int = 0
    while missed < 3:
        hello, addr = pdu_udp.recvUDP(sock_udp)
        if hello == None:
            missed += 1
        else:
            missed = 0
    # Lost connection with server, stopping reception thread
    disconnected.set()
    return


def hello_process_thread():
    send_hello = threading.Thread(target=hello_send_thread)


#####################
# Hello Process END #
#####################

# Initialization function
def _init_():
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
    global subs_attempts
    global disconnected
    disconnected = threading.Event()
    subs_attempts = 0
    # Start Subscription Request
    if subs_process(subs_attempts):
        print("Subscribed")

# Init call
if __name__ == "__main__":
    _init_()
    main()
