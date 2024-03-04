/**
 * @file pududp.c
 * @brief Functions for encoding, decoding, and sending UDP packets.
 * 
 * This file contains functions for encoding and decoding UDP packets
 * into byte arrays and sending data over UDP sockets.
 * 
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
 */

#include <arpa/inet.h>
#include "pduudp.h"
#include "logs.h"
#include <string.h>

#define PDUUDP 103

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
void udpToBytes(const struct Packet *packet, char *bytes) {
    int offset = 0;
    bytes[offset] = packet->type;
    offset += sizeof(packet->type);
    memcpy(bytes + offset, packet->mac, sizeof(packet->mac));
    offset += sizeof(packet->mac);
    memcpy(bytes + offset, packet->rnd, sizeof(packet->rnd));
    offset += sizeof(packet->rnd);
    memcpy(bytes + offset, packet->data, sizeof(packet->data));
}

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
void bytesToUdp(const char *bytes, struct Packet *packet) {
    int offset = 0;
    packet->type = bytes[offset];
    offset += sizeof(packet->type);
    memcpy(packet->mac, bytes + offset, sizeof(packet->mac));
    offset += sizeof(packet->mac);
    memcpy(packet->rnd, bytes + offset, sizeof(packet->rnd));
    offset += sizeof(packet->rnd);
    memcpy(packet->data, bytes + offset, sizeof(packet->data));
}

/**
 * @brief Sends data over UDP socket to a specified address.
 *
 * This function sends the provided byte array 'data' over a UDP socket 
 * with the given 'socketFd' to the destination address specified in 
 * 'address' using the sendto() function.
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param data The byte array containing the data to be sent.
 * @param address The destination address where the data will be sent.
 */
void sendUdp(const int socketFd, const char *data, const struct sockaddr_in *address) {
    socklen_t address_len = sizeof(struct sockaddr_in);

    if (sendto(socketFd, data, sizeof(data), 0, (struct sockaddr *) address, address_len) < 0) {
        lerror("Sendto failed", true);
    }
}

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
struct Packet recvUdp(const int socketFd, struct sockaddr_in *address){
    socklen_t address_len = sizeof(struct sockaddr_in); /* Get size of the address */
    struct Packet pdu_udp; /* Init Packet struct */
    char buffer[PDUUDP]; /* Init buffer */

    /* Execute packet reception */
    if (recvfrom(socketFd, buffer, PDUUDP, 0, (struct sockaddr *) address, &address_len) < 0) {
        lerror("recvfrom failed", true);
    }
    /* Decode bytes into PDU_UDP packet */
    bytesToUdp(buffer,&pdu_udp);

    return pdu_udp;
}




