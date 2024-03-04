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

/**
 * @brief Converts a pdu_udp struct packet into a byte array.
 * 
 * @param packet The struct representing the pdu_udp packet
 * @param bytes the byte array to store the encoded struct
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
 * @brief Converts a byte array into pdu_udp packet format.
 * 
 * @param bytes The byte array to take the data
 * @param packet The packet_udp packet struct to store decoded data
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
 * 'address' using the sendto() function. If the sendTo fails, 
 * it logs an error message using lerror().
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param data The byte array containing the data to be sent.
 * @param address The destination address where the data will be sent.
 */
void sendUdp(int socketFd, char *data, const struct sockaddr *address) {
    socklen_t address_len = sizeof(struct sockaddr_in);

    if (sendto(socketFd, data, sizeof(data), 0, address, address_len) < 0) {
        lerror("Sendto failed", true);
    }
}



