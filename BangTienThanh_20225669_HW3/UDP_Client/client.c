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
    if (sockfd < 0) {
        fprintf(stderr, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);
    char response_buffer[UDP_BUFFER_SIZE];

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("Enter domain name or IPv4 address (or press Enter to quit): ");
        
        char *input_buffer = get_long_stdin_input('\n');
        if (input_buffer == NULL) {
            // EOF or error, exit gracefully
            break;
        }
        
        // Remove newline if present
        input_buffer[strcspn(input_buffer, "\n")] = 0;

        if (strlen(input_buffer) == 0) {
            free(input_buffer);
            break;
        }

        // Send request
        sendto(sockfd, input_buffer, strlen(input_buffer), 0,
               (struct sockaddr*)&server_addr, server_len);

        // Receive response in separate buffer
        memset(response_buffer, 0, UDP_BUFFER_SIZE);
        int bytes_received = recvfrom(sockfd, response_buffer, UDP_BUFFER_SIZE - 1, 0,
                                    (struct sockaddr*)&server_addr, &server_len);
        
        if (bytes_received > 0) {
            response_buffer[bytes_received] = '\0';
            printf("Result: %s\n", response_buffer);
        } else {
            printf("Error receiving response\n");
        }
        
        // Free input buffer after use
        free(input_buffer);
    }

    close(sockfd);
    return 0;
}
