/**
 * @file pududp.h
 * @brief Functions for encoding, decoding, and sending UDP packets.
 * 
 * This file contains function definitions for encoding and decoding UDP packets
 * into byte arrays and sending data over UDP sockets.
 * 
 * @author Eric Bitria Ribes
 * @version 0.2
 * @date 2024-3-4
 */

#ifndef PDUUDP_H
#define PDUUDP_H

#include <arpa/inet.h>

/* Define struct for pdu_udp packet:
   - type (1 byte)           : Represents the type of UDP packet.
   - mac (13 byte)           : Represents the MAC address.
   - rnd (9 byte)            : Represents random data.
   - data (80 byte)          : Represents the data payload.
*/
struct Packet {
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
typedef enum {
    SUBS_REQ = 0x00,
    SUBS_ACK = 0x01,
    SUBS_REJ = 0x02,
    SUBS_INFO = 0x03,
    INFO_ACK = 0x04,
    SUBS_NACK = 0x05
} UDPType;

/**
 * @brief Creates a Packet structure with the provided information.
 *
 * This function constructs and returns a Packet structure with the specified
 * 'type', 'mac' address, 'rnd' data, and 'data' payload. It copies the provided
 * 'mac', 'rnd', and 'data' strings into the corresponding fields of the Packet
 * structure.
 * 
 * @param type The type of the packet.
 * @param mac The MAC address string.
 * @param rnd The random data string.
 * @param data The data payload string.
 * 
 * @return Returns a Packet structure initialized with the provided information.
 */
struct Packet createPacket(const unsigned char type, const char *mac, const char *rnd, const char *data);

/**
 * @brief Converts a Packet struct to a byte array.
 *
 * This function converts the provided Packet struct 'packet' into a byte array 'bytes'.
 * It starts by copying the type field of the packet to the beginning of the byte array,
 * then copies the mac, rnd, and data fields subsequently. It updates the 'offset' to
 * keep track of the current position in the byte array.
 * 
 * @param packet Pointer to the Packet struct to be converted.
 * @param bytes Pointer to the byte array where the Packet struct will be converted.
 */
void udpToBytes(const struct Packet *packet, char *bytes);

/**
 * @brief Converts a byte array to a Packet struct.
 *
 * This function converts the provided byte array 'bytes' into a Packet struct 'packet'.
 * It starts by copying the type field from the byte array to the packet struct,
 * then copies the mac, rnd, and data fields subsequently. It updates the 'offset' to
 * keep track of the current position in the byte array.
 * 
 * @param bytes Pointer to the byte array containing the data to be converted.
 * @param packet Pointer to the Packet struct where the byte array will be converted.
 */
void bytesToUdp(const char *bytes, struct Packet *packet);

/**
 * @brief Sends data over a UDP socket to a specified address.
 *
 * This function sends the data contained in the provided Packet structure 'packet'
 * over a UDP socket represented by the file descriptor 'socketFd' to the destination
 * address specified in the 'address' parameter using the sendto() function.
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param packet The Packet structure containing the data to be sent.
 * @param address Pointer to a sockaddr_in struct representing the destination address.
 */
void sendUdp(const int socketFd, const struct Packet packet, const struct sockaddr_in *address);

/**
 * @brief Receives a UDP packet from a specified socket.
 *
 * This function receives a UDP packet from the given 'socketFd' and stores it
 * in a Packet struct. It uses the recvfrom() function to retrieve the packet
 * into a buffer, then decodes the buffer contents into a Packet struct using
 * the bytesToUdp() function.
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param address Pointer to a sockaddr_in struct where the source address of
 * the received packet will be stored.
 * 
 * @return Returns a Packet struct containing the received UDP packet.
 */
struct Packet recvUdp(const int socketFd, struct sockaddr_in *address);

#endif /* PDUUDP_H */
