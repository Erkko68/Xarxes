/**
 * @file logs.h
 * @brief Functions definitions for logging error, warning, and info messages.
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

#include "commons.h"

/* Boolean to set debug mode. */
extern bool DEBUG;

/**
 * @brief Enables debug mode when called
 */
void enableDebug();

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
const char *save(struct TCPPacket packet, struct Controller controller);

#endif /* LOGS_H */
