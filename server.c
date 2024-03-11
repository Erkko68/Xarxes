#include "utilities/commons.h"

pthread_mutex_t mutex;

/*Subscription Functions*/

/* Subscription Process Thread */
void* subsProcess(void *args) {
    struct subsThreadArgs *subsArgs = (struct subsThreadArgs*)args;
    struct timeval timeout;
    struct sockaddr_in newAddress;
    int newUDPSocket;
    fd_set readfds;
    int received;
    char rnd[9];

    /* Log start of the thread */
    linfo("Starting new subscription process for controller: %s.\n", false, subsArgs->controller->mac);

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
        pthread_mutex_lock(&mutex);
            subsArgs->controller->data.status = DISCONNECTED;
        pthread_mutex_unlock(&mutex);
        /*Close socket*/
        close(newUDPSocket);
    } else {
        /* Handle [SUBS_INFO] */
        handleSubsInfo(subsArgs, &newAddress, rnd, newUDPSocket);
    }

    return NULL;
}

void handleDisconnected(struct UDPPacket *udp_packet, struct Controller *controller, int udp_socket, struct Server *serv_conf) {
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

        /*Start subscription process*/
        if (pthread_create(&subsThread, NULL, subsProcess, (void*)&subsArgs) != 0) {
            lerror("Thread creation failed", true);
        }
    } else { 
        /* Reject Connection sending a [SUBS_REJ] packet */
        linfo("Denied connection to Controller: %s. Reason: Wrong Situation or Code format.", false, udp_packet->mac);
        sendUdp(udp_socket, 
                createUDPPacket(SUBS_REJ, serv_conf->mac, "00000000", "Subscription Denied: Wrong Situation or Code format."), 
                &serv_conf->udp_address
        );
    }
}

/*Mantain periodic communication*/
void handleHello(struct UDPPacket *udp_packet, struct Controller *controller, int udp_socket, struct Server *serv_conf) {
    char* situation;
    char dataCpy[80]; /* Make copy in case we need to send it back */
    strcpy(dataCpy, udp_packet->data);

    strtok(dataCpy, ","); /* Ignore first name */
    situation = strtok(NULL, ",");

    /* Check correct packet data */
    if((strcmp(situation, controller->data.situation) == 0) && 
       (strcmp(udp_packet->mac, controller->mac) == 0) && 
       (strcmp(udp_packet->rnd, controller->data.rand) == 0)){

        /* Reset last packet time stamp */
        controller->data.lastPacketTime = time(NULL);

        /* Send HELLO back */
        sendUdp(udp_socket,
                createUDPPacket(HELLO, serv_conf->mac, controller->data.rand, udp_packet->data),
                &serv_conf->udp_address
        );
        if(controller->data.status == SUBSCRIBED){
            linfo("Controller %s set to SEND_HELLO status.",false, controller->mac);
            controller->data.status = SEND_HELLO;
        }

    } else {
        /* Send HELLO_REJ */
        sendUdp(udp_socket,
                createUDPPacket(HELLO_REJ, serv_conf->mac, controller->data.rand, ""),
                &serv_conf->udp_address
        );
        linfo("Controller %s has sent incorrect HELLO packets, DISCONNECTING controller....",false, controller->mac);
        controller->data.status = DISCONNECTED;
    }
}

/*Subscription Functions END*/

/* TCP Reception */

