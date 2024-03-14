/**
 * @file logs.h
 * @brief Functions definitions for logging error, warning, and info messages.
 * 
 * This file contains functions definitions for logging error, warning, and info messages
 * with optional debug mode.
 * 
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-5
 */

#ifndef LOGS_H
#define LOGS_H

#include "commons.h"

/* Boolean to set debug mode. */
extern bool DEBUG;

/**
 * @brief Enables debug mode when called
 */
void enableDebug();

/**
 * @brief Function to get the current time in [Hour:Minute:Second] format.
 *
 * @return The current time in string format [Hour:Minute:Second]
 */
char* get_current_time();

/**
 * @brief Function to show error messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 * @param ... Additional arguments for formatted output.
 */
void lerror(const char *str, bool override, ...);

/**
 * @brief Function to show warning messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 * @param ... Additional arguments for formatted output.
 */
void lwarning(const char *str, bool override, ...);

/**
 * @brief Function to show info messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 * @param ... Additional arguments for formatted output.
 */
void linfo(const char *str, bool override, ...);

#endif /* LOGS_H */
