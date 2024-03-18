#!/usr/bin/env python3

"""
Description: A python program that simulates the interaction of a client with diferent sensors and sends it to a server.
             It uses diferent modules located in the utilities_p folder to simplify the readability of this main program.
Author: Eric Bitria Ribes
Version: 0.4
Last Modified: 2024-3-1
"""

# Args
import sys
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
from utilities_p import pdu_tcp

######################
# Subscription Phase #
######################
        
def send_subs_req(server: str, port: int) -> None:
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
    
    # Create Packet data: convert TCP port to string, add "," separator, join each Element into a string
    data: str = str(config.client['Local_TCP']) + ',' + ';'.join(config.client['Elements'].keys())

    # Create and encode packet
    spacket: bytes = pdu_udp.to_bytes(pdu_udp.Packet(packet_type, mac, rnd, data))

    # Get UDP Port From Packet
    udp: int = int(packet.data)

    # Send packet
    pdu_udp.send(sock_udp, spacket, config.client['Server_Config']['IP'], udp)

def subs_process() -> bool:
    """
    Process for handling subscription requests.

    Returns:
    - bool: True if subscription is successful, False otherwise.
    """
    global subs_attempts
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
                if wait <= q * t and packets_sent+1 >= p:
                    wait += t
                continue

            elif subs_ack.packet_type == pdu_udp.packet_type['SUBS_REJ']:
                logs.info("Received SUBS_REJ")
                config.set_status('NOT_SUBSCRIBED')
                # Start new subscription process
                subs_process()
                break

        else:  # If we don't receive response from the server, wait and continue new subscription request.
            time.sleep(u)
            # Set client status
            config.set_status('NOT_SUBSCRIBED')
            # Call new subs_process
            subs_process()
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
        subs_process()

##########################
# Subscription Phase END #
##########################
        
#################
# Hello Process #
#################

def check_hello(packet: pdu_udp.Packet, address) -> bool:
    """
    Check if the received HELLO packet matches the data configuration.

    Parameters:
    - packet (pdu_udp.Packet): The received packet.
    - address (tuple): The address from which the packet was received.

    Returns:
    - bool: True if the packet matches the data configuration, False otherwise.
    """
    return (packet.mac == config.client['Server_Config']['MAC'] and 
            packet.rnd == config.client['Server_Config']['rnd'] and 
            address[0] == config.client['Server_Config']['IP'])

def send_hello_rej(hello: pdu_udp.Packet) -> None:
    """
    Send a HELLO_REJ packet in response to a wrongly formatted HELLO packet.

    Parameters:
    - hello (pdu_udp.Packet): The incorrectly formatted HELLO packet.
    """
    hello.packet_type = pdu_udp.packet_type['HELLO_REJ']
    logs.info("Received wrong HELLO packet data.")
    config.set_status('NOT_SUBSCRIBED')
    pdu_udp.send(sock_udp, pdu_udp.to_bytes(hello), config.client['Server_Config']['IP'], int(config.client['Srv_UDP']))
    disconnected.set()

def hello_recv_thread():
    """
    Thread function for receiving and processing HELLO packets.
    """
    # Set first max wait time (2 times original value)
    sock_udp.settimeout(4)
    hello, addr = pdu_udp.recvUDP(sock_udp)
    if hello is None:
        disconnected.set()
        return
    elif check_hello(hello, addr):
        # Set status
        config.set_status('SEND_HELLO')
    else:
        send_hello_rej(hello)
        return
    # Set default timeout
    sock_udp.settimeout(2)

    # Set maximum packets missed
    missed = 0
    while missed < 3:
        hello, addr = pdu_udp.recvUDP(sock_udp)
        if hello is None:
            missed += 1
        else:
            if check_hello(hello, addr):
                missed = 0
            else:
                break
    else:   
        # If reached here lost connection with server, stopping reception thread
        logs.info("Server hasn't sent 3 consecutive HELLO packets.",True)
        config.set_status('NOT_SUBSCRIBED')
        disconnected.set()
        return

    # If reached here controller received wrong HELLO packet data, sending HELLO_REJ
    send_hello_rej(hello)