void* storeData(void* args){
    struct dataThreadArgs *dataArgs = (struct dataThreadArgs*)args;
    struct TCPPacket tcp_packet;
    int controllerIndex;

    /*Get Packet*/
    tcp_packet = recvTcp(dataArgs->client_socket);

    if((controllerIndex = isTCPAllowed(tcp_packet,dataArgs->controllers,dataArgs->numControllers)) != -1){
        printf("Allowed\n");
    } else {
        linfo("Denied connection to Controller: %s. Reason: Not listed in allowed Controllers file.", false, tcp_packet.mac);

    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    /*Create default server sockets file descriptors*/
    int tcp_socket, udp_socket;
    /*Struct for server configuration*/
    struct Server serv_conf;
    /*Array of structs for allowed clients in memory*/
    struct Controller *controllers = NULL;
    int numControllers;
    /*Initialise variables Threads*/
    pthread_t *threads = NULL;
    int num_threads = 0;
    int i;
    /*Initialise file descriptors select*/
    fd_set readfds;
    int max_fd;
    
    /*Get config and controllers file name*/
    char *config_file;
    char *controllers_file;
    readArgs(argc, argv, &config_file, &controllers_file);

    /*Initialise server configuration struct*/
    linfo("Reading server configuration files...",false);
    serv_conf = serverConfig(config_file);

    /*Initialize Sockets*/
    linfo("Initialising socket creation...",false);
        /* Create UDP socket file descriptor */
        if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            lerror("Error creating TCP socket",true);
        }
        /* Create UDP socket file descriptor */
        if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            lerror("Error creating UDP socket",true);
        }

    linfo("Binding sockets to server adress...",false);
        /* Bind TCP socket */
        if (bind(tcp_socket, (struct sockaddr *)&serv_conf.tcp_address, sizeof(serv_conf.tcp_address)) < 0) {
            lerror("Error binding TCP socket",true);
        }
        /* Bind UDP socket */
        if (bind(udp_socket, (struct sockaddr *)&serv_conf.udp_address, sizeof(serv_conf.udp_address)) < 0) {
            lerror("Error binding UDP socket",true);
        }

        /* Initialize listen for TCP file descriptor. */
        if (listen(tcp_socket, 5) == -1) {
            lerror("Unexpected error when calling listen.",true);
        }

    /* Load allowed controllers in memory */
    linfo("Loading controllers...",false);
        numControllers = loadControllers(&controllers, controllers_file);
        if(numControllers==0){
            lerror("0 controllers loaded. Exiting...",true);
        } else {
            linfo("%d controllers loaded. Waiting for incoming connections...",false,numControllers);
        }

    /*Initialise mutex (Locks and unlocks)*/
    pthread_mutex_init(&mutex, NULL);
    
    while (1+1!=3) {
        /*Define timeval struct for the select*/
        struct timeval timeout;

        /* Init file descriptors readers */
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(tcp_socket, &readfds);
        FD_SET(udp_socket, &readfds);
        /* Get max range of file descriptors to check */
        max_fd = (tcp_socket > udp_socket) ? tcp_socket : udp_socket;

        /* Set timeouts */
        timeout.tv_sec = 0;
        timeout.tv_usec = 50; /*Number of microseconds the select waits for the file descriptors, 
                                if set to 0 ms CPU usage increases to ~15%, at 50 ms is less than ~1%.
                                If set to high values affects timeout accuracy for the HELLO packets*/

        /*Start monitoring file descriptors*/
        if (select(max_fd + 1, &readfds, NULL, NULL, &timeout) < 0) {
            lerror("Unexpected error in select.",true);
        }
        
        /* Check if UDP file descriptor has received data */
        if (FD_ISSET(udp_socket, &readfds)) {
            struct UDPPacket udp_packet;
            /* Receive data and find the controller index, if it exists */
            int controllerIndex = 0;
            /*linfo("Received data in file descriptor UDP.", false);*/
            udp_packet = recvUdp(udp_socket, &serv_conf.udp_address);

            /*Checks if incoming packet has allowed name and mac adress*/
            if ((controllerIndex = isAllowed(udp_packet, controllers, numControllers)) != -1) {
                
                if ((controllers[controllerIndex].data.status == DISCONNECTED)){
                    handleDisconnected(&udp_packet, &controllers[controllerIndex], udp_socket, &serv_conf);

                } else if (controllers[controllerIndex].data.status == SUBSCRIBED || controllers[controllerIndex].data.status == SEND_HELLO){
                    handleHello(&udp_packet, &controllers[controllerIndex], udp_socket, &serv_conf);

                } else {
                    linfo("Denied connection to Controller: %s. Reason: Invalid status.", false, udp_packet.mac);
                }

            }else { /* Reject Connection sending a [SUBS_REJ] packet */
                linfo("Denied connection to Controller: %s. Reason: Not listed in allowed Controllers file.", false, udp_packet.mac);
                sendUdp(udp_socket, 
                        createUDPPacket(SUBS_REJ, serv_conf.mac, "00000000", "Subscription Denied: You are not listed in allowed Controllers file."), 
                        &serv_conf.udp_address
                );
            }
        }

        /*Update controllers packet timers*/
        for (i = 0; i < numControllers; i++) {
            if (controllers[i].data.lastPacketTime != 0) {
                time_t current_time = time(NULL);
                /* Check if 6 seconds have passed since the last packet */
                if (current_time - controllers[i].data.lastPacketTime > 6) {

                    linfo("Controller %s hasn't sent 3 consecutive packets. DISCONNECTING...",false,controllers[i].mac);
                    controllers[i].data.status = DISCONNECTED; /* Set DISCONNECTED mode */
                    controllers[i].data.lastPacketTime = 0; /* Reset last packet time */
                }
            }
        }

        /* Check if the TCP file descriptor has received data */   
        if (FD_ISSET(tcp_socket, &readfds)) {
            struct dataThreadArgs threadArgs;
            socklen_t client_addr_len = sizeof(threadArgs.clientAddr);
            pthread_t tcpThread;

            threadArgs.controllers = controllers;

            if ((threadArgs.client_socket = accept(tcp_socket, (struct sockaddr *)&threadArgs.clientAddr, &client_addr_len)) == -1) {
                lerror("Unexpected error while receiving TCP connection.",true);
            }

            if(pthread_create(&tcpThread, NULL, storeData, (void *)&threadArgs) < 0){
                lerror("Unexpected error while starting new TCP thread.",true);
            }
        }

        /* Server commands */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char commandLine[30]; /*30(Worst case scenario) = set(3) + controller_name(8) + device(7) + (value) 7 + \0(1) + spaces(3) + \n(1)*/
            char command[5], controller[9], device[8], value[7];
            int args;

            if (fgets(commandLine, sizeof(commandLine), stdin) == NULL) {
                lerror("Fgets failed",true);
            }
            commandLine[strcspn(commandLine, "\n")] = '\0';
            sanitizeString(commandLine);

            /*Get command and arguments*/
            args = sscanf(commandLine, "%4s %8s %7s %7s", command, controller, device, value);

            if (strcmp(command, "list") == 0 && args == 1) {
                
            } else if (strcmp(command, "set") == 0 && args == 4) {
                /* set command */
            } else if (strcmp(command, "get") == 0 && args == 3) {
                /* get command */
            } else if (strcmp(command, "quit") == 0 && args == 1) {
                /*Close server*/
                break;
            } else {
                linfo("Invalid command. Usage:\n list (Show authorized controllers info.)\n set <controller-name> <device-name> <value> (sends info to the device)\n get <controller-name> <device-name> (Requests info to the device)\n quit (Quits server and closes all communications)",true);
            }
        }
    }

    /*Join threads when finished*/
    for(i=0;i<num_threads;i++){
        pthread_join(threads[i], NULL);
    }
    
    /*Free controllers*/
    free(controllers);
    /*Free threads*/
    free(threads);

    /*Close the socket file descriptors*/
    close(udp_socket);
    close(tcp_socket);

    return 0;
}