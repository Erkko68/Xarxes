/**
 * @file subs.h
 * @brief Function definitions for Server Subscription Process.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.5
 * @date 2024-3-18
 */

#ifndef SUBS_FUNCTIONS_H
#define SUBS_FUNCTIONS_H

#include "../commons.h"

/*
Structure for subscription thread arguments
- struct Server *srvConf;
- struct Controller *controller; 
- int *socket;
- char *situation;   
 */
struct subsThreadArgs {
    struct Server *srvConf;     
    struct Controller *controller;   
    int socket;                       
};

#include "../commons.h"

/**
 * @brief Function to handle subscription process.
 *
 * @param socket The UDP socket for communication with the controller.
 * @param addr Pointer to the socket address structure containing controller's address.
 * @param controller Pointer to the struct containing controller information.
 * @param situation Pointer to the situation information.
 * @param srvConf Pointer to the server configuration struct.
 * @return NULL
 */
void subsProcess(int socket, struct sockaddr_in *addr, struct Controller *controller,  char *situation, struct Server *srvConf);

/**
 * @brief Function to set up a new UDP socket and bind to a random port
 * 
 * @param newAddress Pointer to sockaddr_in structure
 * @return int File descriptor of the UDP socket
 */
int setupUDPSocket(struct sockaddr_in *newAddress);

/**
 * @brief Process [SUBS_ACK]
 * 
 * @param subsArgs Pointer to subscription thread arguments
 * @param newAddress Pointer to sockaddr_in structure
 * @param rnd Random identifier
 */
void handleSubsAck(struct Controller *controller, struct Server *srvConf, struct sockaddr_in *newAddress, struct sockaddr_in *addr, int udpSocket, char *rnd);

/**
 * @brief Handles the SUBS_ACK process.
 *
 * This function handles the SUBS_ACK process by sending a SUBS_ACK packet and updating the controller status.
 *
 * @param controller Pointer to the struct containing controller information.
 * @param srvConf Pointer to the server configuration struct.
 * @param newAddress Pointer to a sockaddr_in structure containing the new socket address information.
 * @param addr Pointer to a sockaddr_in structure containing the original socket address information.
 * @param udpSocket The UDP socket for communication.
 * @param rnd Random identifier.
 */
void handleSubsInfo(struct Server *srvConf, struct sockaddr_in *newAddress, struct Controller *controller, char *rnd, char *situation, int newUDPSocket);

/**
 * @brief Function to handle a disconnected controller.
 *
 * @param udp_packet The UDPPacket struct containing the UDP packet data.
 * @param controller The Controller struct containing information about the controller.
 * @param udp_socket The UDP socket descriptor.
 * @param serv_conf The Server struct containing server configuration.
 * @param clienAddr Pointer to the client addr.
 */
void handleDisconnected(struct UDPPacket *udp_packet, struct Controller *controller, int udp_socket, struct Server *serv_conf, struct sockaddr_in *clienAddr);

/**
 * @brief Function to handle HELLO packets.
 *
 * @param udp_packet The UDPPacket struct containing the UDP packet data.
 * @param controller The Controller struct containing information about the controller.
 * @param udp_socket The UDP socket descriptor.
 * @param serv_conf The Server struct containing server configuration.
 * @param addr The address of the controller
 */
void handleHello(struct UDPPacket udp_packet, struct Controller *controller, int udp_socket, struct Server *serv_conf, struct sockaddr_in *addr);


/**
 * @brief Thread Function to handle a UDP connection.
 *
 * This function processes a UDP packet received from a client.
 * It checks the packet's contents and controller status to determine if it has to do SUBSCRIPTION Process or HELLO Communication.
 * If the packet is allowed and the controller is in a valid state, it proceeds. Otherwise, it rejects the connection.
 *
 * @param udp_args Pointer to a struct subsThreadArgs containing thread arguments.
 * @return NULL
 */
void handleUDPConnection(void* udp_args);

#endif /* SUBS_FUNCTIONS_H */