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
 * @brief Parses the input command line and extracts the command, controller, device, and value.
 *
 * This function parses the input command line and extracts the command, controller, device, and value
 * based on the space delimiter. It also removes trailing newline characters from the input.
 * If any length exceeds the maximum allowed length, it prints a warning message and sets the
 * corresponding variable to an empty string.
 * 
 * @param commandLine The input command line to be parsed.
 * @param command Pointer to store the extracted command.
 * @param controller Pointer to store the extracted controller.
 * @param device Pointer to store the extracted device.
 * @param value Pointer to store the extracted value.
 * @return The number of parameters extracted.
 */
int parseInput(char *commandLine, char *command, char *controller, char *device, char *value) {
    int args = 0; /* Initialize args counter */
    char *current = commandLine; /* Pointer to the beginning of commandLine */
    char *next;
    int length;

    /* Loop until the end of commandLine or until args is 4 */
    while (*current != '\0' && args < 4) {
        /* Skip leading spaces */
        while (*current == ' ') {
            current++;
        }

        /* If end of string is reached after spaces, break the loop */
        if (*current == '\0') {
            break;
        }

        next = strchr(current, ' ');
        if (next == NULL) {
            next = current + strlen(current); /* If no space found, point to the end of string */
        }
        length = next - current; /* Calculate length of the word */
        
        /* Get the next word and store it in the appropriate variable */
        switch (args) {
            case 0:
                strncpy(command, current, length);
                command[length] = '\0';
                break;
            case 1:
                if (length > 8) {
                    lwarning("Controller name exceeds maximum length. (8)", true);
                    controller[0] = '\0'; /* Empty the controller string */
                    return -1;
                } else {
                    strncpy(controller, current, length);
                    controller[length] = '\0';
                }
                break;
            case 2:
                if (length > 7) {
                    lwarning("Device name exceeds maximum length. (7)", true);
                    device[0] = '\0'; /* Empty the device string */
                    return -1;
                } else {
                    strncpy(device, current, length);
                    device[length] = '\0';
                }
                break;
            case 3:
                if (length > 6) {
                    lwarning("Value exceeds maximum length. (6)", true);
                    value[0] = '\0'; /* Empty the value string */
                    return -1;
                } else {
                    strncpy(value, current, length);
                    value[length] = '\0';
                }
                break;
        }
        args++; /* Increment args counter */
        current = next; /* Move current pointer to the next word */
    }

    return args;
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
 * @param threadpool Pointer to thread pool
 */ 
void commandDataPetition(char *controller, char *device, char *value, struct Controller *controllers, struct Server *srvConf, thread_pool_t *threadpool) {
    int controllerNum;
    int deviceNum;
    
    /* Check if the controller exists and is not disconnected */
    mtx_lock(&mutex);
    if ((controllerNum = hasController(controller, controllers,srvConf->numControllers)) != -1 && controllers[controllerNum].data.status != DISCONNECTED) {
        /* Check if the device exists */
        if ((deviceNum = hasDevice(device, &controllers[controllerNum])) != -1) {
            /* Allocate memory to hold the arguments for the independent thread */
            struct dataPetition *args = malloc(sizeof(struct dataPetition));
            if (args == NULL) {
                mtx_unlock(&mutex);
                lerror("Failed memory allocation for commandData Thread", true);
            }
            /* Assign values to the arguments */
            args->controller = &controllers[controllerNum];
            args->device = device;
            args->value = value;
            args->servConf = srvConf;
            mtx_unlock(&mutex);
            thread_pool_submit(threadpool,dataPetition,(void *)args);
        } else {
            lwarning("Device in controller %s not found", true, controllers[controllerNum].mac);
            mtx_unlock(&mutex);
        }
    } else {
        mtx_unlock(&mutex);
        lwarning("Controller not found or disconnected", true);
    }
}