#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Define struct for server config*/
struct server{
    char name[9];
    char mac[13];
    /*Range 0-65535*/
    unsigned short tcp; 
    unsigned short udp;
};

/*Define struct for clients*/
struct client{
    char name[9];
    char situation[13];
    char elements[10][8];
    char mac[13];
    /*Range 0-65535*/
    unsigned short tcp;
    unsigned short udp;
};

/*Define struct for pdu_udp packets*/
struct pdu_udp{
    unsigned char type;
    char mac[13];
    char rnd[9];
    char data[80];
};

/*Function to read server config*/
struct server server_config(const char *filename) {
    /*Create new struct*/
    struct server srv;
    /*Initialise buffer*/
    char buffer[20];

    /*Open file descriptor*/
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
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

/*Function to parse command line arguments and get the file name*/
char *args(int argc, char *argv[]) {
    char *filename = "server.cfg"; /*Default file name*/

    /*Loop through command line arguments*/
    for (int i = 1; i < argc; i++) {
        /*Check for -c flag*/
        if (strcmp(argv[i], "-c") == 0) {
            /*If -c flag is found, check if there's a file name next*/
            if (i + 1 < argc) {
                filename = argv[i + 1];
            } else {
                printf("[Error] -c argument requires a file name.\n");
                exit(1);
            }
        } else if ((strcmp(argv[i], "-d") == 0)) {
            printf("hi");
        } else {
            printf("[Error] Invalid argument: %s\n",argv[i]);
        }
    }

    return filename;
}



int main(int argc, char *argv[]) {
    /* Get */
    struct server my_server = server_config(args(argc,argv));

    printf("Name: %s\n", my_server.name);
    printf("MAC: %s\n", my_server.mac);
    printf("Local TCP: %hu\n", my_server.tcp);
    printf("Server UDP: %hu\n", my_server.udp);

    return 0;
}
