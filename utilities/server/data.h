
#include "../commons.h"

struct dataThreadArgs{
    int client_socket;
    struct Server *servConf;
    struct Controller *controllers;
};
