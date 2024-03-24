#!/usr/bin/env python3

"""
Description: A python program that simulates the interaction of a client with diferent sensors and sends it to a server.
             It uses diferent modules located in the utilities_p folder to simplify the readability of this main program.
Author: Eric Bitria Ribes
Version: 0.4
Last Modified: 2024-3-18
"""

# Args
import sys
import argparse
import signal
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
                logs.warning("Received SUBS_REJ")
                config.set_status('NOT_SUBSCRIBED')
                # Start new subscription process
                return False

        else:  # If we don't receive response from the server, wait and continue new subscription request.
            time.sleep(u)
            # Set client status
            config.set_status('NOT_SUBSCRIBED')
            # Call new subs_process
            return False
    else:
        config.set_status('DISCONNECTED')
        # If subscription attempts ended exit function
        logs.error(f"Could not establish connection with server {config.client['Server']}", True)
    
    # WAIT_ACK_INFO process
    if config.client['status'] == config.status['WAIT_ACK_INFO']:
        info_ack, address = pdu_udp.recvUDP(sock_udp)
        if info_ack == None:
            config.set_status('NOT_SUBSCRIBED')
            logs.warning("Server didn't send INFO_ACK packet.")
            # Call new subs_process
            return False
        # Check server information
        if (info_ack.packet_type == pdu_udp.packet_type['INFO_ACK'] and
            info_ack.mac == config.client['Server_Config']['MAC'] and 
            info_ack.rnd == config.client['Server_Config']['rnd']):
            # Store server TCP Port
            config.client['Server_Config']['TCP'] = info_ack.data
            config.set_status('SUBSCRIBED')
            return True
        config.set_status('NOT_SUBSCRIBED')
        return False

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
    pdu_udp.send(sock_udp, pdu_udp.to_bytes(hello), config.client['Server_Config']['IP'], int(config.client['Srv_UDP']))

def hello_recv_thread():
    global subs_attempts
    """
    Thread function for receiving and processing HELLO packets.
    """
    # Set first max wait time (2 times original value)
    sock_udp.settimeout(4)
    hello, addr = pdu_udp.recvUDP(sock_udp)
    if hello is None:
        logs.warning("Server hasn't sent first HELLO packet.")
        disconnected.set()
        return
    elif hello.packet_type == pdu_udp.packet_type['HELLO_REJ']:
        logs.warning("Received HELLO_REJ.")
        disconnected.set()
        return
    else:
        if check_hello(hello, addr):
            # Set status
            config.set_status('SEND_HELLO')
            open_comm()
        else:
            send_hello_rej(hello)
            disconnected.set()
            logs.warning("Received wrong HELLO packet credentials.")
            return

    # Set default timeout
    sock_udp.settimeout(2)

    # Set maximum packets missed
    missed = 0
    while missed < 3:
        hello, addr = pdu_udp.recvUDP(sock_udp)
        if hello is None:
            missed += 1
        elif hello.packet_type == pdu_udp.packet_type['HELLO_REJ']:
            logs.warning("Received HELLO_REJ.")
            sock_tcp.close()
            disconnected.set()
            tcp_on.clear()
            return
        else:
            if check_hello(hello, addr):
                missed = 0
                subs_attempts = 0
            else:
                break
    else:   
        # If reached here lost connection with server, stopping reception thread
        logs.warning("Server hasn't sent 3 consecutive HELLO packets.")
        config.set_status('NOT_SUBSCRIBED')
        sock_tcp.close()
        disconnected.set()
        tcp_on.clear()
        return

    # If reached here controller received wrong HELLO packet data, sending HELLO_REJ
    send_hello_rej(hello)
    logs.warning("Received wrong HELLO packet data.")
    config.set_status('NOT_SUBSCRIBED')
    sock_tcp.close()
    disconnected.set()
    tcp_on.clear()
    return

