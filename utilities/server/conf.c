/**
 * @file server:conf.h
 * @brief Functions to load and parse arguments for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

#include "../commons.h"

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
void readArgs(int argc, char *argv[], char **config_file, char **controllers) {
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