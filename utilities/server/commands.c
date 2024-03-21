/**
 * @file server:commands.c
 * @brief Functions to execute server commands.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.3
 * @date 2024-3-14
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
 * @param maxControllers  The number of controllers in the array.
 */
void printList(struct Controller *controllers, int maxControllers) {
    int i, j;
    printf("--NOM--- ------IP------- -----MAC---- --RNDM-- ----ESTAT--- --SITUACIÓ-- --ELEMENTS-------------------------------------------\n");
    for (i = 0; i < maxControllers; i++) {
        printf("%s ", controllers[i].name);
        printInfoOrSpaces(controllers[i].data.ip, sizeof(controllers[i].data.ip) - 1);
        printf("%s ", controllers[i].mac);
        printInfoOrSpaces(controllers[i].data.rand, sizeof(controllers[i].data.rand) - 1);
        printf("%s ", getStatusName(controllers[i].data.status));
        printInfoOrSpaces(controllers[i].data.situation, sizeof(controllers[i].data.situation) - 1);
        for (j = 0; j< 10; j++) {
            printf("%s ", controllers[i].data.devices[j]);
        }
        printf("\n");
    }
}


/**
 * @brief Initiates a data petition to a controller.
 *
 * This function initiates a data petition to a controller identified by the provided controller name,
 * device name, and value. It checks if the controller exists and is not disconnected, and if the 
 * device exists in the controller. If all conditions are met, it allocates memory for the arguments
 * of the independent thread, assigns values to the arguments, and creates a new thread to handle 
 * the data petition.
 * 
 * @param controller Pointer to a string containing the controller name.
 * @param device Pointer to a string containing the device name.
 * @param value Pointer to a string containing the value.
 * @param controllers Pointer to an array of Controller structures.
 * @param srvConf Pointer to a Server structure.
 */ 
void commandDataPetition(char *controller, char *device, char *value, struct Controller *controllers, struct Server *srvConf) {
    int controllerNum;
    int deviceNum;
    pthread_t dataThread;
    
    /* Check if the controller exists and is not disconnected */
    if ((controllerNum = hasController(controller, controllers,srvConf->numControllers)) != -1 && controllers[controllerNum].data.status != DISCONNECTED) {
        /* Check if the device exists */
        if ((deviceNum = hasDevice(device, &controllers[controllerNum])) != -1) {
            /* Allocate memory to hold the arguments for the independent thread */
            struct dataPetition *args = malloc(sizeof(struct dataPetition));
            if (args == NULL) {
                lerror("Failed memory allocation for commandData Thread", true);
            }
            /* Assign values to the arguments */
            args->controller = &controllers[controllerNum];
            args->device = device;
            args->value = value;
            args->servConf = srvConf;

            /* Create a new thread to handle the data petition */
            if(pthread_create(&dataThread, NULL, dataPetition, (void *)args) < 0) {
                lerror("Unexpected error while starting new TCP thread.", true);
            } else {
                /* Detach the thread after successful creation */
                if (pthread_detach(dataThread) != 0) {
                    lerror("Failed to detach TCP thread", true);
                }
            }

        } else {
            lwarning("Device in controller %s not found", true, controllers[controllerNum].mac);
        }
    } else {
        lwarning("Controller not found or disconnected", true);
    }
}