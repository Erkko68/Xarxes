#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>

#include <unistd.h>
#include <arpa/inet.h>

#define PDUUDP 103

/*Init debug mode setting*/
bool DEBUG = false;

/**
 * @brief Functions to show log messages
 *
 * @param str The string to print
 * @param bool If set to true this message will ignore debug mode and always display the msg.
 */
void lerror(const char *str, bool override){
    if(DEBUG || override){
        printf("[Error] %s\n",str);
    }
    exit(-1);
}
void lwarning(const char *str, bool override){
    if(DEBUG || override){
        printf("[Warning] %s\n",str);
    }
}
void linfo(const char *str, bool override){
    if(DEBUG || override){
        printf("[Info] %s\n",str);
    }
}

/*Define struct for server config*/
struct server{
    char name[9];
    char mac[13];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
};

/*Define struct for controllers*/
struct controllers{
    char name[9];
    char mac[13];
};

/*Define struct for clients*/
struct client{
    unsigned char status;
    char name[9];
    char situation[13];
    char elements[10][8];
    char mac[13];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
};

/*Define struct for pdu_udp packets*/
struct pdu_udp{
    unsigned char type;
    char mac[13];
    char rnd[9];
    char data[80];
};

/**
 * @brief Converts a pdu_udp struct packet into a byte array 
 * 
 * @param pdu The struct representing the pdu_udp packet
 * @param bytes the byte array to store the encoded struct
 */
void udpToBytes(const struct pdu_udp *pdu, char *bytes) {
    int offset = 0;
    bytes[offset] = pdu->type;
    offset += sizeof(pdu->type);
    memcpy(bytes + offset, pdu->mac, sizeof(pdu->mac));
    offset += sizeof(pdu->mac);
    memcpy(bytes + offset, pdu->rnd, sizeof(pdu->rnd));
    offset += sizeof(pdu->rnd);
    memcpy(bytes + offset, pdu->data, sizeof(pdu->data));
}
/**
 * @brief Converts a byte array into pdu_udp packet format
 * 
 * @param bytes The byte array to take the data
 * @param pdu The pdu_udp packet struct to store decoded data
 */
void bytesToUdp(const char *bytes, struct pdu_udp *pdu) {
    int offset = 0;
    pdu->type = bytes[offset];
    offset += sizeof(pdu->type);
    memcpy(pdu->mac, bytes + offset, sizeof(pdu->mac));
    offset += sizeof(pdu->mac);
    memcpy(pdu->rnd, bytes + offset, sizeof(pdu->rnd));
    offset += sizeof(pdu->rnd);
    memcpy(pdu->data, bytes + offset, sizeof(pdu->data));
}


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
        /*Set config*/
        if (strcmp(key, "Name") == 0) {
            strncpy(srv.name, value, sizeof(srv.name) - 1);
            srv.name[sizeof(srv.name) - 1] = '\0';
        } else if (strcmp(key, "MAC") == 0) {
            strncpy(srv.mac, value, sizeof(srv.mac) - 1);
            srv.mac[sizeof(srv.mac) - 1] = '\0'; 
        } else if (strcmp(key, "Local-TCP") == 0) {
            srv.tcp = atoi(value);
        } else if (strcmp(key, "Srv-UDP") == 0) {
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
                lerror(" -c argument requires a file name",true);
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
            DEBUG = true;
        } else {
            lerror("Invalid argument found",true);
        }
    }
}

/**
 * @brief A function executed by the subscriptoin proces thread, wich handles the main
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
    socklen_t client_len = sizeof(client_addr);
    /* Define buffer size */
    char buffer[PDUUDP];

    /* Init client struct to zeros */
    memset(&client_addr, 0, sizeof(client_addr));

    while (1) {
        int bytes_received = recvfrom(sockfd, buffer, PDUUDP, 0,
                                     (struct sockaddr *)&client_addr, &client_len);
        struct pdu_udp original_pdu;
        if (bytes_received < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        /* Print received package information */
        bytesToUdp(buffer,&original_pdu);
        
        printf("Original Struct:\nType: %d\nMAC: %s\nRnd: %s\nData: %s\n\n",
           original_pdu.type, original_pdu.mac, original_pdu.rnd, original_pdu.data);

        /* Echo the message back to the client */ 
        /*
        if (sendto(sockfd, buffer, strlen(buffer), 0, 
                   (const struct sockaddr *)&client_addr, client_len) < 0) {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }
        */
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

    /* Create socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
    }

    /* Initialize struct to zeros */
    memset(&server_addr, 0, sizeof(server_addr));

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
    
    while(1){
        sleep(1);
        printf("\n");
    };

    /* Join subscription to main thread when finished (Won't happen) */
    pthread_join(subs_thread, NULL);

    /*Close the socket*/
    close(sockfd);

    return 0;
}