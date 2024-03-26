/**
 * @file controllers.c
 * @brief Functions for saving, loading, and managing clients for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.4
 * @date 2024-3-19
 */

#include "../commons.h"

/* Only used to read the controllers */
typedef struct {
    char name[9];
    char mac[13];
} ctrl;

/**
 * @brief Reads a line from a file pointer and extracts controller information.
 * 
 * This function reads a line from the specified file pointer 'file' and parses it 
 * to extract controller information, Name,MAC.
 * It returns a structure of type 'ctrl' containing the extracted name and MAC address.
 * 
 * @param file Pointer to the file to read from.
 * @return Returns a 'ctrl' structure containing the extracted name and MAC address.
 */
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

/**
 * @brief Initializes the controller information structure.
 * 
 * This function initializes a 'ControllerInfo' structure pointed to by 'info'.
 * It sets the status to 'DISCONNECTED', empties the 'situation' and 'rand' strings,
 * and initializes each element of the 'devices' array to an empty string.
 * It sets 'tcp' and 'udp' to zero, and empties the 'ip' string.
 * Finally, it sets 'lastPacketTime' to zero.
 * 
 * @param info Pointer to the 'ControllerInfo' structure to initialize.
 */
void initializeControllerInfo(struct ControllerInfo *info) {
    int i;
    info->status = DISCONNECTED;
    info->situation[0] = '\0';
    info->rand[0] = '\0';
    for (i = 0; i < 10; i++) {
        info->devices[i][0] = '\0';
    }
    info->tcp = 0;
    info->udp = 0;
    info->ip[0] = '\0';
    info->lastPacketTime = 0;
}

/**
 * @brief Adds a new controller to an array of controllers.
 * 
 * This function adds a new controller to the array of controllers.
 * It dynamically reallocates memory for the 'controllers' array to accommodate the new controller.
 * It copies the provided 'name' and 'mac' into the newly added controller's struct.
 * It initializes the controller's data using 'initializeControllerInfo'.
 * 
 * @param controllers Pointer to the array of controllers.
 * @param numControllers Pointer to the number of controllers.
 * @param name The name of the new controller.
 * @param mac The MAC address of the new controller.
 * @return Returns the updated array of controllers.
 */
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
int isUDPAllowed(const struct UDPPacket packet, struct Controller *controllers, int maxControlers) {
    int i;

    /*Iterate over allowed controllers*/
    for (i = 0; i < maxControlers; i++) {
        if (strcmp(packet.mac, controllers[i].mac) == 0 && 
            strstr(packet.data, controllers[i].name) != NULL) {
            /*Return index*/
            return i;
        }
    }


    return -1;
}

/**
 * @brief Checks if a TCP packet is allowed.
 *
 * This function iterates through the provided array of controllers and compares the MAC address 
 * of the given TCP packet with each controller. If a matching controller is found, the function 
 * returns the index of the controller in the array. Otherwise, it returns -1 indicating that the 
 * packet is not allowed.
 * 
 * @param packet The TCPPacket struct representing the TCP packet to check.
 * @param controllers Pointer to the array of Controller structs containing allowed controllers.
 * @param maxControllers The number of controllers
 * @return Returns the index of the allowed controller if found, otherwise returns -1.
 */
int isTCPAllowed(const struct TCPPacket* packet, struct Controller *controllers, int maxControllers) {
    int i;
        
    /*Iterate over allowed controllers*/
    for (i = 0; i < maxControllers ; i++) {
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
 * @param maxControllers The number of controllers
 * @return int The index of the controller if found, otherwise -1.
 */
int hasController(char *name,struct Controller *controllers, int maxControllers){
    int i;
    for (i = 0; i < maxControllers; i++) {
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
    
    for (i = 0; i < 10; i++) {
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
    pthread_mutex_lock(&mutex);
        initializeControllerInfo(&controller->data);
    pthread_mutex_unlock(&mutex);
}