/**
 * @file server:commands.c
 * @brief Functions to execute server commands.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-12
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
void sanitizeString(char *str) {
    int start = 0, end = strlen(str) - 1;

    /*Find the first non-space character*/
    while (str[start] == ' ') {
        start++;
    }
    /*Find the last non-space character*/
    while (end > start && (str[end] == ' ')) {
        end--;
    }
    /*Shift the substring to the beginning of the original string*/
    memmove(str, str + start, end - start + 1);
    str[end - start + 1] = '\0'; /*Null-terminate the new string*/
}

/**
 * @brief Gets the string representation of the given status.
 *
 * This function returns the string representation of the status specified by the enum Status.
 * It maps each status to its corresponding string representation.
 * 
 * @param status The status to get the name for.
 * @return The string representation of the status.
 */
const char* getStatusName(enum Status status) {
    switch (status) {
        case DISCONNECTED: return "DISCONNECTED";
        case NOT_SUBSCRIBED: return "NOT_SUBSCRIBED";
        case WAIT_ACK_SUBS: return "WAIT_ACK_SUBS";
        case WAIT_INFO: return "WAIT_INFO   ";
        case WAIT_ACK_INFO: return "WAIT_ACK_INFO";
        case SUBSCRIBED: return "SUBSCRIBED  ";
        case SEND_HELLO: return "SEND_HELLO  ";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Prints the given string with appropriate spaces if empty.
 *
 * This function prints the given string with appropriate spaces if it is empty.
 * It ensures that the string is aligned within the specified width when printed.
 * 
 * @param str The string to print.
 * @param width The desired width for the string.
 */
void printInfoOrSpaces(char* str, int width) {
    if (strlen(str) == 0) {
        printf("%*s ", width, "");
    } else {
        printf("%-*s ", width, str);
    }
}

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
void printList(struct Controller *controllers) {
    int i, j;
    printf("--NOM--- ------IP------- -----MAC---- --RNDM-- ----ESTAT--- --SITUACIÓ-- --ELEMENTS-------------------------------------------\n");
    for (i = 0; strcmp(controllers[i].name,"NULL") != 0; i++) {
        printf("%s ", controllers[i].name);
        printInfoOrSpaces(controllers[i].data.ip, sizeof(controllers[i].data.ip) - 1);
        printf("%s ", controllers[i].mac);
        printInfoOrSpaces(controllers[i].data.rand, sizeof(controllers[i].data.rand) - 1);
        printf("%s ", getStatusName(controllers[i].data.status));
        printInfoOrSpaces(controllers[i].data.situation, sizeof(controllers[i].data.situation) - 1);
        for (j = 0; strcmp(controllers[i].data.devices[j], "NULL") != 0; j++) {
            printf("%s ", controllers[i].data.devices[j]);
        }
        printf("\n");
    }
}
