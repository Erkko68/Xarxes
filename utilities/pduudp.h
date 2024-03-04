/**
 * @file pududp.h
 * @brief Functions for encoding, decoding, and sending UDP packets.
 * 
 * This file contains function definitions for encoding and decoding UDP packets
 * into byte arrays and sending data over UDP sockets.
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

#ifndef PDUUDP_H
#define PDUUDP_H

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
 * @brief Converts a pdu_udp struct packet into a byte array 
 * 
 * @param packet The struct representing the pdu_udp packet
 * @param bytes the byte array to store the encoded struct
 */
void udpToBytes(const struct Packet *packet, char *bytes);

/**
 * @brief Converts a byte array into pdu_udp packet format
 * 
 * @param bytes The byte array to take the data
 * @param packet The pdu_udp packet struct to store decoded data
 */
void bytesToUdp(const char *bytes, struct Packet *packet);

/**
 * @brief Sends data over UDP socket to a specified address.
 *
 * This function sends the provided byte array 'data' over a UDP socket 
 * with the given 'socketFd' to the destination address specified in 
 * 'address' using the sendto() function. If the send operation fails, 
 * it logs an error message using lerror().
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param data The byte array containing the data to be sent.
 * @param address The destination address where the data will be sent.
 */
void sendUdp(int socketFd, char *data, const struct sockaddr *address);


#endif /* PDUUDP_H */
