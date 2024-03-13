
#include "../commons.h"

void setData(struct Controller *controller, char *device, char *value, struct Server *srvConf){
    int sockfd;
    struct sockaddr_in client_addr;
    struct TCPPacket dataPacket;

    /* Create socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        lerror("Unexpected error opening socket", true);
    }

    /* Initialize server address struct */
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(controller->data.tcp);

    if (inet_pton(AF_INET, controller->data.ip, &client_addr.sin_addr) <= 0) {
        lerror("Unexpected error when setting adress:",true);
    }

    if (connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        lerror("Connection to controller %s failed", true,controller->mac);
    }

    /* Create and send SET_DATA packet */
    sendTcp(sockfd,createTCPPacket(SET_DATA,srvConf->mac,controller->data.rand,device,value,"Hello"));

    /**************/
    /*Set a select*/
    /**************/
    /* Recv packet */

    dataPacket = recvTcp(sockfd);
    
    switch (dataPacket.type) {
        case DATA_ACK:
            printf("Received SEND_ACK packet\n");
            // Handle SEND_ACK packet
            break;
        case DATA_NACK:
            printf("Received SEND_NACK packet\n");
            // Handle SEND_NACK packet
            break;
        case DATA_REJ:
            printf("Received SEND_REJ packet\n");
            // Handle SEND_REJ packet
            break;
        default:
            printf("Unknown packet type\n");
            // Handle unknown packet type
            break;
    }

    close(sockfd);
}