/**
 * @file server.c
 * @brief Server Program for Controller Communication.
 * 
 * @details This program handles communication between a server and multiple controllers using TCP and UDP protocols. It manages server configuration, socket creation, controller connections, and data transmission/reception.
 * 
 * @author Eric Bitria Ribes
 * @version 1.0
 * @date 2024-3-14
 * 
 * @section Scope
 * - Server Configuration Management: Reads and initializes server settings from configuration files.
 * - Socket Initialization: Creates TCP and UDP socket file descriptors for communication with controllers.
 * - Controller Management:
 *      - Loads a list of allowed controllers into memory.
 *      - Monitors and validates incoming connections from controllers.
 *      - Handles subscription requests and manages connection status updates.
 *      - Detects and handles disconnections and inactive controller detection.
 * - Concurrency Control: Implements mutexes to manage concurrent access to shared resources, ensuring data integrity.
 * - Communication Handling:
 *      - Receives and processes UDP packets from controllers.
 *      - Validates packet content and controller status.
 *      - Responds to controllers based on their status and request type.
 *      - Detects and handles TCP connections for data transmission between the server and controllers.
 * - User Interaction:
 *      - Accepts commands from the user via standard input for server management.
 *      - Supports commands for listing controllers, setting device values, getting device data, and quitting the server.
 * 
 * @section Organization
 * The program is organized into several modules and files:
 * - `utilities/pdu/udp.c`: Contains functions for UDP packet handling.
 * - `utilities/pdu/tcp.c`: Contains functions for TCP packet handling.
 * - `utilities/logs.c`: Provides logging functionality for the program.
 * - `utilities/server/controllers.c`: Manages controller loading and data.
 * - `utilities/server/conf.c`: Handles server configuration.
 * - `utilities/server/subs.c`: Manages controller subscription requests and periodic communication.
 * - `utilities/server/commands.c`: Executes server management commands.
 * - `utilities/server/data.c`: Handles data transmission, reception and storage.
 * 
 */

#include "utilities/commons.h"

/* Init global mutex between threads */
pthread_mutex_t mutex;

/*Create default server sockets file descriptors*/
int tcp_socket, udp_socket;

/*Array of structs for allowed clients in memory*/
struct Controller *controllers = NULL;

