
#include "../commons.h"

struct dataThreadArgs{
    int client_socket;
    struct Server *servConf;
    struct Controller *controllers;
};


void dataPetition(struct Controller *controller, char *device, char *value, struct Server *srvConf);