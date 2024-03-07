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

void* subsProcess(void* args){
    /*Cast controller*/
    struct Controller controller = *((struct Controller*)args);
    linfo("Starting a new thread for controller: %s.\n",false,controller.mac);

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
    /*Initialise file descriptors select*/
    fd_set readfds;
    int max_fd;
    
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

    /*Initialise mutex*/
    pthread_mutex_init(&mutex, NULL);
    
    while (1+1!=3) {
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
            /*Receive data and find the controller index, in case it exists*/
            int controllerIndex;
            linfo("Received data in file descriptor UDP.",false);
            udp_packet = recvUdp(udp_socket,&serv_conf.udp_address);

            if ((controllerIndex = isAllowed(udp_packet,controllers,numControllers)) != -1 ) {
                if (controllers[controllerIndex].data.status == DISCONNECTED && strcmp(udp_packet.rnd, "00000000") == 0){
                    /*Start new HELLO Protocol for the new Client*/
                    num_threads++;
                    if((threads = (pthread_t *)realloc(threads, num_threads * sizeof(pthread_t))) == NULL){
                        lerror("Failed to reallocate thread.",true);
                    }
                    if (pthread_create(&threads[num_threads - 1], NULL, subsProcess,(void*)&controllers[controllerIndex]) != 0) {
                        lerror("Thread creation failed",true);
                    }
                    /*Join thread when finished*/
                    pthread_join(threads[num_threads], NULL);
                    
                }else{ /* Reject Connection sending a [SUBS_REJ] packet*/
                    linfo("Denied connection to Controller: %s. Reason: Wrong Situation or Code format.",false,udp_packet.mac);
                    sendUdp(
                        udp_socket,
                        createPacket(SUBS_REJ,serv_conf.mac,"00000000","Subscription Denied: Wrong Situation or Code format."),
                        &serv_conf.udp_address
                    );
                }
            }else{ /* Reject Connection sending a [SUBS_REJ] packet*/
                linfo("Denied connection to Controller: %s. Reason: Not listed in allowed Controllers file.",false,udp_packet.mac);
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
    /*Free threads*/
    free(threads);

    /*Close the socket file descriptors*/
    close(udp_socket);
    close(tcp_socket);

    return 0;
}