/* Closes the server */
void quit(int signum) {
    if (signum == SIGINT) {
        printf("Exiting via SIGINT...\n");
    } else if(signum == 0) {
        printf("Closing server...");
    }
    /*Free controllers*/
    free(controllers);
    /*Close the socket file descriptors*/
    close(udp_socket);
    close(tcp_socket);

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    /*Struct for server configuration*/
    struct Server serv_conf;
    int numControllers;
    int i;
    /*Initialise file descriptors select*/
    fd_set readfds;
    int max_fd;
    /*Get config and controllers file name*/
    char *config_file;
    char *controllers_file;
    readArgs(argc, argv, &config_file, &controllers_file);

    signal(SIGINT, quit);

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
            lerror("Unexpected error in select",true);
        }
        
        /* Check if UDP file descriptor has received data */
        if (FD_ISSET(udp_socket, &readfds)) {
            struct UDPPacket udp_packet;
            /* Receive data and find the controller index, if it exists */
            int controllerIndex = 0;
            /*linfo("Received data in file descriptor UDP.", false);*/
            udp_packet = recvUdp(udp_socket, &serv_conf.udp_address);

            /*Checks if incoming packet has allowed name and mac adress*/
            if ((controllerIndex = isUDPAllowed(udp_packet, controllers)) != -1) {
                
                if ((controllers[controllerIndex].data.status == DISCONNECTED)){
                    handleDisconnected(&udp_packet, &controllers[controllerIndex], udp_socket, &serv_conf);

                } else if (controllers[controllerIndex].data.status == SUBSCRIBED || controllers[controllerIndex].data.status == SEND_HELLO){
                    handleHello(udp_packet, &controllers[controllerIndex], udp_socket, &serv_conf);

                } else {
                    linfo("Denied connection to Controller: %s. Reason: Invalid status.", false, udp_packet.mac);
                    sendUdp(udp_socket, 
                        createUDPPacket(SUBS_REJ, serv_conf.mac, "00000000", "Subscription Denied: Invalid Status."), 
                        &serv_conf.udp_address
                    );
                    pthread_mutex_lock(&mutex);
                        controllers[i].data.lastPacketTime = 0; /* Reset last packet time */
                    pthread_mutex_unlock(&mutex);
                }

            }else { /* Reject Connection sending a [SUBS_REJ] packet */
                linfo("Denied connection to Controller: %s. Reason: Not listed in allowed Controllers file.", false, udp_packet.mac);
                sendUdp(udp_socket, 
                        createUDPPacket(SUBS_REJ, serv_conf.mac, "00000000", "Subscription Denied: You are not listed in allowed Controllers file."), 
                        &serv_conf.udp_address
                );
                pthread_mutex_lock(&mutex);
                    controllers[i].data.lastPacketTime = 0; /* Reset last packet time */
                pthread_mutex_unlock(&mutex);
            }
        }

        /*Update controllers packet timers*/
        for (i = 0; i < numControllers; i++) {
            if (controllers[i].data.lastPacketTime != 0) {
                time_t current_time = time(NULL);
                /* Check if 6 seconds have passed since the last packet */
                if (current_time - controllers[i].data.lastPacketTime > 6) {
                    pthread_mutex_lock(&mutex);
                        linfo("Controller %s hasn't sent 3 consecutive packets. DISCONNECTING...",false,controllers[i].name);
                        disconnectController(&controllers[i]);
                    pthread_mutex_unlock(&mutex);
                }
            }
        }

        /* Check if the TCP file descriptor has received data */   
        if (FD_ISSET(tcp_socket, &readfds)) {
            /* TCP timeout settings */
            struct timeval tcpTimeout;
            /* Thread args */
            struct dataThreadArgs threadArgs;
            struct sockaddr_in clientAddr;
            socklen_t client_addr_len = sizeof(struct sockaddr_in);
            pthread_t tcpThread;

            threadArgs.controllers = controllers;
            threadArgs.servConf = &serv_conf;

            if ((threadArgs.client_socket = accept(tcp_socket, (struct sockaddr *)&clientAddr, &client_addr_len)) == -1) {
                lerror("Unexpected error while receiving TCP connection",true);
            }

            /*Set TCP socket max recv time*/
            tcpTimeout.tv_sec = 3;
            tcpTimeout.tv_usec = 0;
            if(setsockopt(threadArgs.client_socket,SOL_SOCKET,SO_RCVTIMEO,(const char*)&tcpTimeout,sizeof(tcpTimeout)) < 0){
                lerror("Unexpected error when setting TCP socket settings",true);
            }

            if(pthread_create(&tcpThread, NULL, dataReception, (void *)&threadArgs) < 0){
                lerror("Unexpected error while starting new TCP thread",true);
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
            args = sscanf(commandLine, "%4s %8s %7s %6s", command, controller, device, value);

            if (strcmp(command, "list") == 0 && args == 1) {
                printList(controllers);
            } else if (strcmp(command, "set") == 0 && args == 4) {
                if (strlen(controller) > 8) {
                    linfo("Controller name exceeds maximum length. (8)", true);
                } else if (strlen(device) > 7) {
                    linfo("Device name exceeds maximum length. (7)", true);
                } else if (strlen(value) > 6) {
                    linfo("Value exceeds maximum length. (6)", true);
                } else {
                    commandDataPetition(controller, device, value, controllers,&serv_conf);
                }
            } else if (strcmp(command, "get") == 0 && args == 3) {
                if (strlen(controller) > 8) {
                    linfo("Controller name exceeds maximum length. (8)", true);
                } else if (strlen(device) > 7) {
                    linfo("Device name exceeds maximum length. (7)", true);
                } else {
                    commandDataPetition(controller, device, "", controllers,&serv_conf);
                }
            } else if (strcmp(command, "quit") == 0 && args == 1) {
                quit(0);
            } else {
                linfo("Usage: list | set <controller-name> <device-name> <value> | get <controller-name> <device-name> | quit", 1);
            }
        }
    }

}