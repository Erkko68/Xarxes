
#include "../commons.h"

void setData(struct Controller *controller,int device){
    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[113];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Error opening socket");

    // Initialize server address struct
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(controller->data.tcp);

    // Bind socket to address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Error on binding");

    // Listen for incoming connections
    listen(sockfd, 5);

    clilen = sizeof(cli_addr);

    // Accept connection from client
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
        error("Error on accept");

    // Connection established, send data to client
    strcpy(buffer, "Hello, client! This is a message from the server.");
    n = write(newsockfd, buffer, strlen(buffer));
    if (n < 0)
        error("Error writing to socket");

    printf("Message sent to client: %s\n", buffer);

    close(newsockfd);
    close(sockfd);
    return 0;

}