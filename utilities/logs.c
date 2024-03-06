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

#include "logs.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>

/*Init debug mode setting*/
bool DEBUG = false;

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
 * @brief Enables debug mode when called
 */
void enableDebug(){
    DEBUG = true;
}