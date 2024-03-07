/**
 * @file Controllers.h
 * @brief Functions for saving, loading, and managing clients for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-6
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logs.h"
#include "pduudp.h"
#include "controllers.h"

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
    char line[25]; 

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        lerror("Could not open filedescriptor while reading controllers.",true);
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char name[9];
        char mac[13];
        if (sscanf(line, "%9[^,],%13s", name, mac) == 2) {
            /*Dynamically reallocate memory for additional controllers*/
            *controllers = (struct Controller*) realloc(*controllers, (numControllers + 1) * sizeof(struct Controller));
            if (*controllers == NULL) {
                lerror("Failed memory allocation while reading controllers.",true);
            }
            /*Copy name and mac to the new controller*/
            strncpy((*controllers)[numControllers].name, name, sizeof((*controllers)[numControllers].name));
            strncpy((*controllers)[numControllers].mac, mac, sizeof((*controllers)[numControllers].mac));
            /* Set to zeros any other value */
            memset(&((*controllers)[numControllers].data), 0, sizeof((*controllers)[numControllers].data));
            
            /*Set disconected status*/
            (*controllers)[numControllers].data.status=DISCONNECTED;
            /*Increase number of controllers*/
            numControllers++;
        } else {
            lwarning("Wrong controller format in line: %d. Correct format (CTRL-XXX,YYYYYYYYYYY)",true,numControllers+1);
        }
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
 * @param numControllers The number of controllers in the array.
 * @return Returns 1 if the controller is allowed, 0 otherwise.
 */
int isAllowed(const struct Packet packet, struct Controller *controllers, int numControllers) {
    int i;
    /* Make a copy of packet data*/
    char data_copy[80];
    char* name;
    char* situation;
    
    strcpy(data_copy, packet.data);
    /*Tokenize copied data*/
    name = strtok(data_copy, ",");
    situation = strtok(NULL, ",");

    /*Iterate over allowed controllers*/
    for (i = 0; i < numControllers; i++) {
        if (strcmp(packet.mac, controllers[i].mac) == 0 && 
            strcmp(name, controllers[i].name) == 0 && 
            strcmp(situation, "000000000000") != 0) {
            /*Copy situation*/
            strcpy(controllers[i].data.situation, situation);
            /*Return index*/
            return i;
        }
    }
    return -1;
}