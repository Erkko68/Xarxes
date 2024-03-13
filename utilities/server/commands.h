/**
 * @file server:commands.h
 * @brief Function declarations to execute server commands.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */
#include "../commons.h"


/**
 * @brief Sanitizes a string by removing leading and trailing spaces.
 *
 * This function modifies the given string by removing leading and trailing spaces.
 * It shifts the substring containing non-space characters to the beginning of the original string
 * and null-terminates the new string.
 * 
 * @param str The string to sanitize.
 */
void sanitizeString(char *str);

/**
 * @brief Gets the string representation of the given status.
 *
 * This function returns the string representation of the status specified by the enum Status.
 * It maps each status to its corresponding string representation.
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
 * @param numControllers The number of controllers in the array.
 */
void printList(struct Controller *controllers);

void commandSet(char *controller, char *device, char *value, struct Controller *controllers, struct Server *srvConf);