def hello_process_thread():
    """
    Thread function for sending periodic HELLO packets.
    """
    # Clear flag
    disconnected.clear()
    # Start reception thread
    recv_hello = threading.Thread(target=hello_recv_thread,daemon=True)
    recv_hello.start()
    # Create HELLO packet
    hello_packet = pdu_udp.Packet(pdu_udp.packet_type['HELLO'], 
                                  config.client['MAC'], 
                                  config.client['Server_Config']['rnd'], 
                                  config.client['Name'] + ',' + config.client['Situation']
                                  )
    # Start periodic hello send
    while not disconnected.is_set():
        pdu_udp.send(sock_udp, pdu_udp.to_bytes(hello_packet), config.client['Server_Config']['IP'], int(config.client['Srv_UDP']))
        time.sleep(2)

#####################
# Hello Process END #
#####################

################
# Data Process #
################
        
def set_data(packet: pdu_tcp.Packet, socket: socket.socket) -> None:
    """
    Processes a SET_DATA request and updates the device value if it exists in the client's configuration.
    Sends a corresponding data packet notification if the device is a sensor.

    Parameters:
    - packet (pdu_tcp.Packet): The received SET_DATA request packet.
    - socket (socket.socket): The socket used for communication.

    Returns:
    - None
    """
    if packet.device in config.client['Elements'].keys():
        if packet.device[-1] == "I":
            config.client['Elements'][packet.device] = packet.val
            logs.info(f"Device {packet.device} updated with value: {packet.val}.")

            pdu_tcp.send(socket,
                         pdu_tcp.to_bytes(
                             pdu_tcp.Packet(
                                 pdu_tcp.packet_type['DATA_ACK'],
                                 config.client['MAC'],
                                 config.client['Server_Config']['rnd'],
                                 packet.device,packet.val,""
                                )
                            )
                        )
        else:
            logs.warning("Device is a sensor and can't be assigned with values.")
            pdu_tcp.send(socket,
                         pdu_tcp.to_bytes(pdu_tcp.Packet(pdu_tcp.packet_type['DATA_NACK'],config.client['MAC'],config.client['Server_Config']['rnd'],"","","Device is a sensor and can't be assigned with values."))
                        )
    else:
        logs.warning("Received SET_DATA request for an unowned device.")
        pdu_tcp.send(socket,
                     pdu_tcp.to_bytes(pdu_tcp.Packet(pdu_tcp.packet_type['DATA_NACK'],config.client['MAC'],config.client['Server_Config']['rnd'],"","","Received SET_DATA request for an unowned device."))
                     )

def get_data(packet: pdu_tcp.Packet, socket: socket.socket) -> None:
    """
    Processes a GET_DATA request and sends the corresponding device value if it exists in the client's configuration.
    Sends a warning if the device is not owned by the client.

    Parameters:
    - packet (pdu_tcp.Packet): The received GET_DATA request packet.
    - socket (socket.socket): The socket used for communication.

    Returns:
    - None
    """
    if packet.device in config.client['Elements'].keys():
        config.client['Elements'][packet.device] = packet.val
        logs.info(f"Sent {packet.device} value: {packet.val}.")

        pdu_tcp.send(socket,
                    pdu_tcp.to_bytes(
                        pdu_tcp.Packet(
                            pdu_tcp.packet_type['DATA_ACK'],
                            config.client['MAC'],
                            config.client['Server_Config']['rnd'],
                            packet.device,packet.val,""
                            )
                        )
                    )
    else:
        logs.warning("Received GET_DATA request for an unowned device.")
        pdu_tcp.send(socket,
                    pdu_tcp.to_bytes(pdu_tcp.Packet(pdu_tcp.packet_type['DATA_NACK'],config.client['MAC'],config.client['Server_Config']['rnd'],"","","Received GET_DATA request for an unowned device."))
                    )

