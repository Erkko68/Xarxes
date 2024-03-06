#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <pthread.h>

#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "utilities/pduudp.h"
#include "utilities/logs.h"
#include "utilities/controllers.h"
#include "utilities/server_conf.h"

/*Struct for thread arguments*/
struct ThreadArgs{
    pthread_mutex_t mutex;
    int socket;
    struct Packet packet;
};

/**
 * @brief A function executed by the subscription proces thread, wich handles the
 *        proces of subscription for the incoming clients.
 * 
 * @param arg The socket file descriptor
 * @return void* 
 */
void *subsReq(void *arg) {
    /* Get args */
    struct ThreadArgs ar = *((struct ThreadArgs*)arg);
    /* Init client info */
    struct sockaddr_in client_addr;
    struct Packet original_pdu;
    char buffer[103];

    while (true) {
        
        original_pdu = recvUdp(ar.socket,&client_addr);
        
        printf("Original Struct:\nType: %d\nMAC: %s\nRnd: %s\nData: %s\n\n",
           original_pdu.type, original_pdu.mac, original_pdu.rnd, original_pdu.data);

        udpToBytes(&original_pdu,buffer);

        sendUdp(ar.socket,buffer,&client_addr);
        
    }
}

int main(int argc, char *argv[]) {
    /*Create Ints for sockets file descriptors*/
    int tcp_socket, udp_socket, max_fd;
    fd_set readfds;
    /*Struct for server configuration*/
    struct Server serv_conf;
    /*Array of structs for allowed clients in memory*/
    struct Controller *controllers = NULL;
    int numControllers;
    
    /*Get config and controllers file name*/
    char *config_file;
    char *controllers_file;
    readArgs(argc, argv, &config_file, &controllers_file);

    /*Initialize Sockets*/

        /* Create UDP socket file descriptor */
        if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            lerror("Error creating TCP socket",true);
        }
        /* Create UDP socket file descriptor */
        if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            lerror("Error creating UDP socket",true);
        }

        /*Initialise server configuration struct*/
        serv_conf = serverConfig(config_file);

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
    numControllers = initialiseControllers(&controllers, controllers_file);

    while (1) {
        struct Packet udp_packet;

        /* Init file descriptors macros */
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
            linfo("Received data in file descriptor UDP.",false);
            udp_packet = recvUdp(udp_socket,&serv_conf.udp_address);
            if(isAllowed(udp_packet.mac,controllers,numControllers)==1){
                printf("Allowed MAC: %s\n",udp_packet.mac);
            }else{
                printf("Not allowed\n");
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

    /*Mem dealloc the controllers*/
    free(controllers);

    /*Close the socket*/
    close(udp_socket);
    close(tcp_socket);

    return 0;
}