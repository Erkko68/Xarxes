/**
 * @file controllers.c
 * @brief Functions for saving, loading, and managing clients for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.3
 * @date 2024-3-14
 */

#include "../commons.h"

/* Only used to read the controllers */
typedef struct {
    char name[9];
    char mac[13];
} ctrl;

ctrl getNextLine(FILE* file) {
    char line[25];
    ctrl result;

    if (fgets(line, 25, file) != NULL) {
        char *token = strtok(line, ",");
        if (token != NULL) {
            strncpy(result.name, token, 8);
            result.name[8] = '\0';
            token = strtok(NULL, ",");
            if (token != NULL) {
                strncpy(result.mac, token, 12);
                result.mac[12] = '\0';
            }
        }
    }

    return result;
}

void initializeControllerInfo(struct ControllerInfo *info) {
    int i;
    info->status = DISCONNECTED;
    info->situation[0] = '\0';
    info->rand[0] = '\0';
    for (i = 0; i < 11; i++) {
        info->devices[i][0] = '\0';
    }
    info->tcp = 0;
    info->udp = 0;
    info->ip[0] = '\0';
    info->lastPacketTime = 0;
}

struct Controller* addController(struct Controller *controllers, int *numControllers, const char *name, const char *mac) {
    (*numControllers)++;
    
    controllers = realloc(controllers, (*numControllers) * sizeof(struct Controller));
    if (controllers == NULL) {
        lerror("Memory reallocation failed\n",true);
    }

    strncpy(controllers[*numControllers - 1].name, name, 8);
    controllers[*numControllers - 1].name[8] = '\0';
    strncpy(controllers[*numControllers - 1].mac, mac, 12);
    controllers[*numControllers - 1].mac[12] = '\0';
    initializeControllerInfo(&controllers[*numControllers - 1].data);

    printf("%s,%s\n",controllers[*numControllers-1].name,controllers[*numControllers-1].mac);

    return controllers;
}

/**
 * @brief Reads controller data from a file and dynamically allocates memory for each controller struct.
 *
 * This function reads controller data from the specified file 'filename' and dynamically allocates memory 
 * for each controller struct found in the file. It parses each line of the file, extracting the name and 
 * MAC address of each controller. Memory is allocated for each controller struct using realloc as they 
 * are encountered in the file. The total number of controllers read is returned.
 * 
 * @param controllers Pointer to a pointer to the Controller struct array where the controller data will be stored.
 * @param filename The name of the file to read controller data from.
 * @return Returns the total number of controllers read from the file on success, or -1 on failure.
 */
int loadControllers(struct Controller **controllers, const char *filename) {
    int numControllers = 0;
    ctrl controller;
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        lerror("Could not open filedescriptor while reading controllers.", true);
        return -1;
    }

    while (!feof(file)) {
        controller = getNextLine(file);
        *controllers = addController(*controllers,&numControllers,controller.name,controller.mac);
    }

    *controllers = addController(*controllers,&numControllers,"NULL","NULL");
    numControllers--;

    fclose(file);

    return numControllers;
}



/**
 * @brief Checks if a controller is allowed.
 *
 * This function checks if the given packet, specified by the MAC address and name, is allowed based on the
 * provided array of controllers. It iterates through the array of controllers and compares the MAC address
 * and name of each controller with the given packet. If a both values are equal, the controller is
 * considered allowed and the function returns the index of the controller. Otherwise, the controller is considered
 * not allowed and the function returns -1.
 * 
 * @param packet The packet struct to check.
 * @param controllers Pointer to the array of Controller structs containing allowed controllers.
 * @return Returns 1 if the controller is allowed, 0 otherwise.
 */
