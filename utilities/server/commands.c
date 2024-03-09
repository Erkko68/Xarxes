/**
 * @file server:commands.c
 * @brief Functions to execute server commands.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */
#include "../commons.h"

void processCommand(char* commandLine) {
    char command[5], controller[9], device[8], value[12];
    int args;

    sanitizeString(commandLine);

    /*Get command and arguments*/
    args = sscanf(commandLine, "%4s %8s %7s %11s", command, controller, device, value);

    if (strcmp(command, "list") == 0 && args == 1) {
        
    } else if (strcmp(command, "set") == 0 && args == 4) {
        /* set command */
    } else if (strcmp(command, "get") == 0 && args == 3) {
        /* get command */
    } else if (strcmp(command, "quit") == 0 && args == 1) {
        /* quit command */
    } else {
        linfo("Invalid command. Usage:\n list (Show authorized controllers info.)\n set <controller-name> <device-name> <value> (sends info to the device)\n get <controller-name> <device-name> (Requests info to the device)\n quit (Quits server and closes all communications)",true);
    }
}

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