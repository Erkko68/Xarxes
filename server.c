#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>

/*Init debug mode setting*/
int DEBUG = false;

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
 * @brief Functions to show log messages
 * 
 * @param str The string to print
 * @param bool If set to true this message will ignore debug mode and always print the msg.
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

/**
 * @brief Returns a struct with the server configuration.
 * 
 * @param filename The name of the file to read the configuration.
 * @return struct server 
 */
struct server server_config(const char *filename) {
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
 * @param controlers A pointer to the controllers string
 */
void args(int argc, char *argv[], char **config_file, char **controlers) {
    int i = 1;
    /* Default file names */
    *config_file = "server.cfg";
    *controlers = "controllers.dat";
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
                *controlers = argv[i + 1];
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
    
int main(int argc, char *argv[]) {
    struct server my_server;
    /*Get config and controllers file name*/
    char *config_file;
    char *controlers;
    args(argc, argv, &config_file, &controlers);
    /*Initialise server configuration struct*/
    my_server = server_config(config_file);

    printf("Name: %s\n", my_server.name);
    printf("MAC: %s\n", my_server.mac);
    printf("Local TCP: %hu\n", my_server.tcp);
    printf("Server UDP: %hu\n", my_server.udp);

    return 0;
}