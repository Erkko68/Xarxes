#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main() {
    int tcp_socket, udp_socket, max_fd, activity, new_socket, client_sockets[MAX_CLIENTS], sd;
    struct sockaddr_in server_tcp_addr, server_udp_addr, client_addr;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Create TCP socket
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Create UDP socket
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the server addresses
    memset(&server_tcp_addr, 0, sizeof(server_tcp_addr));
    memset(&server_udp_addr, 0, sizeof(server_udp_addr));
    server_tcp_addr.sin_family = AF_INET;
    server_udp_addr.sin_family = AF_INET;
    server_tcp_addr.sin_addr.s_addr = INADDR_ANY;
    server_udp_addr.sin_addr.s_addr = INADDR_ANY;
    server_tcp_addr.sin_port = htons(2018); // Example TCP port
    server_udp_addr.sin_port = htons(2018); // Example UDP port

    // Bind TCP socket
    if (bind(tcp_socket, (struct sockaddr *)&server_tcp_addr, sizeof(server_tcp_addr)) == -1) {
        perror("TCP bind failed");
        exit(EXIT_FAILURE);
    }

    // Bind UDP socket
    if (bind(udp_socket, (struct sockaddr *)&server_udp_addr, sizeof(server_udp_addr)) == -1) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming TCP connections
    if (listen(tcp_socket, 5) == -1) {
        perror("TCP listen failed");
        exit(EXIT_FAILURE);
    }

    // Initialize client sockets array
    memset(client_sockets, 0, sizeof(client_sockets));

    // Main server loop
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(tcp_socket, &readfds);
        FD_SET(udp_socket, &readfds);
        max_fd = (tcp_socket > udp_socket) ? tcp_socket : udp_socket;

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_fd) {
                max_fd = sd;
            }
        }

        // Wait for activity on sockets
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity == -1) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        // Handle incoming TCP connection
        if (FD_ISSET(tcp_socket, &readfds)) {
            if ((new_socket = accept(tcp_socket, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr)) == -1) {
                perror("TCP accept failed");
                exit(EXIT_FAILURE);
            }
            printf("New TCP connection, socket fd is %d, IP is : %s, port : %d\n",
                   new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Add new socket to array of client sockets
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Handle UDP message
        if (FD_ISSET(udp_socket, &readfds)) {
            memset(buffer, 0, BUFFER_SIZE);
            socklen_t len = sizeof(client_addr);
            if (recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &len) == -1) {
                perror("UDP receive failed");
                exit(EXIT_FAILURE);
            }
            printf("UDP message from %s:%d - %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
        }

        // Handle client sockets
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                if (recv(sd, buffer, BUFFER_SIZE, 0) == 0) {
                    // Client disconnected
                    printf("Client disconnected: %d\n", sd);
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Echo message back to client
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    // Close sockets
    close(tcp_socket);
    close(udp_socket);

    return 0;
}
