/**
 * @file server:commands.h
 * @brief Function declarations to execute server commands.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-14
 */
#include "../commons.h"


/**
 * @brief Sanitizes a string by removing leading and trailing spaces.
 * 
 * @param str The string to sanitize.
 */
void sanitizeString(char *str);

/**
 * @brief Gets the string representation of the given status.
 * 
 * @param status The status to get the name for.
 * @return The string representation of the status.
 */
const char* getStatusName(enum Status status);

/**
 * @brief Prints the information of the given controllers.
 *
 * This function prints the information of the given controllers, including name, IP address,
 * MAC address, random data, status, situation, and devices. It ensures proper alignment of
 * information by printing spaces for empty strings.
 * 
 * @param controllers Pointer to an array of Controller structures.
 * @param maxControllers  The number of controllers in the array.
 */
void printList(struct Controller *controllers, int maxControllers);


/**
 * @brief Initiates a data petition to a controller.
 * 
 * @param controller Pointer to a string containing the controller name.
 * @param device Pointer to a string containing the device name.
 * @param value Pointer to a string containing the value.
 * @param controllers Pointer to an array of Controller structures.
 * @param srvConf Pointer to a Server structure.
 */ 
void commandDataPetition(char *controller, char *device, char *value, struct Controller *controllers, struct Server *srvConf);
