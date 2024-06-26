/**
 * @file udp.c
 * @brief Functions for encoding, decoding, and sending UDP packets.
 * 
 * This file contains functions for encoding and decoding UDP packets
 * into byte arrays and sending data over UDP sockets.
 * 
 * @author Eric Bitria Ribes
 * @version 0.4
 * @date 2024-3-14
 */

#include "../commons.h"

#define PDUUDP 103

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
struct UDPPacket* createUDPPacket(const unsigned char type, const char *mac, const char *rnd, const char *data) {
    struct UDPPacket* packet = malloc(sizeof(struct UDPPacket));
    if (packet == NULL) {
        return NULL;
    }
    packet->type = type;
    strncpy(packet->mac, mac, sizeof(packet->mac) - 1);
    packet->mac[sizeof(packet->mac) - 1] = '\0';
    strncpy(packet->rnd, rnd, sizeof(packet->rnd) - 1);
    packet->rnd[sizeof(packet->rnd) - 1] = '\0';
    strncpy(packet->data, data, sizeof(packet->data) - 1);
    packet->data[sizeof(packet->data) - 1] = '\0';

    return packet;
}


/**
 * @brief Converts a UDPPacket struct to a byte array.
 *
 * This function converts the provided UDPPacket struct 'packet' into a byte array 'bytes'.
 * It starts by copying the type field of the packet to the beginning of the byte array,
 * then copies the mac, rnd, and data fields subsequently. It updates the 'offset' to
 * keep track of the current position in the byte array.
 * 
 * @param packet Pointer to the UDPPacket struct to be converted.
 * @param bytes Pointer to the byte array where the UDPPacket struct will be converted.
 */
void udpToBytes(const struct UDPPacket *packet, char *bytes) {
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
struct UDPPacket bytesToUdp(const char *bytes) {
    struct UDPPacket packet;
    int offset = 0;
    packet.type = bytes[offset];
    offset += sizeof(packet.type);
    memcpy(packet.mac, bytes + offset, sizeof(packet.mac));
    offset += sizeof(packet.mac);
    memcpy(packet.rnd, bytes + offset, sizeof(packet.rnd));
    offset += sizeof(packet.rnd);
    memcpy(packet.data, bytes + offset, sizeof(packet.data));
    return packet;
}


/**
 * @brief Sends data over a UDP socket to a specified address.
 *
 * This function sends the data contained in the provided UDPPacket structure 'packet'
 * over a UDP socket represented by the file descriptor 'socketFd' to the destination
 * address specified in the 'address' parameter using the sendto() function.
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param packet The UDPPacket structure containing the data to be sent.
 * @param address Pointer to a sockaddr_in struct representing the destination address.
 */
void sendUdp(const int socketFd, struct UDPPacket* packet, const struct sockaddr_in *address) {
    char data[PDUUDP];
    socklen_t address_len = sizeof(struct sockaddr_in);

    if (packet == NULL) {
        lerror("Error: NULL packet provided to sendUdp", true);
        return;
    }

    udpToBytes(packet, data);

    if (sendto(socketFd, data, sizeof(data), 0, (struct sockaddr *) address, address_len) < 0) {
        lerror("Sendto failed", true);
    }

    free(packet);
}


/**
 * @brief Receives a UDP packet from a specified socket.
 *
 * This function receives a UDP packet from the given 'socketFd' and stores it
 * in a UDPPacket struct. It uses the recvfrom() function to retrieve the packet
 * into a buffer, then decodes the buffer contents into a UDPPacket struct using
 * the bytesToUdp() function.
 * 
 * @param socketFd The file descriptor of the UDP socket.
 * @param address Pointer to a sockaddr_in struct where the source address of
 *                the received packet will be stored.
 * 
 * @return Returns a UDPPacket struct containing the received UDP packet.
 */
struct UDPPacket recvUdp(const int socketFd, struct sockaddr_in *address){
    socklen_t address_len = sizeof(struct sockaddr_in); /* Get size of the address */
    char buffer[PDUUDP]; /* Init buffer */

    /* Execute packet reception */
    if (recvfrom(socketFd, buffer, PDUUDP, 0, (struct sockaddr *) address, &address_len) < 0) {
        lerror("recvfrom failed", true);
    }
    /* Decode  and return bytes into PDU_UDP packet */
    return bytesToUdp(buffer);
}

/**
 * @brief Generates a random 8-digit number as a string.
 *
 * This function generates a random 8-digit number between 00000000 and
 * 99999999 and stores it as a string in the provided character array 
 * 'str'. The result is formatted with leading zeros if necessary.
 * 
 * @param str A char array where the random number will be stored as string.
 */
void generateIdentifier(char str[9]) {
    int random_num;
    int quotient;
    int i;
    /* Set Seed */
    srand(time(NULL));
    /* Number between 0 and 99999999 */
    random_num = rand() % 100000000;
    quotient = random_num;
    for (i = 7; i >= 0; i--) {
        str[i] = '0' + quotient % 10;
        quotient /= 10;
    }
    str[8] = '\0'; /*Null-terminate the string*/
}