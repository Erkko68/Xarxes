/**
 * @file tcp.c
 * @brief Functions for encoding, decoding, and sending TCP packets.
 * 
 * This file contains function definitions for encoding and decoding TCP packets
 * into byte arrays and sending data over TCP sockets.
 *  
 * @author Eric Bitria Ribes
 * @version 0.3
 * @date 2024-3-12
 */

#include "../commons.h"

#define PDUTCP 118

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
struct TCPPacket createTCPPacket(const unsigned char type, const char *mac, const char *rnd, const char *device, const char *value, const char *data){
    struct TCPPacket packet;
        packet.type = type;
        strcpy(packet.mac,mac);
        strcpy(packet.rnd,rnd);
        strcpy(packet.device,device);
        strcpy(packet.value,value);
        strcpy(packet.data,data);
    return packet;
}

/**
 * @brief Converts a TCPPacket struct to a byte array.
 * 
 * @param packet Pointer to the TCPPacket struct to be converted.
 * @param bytes Pointer to the byte array where the TCPPacket struct will be converted.
 */
void tcpToBytes(const struct TCPPacket *packet, char *bytes){
    int offset = 0;
    bytes[offset] = packet->type;
    offset += sizeof(packet->type);
    memcpy(bytes + offset, packet->mac, sizeof(packet->mac));
    offset += sizeof(packet->mac);
    memcpy(bytes + offset, packet->rnd, sizeof(packet->rnd));
    offset += sizeof(packet->rnd);
    memcpy(bytes + offset, packet->device, sizeof(packet->device));
    offset += sizeof(packet->device);
    memcpy(bytes + offset, packet->value, sizeof(packet->value));
    offset += sizeof(packet->value);
    memcpy(bytes + offset, packet->data, sizeof(packet->data));
}

/**
 * @brief Converts a byte array to a TCPPacket struct.
 *
 * This function takes a byte array 'bytes' containing data representing
 * a TCPPacket struct. It decodes the byte array and populates the fields
 * of a TCPPacket struct accordingly. The byte array is expected to contain
 * serialized data in a specific format, and this function parses it to
 * reconstruct the original TCPPacket struct.
 * 
 * @param bytes Pointer to the byte array containing the data to be converted.
 * 
 * @return Returns a TCPPacket struct with the data decoded from the byte array.
 */
struct TCPPacket bytesToTcp(const char *bytes) {
    struct TCPPacket packet;
    int offset = 0;
    packet.type = bytes[offset];
    offset += sizeof(packet.type);
    memcpy(packet.mac, bytes + offset, sizeof(packet.mac));
    offset += sizeof(packet.mac);
    memcpy(packet.rnd, bytes + offset, sizeof(packet.rnd));
    offset += sizeof(packet.rnd);
    memcpy(packet.device, bytes + offset, sizeof(packet.device));
    offset += sizeof(packet.device);
    memcpy(packet.value, bytes + offset, sizeof(packet.value));
    offset += sizeof(packet.value);
    memcpy(packet.data, bytes + offset, sizeof(packet.data));
    return packet;
}

/**
 * @brief Sends a TCP packet over the specified socket.
 *
 * This function converts the given TCPPacket struct to bytes using the tcpToBytes function,
 * then sends the resulting data over the specified socket. It ensures that the entire data
 * is sent by looping until all bytes are sent. If sending fails, it logs an error using the
 * lerror function and terminates the program if specified.
 * 
 * @param socketFd The file descriptor of the socket to send data over.
 * @param packet The TCPPacket struct containing the data to send.
 */
void sendTcp(const int socketFd, const struct TCPPacket packet) {
    char data[PDUTCP];
    tcpToBytes(&packet,data);

    if (send(socketFd, data, sizeof(data), 0) < 0) {
        lerror("send failed", true);
    }
}

/**
 * @brief Receives a TCP packet from a socket and converts it to a TCPPacket struct.
 *
 * This function receives a TCP packet from the specified socket 'socketFd'.
 * The received packet is expected to be in byte array format, representing
 * a TCPPacket struct. It decodes the byte array and populates the fields
 * of a TCPPacket struct accordingly. The received byte array is decoded
 * using the bytesToTcp() function.
 * 
 * @param socketFd The file descriptor of the socket from which to receive the packet.
 * 
 * @return Returns a TCPPacket struct with the data decoded from the received byte array.
 */
struct TCPPacket recvTcp(const int socketFd){
    int val;
    char buffer[PDUTCP]; /* Init buffer */

    /* Execute packet reception */
    val = recv(socketFd, buffer, PDUTCP,0);
    if ( val == 0) {
        lwarning("Host disconnected.",false);
        return createTCPPacket(0xF,"","","","","");
    } else if ( val < 0 ) {
        if (errno == EAGAIN || errno == EWOULDBLOCK){
            return createTCPPacket(0xF,"","","","","");
        } else {
            lerror("TCP recv failed", true);
        }
    }
    /* Decode  and return bytes into PDU_UDP packet */
    return bytesToTcp(buffer);
}

/* Debug */
void printTCPPacket(struct TCPPacket packet) {
    printf("Type: %u\n", packet.type);
    printf("MAC: %s\n", packet.mac);
    printf("RND: %s\n", packet.rnd);
    printf("Device: %s\n", packet.device);
    printf("Value: %s\n", packet.value);
    printf("Data: %s\n", packet.data);
}