#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "resolver_lib.h"

#define BUFFER_SIZE 1024
#define LOG_FILENAME "log_20225669.txt"

void log_request(FILE *log_file, const char *request, const char *response) {
    if (!log_file) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log_file, "[%02d/%02d/%04d %02d:%02d:%02d]$%s$%s\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            request, response);
    fflush(log_file); // Flush để đảm bảo data được ghi ngay
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PortNumber>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    
    // Mở log file
    FILE *log_file = fopen(LOG_FILENAME, "a");
    if (!log_file) {
        fprintf(stderr, "Cannot open log file %s\n", LOG_FILENAME);
        exit(EXIT_FAILURE);
    }
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Socket creation failed\n");
        fclose(log_file);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Bind failed\n");
        close(sockfd);
        fclose(log_file);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", port);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                    (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received < 0) {
            continue;
        }
        
        buffer[bytes_received] = '\0'; // Null terminate
        
        char response[BUFFER_SIZE] = {0};
        
        // Trim whitespace
        char *input = buffer;
        while (*input == ' ' || *input == '\t' || *input == '\r' || *input == '\n') input++;
        
        // Remove trailing whitespace
        int len = strlen(input);
        while (len > 0 && (input[len-1] == ' ' || input[len-1] == '\t' || 
               input[len-1] == '\r' || input[len-1] == '\n')) {
            input[--len] = '\0';
        }
        
        if (len == 0) {
            snprintf(response, BUFFER_SIZE, "-Invalid input");
        } else {
            // Check if input is an IPv4 address (similar to resolver.c logic)
            struct sockaddr_in *ipv4 = string_to_ipv4(input);
            if (ipv4 != NULL) {
                // Reverse DNS lookup
                char *domain = reverse_DNS_lookup(ipv4);
                if (domain != NULL) {
                    snprintf(response, BUFFER_SIZE, "+%s", domain);
                    free(domain);
                } else {
                    snprintf(response, BUFFER_SIZE, "-Not found information");
                }
                free(ipv4);
            } else {
                // Resolve domain to IPv4 addresses
                struct Ipv4AddressList list = resolve_domain(input);
                if (list.count == 0) {
                    snprintf(response, BUFFER_SIZE, "-Not found information");
                } else {
                    strcpy(response, "+");
                    for (int i = 0; i < list.count; i++) {
                        char *ip_str = ipv4_to_string(list.addresses[i]);
                        if (ip_str != NULL) {
                            if (i > 0) strcat(response, " ");
                            strcat(response, ip_str);
                            free(ip_str);
                        }
                    }
                    // Free allocated memory for addresses
                    if (list.addresses != NULL) {
                        free(list.addresses);
                    }
                }
            }
        }

        sendto(sockfd, response, strlen(response), 0,
               (struct sockaddr*)&client_addr, client_len);

        log_request(log_file, input, response);
    }

    close(sockfd);
    fclose(log_file);
    return 0;
}
