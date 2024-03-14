/**
 * @file data.h
 * @brief Function definitions for handling data communication.
 * 
 * This file contains function definitions for handling data communication, including
 * saving TCPPacket data to a file and handling data petition communication.
 * 
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-14
 */

#ifndef DATA_H
#define DATA_H

#include "../commons.h"

/**
 * @brief Struct containing arguments for a data thread.
 *
 * This struct holds necessary arguments for the data thread, including the client socket,
 * server configuration, and controller information.
 */
struct dataThreadArgs {
    int client_socket; /**< Client socket descriptor */
    struct Server *servConf; /**< Pointer to server configuration */
    struct Controller *controllers; /**< Pointer to controller information */
};

/**
 * @brief Struct containing arguments for a data petition.
 *
 * This struct holds necessary arguments for a data petition, including controller information,
 * server configuration, device identifier, and data value.
 */
struct dataPetition {
    struct Controller *controller; /**< Pointer to controller information */
    struct Server *servConf; /**< Pointer to server configuration */
    char *device; /**< Device identifier */
    char *value; /**< Data value */
};

/**
 * @brief Function to save TCPPacket data to a file.
 *
 * @param packet The TCPPacket struct containing data to be saved.
 * @param controller The Controller struct containing information about the controller.
 * @return 0 if successful, -1 if failed to open/create file.
 */
const char *save(struct TCPPacket *packet, struct Controller *controller);

/**
 * @brief Function to handle data petition communication.
 *
 * @param st Pointer to a struct dataPetition containing necessary arguments.
 * @return NULL
 */
void *dataPetition(void *st);


/**
 * @brief Function to handle storing data received over TCP.
 *
 * @param args Pointer to a struct dataThreadArgs containing necessary arguments.
 * @return NULL
 */
void* dataReception(void* args);

#endif /* DATA_HANDLER_H */