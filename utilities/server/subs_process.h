/**
 * @file subs_process.h
 * @brief Function definitions for Server Subscription Process.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.3
 * @date 2024-3-8
 */

#ifndef SUBS_FUNCTIONS_H
#define SUBS_FUNCTIONS_H

#include "../commons.h"

/* Define global mutex between threads */
extern pthread_mutex_t mutex;

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

/**
 * @brief Function to set up the UDP socket and bind to a random port
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

#endif /* SUBS_FUNCTIONS_H */