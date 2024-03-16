/**
 * @file subs.c
 * @brief Functions for the Server Subscription Process.
 * 
 * @author Eric Bitria Ribes
 * @version 0.5
 * @date 2024-3-14
 */

#include "../commons.h"

/**
 * @brief Function to handle subscription process.
 *
 * This function initiates a subscription process for a controller. It generates a random
 * identifier, sets up a UDP socket, and handles acknowledgment and information packets
 * received from the controller. It waits for a certain time for the controller to send
 * subscription information and disconnects if no information is received within the timeout.
 *
 * @param args Pointer to a struct subsThreadArgs containing necessary arguments.
 * @return NULL
 */
void* subsProcess(void *args) {
    struct subsThreadArgs *subsArgs = (struct subsThreadArgs*)args;
    struct timeval timeout;
    struct sockaddr_in newAddress;
    int newUDPSocket;
    fd_set readfds;
    int received;
    char rnd[9];

    /* Log start of the thread */
    linfo("Starting new subscription process for controller: %s.", false, subsArgs->controller->mac);

    /* Generate random identifier */
    generateIdentifier(rnd);

    /* Set up UDP socket */
    newUDPSocket = setupUDPSocket(&newAddress);

    /* Handle SUBS_ACK */
    handleSubsAck(subsArgs, &newAddress, rnd);

    /* Initialize file descriptor and set timeout */
    FD_ZERO(&readfds);
    FD_SET(newUDPSocket, &readfds);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    /* Wait for SUBS_INFO or timeout */
    if ((received = select(newUDPSocket + 1, &readfds, NULL, NULL, &timeout)) < 0) {
        lerror("Error initializing select during subscription process thread by client: %s", true, subsArgs->srvConf->mac);
    } else if (received == 0) {
        /* Handle timeout */
        linfo("Controller %s hasn't sent SUBS_INFO in the last 2 seconds. Disconnecting...",false,subsArgs->controller->mac);
        pthread_mutex_lock(&mutex);
            disconnectController(subsArgs->controller);
        pthread_mutex_unlock(&mutex);
    } else {
        /* Handle [SUBS_INFO] */
        handleSubsInfo(subsArgs, &newAddress, rnd, newUDPSocket);
    }

    /*Close socket*/
    close(newUDPSocket);

    return NULL;
}

/* Function to set up a new UDP socket and bind to a random port
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
            createUDPPacket(SUBS_ACK, subsArgs->srvConf->mac, rnd, newPort), 
            subsArgs->addr
    );
    /* Update controller status to WAIT_INFO */
    pthread_mutex_lock(&mutex);
        linfo("Sent [SUBS_ACK]. Controller %s set to WAIT_INFO status...",true,subsArgs->controller->mac);
        subsArgs->controller->data.status = WAIT_INFO;
    pthread_mutex_unlock(&mutex);
}


/* Function to handle SUBS_INFO
 *
 * @brief Handles the SUBS_INFO process by receiving and processing the SUBS_INFO packet,
 *        sending the packet responses, and updating the controller status.
 *
 * @param subsArgs Pointer to subscription thread arguments.
 * @param newAddress Pointer to a sockaddr_in structure containing the socket address information.
 * @param rnd Random identifier.
 * @param newUDPSocket File descriptor of the UDP socket.
 */
void handleSubsInfo(struct subsThreadArgs *subsArgs, struct sockaddr_in *newAddress, char *rnd, int newUDPSocket) {
    char *tcp;
    char *devices;
    struct UDPPacket subsPacket;

    /* Receive SUBS_INFO packet */
    subsPacket = recvUdp(newUDPSocket, newAddress);

    /* Extract TCP and devices information */
    tcp = strtok(subsPacket.data, ",");
    devices = strtok(NULL, ",");
 
    /* Check if SUBS_INFO packet is valid */
    if (strcmp(subsPacket.mac, subsArgs->controller->mac) == 0 && strcmp(subsPacket.rnd, rnd) == 0 && tcp != NULL && devices != NULL) {
        char tcpPort[6];
        sprintf(tcpPort, "%d", subsArgs->srvConf->tcp);

        /* Create INFO_ACK packet */
        sendUdp(newUDPSocket, createUDPPacket(INFO_ACK, subsArgs->srvConf->mac, rnd, tcpPort), newAddress);
        /* Save controller Data and set SUBSCRIBED status */
        pthread_mutex_lock(&mutex);
            linfo("Controller: %s successfully subscribed. Status set to SUBSCRIBED.", true, subsArgs->controller->mac);
            subsArgs->controller->data.tcp = atoi(tcp);
            inet_ntop(AF_INET, &(newAddress->sin_addr), subsArgs->controller->data.ip, INET_ADDRSTRLEN);
            strcpy(subsArgs->controller->data.rand, rnd);
            strcpy(subsArgs->controller->data.situation, subsArgs->situation);
            storeDevices(devices, subsArgs->controller->data.devices, ";");
            subsArgs->controller->data.status = SUBSCRIBED;
            subsArgs->controller->data.lastPacketTime = time(NULL);
        pthread_mutex_unlock(&mutex);
    } else {
        /* Invalid SUBS_INFO packet, update status to DISCONNECTED */
        linfo("Subscription process ended for Controller: %s. Reason: Wrong Info in SUBS_INFO packet. Disconnecting...", true, subsArgs->controller->mac);
        /*Send rejection packet*/
        sendUdp(newUDPSocket, 
                createUDPPacket(SUBS_REJ, subsArgs->srvConf->mac, "00000000", "Subscription Denied: Wrong Info in SUBS_INFO packet."), 
                newAddress
        );

        pthread_mutex_lock(&mutex);
            disconnectController(subsArgs->controller);
        pthread_mutex_unlock(&mutex);
    }
}

