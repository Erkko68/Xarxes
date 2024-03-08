#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <pthread.h>

#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "utilities/pduudp.h"
#include "utilities/logs.h"
#include "utilities/controllers.h"
#include "utilities/server_conf.h"

/*Define global mutex between threads*/
pthread_mutex_t mutex;

struct subsThreadArgs{
    struct Server *srvConf;
    struct Controller *controller;
    int *socket;
    char *situation;
};

void* subsProcess(void *args){
    /*Cast arguments*/
    struct subsThreadArgs *subsArgs = (struct subsThreadArgs*)args;

    /*Connection with client params*/
    struct timeval timeout;
    struct Packet subsPacket;
    struct sockaddr_in newAddress;
    int newUDPSocket;
    socklen_t addrlen = sizeof(newAddress);
    fd_set readfds;
    int received;

    /*SUBS_ACK data*/
    char rnd[9];
    char newPort[6];

    linfo("Starting a new thread for controller: %s.\n",false,subsArgs->controller->mac);

    /* Initialise new adress and port*/

    memset(&newAddress, 0, sizeof(newAddress));
    newAddress.sin_family = AF_INET; /* Set IPv4 */
    newAddress.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept any incoming address */
    newAddress.sin_port = 0; /* Port number */
    if ((newUDPSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        lerror("Error creating TCP socket for controller: %s",true,subsArgs->controller->mac);
    }
    if (bind(newUDPSocket, (struct sockaddr*)&newAddress, sizeof(newAddress)) < 0) {
        lerror("Error binding UDP socket",true);
    }

    /* Get the new Port number */
    if (getsockname(newUDPSocket, (struct sockaddr*)&newAddress, &addrlen) < 0) {
        lerror("Error getting UDP socket name",true);
    }

    /* [SUBS_ACK] */

        /* Create packet*/
        generateIdentifier(rnd);
        /*Convert uint16_t to string*/
        sprintf(newPort, "%d", ntohs(newAddress.sin_port));
        subsPacket = createPacket(SUBS_ACK,subsArgs->srvConf->mac,rnd,newPort);

        /* Send packet */
        sendUdp(*subsArgs->socket,subsPacket,&subsArgs->srvConf->udp_address);
    
        /*Set client to WAIT_INFO Status and assign identifier*/
        pthread_mutex_lock(&mutex); /*Lock variable*/
        subsArgs->controller->data.status=WAIT_INFO;
        pthread_mutex_unlock(&mutex); /*UNlock variable*/

    /* [SUBS_ACK] */

    /*Monitorize file descriptors*/
    FD_ZERO(&readfds);
    FD_SET(newUDPSocket, &readfds);
    /*Set select timeouts*/
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    /* [SUBS_INFO] */

    if((received=select(newUDPSocket + 1, &readfds, NULL, NULL, &timeout))<0){
        lerror("Error initialising select for controller thread: %s",true,subsArgs->srvConf->mac);
        
    }else if(received==0){ /*No packet received*/

        /*Set client to DISCONNECTED Status*/
        pthread_mutex_lock(&mutex); /*Lock variable*/
            subsArgs->controller->data.status=DISCONNECTED;
        pthread_mutex_unlock(&mutex); /*Unlock variable*/

        /*Close Socket connection*/
        close(newUDPSocket);
    }else{
        char *tcp;
        char *devices;
        /*Expect a SUBS_INFO Packet*/
        subsPacket = recvUdp(newUDPSocket,&newAddress);
        
        /*Lock strtok*/
        pthread_mutex_lock(&mutex);
            tcp = strtok(subsPacket.data,",");
            devices = strtok(NULL,",");
        pthread_mutex_unlock(&mutex); 

        /* Check Correct MAC, Identifier, TCP and devices */
        if(strcmp(subsPacket.mac,subsArgs->controller->mac) == 0 && strcmp(subsPacket.rnd,rnd) == 0 && tcp != NULL && devices != NULL){
            char tcpPort[6];
            sprintf(tcpPort,"%d",subsArgs->srvConf->tcp);

            /* [INFO_ACK] */
            
            sendUdp(newUDPSocket,createPacket(INFO_ACK,subsArgs->srvConf->mac,rnd,tcpPort),&newAddress);
            
            pthread_mutex_lock(&mutex); /*Lock variable*/
                /* Store Controller DATA */
                strcpy(subsArgs->controller->data.rand,rnd);
                strcpy(subsArgs->controller->data.situation,subsArgs->situation);
                /* Store devices */
                storeDevices(devices,subsArgs->controller->data.devices,";");
                /*Set SUBSCRIBED status*/
                subsArgs->controller->data.status=SUBSCRIBED;
            pthread_mutex_unlock(&mutex); /*Unlock variable*/
            
            /* [INFO_ACK] */

        } else { /*Wrong SUBS_INFO data*/
            linfo("Subscription ended for Controller: %s. Reason: Wrong Info in SUBS_INFO packet. Controller set to DISCONNECTED mode.", false, subsArgs->controller->mac);
            sendUdp(newUDPSocket, createPacket(SUBS_REJ, subsArgs->srvConf->mac, "00000000", "Subscription Denied: Wrong Info in SUBS_INFO packet."), &newAddress);
            
            /*Set client to DISCONNECTED Status*/
            pthread_mutex_lock(&mutex); /*Lock variable*/
                subsArgs->controller->data.status=DISCONNECTED;
            pthread_mutex_unlock(&mutex); /*Unlock variable*/
        }
    }

    /* [SUBS_INFO] */

    return NULL;
}

int main(int argc, char *argv[]) {
    /*Create Ints for sockets file descriptors*/
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
        linfo("Reading server configuration files...",false);
        /*Initialise server configuration struct*/
        serv_conf = serverConfig(config_file);

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

    linfo("Loading controllers...",false);
    /* Load allowed controllers in memory */
    numControllers = loadControllers(&controllers, controllers_file);
    if(numControllers==0){
        lerror("0 controllers loaded. Exiting...",true);
    } else {
        linfo("%d controllers loaded. Waiting for incoming connections...",false,numControllers);
    }

    /*Initialise mutex (Locks and unlocks)*/
    pthread_mutex_init(&mutex, NULL);
    
    while (1+1!=3) {
        struct subsThreadArgs subsArgs;
        struct Packet udp_packet;

        /* Init file descriptors readers */
        FD_ZERO(&readfds);
        FD_SET(tcp_socket, &readfds);
        FD_SET(udp_socket, &readfds);
        /* Get max range of file descriptor to check */
        max_fd = (tcp_socket > udp_socket) ? tcp_socket : udp_socket;

        /*Start monitoring file descriptors*/
        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            lerror("Unexpected error in select.",true);
        }
        
        /* Check if UDP file descriptor has received data */
        if (FD_ISSET(udp_socket, &readfds)) {
            /* Receive data and find the controller index, if it exists */
            int controllerIndex = 0;
            linfo("Received data in file descriptor UDP.", false);
            udp_packet = recvUdp(udp_socket, &serv_conf.udp_address);

            if ((controllerIndex = isAllowed(udp_packet, controllers, numControllers)) != -1) {
                
                /* Check if controller is disconnected */
                if ((controllers[controllerIndex].data.status == DISCONNECTED)){
                    /*Get situation*/
                    char* situation;
                    situation = strtok(udp_packet.data, ",");
                    situation = strtok(udp_packet.data, ",");

                    /* Check if packet has correct identifier and situation */
                    if((strcmp(udp_packet.rnd, "00000000") == 0) && (strcmp(situation, "000000000000") != 0)) {

                        /* Start new thread for the new Client connection */
                        num_threads++;
                        if ((threads = (pthread_t *)realloc(threads, num_threads * sizeof(pthread_t))) == NULL) {
                            lerror("Failed to reallocate thread.", true);
                        }
                        /*Create thread arguments*/
                        subsArgs.situation=situation;
                        subsArgs.srvConf = &serv_conf;
                        subsArgs.controller = &controllers[controllerIndex];
                        subsArgs.socket = &udp_socket;

                        if (pthread_create(&threads[num_threads - 1], NULL, subsProcess, (void*)&subsArgs) != 0) {
                            lerror("Thread creation failed", true);
                        }
                    } else { 
                        /* Reject Connection sending a [SUBS_REJ] packet */
                        linfo("Denied connection to Controller: %s. Reason: Wrong Situation or Code format.", false, udp_packet.mac);
                        sendUdp(udp_socket, createPacket(SUBS_REJ, serv_conf.mac, "00000000", "Subscription Denied: Wrong Situation or Code format."), &serv_conf.udp_address);
                    }
                }

            }else { /* Reject Connection sending a [SUBS_REJ] packet */
                linfo("Denied connection to Controller: %s. Reason: Not listed in allowed Controllers file.", false, udp_packet.mac);
                sendUdp(udp_socket, createPacket(SUBS_REJ, serv_conf.mac, "00000000", "Subscription Denied: You are not listed in allowed Controllers file."), &serv_conf.udp_address);
            }
        }

        /* Check if the TCP file descriptor has received data */   
        if (FD_ISSET(tcp_socket, &readfds)) {
            int client_socket;
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            if ((client_socket = accept(tcp_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                lerror("Unexpected error while receiving TCP connection.",true);
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