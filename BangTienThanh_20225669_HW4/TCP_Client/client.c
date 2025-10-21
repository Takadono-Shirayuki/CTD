#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define BUFFER_SIZE 16384 // 16KB

void error_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP_Address> <Port_Number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    char buffer[BUFFER_SIZE];

    while (1) {
        char filepath[1024];
        printf("Enter file path (leave empty to exit): ");
        fgets(filepath, sizeof(filepath), stdin);
        filepath[strcspn(filepath, "\n")] = '\0';

        if (strlen(filepath) == 0) break;

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            continue;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &server_addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect");
            close(sock);
            continue;
        }

        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            perror("recv");
            close(sock);
            continue;
        }
        buffer[bytes_received] = '\0';
        printf("Server: %s\n", buffer);

        FILE *fp = fopen(filepath, "rb");
        if (!fp) {
            perror("Failed to open file");
            close(sock);
            continue;
        }

        char *filename = strrchr(filepath, '/');
        filename = filename ? filename + 1 : filepath;

        fseek(fp, 0, SEEK_END);
        unsigned long filesize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        snprintf(buffer, sizeof(buffer), "UPLD %s %lu", filename, filesize);
        send(sock, buffer, strlen(buffer), 0);

        bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            fclose(fp);
            printf("Connection closed by server.\n");
            close(sock);
            continue;
        }
        buffer[bytes_received] = '\0';
        printf("Server: %s\n", buffer);

        if (strncmp(buffer, "+OK", 3) != 0) {
            fclose(fp);
            close(sock);
            continue;
        }

        unsigned long total_sent = 0;
        while (total_sent < filesize) {
            size_t to_read = BUFFER_SIZE;
            if (filesize - total_sent < BUFFER_SIZE)
                to_read = filesize - total_sent;

            size_t bytes_read = fread(buffer, 1, to_read, fp);
            if (bytes_read <= 0) break;

            size_t bytes_sent = send(sock, buffer, bytes_read, 0);
            if (bytes_sent <= 0) break;

            total_sent += bytes_sent;
        }

        fclose(fp);

        bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Connection closed by server.\n");
            close(sock);
            continue;
        }
        buffer[bytes_received] = '\0';
        printf("Server: %s\n", buffer);

        close(sock);
    }

    return 0;
}
