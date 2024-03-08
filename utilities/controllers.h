/**
 * @file Controllers.h
 * @brief Functions for saving, loading, and managing clients for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "pduudp.h"

/*Define struct for controller info*/
struct ControllerInfo{
    unsigned char status;
    char situation[13];
    char rand[9];
    char devices[10][8];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
};

/*Define struct to load authorized clients*/
struct Controller{
    char name[9];
    char mac[13];
    struct ControllerInfo data;
};

/* 
Define enum for all possible controller status: 
- DISCONNECTED = 0xa0,
- NOT_SUBSCRIBED = 0xa1,
- WAIT_ACK_SUBS = 0xa2,
- WAIT_INFO = 0xa3,
- WAIT_ACK_INFO = 0xa4,
- SUBSCRIBED = 0xa5,
- SEND_HELLO = 0xa6
*/
enum Status{
    DISCONNECTED = 0xa0,
    NOT_SUBSCRIBED = 0xa1,
    WAIT_ACK_SUBS = 0xa2,
    WAIT_INFO = 0xa3,
    WAIT_ACK_INFO = 0xa4,
    SUBSCRIBED = 0xa5,
    SEND_HELLO = 0xa6
};

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
 * @return Returns the total number of controllers read from the file on success.
 * 
 * @throw Error if no controller has been read, when memory allocation fails, or when file descriptor fails.
 */
int loadControllers(struct Controller **controllers, const char *filename);

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
int isAllowed(const struct Packet packet, struct Controller *controllers, int numControllers);

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
void storeDevices(char *devices, char (*deviceArray)[8], char *delimiter);


#endif /*CONTROLLERS_H*/