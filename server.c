#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>

#include <unistd.h>
#include <arpa/inet.h>

#include "utilities/pduudp.h"
#include "utilities/logs.h"
#include "utilities/controllers.h"

/*Define struct for server config*/
struct server{
    char name[9];
    char mac[13];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
};

/*Define struct for controllers*/
struct controllers{
    char name[8];
    char mac[13];
};

/**
 * @brief Returns a struct with the server configuration.
 * 
 * @param filename The name of the file to read the configuration.
 * @return struct server 
 */
struct server serverConfig(const char *filename) {
    /*Create new struct*/
    struct server srv;
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
    /* Get socket */
    int sockfd = *((int *)arg);
    /* Init client info */
    struct sockaddr_in client_addr;
    struct Packet original_pdu;
    char buffer[103];

    while (true) {
        
        original_pdu = recvUdp(sockfd,&client_addr);
        
        printf("Original Struct:\nType: %d\nMAC: %s\nRnd: %s\nData: %s\n\n",
           original_pdu.type, original_pdu.mac, original_pdu.rnd, original_pdu.data);

        udpToBytes(&original_pdu,buffer);

        sendUdp(sockfd,buffer,&client_addr);
        
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    pthread_t subs_thread;
    struct server serv_conf;
    struct sockaddr_in server_addr;
    
    /*Get config and controllers file name*/
    char *config_file;
    char *controllers;
    args(argc, argv, &config_file, &controllers);

    /*Initialise server configuration struct*/
    serv_conf = serverConfig(config_file);

    lwarning("hi",false);

    /* Create socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
    }

    /* Configure server address */
    server_addr.sin_family = AF_INET; /* Set IPv4 */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept any incoming address */
    server_addr.sin_port = htons(serv_conf.udp); /* Port number */

    /* Bind socket to the specified port */
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 11001...\n");

    /* Create a thread to handle the subscription request proces */
    if (pthread_create(&subs_thread, NULL,subsReq, &sockfd) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }

    /* Join subscription to main thread when finished */
    pthread_join(subs_thread, NULL);

    /*Close the socket*/
    close(sockfd);

    return 0;
}