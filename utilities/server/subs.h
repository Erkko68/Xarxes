/**
 * @file subs.h
 * @brief Function definitions for Server Subscription Process.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.3
 * @date 2024-3-14
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
    int *socket;                
    char *situation;           
};

#include "../commons.h"

/**
 * @brief Function to handle subscription process.
 *
 * @param args Pointer to a struct subsThreadArgs containing necessary arguments.
 * @return NULL
 */
void* subsProcess(void *args);

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
void handleSubsAck(struct subsThreadArgs *subsArgs, struct sockaddr_in *newAddress, char *rnd);

/**
 * @brief Function to handle SUBS_INFO
 * 
 * @param subsArgs Pointer to subscription thread arguments
 * @param newAddress Pointer to sockaddr_in structure
 * @param rnd Random identifier
 * @param newUDPSocket File descriptor of the UDP socket
 */
void handleSubsInfo(struct subsThreadArgs *subsArgs, struct sockaddr_in *newAddress, char *rnd, int newUDPSocket);

/**
 * @brief Function to handle a disconnected controller.
 *
 * @param udp_packet The UDPPacket struct containing the UDP packet data.
 * @param controller The Controller struct containing information about the controller.
 * @param udp_socket The UDP socket descriptor.
 * @param serv_conf The Server struct containing server configuration.
 */
void handleDisconnected(struct UDPPacket *udp_packet, struct Controller *controller, int udp_socket, struct Server *serv_conf);

/**
 * @brief Function to handle HELLO packets.
 *
 * @param udp_packet The UDPPacket struct containing the UDP packet data.
 * @param controller The Controller struct containing information about the controller.
 * @param udp_socket The UDP socket descriptor.
 * @param serv_conf The Server struct containing server configuration.
 */
void handleHello(struct UDPPacket udp_packet, struct Controller *controller, int udp_socket, struct Server *serv_conf);

#endif /* SUBS_FUNCTIONS_H */