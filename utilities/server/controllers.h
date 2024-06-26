/**
 * @file Controllers.h
 * @brief Functions definitions for saving, loading, and managing clients for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-14
 */

#ifndef CONTROLLERS_H
#define CONTROLLERS_H

#include "../commons.h"
 
/*Define struct for controller info*/
struct ControllerInfo{
    unsigned char status;
    char situation[13];
    char rand[9];
    char devices[10][8];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
    char ip[INET_ADDRSTRLEN];
    time_t lastPacketTime;
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
 * @param packet The packet struct to check.
 * @param controllers Pointer to the array of Controller structs containing allowed controllers.
 * @param maxControllers The number of controllers
 * @return Returns 1 if the controller is allowed, 0 otherwise.
 */
int isUDPAllowed(const struct UDPPacket packet, struct Controller *controllers, int maxControllers);


/**
 * @brief Checks if a TCP packet is allowed.
 * 
 * @param packet The TCPPacket struct representing the TCP packet to check.
 * @param controllers Pointer to the array of Controller structs containing allowed controllers.
 *  @param maxControllers The number of controllers
 * @return Returns the index of the allowed controller if found, otherwise returns -1.
 */
int isTCPAllowed(const struct TCPPacket* packet, struct Controller *controllers, int maxControllers);
/**
 * @brief Tokenizes and stores an string into diferent devices names.
 * 
 * @param devices The string to tokenize.
 * @param deviceArray Pointer to the array where tokens will be stored.
 * @param delimiter The delimiter used to tokenize the string.
 */
void storeDevices(char *devices, char [][8], char *delimiter);

/**
 * @brief Checks if a device exists in the deviceArray.
 * 
 * @param device The device name to search for.
 * @param deviceArray Pointer to the array containing device names.
 * @return If the device is found, returns the index of the device in the array. Otherwise, returns -1.
 */
int hasDevice(const char *device, const struct Controller *controller);

/**
 * @brief Checks if a controller with the given name exists in the array of controllers.
 * 
 * @param name The name of the controller to search for.
 * @param controllers Pointer to the array of controllers.
 * @param maxControllers The number of controllers
 * @return int The index of the controller if found, otherwise -1.
 */
int hasController(char *name,struct Controller *controllers, int maxControllers);

/**
 * @brief Disconnects a controller and sets its status to DISCONNECTED.
 * 
 * @param controller Pointer to the controller struct to disconnect.
 */
void disconnectController(struct Controller *controller) ;

#endif /*CONTROLLERS_H*/