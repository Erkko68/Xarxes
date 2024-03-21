/**
 * @file subs.c
 * @brief Functions for the Server Subscription Process.
 * 
 * @author Eric Bitria Ribes
 * @version 0.6
 * @date 2024-3-18
 */

#include "../commons.h"

/**
 * @brief Handles the subscription process for a controller.
 *
 * This function initiates a subscription process for a controller. It generates a random
 * identifier, sets up a UDP socket, and handles acknowledgment and information packets
 * received from the controller. It waits for a certain time for the controller to send
 * subscription information and disconnects if no information is received within the timeout.
 *
 * @param socket Pointer to the UDP socket for communication with the controller.
 * @param addr Pointer to the socket address structure containing controller's address.
 * @param controller Pointer to the struct containing controller information.
 * @param situation Pointer to the situation information.
 * @param srvConf Pointer to the server configuration struct.
 */
void subsProcess(int socket, struct sockaddr_in *addr, struct Controller *controller,  char *situation, struct Server *srvConf) {
    struct timeval timeout;
    struct sockaddr_in newAddress;
    int newUDPSocket;
    fd_set readfds;
    int received;
    char rnd[9];

    /* Log start of the thread */
    linfo("Starting new subscription process for: %s.", false, controller->name);

    /* Generate random identifier */
    generateIdentifier(rnd);

    /* Set up UDP socket */
    newUDPSocket = setupUDPSocket(&newAddress);

    /* Handle SUBS_ACK */
    handleSubsAck(controller,srvConf,&newAddress,addr,newUDPSocket,rnd);

    /* Initialize file descriptor and set timeout */
    FD_ZERO(&readfds);
    FD_SET(newUDPSocket, &readfds);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    /* Wait for SUBS_INFO or timeout */
    if ((received = select(newUDPSocket + 1, &readfds, NULL, NULL, &timeout)) < 0) {
        lerror("Error initializing select during subs process by: %s", true, srvConf->name);
    } else if (received == 0) {
        /* Handle timeout */
        linfo("Controller %s hasn't sent [SUBS_INFO] in the last 2 seconds. Disconnecting...",false, controller->name);
        pthread_mutex_lock(&mutex);
            disconnectController(controller);
        pthread_mutex_unlock(&mutex);
    } else {
        /* Handle [SUBS_INFO] */
        handleSubsInfo(srvConf, &newAddress, controller, rnd, situation, newUDPSocket);
    }

    /*Close socket*/
    close(newUDPSocket);
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
void handleSubsAck(struct Controller *controller, struct Server *srvConf, struct sockaddr_in *newAddress, struct sockaddr_in *addr, int udpSocket, char *rnd) {
    char newPort[6];

    /*Get new Port*/
    sprintf(newPort, "%d", ntohs(newAddress->sin_port));

    /* Create and send SUBS_ACK packet */
    sendUdp(udpSocket, 
            createUDPPacket(SUBS_ACK, srvConf->mac, rnd, newPort), 
            addr
    );
    /* Update controller status to WAIT_INFO */
    pthread_mutex_lock(&mutex);
        linfo("Controller %s [WAIT_INFO]. Sent [SUBS_ACK]. ",true,controller->name);
        controller->data.status = WAIT_INFO;
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
void handleSubsInfo(struct Server *srvConf, struct sockaddr_in *newAddress, struct Controller *controller, char *rnd, char *situation, int newUDPSocket) {
    char *tcp;
    char *devices;
    struct UDPPacket subsPacket;

    /* Receive SUBS_INFO packet */
    subsPacket = recvUdp(newUDPSocket, newAddress);

    /* Extract TCP and devices information */
    tcp = strtok(subsPacket.data, ",");
    devices = strtok(NULL, ",");
 
    /* Check if SUBS_INFO packet is valid */
    if (strcmp(subsPacket.mac, controller->mac) == 0 && strcmp(subsPacket.rnd, rnd) == 0 && tcp != NULL && devices != NULL) {
        char tcpPort[6];
        sprintf(tcpPort, "%d", srvConf->tcp);

        /* Create INFO_ACK packet */
        sendUdp(newUDPSocket, createUDPPacket(INFO_ACK, srvConf->mac, rnd, tcpPort), newAddress);
        /* Save controller Data and set SUBSCRIBED status */
        pthread_mutex_lock(&mutex);
            linfo("Controller: %s [SUBSCRIBED].", true, controller->name);
            controller->data.tcp = atoi(tcp);
            inet_ntop(AF_INET, &(newAddress->sin_addr), controller->data.ip, INET_ADDRSTRLEN);
            strcpy(controller->data.rand, rnd);
            strcpy(controller->data.situation, situation);
            storeDevices(devices, controller->data.devices, ";");
            controller->data.status = SUBSCRIBED;
            controller->data.lastPacketTime = time(NULL);
        pthread_mutex_unlock(&mutex);
    } else {
        /* Invalid SUBS_INFO packet, update status to DISCONNECTED */
        linfo("Controller: %s [DISCONNECTED]. Reason: Wrong Info in SUBS_INFO packet.", true, controller->name);
        /*Send rejection packet*/
        sendUdp(newUDPSocket, 
                createUDPPacket(SUBS_REJ, srvConf->mac, "00000000", "Subscription Denied: Wrong Info in SUBS_INFO packet."), 
                newAddress
        );

        pthread_mutex_lock(&mutex);
            disconnectController(controller);
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

    /* Check if its SUBS_REJ */
    if(udp_packet.type == HELLO_REJ){
        linfo("Received [SUBS_REJ] by %s, Disconnecting....",true, controller->name);
        disconnectController(controller);
        return;
    } else if (udp_packet.type != HELLO){
        sendUdp(udp_socket,
                createUDPPacket(HELLO_REJ, serv_conf->mac, controller->data.rand, ""),
                addr
        );
        return;
    }
    strcpy(dataCpy, udp_packet.data);

    strtok(dataCpy, ","); /* Ignore first name (Since it was checked at isUDPAllowed) */
    situation = strtok(NULL, ",");

    /* DEBUG
        printf("Expe: %s,%s,%s\n",controller->data.situation,controller->mac,controller->data.rand);
        printf("Sent: %s,%s,%s\n",situation,udp_packet.mac,udp_packet.rnd);
    */
    pthread_mutex_lock(&mutex);
        /* Check correct packet data */
        if(situation != NULL && (strcmp(situation, controller->data.situation) == 0) && 
        (strcmp(udp_packet.mac, controller->mac) == 0) && 
        (strcmp(udp_packet.rnd, controller->data.rand) == 0)){
            char data[80];
            /* Reset last packet time stamp */
            controller->data.lastPacketTime = time(NULL);

            /* Get data */
            strcpy(data, controller->name);
            strcat(data, ",");
            strcat(data, controller->data.situation);

            /* Send HELLO back */
            sendUdp(udp_socket,
                    createUDPPacket(HELLO, serv_conf->mac, controller->data.rand, data),
                    addr
            );
            if(controller->data.status == SUBSCRIBED){
                linfo("Controller %s set to [SEND_HELLO] status.",true, controller->name);
                controller->data.status = SEND_HELLO;
            }

        } else {
            /* Send HELLO_REJ */
            sendUdp(udp_socket,
                    createUDPPacket(HELLO_REJ, serv_conf->mac, controller->data.rand, ""),
                    addr
            );
            linfo("Controller %s has sent incorrect HELLO packets, Disconnecting....",true, controller->name);
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
    if (situation != NULL && (strlen(udp_packet->rnd) == 8) && (strlen(situation) == 12)) {
        subsProcess(udp_socket, clienAddr, controller, situation, serv_conf);

    } else { 
        /* Reject Connection sending a [SUBS_REJ] packet */
        linfo("Denied connection to: %s. Reason: Wrong Situation or Code format.", false, udp_packet->mac);
        sendUdp(udp_socket, 
                createUDPPacket(SUBS_REJ, serv_conf->mac, "00000000", "Subscription Denied: Wrong Situation or Code format."), 
                clienAddr
        );
    }
}

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
void* handleUDPConnection(void* udp_args){
    struct subsThreadArgs *args = (struct subsThreadArgs*)udp_args;
    struct sockaddr_in clienAddr;
    struct UDPPacket udp_packet;
    /* Receive data and find the controller index, if it exists */
    int controllerIndex = 0;
    /*linfo("Received data in file descriptor UDP.", false);*/
    udp_packet = recvUdp(args->socket, &clienAddr);

    /*Checks if incoming packet has allowed name and mac adress*/
    if ((controllerIndex = isUDPAllowed(udp_packet, args->controller)) != -1) {
        
        if ((args->controller[controllerIndex].data.status == DISCONNECTED)){
            handleDisconnected(&udp_packet, &args->controller[controllerIndex], args->socket, args->srvConf, &clienAddr);

        } else if (args->controller[controllerIndex].data.status == SUBSCRIBED || args->controller[controllerIndex].data.status == SEND_HELLO){
            handleHello(udp_packet, &args->controller[controllerIndex], args->socket, args->srvConf, &clienAddr);

        } else {
            /* linfo("Denied connection to: %s. Reason: Invalid status.", false, udp_packet.mac); */
            sendUdp(args->socket, 
                createUDPPacket(SUBS_REJ, args->srvConf->mac, "00000000", "Subscription Denied: Invalid Status."), 
                &clienAddr
            );
            pthread_mutex_lock(&mutex);
                args->controller[controllerIndex].data.lastPacketTime = 0; /* Reset last packet time */
            pthread_mutex_unlock(&mutex);
        }

    }else { /* Reject Connection sending a [SUBS_REJ] packet */
        linfo("Denied connection: %s. Reason: Not listed in allowed Controllers file.", false, udp_packet.mac);
        sendUdp(args->socket,
                createUDPPacket(SUBS_REJ, args->srvConf->mac, "00000000", "Subscription Denied: You are not listed in allowed Controllers file."), 
                &clienAddr
        );
        pthread_mutex_lock(&mutex);
            args->controller[controllerIndex].data.lastPacketTime = 0; /* Reset last packet time */
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}