/**
 * @brief Function to handle HELLO packets.
 *
 * This function processes a HELLO packet received from a controller. It validates the packet's data
 * against the expected situation, controller MAC address, and random identifier. If the validation
 * is successful, it sends a HELLO response back to the controller and updates the controller's status
 * accordingly. If the validation fails, it sends a HELLO_REJ response and disconnects the controller.
 *
 * @param udp_packet The UDPPacket struct containing the UDP packet data.
 * @param controller The Controller struct containing information about the controller.
 * @param udp_socket The UDP socket descriptor.
 * @param serv_conf The Server struct containing server configuration.
 * @param addr The address of the controller
 */
void handleHello(struct UDPPacket udp_packet, struct Controller *controller, int udp_socket, struct Server *serv_conf, struct sockaddr_in *addr) {
    char* situation;
    char dataCpy[80]; /* Make copy in case we need to send it back */
    strcpy(dataCpy, udp_packet.data);

    strtok(dataCpy, ","); /* Ignore first name */
    situation = strtok(NULL, ",");

    /* DEBUG
        printf("Expe: %s,%s,%s\n",controller->data.situation,controller->mac,controller->data.rand);
        printf("Sent: %s,%s,%s\n",situation,udp_packet.mac,udp_packet.rnd);
    */
    pthread_mutex_lock(&mutex);
        /* Check correct packet data */
        if((strcmp(situation, controller->data.situation) == 0) && 
        (strcmp(udp_packet.mac, controller->mac) == 0) && 
        (strcmp(udp_packet.rnd, controller->data.rand) == 0)){
            
            /* Reset last packet time stamp */
            controller->data.lastPacketTime = time(NULL);

            /* Send HELLO back */
            sendUdp(udp_socket,
                    createUDPPacket(HELLO, serv_conf->mac, controller->data.rand, udp_packet.data),
                    addr
            );
            if(controller->data.status == SUBSCRIBED){
                linfo("Controller %s set to SEND_HELLO status.",true, controller->mac);
                controller->data.status = SEND_HELLO;
            }

        } else {
            /* Send HELLO_REJ */
            sendUdp(udp_socket,
                    createUDPPacket(HELLO_REJ, serv_conf->mac, controller->data.rand, ""),
                    addr
            );
            linfo("Controller %s has sent incorrect HELLO packets, DISCONNECTING controller....",true, controller->mac);
            disconnectController(controller);
        }
    pthread_mutex_unlock(&mutex);
}


/**
 * @brief Function to handle a disconnected controller.
 *
 * This function processes a UDP packet received from a disconnected controller.
 * It checks the packet's identifier and situation. If the packet has the correct identifier
 * and situation, it starts a subscription process by creating a new thread. If not, it rejects
 * the connection by sending a [SUBS_REJ] packet back to the controller.
 *
 * @param udp_packet The UDPPacket struct containing the UDP packet data.
 * @param controller The Controller struct containing information about the controller.
 * @param udp_socket The UDP socket descriptor.
 * @param serv_conf The Server struct containing server configuration.
 * @param clienAddr Pointer to the client addr.
 */
void handleDisconnected(struct UDPPacket *udp_packet, struct Controller *controller, int udp_socket, struct Server *serv_conf, struct sockaddr_in *clienAddr) {
    char* situation;
    strtok(udp_packet->data, ","); /* Ignore first name */
    situation = strtok(NULL, ",");

    /* Check if packet has correct identifier and situation */
    if((strcmp(udp_packet->rnd, "00000000") == 0) && (strcmp(situation, "000000000000") != 0)) {
        pthread_t subsThread;
        /* Create thread arguments */
        struct subsThreadArgs subsArgs;
        subsArgs.situation = situation;
        subsArgs.srvConf = serv_conf;
        subsArgs.controller = controller;
        subsArgs.socket = &udp_socket;
        subsArgs.addr = clienAddr;

        /*Start subscription process*/
        if (pthread_create(&subsThread, NULL, subsProcess, (void*)&subsArgs) != 0) {
            lerror("Thread creation failed", true);
        }
    } else { 
        /* Reject Connection sending a [SUBS_REJ] packet */
        linfo("Denied connection to Controller: %s. Reason: Wrong Situation or Code format.", false, udp_packet->mac);
        sendUdp(udp_socket, 
                createUDPPacket(SUBS_REJ, serv_conf->mac, "00000000", "Subscription Denied: Wrong Situation or Code format."), 
                clienAddr
        );
    }
}