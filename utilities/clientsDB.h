/**
 * @file clientsDB.h
 * @brief Functions for saving, loading, and managing clients for the server.
 * 
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

/*Define struct for clients*/
struct client{
    unsigned char status;
    char name[9];
    char situation[13];
    char elements[10][8];
    char mac[13];
    unsigned short tcp; /*Range 0-65535*/
    unsigned short udp; /*Range 0-65535*/
};