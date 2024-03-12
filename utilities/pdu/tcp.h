/**
 * @file tcp.h
 * @brief Functions definitions for encoding, decoding, and sending TCP packets.
 * 
 * This file contains function definitions for encoding and decoding TCP packets
 * into byte arrays and sending data over TCP sockets.
 *  
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-4
 */

#ifndef PDUTCP_H
#define PDUTCP_H

#include "../commons.h"

/* Define struct for TCP packet:
   - type (1 byte)           : Represents the type of TCP packet.
   - mac (13 byte)           : Represents the MAC address.
   - rnd (9 byte)            : Represents random data.
   - char device[8];         : Represents the device identifier.
   - char value[7];          : Repesentes the value associated to the device.
   - data (80 byte)          : Represents the data payload.
*/
struct TCPPacket {
    unsigned char type;
    char mac[13];
    char rnd[9];
    char device[8];
    char value[7];
    char data[80];
};

/* Enum to represent different types of TCP packets:
    - SEND_DATA = 0x20,
    - SET_DATA = 0x21,
    - GET_DATA = 0x22,
    - DATA_ACK = 0x23,
    - DATA_NACK = 0x24,
    - DATA_REJ = 0x25,
*/
enum TCPType{
    SEND_DATA = 0x20,
    SET_DATA = 0x21,
    GET_DATA = 0x22,
    DATA_ACK = 0x23,
    DATA_NACK = 0x24,
    DATA_REJ = 0x25
};

/**
 * @brief Creates a TCPPacket structure with the provided information.
 *
 * This function constructs and returns a TCPPacket structure with the specified
 * 'type', 'mac' address, 'rnd' data, and 'data' payload. It copies the provided
 * 'mac', 'rnd', and 'data' strings into the corresponding fields of the TCPPacket
 * structure.
 * 
 * @param type The type of the packet.
 * @param mac The MAC address string.
 * @param rnd The random data string.
 * @param device The device of the controller.
 * @param value The value associated to the device.
 * @param data The data payload string.
 * 
 * @return Returns a TCPPacket structure initialized with the provided information.
 */
struct TCPPacket createTCPPacket(const unsigned char type, const char *mac, const char *rnd, const char *device, const char *value, const char *data);

/**
 * @brief Converts a TCPPacket struct to a byte array.
 * 
 * @param packet Pointer to the TCPPacket struct to be converted.
 * @param bytes Pointer to the byte array where the TCPPacket struct will be converted.
 */
void tcpToBytes(const struct TCPPacket *packet, char *bytes);

/**
 * @brief Converts a byte array to a TCPPacket struct.
 * 
 * @param bytes Pointer to the byte array containing the data to be converted.
 * 
 * @return Returns a TCPPacket struct with the data decoded from the byte array.
 */

struct TCPPacket bytesToTcp(const char *bytes);

/**
 * @brief Sends a TCP packet over the specified socket.
 * 
 * @param socketFd The file descriptor of the socket to send data over.
 * @param packet The TCPPacket struct containing the data to send.
 */
void sendTcp(const int socketFd, const struct TCPPacket packet);

/**
 * @brief Receives a TCP packet from a socket and converts it to a TCPPacket struct.
 * 
 * @param socketFd The file descriptor of the socket from which to receive the packet.
 * 
 * @return Returns a TCPPacket struct with the data decoded from the received byte array.
 */
struct TCPPacket recvTcp(const int socketFd);

/* Debug */
void printTCPPacket(struct TCPPacket packet);

#endif /* PDUTCP_H */
