
#include "../commons.h"

void setData(struct Controller *controller, char *device, char *value, struct Server *srvConf){
    int dataSckt;
    struct sockaddr_in client_addr;
    struct TCPPacket dataPacket;
    struct timeval tcpTimeout;
    /* Packet msg */
    const char *result;
    char msg[80];

    /* Create socket */
    if ((dataSckt = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        lerror("Unexpected error opening socket", true);
    }

    /* Initialize server address struct */
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(controller->data.tcp);

    if (inet_pton(AF_INET, controller->data.ip, &client_addr.sin_addr) <= 0) {
        lerror("Unexpected error when setting adress:",true);
    }

    if (connect(dataSckt, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        lerror("Connection to controller %s failed", true,controller->mac);
    }

    /* Create and send SET_DATA packet */
    sendTcp(dataSckt,createTCPPacket(SET_DATA,srvConf->mac,controller->data.rand,device,value,"Hello"));

    /*Set select timeout*/
    tcpTimeout.tv_sec = 3;
    tcpTimeout.tv_usec = 0;
    if(setsockopt(dataSckt,SOL_SOCKET,SO_RCVTIMEO,(const char*)&tcpTimeout,sizeof(tcpTimeout)) < 0){
        lerror("Unexpected error when setting TCP socket settings",true);
    }

    /* Recv packet */
    dataPacket = recvTcp(dataSckt);

    switch (dataPacket.type) {
        case DATA_ACK:
            linfo("Received confirmation for device %s. Storing data...",true,device);
            if ((result = save(&dataPacket,controller)) == NULL){
                linfo("Controller %s updated %s. Value: %s", false, dataPacket.mac,dataPacket.device,dataPacket.value);
            } else {
                /* Print fail messages */
                sprintf(msg,"Couldn't store %s data %s.",dataPacket.device,result);
                lwarning("Couldn't store %s data from Controller: %s. Reason: %s", false,dataPacket.device,dataPacket.mac,result);
                /* Send error packet */
                sendTcp(dataSckt, createTCPPacket(DATA_NACK,controller->mac,controller->data.rand,dataPacket.device,dataPacket.value,msg));
                /* Disconnect packet */
                disconnectController(controller);
            }
            break;
        case DATA_NACK:
            lwarning("Couldn't set device info: %s",true,dataPacket.data);
            break;
        case DATA_REJ:
            lwarning("Controller rejected data. Disconecting...",true);
            disconnectController(controller);
            break;
        default:
            lwarning("Unknown packet received",true);
            break;
    }

    close(dataSckt);
}