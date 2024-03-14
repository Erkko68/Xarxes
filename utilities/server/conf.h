/**
 * @file server:conf.h
 * @brief Functions definitions to load and parse arguments for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-14
 */

#ifndef SERVER_CONF_H
#define SERVER_CONF_H

#include "../commons.h"

/*
Define struct for server config
- char name[9];
- char mac[13];
- unsigned short tcp; Range 0-65535
- unsigned short udp; Range 0-65535
- struct sockaddr_in tcp_address;
- struct sockaddr_in udp_address;
*/
struct Server{
    char name[9];
    char mac[13];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
    struct sockaddr_in tcp_address;
    struct sockaddr_in udp_address;
};

/**
 * @brief Returns a struct with the server configuration.
 * 
 * @param filename The name of the file to read the configuration.
 * @return struct server 
 */
struct Server serverConfig(const char *filename);

/**
 * @brief Function to parse command line arguments and get the file name.
 * 
 * @param argc The number of args
 * @param argv An array containing all the args
 * @param config_file A pointer to the config_file string
 * @param controllers A pointer to the controllers string
 */
void readArgs(int argc, char *argv[], char **config_file, char **controllers);

#endif /* SERVER_CONF_H */