def recv_data():
    # Accept new server connection
    server_socket, server_address = sock_tcp.accept()
    # Set max recv timeout
    server_socket.settimeout(3)
    packet = pdu_tcp.recvTCP(server_socket)

    if packet == None:
        logs.warning("Data packet not received. Closing connection with server.")
        server_socket.close()
        return
    
    if (packet.mac == config.client['Server_Config']['MAC'] and 
        packet.rnd == config.client['Server_Config']['rnd'] and 
        server_address[0] == config.client['Server_Config']['IP']):

        if packet.ptype == pdu_tcp.packet_type['SET_DATA']:
            set_data(packet,server_socket)
        elif packet.ptype == pdu_tcp.packet_type['GET_DATA']:
            get_data(packet,server_socket)
        else:
            logs.warning("Received unexpected packet.")
            config.set_status('NOT_SUBSCRIBED')
            disconnected.set()
    else:
        logs.warning(f"Received wrong data packet credentials.")
        pdu_tcp.send(server_socket,
                     pdu_tcp.to_bytes(pdu_tcp.Packet(pdu_tcp.packet_type['DATA_REJ'],config.client['MAC'],config.client['Server_Config']['rnd'],packet.device,packet.val,"Wrong packet credentials."))
                    )
        config.set_status('NOT_SUBSCRIBED')
        disconnected.set()

    server_socket.close()
    return

def send_data(device: str) -> None:
    """
    Sends data of the specified device to the server.

    Parameters:
    - device (str): The device identifier.

    Returns:
    - None
    """
    try:
        # Create new TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as tcp_sock:
            # Connect to the server
            tcp_sock.connect((config.client['Server'], config.client['Srv_UDP']))
            
            # Send the packet
            pdu_tcp.send(tcp_sock,
                pdu_tcp.to_bytes(
                    pdu_tcp.Packet(
                        pdu_tcp.packet_type['DATA_ACK'],
                        config.client['MAC'],
                        config.client['Server_Config']['rnd'],
                        device,config.client['Elements'][device],
                        ""
                    )
                )
            )
        # Wait for server response
        tcp_sock.settimeout(3)
        response = pdu_tcp.recvTCP(tcp_sock)

        if response == None:
            logs.warning("Haven't received response after send command. Closing communication...")

        elif (response.mac == config.client['Server_Config']['MAC'] and 
              response.rnd == config.client['Server_Config']['rnd']):
            
            logs.warning("Received wrong server credentials in send packet response.")
            config.set_status('NOT_SUBSCRIBED')
            disconnected.set()
        
        elif response.ptype == pdu_tcp.packet_type['DATA_ACK']:
            logs.info("Server succesfully stored device info.")
        elif response.ptype == pdu_tcp.packet_type['DATA_NACK']:
            logs.warning(f"Send rejected: {response.info}")
        elif response.ptype == pdu_tcp.packet_type['DATA_REJ']:
            logs.warning("Server rejected data. Disconnecting...")
            config.set_status('NOT_SUBSCRIBED')
            disconnected.set()
        else:
            logs.warning("Received unexpected data during send validation. Disconnecting")
            config.set_status('NOT_SUBSCRIBED')
            disconnected.set()
    
    except Exception as e:
        logs.error(f"Unexpected error while executing send: {e}")
    finally:
        tcp_sock.close()
        return
    
####################
# Data Process END #
####################
    
############
# COMMANDS #
############
    

    
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
    # UDP
    sock_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  
    # TCP
    sock_tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock_tcp.bind((config.client['Server'], int(config.client['Local_TCP'])))
    sock_tcp.listen(1)

def main():
    # Create global variables
    global subs_attempts
    global disconnected
    disconnected = threading.Event()

    # Init variables
    disconnected.set()
    subs_attempts = 0

    # Main loop
    try:
        while True:
            # Set select
            readable, _, _ = select.select([sock_tcp,sys.stdin], [], [], 1)

            # Start subscription and periodic communication
            if disconnected.is_set() and subs_process():
                hello_process = threading.Thread(target=hello_process_thread,daemon=True)
                hello_process.start()
            
            # Check file descriptors
            for sock_or_input in readable:

                # TCP data reception
                if sock_or_input is sock_tcp:
                    recv_data()
                
                # Commands
                elif sock_or_input is sys.stdin:
                    # Read command input
                    command = sys.stdin.readline().strip()
                    # Process the command
                    

    except Exception as e:
        logs.error(f"An exception has ocurred: {e}",True)

    finally:
        sock_udp.close()
        sock_tcp.close()

# Program Call
if __name__ == "__main__":
    _init_()
    main()