def hello_process_thread():
    """
    Thread function for sending periodic HELLO packets.
    """
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
            config.client['Elements'][packet.device] = str(packet.val)
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
                         pdu_tcp.to_bytes(pdu_tcp.Packet(pdu_tcp.packet_type['DATA_NACK'],config.client['MAC'],config.client['Server_Config']['rnd'],packet.device,packet.val,"Device is a sensor and can't be assigned with values."))
                        )
    else:
        logs.warning("Received SET_DATA request for an unowned device.")
        pdu_tcp.send(socket,
                     pdu_tcp.to_bytes(pdu_tcp.Packet(pdu_tcp.packet_type['DATA_NACK'],config.client['MAC'],config.client['Server_Config']['rnd'],packet.device,packet.val,"Received SET_DATA request for an unowned device."))
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
        logs.info(f"Sent {packet.device} value: {config.client['Elements'][packet.device]}.")

        pdu_tcp.send(socket,
                    pdu_tcp.to_bytes(
                        pdu_tcp.Packet(
                            pdu_tcp.packet_type['DATA_ACK'],
                            config.client['MAC'],
                            config.client['Server_Config']['rnd'],
                            packet.device,
                            str(config.client['Elements'][packet.device]),
                            ""
                            )
                        )
                    )
    else:
        logs.warning("Received GET_DATA request for an unowned device.")
        pdu_tcp.send(socket,
                    pdu_tcp.to_bytes(pdu_tcp.Packet(pdu_tcp.packet_type['DATA_NACK'],config.client['MAC'],config.client['Server_Config']['rnd'],packet.device,packet.val,"Received GET_DATA request for an unowned device."))
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
    # Create new TCP socket
    try:
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # Connect to the server
        server_socket.connect((config.client['Server'], int(config.client['Server_Config']['TCP'])))
    except Exception as e:
        logs.error(f"An error has occurred while connecting to the server: {e}")

    logs.info(f"Open port {config.client['Server_Config']['TCP']} to send data.")

    # Send the packet
    pdu_tcp.send(server_socket,
        pdu_tcp.to_bytes(
            pdu_tcp.Packet(
                pdu_tcp.packet_type['SEND_DATA'],
                config.client['MAC'],
                config.client['Server_Config']['rnd'],
                device,
                str(config.client['Elements'][device]),
                ""
            )
        )
    )
    # Wait for server response
    server_socket.settimeout(3)
    response = pdu_tcp.recvTCP(server_socket)

    if response == None:
        logs.warning("Haven't received response after send command. Closing communication...")

    elif (response.mac != config.client['Server_Config']['MAC'] or 
            response.rnd != config.client['Server_Config']['rnd']):
        
        logs.warning("Received wrong server credentials in send packet response.")
        config.set_status('NOT_SUBSCRIBED')
        server_socket.close()
        disconnected.set()
    
    elif (response.device != device or response.val != str(config.client['Elements'][device])):
        logs.warning("Received wrong device name or value in send packet response.")
        config.set_status('NOT_SUBSCRIBED')
        server_socket.close()
        disconnected.set()
    
    elif response.ptype == pdu_tcp.packet_type['DATA_ACK']:
        logs.info("Server succesfully stored device info.")

    elif response.ptype == pdu_tcp.packet_type['DATA_NACK']:
        logs.warning(f"Send rejected: {response.info}")

    elif response.ptype == pdu_tcp.packet_type['DATA_REJ']:
        logs.warning("Server rejected data. Disconnecting...")
        config.set_status('NOT_SUBSCRIBED')
        server_socket.close()
        disconnected.set()

    else:
        logs.warning("Received unexpected data during send validation. Disconnecting")
        config.set_status('NOT_SUBSCRIBED')
        server_socket.close()
        disconnected.set()

    server_socket.close()
    return
    
####################
# Data Process END #
####################
    
############
# COMMANDS #
############

def stat():
    print("******************** CONTROLLER DATA *********************")
    print(f"MAC: {config.client['MAC']}, Nom: {config.client['Name']}, Situació: {config.client['Situation']}\n")
    print(f"Status: {config.client['MAC']}\n")
    print("    Device       Value")
    print("    ------       -----")
    for key, value in config.client['Elements'].items():
        print(f"    {key}       {value}")
    print("\n**********************************************************")

def set_val(device: str, value: str):
    if len(value) > 6:
        print("Value must be maximum 6 characters long.")
    else:
        config.client['Elements'][device] = value
        logs.info(f"Updated data for device {device}: {value}")

def selector(line: str) -> None:
    """
    Process user input commands.

    Parameters:
    - line (str): The input command line.

    Returns:
    - None
    """
    # Split the input line into tokens
    tokens = line.strip().split()

    if not tokens:
        return

    # Extract the command and its arguments
    command = tokens[0]
    args = tokens[1:]

    # Process the command
    if command == 'stat':
        stat()
    elif command == 'set':
        if len(args) != 2:
            print("Usage: set <device-name> <value>")
            return
        if args[0] in config.client['Elements'].keys():
            set_val(args[0],args[1])
        else:
            logs.warning("Device not found",True)
    elif command == 'send':
        if len(args) != 1:
            print("Usage: send <device-name>")
            return
        if args[0] in config.client['Elements'].keys():
            data = threading.Thread(target=send_data,args=(args[0],),daemon=True)
            data.start()
        else:
            logs.warning("Device not found",True)

    elif command == 'quit':
        disconnected.set()
        sock_udp.close()
        if tcp_on.is_set():
            sock_tcp.close()
        exit()
    else:
        print("Available commands:")
        print("- stat: Displays controller and devices information.")
        print("- set <device-name> <value>: Sets the value of a device.")
        print("- send <device-name>: Sends the value of a device to the server.")
        print("- quit: Exits the program.")

################
# COMMANDS END #
################
        
# Initialization function
def _init_():
    # Create global variables for sockets
    global sock_udp

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

    # Set SIGNAL
    signal.signal(signal.SIGINT, handle_SIGINT)

    # Create sockets
    # UDP
    sock_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def open_comm():
    global sock_tcp

    # Init socket
    sock_tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Call bind
    try:
        sock_tcp.bind((config.client['Server'], int(config.client['Local_TCP'])))
    except OSError:
        logs.error(f"Failed to bind to local TCP port: Address might already be in use by another client.")
    
    # Call listen
    try:
        sock_tcp.listen(5)
    except OSError as e:
        logs.error(f"Failed to start listening for TCP connections: {e}")

    logs.info(f"Opened TCP connection on port {config.client['Local_TCP']}")
    # Set flag
    tcp_on.set()

def handle_SIGINT(sig, frame):
    print("\nExiting")
    disconnected.set()
    sock_udp.close()
    try:
        sock_tcp.close()
    except:
        pass
    exit(0)

def main():
    # Create global variables
    global tcp_on
    global subs_attempts
    global disconnected
    disconnected = threading.Event()
    tcp_on = threading.Event()

    # Init variables
    disconnected.set()
    subs_attempts = 0

    config.set_status('NOT_SUBSCRIBED')
    
    # Main loop
    try:
        while True:
                
            if tcp_on.is_set():
                readable, _, _ = select.select([sock_tcp,sys.stdin], [], [], 0.1)
            else:
                readable, _, _ = select.select([sys.stdin], [], [], 1)

            # Start subscription and periodic communication
            if disconnected.is_set() and subs_process():
                disconnected.clear()
                hello_process = threading.Thread(target=hello_process_thread,daemon=True)
                hello_process.start()

            # Check file descriptors
            for sock_or_input in readable:

                # TCP data reception
                if tcp_on.is_set() and sock_or_input is sock_tcp:
                    recv_data()
                
                # Commands
                elif sock_or_input is sys.stdin:
                    if config.client['status'] == config.status['SEND_HELLO']:
                        # Read command input
                        selector(sys.stdin.readline().strip())

    except Exception as e:
        logs.error(f"An exception has ocurred: {e}",True)
    finally:
        disconnected.set()
        sock_udp.close()
        try:
            sock_tcp.close()
        except:
            pass

# Program Call
if __name__ == "__main__":
    _init_()
    main()