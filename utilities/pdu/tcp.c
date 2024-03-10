/**
 * @file tcp.c
 * @brief Functions for encoding, decoding, and sending TCP packets.
 * 
 * This file contains function definitions for encoding and decoding TCP packets
 * into byte arrays and sending data over TCP sockets.
 *  
 * @author Eric Bitria Ribes
 * @version 0.1
 * @date 2024-3-4
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
struct TCPPacket bytesTotcp(const char *bytes) {
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

