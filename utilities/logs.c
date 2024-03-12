/**
 * @file logs.c
 * @brief Functions for logging error, warning, and info messages.
 * 
 * This file contains functions for logging error, warning, and info messages
 * with optional debug mode.
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

#include "commons.h"
#include <errno.h>

/*Init debug mode setting*/
bool DEBUG = false;

/**
 * @brief Enables debug mode when called
 */
void enableDebug(){
    DEBUG = true;
}

/**
 * @brief Function to get the current time in [Hour:Minute:Second] format.
 *
 * @return The current time in string format [Hour:Minute:Second]
 */
char* get_current_time() {
    time_t rawtime;
    struct tm * timeinfo;
    static char time_str[9]; /*HH:MM:SS\0*/ 

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
    return time_str;
}

/**
 * @brief Function to show error messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 * @param ... Additional arguments for formatted output.
 */
void lerror(const char *str, bool override, ...) {
    if (DEBUG || override) {
        va_list args;
        va_start(args, override);
        fprintf(stderr, "[%s] [Error] ", get_current_time());
        vfprintf(stderr, str, args);
        fprintf(stderr, ": ");
        perror(NULL);
        va_end(args);
    }
    exit(EXIT_FAILURE);
}

/**
 * @brief Function to show warning messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 * @param ... Additional arguments for formatted output.
 */
void lwarning(const char *str, bool override, ...) {
    if (DEBUG || override) {
        va_list args;
        va_start(args, override);
        printf("[%s] [Warning] ", get_current_time());
        vprintf(str, args);
        printf("\n");
        va_end(args);
    }
}

/**
 * @brief Function to show info messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 * @param ... Additional arguments for formatted output.
 */
void linfo(const char *str, bool override, ...) {
    if (DEBUG || override) {
        va_list args;
        va_start(args, override);
        printf("[%s] [Info] ", get_current_time());
        vprintf(str, args);
        printf("\n");
        va_end(args);
    }
}

/**
 * @brief Function to get the name of a TCP type enum value.
 *
 * This function takes a TCP type enum value and returns its corresponding name as a string.
 *
 * @param type The TCP type enum value.
 * @return The name of the TCP type enum value as a string.
 */
const char* getTCPName(enum TCPType type) {
    switch (type) {
        case SEND_DATA: return "SEND_DATA";
        case SET_DATA: return "SET_DATA";
        case GET_DATA: return "GET_DATA";
        case DATA_ACK: return "DATA_ACK";
        case DATA_NACK: return "DATA_NACK";
        case DATA_REJ: return "DATA_REJ";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Function to save TCPPacket data to a file.
 *
 * This function saves the data from a TCPPacket struct to a file, appending it to an existing file
 * or creating a new one if necessary. The data is formatted and written to the file along with
 * information about the controller and the current timestamp.
 *
 * @param packet The TCPPacket struct containing data to be saved.
 * @param controller The Controller struct containing information about the controller.
 * @return 0 if successful, -1 if failed to open/create file.
 */
const char* save(struct TCPPacket packet, struct Controller controller) {
    char filename[50], date_str[9], time_str[9];
    FILE *file;
    time_t now;
    struct tm *local_time;
    /* Get current time */
    time(&now);
    local_time = localtime(&now);

    /* Create filename using name and situation */
    /**!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!REMOVE LOGS/!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!**/
    /**!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!REMOVE LOGS/!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!**/
    /**!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!REMOVE LOGS/!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!**/
    sprintf(filename, "logs/%s-%s.data", controller.name, controller.data.situation);

    /* Open file or create one */
    if ((file = fopen(filename, "a")) == NULL) {
        return strerror(errno);
    }

    /* Save data */
    strftime(date_str, sizeof(date_str), "%d-%m-%y", local_time);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);

    /* Write data to file */
    if (fprintf(file, "%s,%s,%s,%s,%s\n", date_str, time_str, getTCPName(packet.type), packet.device, packet.value) < 0) {
        return strerror(errno);
    }

    fclose(file);
    return NULL;
}