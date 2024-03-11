
#include "../commons.h"

struct dataThreadArgs{
    int client_socket;
    struct Server servConf;
    struct sockaddr_in clientAddr;
    struct Controller *controllers;
    int numControllers;
};
