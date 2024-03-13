
#include "../commons.h"

struct dataThreadArgs{
    int client_socket;
    struct Server *servConf;
    struct Controller *controllers;
};


void setData(struct Controller *controller,int device);