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


/*Define struct for controller info*/
struct ControllerInfo{
    unsigned char status;
    char situation[13];
    char elements[10][8];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
};

/*Define struct to load authorized clients*/
struct Controller{
    char name[9];
    char mac[13];
    struct ControllerInfo data;
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
int initialiseControllers(struct Controller **controllers, const char *filename);

/**
 * @brief Checks if a controller is allowed based on its MAC address.
 *
 * This function checks if the given controller, specified by the MAC address, is allowed based on the
 * provided array of controllers. It iterates through the array of controllers and compares the MAC address
 * of each controller with the MAC address of the given controller. If a matching MAC address is found,
 * the controller is considered allowed and the function returns 1. Otherwise, the controller is considered
 * not allowed and the function returns 0.
 * 
 * @param mac The controller mac to check.
 * @param controllers Pointer to the array of Controller structs containing allowed controllers.
 * @param numControllers The number of controllers in the array.
 * @return Returns 1 if the controller is allowed, 0 otherwise.
 */
int isAllowed(char *mac,struct Controller *controllers, int numControllers);


#endif /*CONTROLLERS_H*/