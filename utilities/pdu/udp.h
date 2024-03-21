/**
 * @file udp.h
 * @brief Functions definitions for encoding, decoding, and sending UDP packets.
 * 
 * This file contains function definitions for encoding and decoding UDP packets
 * into byte arrays and sending data over UDP sockets.
 *  
 * @author Eric Bitria Ribes
 * @version 0.3
 * @date 2024-3-14
 */

#ifndef PDUUDP_H
#define PDUUDP_H

#include "../commons.h"

/* Define struct for pdu_udp packet:
   - type (1 byte)           : Represents the type of UDP packet.
   - mac (13 byte)           : Represents the MAC address.
   - rnd (9 byte)            : Represents random data.
   - data (80 byte)          : Represents the data payload.
*/
struct UDPPacket {
    unsigned char type;
    char mac[13];
    char rnd[9];
    char data[80];
};

/* Enum to represent different types of UDP packets:
   - SUBS_REQ (0x00):
   - SUBS_ACK (0x01):
   - SUBS_REJ (0x02):
   - SUBS_INFO (0x03):
   - INFO_ACK (0x04): 
   - SUBS_NACK (0x05): 
*/
enum UDPType{
    SUBS_REQ = 0x00,
    SUBS_ACK = 0x01,
    SUBS_REJ = 0x02,
    SUBS_INFO = 0x03,
    INFO_ACK = 0x04,
    SUBS_NACK = 0x05,
    HELLO = 0x10,
    HELLO_REJ = 0x11
};

/**
 * @brief Creates a UDPPacket structure with the provided information.
 *
 * This function constructs and returns a UDPPacket structure with the specified
 * 'type', 'mac' address, 'rnd' data, and 'data' payload. It copies the provided
 * 'mac', 'rnd', and 'data' strings into the corresponding fields of the UDPPacket
 * structure.
 * 
 * @param type The type of the packet.
 * @param mac The MAC address string.
 * @param rnd The random data string.
 * @param data The data payload string.
 * 
 * @return Returns a UDPPacket structure initialized with the provided information.
 */
struct UDPPacket* createUDPPacket(const unsigned char type, const char *mac, const char *rnd, const char *data);

/**
 * @brief Converts a UDPPacket struct to a byte array.
 * 
 * @param packet Pointer to the UDPPacket struct to be converted.
 * @param bytes Pointer to the byte array where the UDPPacket struct will be converted.
 */
void udpToBytes(const struct UDPPacket *packet, char *bytes);

/**
 * @brief Converts a byte array to a UDPPacket struct.
 *
 * This function converts the provided byte array 'bytes' into a UDPPacket struct.
 * It begins by copying the type field from the byte array to the packet struct,
 * then proceeds to copy the mac, rnd, and data fields subsequently. It updates the
 * 'offset' to keep track of the current position in the byte array.
 * 
 * @param bytes Pointer to the byte array containing the data to be converted.
 * 
 * @return Returns a UDPPacket struct containing the data decoded from the byte array.
 */
struct UDPPacket bytesToUdp(const char *bytes);

/**
 * @brief Sends data over a UDP socket to a specified address.
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param packet The UDPPacket structure containing the data to be sent.
 * @param address Pointer to a sockaddr_in struct representing the destination address.
 */
void sendUdp(const int socketFd, struct UDPPacket *packet, const struct sockaddr_in *address);

/**
 * @brief Receives a UDP packet from a specified socket.
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param address Pointer to a sockaddr_in struct where the source address of
 * the received packet will be stored.
 * 
 * @return Returns a UDPPacket struct containing the received UDP packet.
 */
struct UDPPacket recvUdp(const int socketFd, struct sockaddr_in *address);

/**
 * @brief Generates a random 8-digit number as a string.
 * 
 * @param str A char array where the random number will be stored as string.
 */
void generateIdentifier(char str[9]);

#endif /* PDUUDP_H */
