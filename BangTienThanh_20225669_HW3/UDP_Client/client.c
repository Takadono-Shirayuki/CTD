#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "utils.h"

#define UDP_BUFFER_SIZE 65535

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ServerIP> <PortNumber>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);
    char *buffer = NULL;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    while (1) {
        printf("Enter domain name or IPv4 address (or press Enter to quit): ");
        buffer = get_long_stdin_input('\n');
        buffer[strcspn(buffer, "\n")] = 0;

        if (strlen(buffer) == 0) break;

        sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr*)&server_addr, server_len);

        memset(buffer, 0, UDP_BUFFER_SIZE);
        recvfrom(sockfd, buffer, UDP_BUFFER_SIZE, 0,
                 (struct sockaddr*)&server_addr, &server_len);

        printf("Result: %s\n", buffer);
    }

    close(sockfd);
    return 0;
}
