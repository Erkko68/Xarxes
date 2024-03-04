/**
 * @file logs.h
 * @brief Functions for logging error, warning, and info messages.
 * 
 * This file contains functions definitions for logging error, warning, and info messages
 * with optional debug mode.
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

#ifndef LOGS_H
#define LOGS_H

#include <stdbool.h>

/* Boolean to set debug mode. */
extern bool DEBUG;

/**
 * @brief Function to show error messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 */
void lerror(const char *str, bool override);

/**
 * @brief Function to show warning messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 */
void lwarning(const char *str, bool override);

/**
 * @brief Function to show info messages.
 *
 * @param str The string to print
 * @param override If set to true this message will ignore debug mode and always display the msg.
 */
void linfo(const char *str, bool override);

#endif /* LOGS_H */
