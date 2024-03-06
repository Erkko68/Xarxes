#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>

#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "utilities/pduudp.h"
#include "utilities/logs.h"
#include "utilities/controllers.h"

/*Define struct for server config*/
struct Server{
    char name[9];
    char mac[13];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
    struct sockaddr_in tcp_address;
    struct sockaddr_in udp_address;
};

/*Struct for thread arguments*/
struct ThreadArgs{
    pthread_mutex_t mutex;
    int socket;
    struct Packet packet;
};

/**
 * @brief Returns a struct with the server configuration.
 * 
 * @param filename The name of the file to read the configuration.
 * @return struct server 
 */
struct Server serverConfig(const char *filename) {
    /*Create new struct*/
    struct Server srv;
    /*Initialise buffer*/
    char buffer[20];

    /*Open file descriptor*/
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        lerror("Error opening file",false);
    }
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        /*String tokenizer (Split key/value)*/
        char *key = strtok(buffer, "=");
        char *value = strtok(NULL, "\n");
        /*Set config, CAUTTION: We increase the value pointer by one to point the string after the space */
        if (strcmp(key, "Name ") == 0) {
            strncpy(srv.name, value+1, sizeof(srv.name) - 1); 
            srv.name[sizeof(srv.name) - 1] = '\0';
        } else if (strcmp(key, "MAC ") == 0) {
            strncpy(srv.mac, value+1, sizeof(srv.mac) - 1);
            srv.mac[sizeof(srv.mac) - 1] = '\0'; 
        } else if (strcmp(key, "TCP-Port ") == 0) {
            srv.tcp = atoi(value);
        } else if (strcmp(key, "UDP-Port ") == 0) {
            srv.udp = atoi(value);
        }
    }
    /* Configure UDP server address */
    memset(&srv.udp_address, 0, sizeof(srv.udp_address));
    srv.udp_address.sin_family = AF_INET; /* Set IPv4 */
    srv.udp_address.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept any incoming address */
    srv.udp_address.sin_port = htons(srv.udp); /* Port number */

    /* Configure TCP server address */
    memset(&srv.tcp_address, 0, sizeof(srv.tcp_address));
    srv.tcp_address.sin_family = AF_INET; /* Set IPv4 */
    srv.tcp_address.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept any incoming address */
    srv.tcp_address.sin_port = htons(srv.tcp); /* Port number */

    /*close file descriptor*/ 
    fclose(file);
    /*return new struct*/ 
    return srv;
}

/**
 * @brief Function to parse command line arguments and get the file name.
 * 
 * @param argc The number of args
 * @param argv An array containing all the args
 * @param config_file A pointer to the config_file string
 * @param controllers A pointer to the controllers string
 */
void args(int argc, char *argv[], char **config_file, char **controllers) {
    int i = 1;
    /* Default file names */
    *config_file = "server.cfg";
    *controllers = "controllers.dat";
    /* Loop through command line arguments */
    for (;i < argc; i++) {
        /* Check for -c flag */
        if (strcmp(argv[i], "-c") == 0) {
            /* If -c flag is found, check if there's a file name next */
            if (i + 1 < argc) {
                *config_file = argv[i + 1];
                i++; /* Ignore next arg */
            } else {
                lerror(" -c argument requires a file name.",true);
            }
        } else if (strcmp(argv[i], "-u") == 0) {
            /* If -u flag is found, check if there's a file name next */
            if (i + 1 < argc) {
                *controllers = argv[i + 1];
                i++; /* Ignore next arg */
            } else {
                lerror(" -u argument requires a file name.",true);
            }
        } else if (strcmp(argv[i], "-d") == 0) {
            /* If -d flag is found, set debug mode*/
            enableDebug();
        } else {
            lerror("Invalid argument found.",true);
        }
    }
}

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
    int tcp_socket, udp_socket;
    /*Struct for server configuration*/
    struct Server serv_conf;
    /*Array of structs for allowed clients in memory*/
    struct Controller *controllers = NULL;
    int numControllers;

    /*Threads*/
    /*pthread_t subs_thread;*/
    struct sockaddr_in client_addr;
    struct Packet pduUdp;
    
    /*Get config and controllers file name*/
    char *config_file;
    char *controllers_file;
    args(argc, argv, &config_file, &controllers_file);

    /*Initialise server configuration struct*/
    serv_conf = serverConfig(config_file);

    /*Initialize Sockets*/

        /* Create UDP socket file descriptor */
        if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            lerror("Error creating TCP socket",true);
        }
        /* Create UDP socket file descriptor */
        if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            lerror("Error creating UDP socket",true);
        }
        /* Bind TCP socket */
        if (bind(tcp_socket, (const struct sockaddr *)&serv_conf.tcp_address, sizeof(serv_conf.tcp_address)) < 0) {
            lerror("Error binding TCP socket",true);
        }
        /* Bind UDP socket */
        if (bind(udp_socket, (const struct sockaddr *)&serv_conf.udp_address, sizeof(serv_conf.udp_address)) < 0) {
            lerror("Error binding UDP socket",true);
        }

    /* Load allowed controllers in memory */
    numControllers = initialiseControllers(&controllers, controllers_file);

    if (listen(tcp_socket, 5) == -1) {
        perror("TCP listen failed");
        exit(EXIT_FAILURE);
    }

    fd_set fds;
    int max_fd;
    while (1) {

        FD_ZERO(&fds);
        FD_SET(tcp_socket, &fds);
        FD_SET(udp_socket, &fds);

        max_fd = (tcp_socket > udp_socket) ? tcp_socket : udp_socket;

        if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) {
            lerror("Unexpected error in select.",true);
        }
        
        if (FD_ISSET(udp_socket, &fds)) {
            printf("hi");
            pduUdp = recvUdp(udp_socket,&client_addr);
            struct Controller a;
            strcpy(a.mac,pduUdp.mac);
            if(isAllowed(a,controllers,numControllers)){
                printf("Allowed");
            }else{
                printf("hi");
            }

            /*
            if (pthread_create(&subs_thread, NULL,subsReq, &targs) != 0) {
                perror("pthread_create failed");
                exit(EXIT_FAILURE);
            }
            */
        }
                
        if (FD_ISSET(tcp_socket, &fds)) {
            int client_socket;
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            if ((client_socket = accept(tcp_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
                lerror("Unexpected error while receiving TCP connection.",true);
            }

            /*pthread_create(&client_threads[num_clients], NULL, tcp_client_handler, (void *)&client_socket);*/
        }
        

    }

    /* Join subscription to main thread when finished */
    /*pthread_join(subs_thread, NULL);*/

    /*Mem dealloc the controllers*/
    free(controllers);

    /*Close the socket*/
    close(udp_socket);
    close(tcp_socket);

    return 0;
}