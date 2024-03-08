/**
 * @file subs_process.h
 * @brief Functions for the Server Subscription Process.
 * 
 * @author Eric Bitria Ribes
 * @version 0.3
 * @date 2024-3-8
 */

#include "../commons.h"

/* Function to set up the UDP socket and bind to a random port
 *
 * @brief Initializes a UDP socket and binds it to a random port.
 *        Retrieves the port number assigned by the operating system.
 *
 * @param newAddress Pointer to a sockaddr_in structure to store the socket address information.
 *
 * @return int File descriptor of the UDP socket.
 */
int setupUDPSocket(struct sockaddr_in *newAddress) {
    int newUDPSocket;
    socklen_t addrlen;

    /* Initialize socket address */
    memset(newAddress, 0, sizeof(*newAddress));
    newAddress->sin_family = AF_INET; 
    newAddress->sin_addr.s_addr = htonl(INADDR_ANY); 
    newAddress->sin_port = 0; 

    /* Create UDP socket */
    if ((newUDPSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        lerror("Error creating UDP socket.", true);
    }
    /* Bind the socket to the address */
    if (bind(newUDPSocket, (struct sockaddr*)newAddress, sizeof(*newAddress)) < 0) {
        lerror("Error binding UDP socket", true);
    }

    /* Obtain the port number assigned by the operating system */
    addrlen = sizeof(*newAddress);
    if (getsockname(newUDPSocket, (struct sockaddr*)newAddress, &addrlen) < 0) {
        lerror("Error getting UDP socket name", true);
    }

    return newUDPSocket;
}

/* Process [SUBS_ACK]
 *
 * @brief Handles the SUBS_ACK process by sending a SUBS_ACK packet and updating the controller status.
 *
 * @param subsArgs Pointer to subscription thread arguments.
 * @param newAddress Pointer to a sockaddr_in structure containing the socket address information.
 * @param rnd Random identifier.
 */
void handleSubsAck(struct subsThreadArgs *subsArgs, struct sockaddr_in *newAddress, char *rnd) {
    char newPort[6];
    
    /*Get new Port*/
    sprintf(newPort, "%d", ntohs(newAddress->sin_port));

    /* Create and send SUBS_ACK packet */
    sendUdp(*subsArgs->socket, 
            createPacket(SUBS_ACK, subsArgs->srvConf->mac, rnd, newPort), 
            &subsArgs->srvConf->udp_address
    );
    /* Update controller status to WAIT_INFO */
    pthread_mutex_lock(&mutex);
        subsArgs->controller->data.status = WAIT_INFO;
    pthread_mutex_unlock(&mutex);
}

/* Function to handle SUBS_INFO
 *
 * @brief Handles the SUBS_INFO process by receiving and processing the SUBS_INFO packet,
 *        sending appropriate responses, and updating the controller status.
 *
 * @param subsArgs Pointer to subscription thread arguments.
 * @param newAddress Pointer to a sockaddr_in structure containing the socket address information.
 * @param rnd Random identifier.
 * @param newUDPSocket File descriptor of the UDP socket.
 */
void handleSubsInfo(struct subsThreadArgs *subsArgs, struct sockaddr_in *newAddress, char *rnd, int newUDPSocket) {
    char *tcp;
    char *devices;
    struct Packet subsPacket;

    /* Receive SUBS_INFO packet */
    subsPacket = recvUdp(newUDPSocket, newAddress);

    /* Extract TCP and devices information */
    pthread_mutex_lock(&mutex);
        tcp = strtok(subsPacket.data, ",");
        devices = strtok(NULL, ",");
    pthread_mutex_unlock(&mutex);

    /* Check if SUBS_INFO packet is valid */
    if (strcmp(subsPacket.mac, subsArgs->controller->mac) == 0 && strcmp(subsPacket.rnd, rnd) == 0 && tcp != NULL && devices != NULL) {
        char tcpPort[6];
        sprintf(tcpPort, "%d", subsArgs->srvConf->tcp);

        /* Create INFO_ACK packet */
        sendUdp(newUDPSocket, createPacket(INFO_ACK, subsArgs->srvConf->mac, rnd, tcpPort), newAddress);

        /* Save controller Data and set SUBSCRIBED status */
        pthread_mutex_lock(&mutex);
            strcpy(subsArgs->controller->data.rand, rnd);
            strcpy(subsArgs->controller->data.situation, subsArgs->situation);
            storeDevices(devices, subsArgs->controller->data.devices, ";");
            subsArgs->controller->data.status = SUBSCRIBED;
        pthread_mutex_unlock(&mutex);
    } else {
        /* Invalid SUBS_INFO packet, update status to DISCONNECTED */
        linfo("Subscription ended for Controller: %s. Reason: Wrong Info in SUBS_INFO packet. Controller set to DISCONNECTED mode.", false, subsArgs->controller->mac);
        /*Send rejection packet*/
        sendUdp(newUDPSocket, 
                createPacket(SUBS_REJ, subsArgs->srvConf->mac, "00000000", "Subscription Denied: Wrong Info in SUBS_INFO packet."), 
                newAddress
        );

        pthread_mutex_lock(&mutex);
            subsArgs->controller->data.status = DISCONNECTED;
        pthread_mutex_unlock(&mutex);
    }
}
