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

/*Struct for thread arguments*/
struct ThreadArgs{
    pthread_mutex_t mutex;
    int socket;
    struct Packet packet;
};

void* subsConnection(void* args){
    /* Cast arguments */
    struct ThreadArgs arguments = *((struct ThreadArgs*)args);

    
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
    numControllers = loadControllers(&controllers, controllers_file);

    while (1+1!=3) {
        struct Packet udp_packet;
        struct Controller new_controller;

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
            int controllerIndex;
            linfo("Received data in file descriptor UDP.",false);
            udp_packet = recvUdp(udp_socket,&serv_conf.udp_address);
            new_controller = udpToController(udp_packet);

            if ((controllerIndex = isAllowed(new_controller,controllers,numControllers)) != -1 ) {
                if (controllers[controllerIndex].data.status == DISCONNECTED &&   /*Client not connected*/
                    strcmp(udp_packet.rnd, "00000000") == 0 &&        /*Rnd number to zeros*/ 
                    strcmp(new_controller.data.situation, "00000000000") != 0) /*Situation initialized*/
                {
                    printf("Client is allowed: MAC: %s\n", udp_packet.mac);

                }else{ /* Reject Connection sending a [SUBS_REJ] packet*/
                    sendUdp(
                        udp_socket,
                        createPacket(SUBS_REJ,serv_conf.mac,"00000000","Subscription Denied: Wrong Situation or Code initialisation."),
                        &serv_conf.udp_address
                    );
                }
            }else{ /* Reject Connection sending a [SUBS_REJ] packet*/
                sendUdp(
                    udp_socket,
                    createPacket(SUBS_REJ,serv_conf.mac,"00000000","Subscription Denied: Not listed in allowed Controllers file."),
                    &serv_conf.udp_address
                );
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

    /*Free controllers*/
    free(controllers);

    /*Close the socket file descriptors*/
    close(udp_socket);
    close(tcp_socket);

    return 0;
}