int isUDPAllowed(const struct UDPPacket packet, struct Controller *controllers) {
    int i;
    /* Make a copy of packet data*/
    char data_copy[80];
    char* name;
    pthread_mutex_lock(&mutex);

        strcpy(data_copy, packet.data);
        /*Tokenize copied data*/
        
        name = strtok(data_copy, ",");
        if(name == NULL) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }

        /*Iterate over allowed controllers*/
        for (i = 0; strcmp(controllers[i].name,"NULL") != 0; i++) {
            if (strcmp(packet.mac, controllers[i].mac) == 0 && 
                strcmp(name, controllers[i].name) == 0) {
                /*Return index*/
                pthread_mutex_unlock(&mutex);
                return i;
            }
        }

    pthread_mutex_unlock(&mutex);

    return -1;
}
/**
 * @brief Checks if a TCP packet is allowed.
 *
 * This function iterates through the provided array of controllers and compares the MAC address and
 * random data (rnd) of the given TCP packet with each controller. If a matching controller is found,
 * the function returns the index of the controller in the array. Otherwise, it returns -1 indicating
 * that the packet is not allowed.
 * 
 * @param packet The TCPPacket struct representing the TCP packet to check.
 * @param controllers Pointer to the array of Controller structs containing allowed controllers.
 * @return Returns the index of the allowed controller if found, otherwise returns -1.
 */
int isTCPAllowed(const struct TCPPacket* packet, struct Controller *controllers) {
    int i;
        
    /*Iterate over allowed controllers*/
    for (i = 0; strcmp(controllers[i].name,"NULL") != 0 ; i++) {
        if (strcmp(packet->mac, controllers[i].mac) == 0 ) {
            /*Return index*/
            return i;
        }
    }

    return -1;
}


/**
 * @brief Tokenizes and stores an string into diferent devices names.
 *
 * This function tokenizes the given string using the specified delimiter and stores the tokens into the provided array.
 * It iterates through the string, extracting tokens using strtok function until no more tokens are found or the maximum
 * number of tokens is reached. Each token is copied into devices struct.
 * 
 * @param devices The string to tokenize.
 * @param deviceArray Pointer to the array where tokens will be stored.
 * @param delimiter The delimiter used to tokenize the string.
 */
void storeDevices(char *devices, char deviceArray[][8], char *delimiter) {
    int i = 0;
    char *device;
    device = strtok(devices, delimiter);
    while (device != NULL && i < 10) {
        strncpy(deviceArray[i], device, sizeof(deviceArray[i]) - 1);
        deviceArray[i][sizeof(deviceArray[i]) - 1] = '\0';
        device = strtok(NULL, delimiter);
        i++;
    }
    strncpy(deviceArray[i],"NULL",sizeof(deviceArray[i]) - 1);
}

/**
 * @brief Checks if a controller with the given name exists in the array of controllers.
 *
 * This function iterates through the array of controllers and compares each controller's name with the given name.
 * If a controller with the same name is found, its index is returned. If no matching controller is found, -1 is returned.
 * 
 * @param name The name of the controller to search for.
 * @param controllers Pointer to the array of controllers.
 * @return int The index of the controller if found, otherwise -1.
 */
int hasController(char *name,struct Controller *controllers){
    int i;
    for (i = 0; strcmp(controllers[i].name,"NULL"); i++) {
        if (strcmp(name, controllers[i].name) == 0) {
            /*Return index*/
            return i;
        }
    }
    return -1;
}


/**
 * @brief Checks if a device exists in the deviceArray.
 *
 * This function iterates through the deviceArray to check if the specified device exists in the array.
 * It compares each device name with the given device until a match is found or the end of the array is reached.
 * 
 * @param device The device name to search for.
 * @param deviceArray Pointer to the array containing device names.
 * @return If the device is found, returns the index of the device in the array. Otherwise, returns -1.
 */
int hasDevice(const char *device, const struct Controller *controller) {
    int i;
    
    for (i = 0; strcmp(controller->data.devices[i],"NULL") != 0; i++) {
        if (strcmp(device, controller->data.devices[i]) == 0) {
            
            return i;
        }
    }

    return -1;
}

/**
 * @brief Disconnects a controller and sets its status to DISCONNECTED.
 *
 * This function resets the data of the provided controller to 0's and sets its status to DISCONNECTED.
 * 
 * @param controller Pointer to the controller struct to disconnect.
 */
void disconnectController(struct Controller *controller) {

    controller->data.status = DISCONNECTED;
    controller->data.lastPacketTime = 0;

}