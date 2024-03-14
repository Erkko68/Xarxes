
#include "../commons.h"

/**
 * @brief Function to get the name of a TCP type enum value.
 *
 * This function takes a TCP type enum value and returns its corresponding name as a string.
 *
 * @param type The TCP type enum value.
 * @return The name of the TCP type enum value as a string.
 */
const char* getTCPName(enum TCPType type) {
    switch (type) {
        case SEND_DATA: return "SEND_DATA";
        case SET_DATA: return "SET_DATA";
        case GET_DATA: return "GET_DATA";
        case DATA_ACK: return "DATA_ACK";
        case DATA_NACK: return "DATA_NACK";
        case DATA_REJ: return "DATA_REJ";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Function to save TCPPacket data to a file.
 *
 * This function saves the data from a TCPPacket struct to a file, appending it to an existing file
 * or creating a new one if necessary. The data is formatted and written to the file along with
 * information about the controller and the current timestamp.
 *
 * @param packet The TCPPacket struct containing data to be saved.
 * @param controller The Controller struct containing information about the controller.
 * @return 0 if successful, -1 if failed to open/create file.
 */
const char* save(struct TCPPacket *packet, struct Controller *controller) {
    char filename[50], date_str[9];
    FILE *file;
    time_t now;
    struct tm *local_time;
    /* Get current time */
    time(&now);
    local_time = localtime(&now);

    /* Create filename using name and situation */
    /**!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!REMOVE LOGS/!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!**/
    /**!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!REMOVE LOGS/!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!**/
    /**!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!REMOVE LOGS/!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!**/
    sprintf(filename, "logs/%s-%s.data", controller->name, controller->data.situation);

    /* Open file or create one */
    if ((file = fopen(filename, "a")) == NULL) {
        return strerror(errno);
    }

    /* Save data */
    strftime(date_str, sizeof(date_str), "%d-%m-%y", local_time);

    /* Write data to file */
    if (fprintf(file, "%s,%s,%s,%s,%s\n", date_str, get_current_time(), getTCPName(packet->type), packet->device, packet->value) < 0) {
        return strerror(errno);
    }

    fclose(file);
    return NULL;
}


/**
 * @brief Function to handle data petition communication.
 *
 * This function establishes a connection to a controller, sends a packet containing data, and handles
 * the response accordingly. If successful, it stores the received data to a file. If unsuccessful, it
 * logs the error and may send a corresponding error packet back to the controller.
 *
 * @param st Pointer to a struct dataPetition containing necessary arguments.
 * @return NULL
 */
void *dataPetition(void *st){
    struct dataPetition *args = (struct dataPetition*)st;
    int dataSckt;
    unsigned char packetType;
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
    client_addr.sin_port = htons(args->controller->data.tcp);

    if (inet_pton(AF_INET, args->controller->data.ip, &client_addr.sin_addr) <= 0) {
        lerror("Unexpected error when setting adress:",true);
    }

    if (connect(dataSckt, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        close(dataSckt);
        lerror("Connection to controller %s failed", true,args->controller->mac);
    }
    /* Check if we want to send or get data */
    if( strcmp(args->value,"") == 0 ){
        packetType = GET_DATA;
    } else {
        packetType = SET_DATA;
    }

    /* Create and send SET_DATA packet */
    sendTcp(dataSckt,createTCPPacket(packetType,args->servConf->mac,args->controller->data.rand,args->device,args->value,""));

    /*Set select timeout*/
    tcpTimeout.tv_sec = 3;
    tcpTimeout.tv_usec = 0;
    if(setsockopt(dataSckt,SOL_SOCKET,SO_RCVTIMEO,(const char*)&tcpTimeout,sizeof(tcpTimeout)) < 0){
        close(dataSckt);
        lerror("Unexpected error when setting TCP socket settings",true);
    }

    /* Recv packet */
    dataPacket = recvTcp(dataSckt);
    
    pthread_mutex_lock(&mutex);
        switch (dataPacket.type) {
            case DATA_ACK:

                linfo("Received confirmation for device %s. Storing data...",true,args->device);
                if ((result = save(&dataPacket,args->controller)) == NULL){
                    linfo("Controller %s updated %s. Value: %s", false, dataPacket.mac,dataPacket.device,dataPacket.value);
                } else {
                    /* Print fail messages */
                    sprintf(msg,"Couldn't store %s data %s.",dataPacket.device,result);
                    lwarning("Couldn't store %s data from Controller: %s. Reason: %s", false,dataPacket.device,dataPacket.mac,result);
                    /* Send error packet */
                    sendTcp(dataSckt, createTCPPacket(DATA_NACK,args->controller->mac,args->controller->data.rand,dataPacket.device,dataPacket.value,msg));
                    /* Disconnect packet */
                    disconnectController(args->controller);
                }
                break;

            case DATA_NACK:

                lwarning("Couldn't set device info: %s",true,dataPacket.data);
                break;

            case DATA_REJ:

                lwarning("Controller rejected data. Disconecting...",true);
                disconnectController(args->controller);
                break;

            default:

                lwarning("Unknown packet received",true);
                break;
        }
    pthread_mutex_unlock(&mutex);

    close(dataSckt);
    free(args);

    return NULL;
}


/**
 * @brief Function to handle storing data received over TCP.
 *
 * This function receives data over a TCP socket, validates the packet, and stores the data
 * if the conditions are met. It then sends an acknowledgment or rejection packet back to
 * the controller based on the outcome.
 *
 * @param args Pointer to a struct dataThreadArgs containing necessary arguments.
 * @return NULL
 */
void* dataReception(void* args){
    struct dataThreadArgs *dataArgs = (struct dataThreadArgs*)args;
    struct TCPPacket tcp_packet;

    int controllerIndex;
    unsigned char packetType;
    char msg[80];

    /*Get Packet*/
    tcp_packet = recvTcp(dataArgs->client_socket);
    if(tcp_packet.type == 0xF){
        lwarning("Haven't received data trough TCP socket in 3 seconds. Clossing socket...",false);
        close(dataArgs->client_socket);
        return NULL;
    }

    /*Check its SEND_DATA*/
    if(tcp_packet.type != SEND_DATA){
        lwarning("Received unexpected packet by controller %s. Expected [SEND_DATA].",false,tcp_packet.mac);
        return NULL;
    }
    pthread_mutex_lock(&mutex);
        /*Check allowed controller*/
        if((controllerIndex = isTCPAllowed(tcp_packet, dataArgs->controllers)) != -1){ 
            /*Check correct status*/
            if(dataArgs->controllers[controllerIndex].data.status == SEND_HELLO){
                /*Check if controller has device*/
                if(hasDevice(tcp_packet.device,&dataArgs->controllers[controllerIndex]) != -1){
                    const char *result;
                    /*Check error msg*/
        /*---->*/    if ((result = save(&tcp_packet,&dataArgs->controllers[controllerIndex])) == NULL){
                        linfo("Controller %s updated %s. Value: %s", false, tcp_packet.mac,tcp_packet.device,tcp_packet.value);
                        packetType = DATA_ACK;
                    } else {
                        sprintf(msg,"Couldn't store %s data %s.",tcp_packet.device,result);
                        lwarning("Couldn't store %s data from Controller: %s. Reason: %s", false,tcp_packet.device,tcp_packet.mac,result);
                        packetType = DATA_NACK;
                        disconnectController(&dataArgs->controllers[controllerIndex]);
                    }
                } else {
                    sprintf(msg,"Controller doesn't have %s device.",tcp_packet.device);
                    linfo("Denied connection to Controller: %s. Reason: Controller doesn't have %s device.", false, tcp_packet.mac,tcp_packet.device);
                    packetType = DATA_NACK;
                    disconnectController(&dataArgs->controllers[controllerIndex]);
                }
            } else {
                sprintf(msg,"Controller is not in SEND_HELLO status.");
                linfo("Denied connection to Controller: %s. Reason: Controller is not in SEND_HELLO status.", false, tcp_packet.mac);
                packetType = DATA_REJ;
                disconnectController(&dataArgs->controllers[controllerIndex]);
            }
        } else {
            sprintf(msg,"Not listed in allowed Controllers file.");
            linfo("Denied connection to Controller: %s. Reason: Not listed in allowed Controllers file.", false, tcp_packet.mac);
            packetType = DATA_REJ;
            disconnectController(&dataArgs->controllers[controllerIndex]);
        }

        /* Send response */
        sendTcp(dataArgs->client_socket, 
                createTCPPacket(packetType,
                                dataArgs->servConf->mac,
                                dataArgs->controllers[controllerIndex].data.rand,
                                tcp_packet.device,
                                tcp_packet.value,
                                msg
                                )
                );
        /*Close comunication*/
        close(dataArgs->client_socket);

    pthread_mutex_unlock(&mutex);

    return NULL;
}