#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <time.h>

#define BUFFER_SIZE 16384
#define LOG_FILENAME "log_20225669.txt"

void log_activity(FILE *log_fp, struct sockaddr_in client_addr, const char *request, const char *response) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "[%d/%m/%Y %H:%M:%S]", tm_info);

    char client_info[64];
    snprintf(client_info, sizeof(client_info), "$%s:%d$", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    fprintf(log_fp, "%s%s%s%s\n", time_str, client_info, request, response);
    fflush(log_fp);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Port_Number> <Storage_Directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    char *storage_dir = argv[2];
    mkdir(storage_dir, 0777);

    FILE *log_fp = fopen(LOG_FILENAME, "a");
    if (!log_fp) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", port);

    while (1) {
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        const char *welcome_msg = "+OK Welcome to file server";
        send(client_sock, welcome_msg, strlen(welcome_msg), 0);
        log_activity(log_fp, client_addr, "\\", welcome_msg);

        char buffer[BUFFER_SIZE];
        int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            close(client_sock);
            continue;
        }
        buffer[bytes_received] = '\0';

        char command[5], filename[256];
        unsigned long filesize;
        if (sscanf(buffer, "%4s %255s %lu", command, filename, &filesize) != 3 || strcmp(command, "UPLD") != 0) {
            close(client_sock);
            continue;
        }

        const char *ready_msg = "+OK Please send file";
        send(client_sock, ready_msg, strlen(ready_msg), 0);
        log_activity(log_fp, client_addr, buffer, ready_msg);

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", storage_dir, filename);

        FILE *fp = fopen(filepath, "wb");
        if (!fp) {
            perror("Failed to open file");
            close(client_sock);
            continue;
        }

        unsigned long total_received = 0;
        while (total_received < filesize) {
            int to_read = BUFFER_SIZE;
            if (filesize - total_received < BUFFER_SIZE)
                to_read = filesize - total_received;

            bytes_received = recv(client_sock, buffer, to_read, 0);
            if (bytes_received <= 0) break;

            fwrite(buffer, 1, bytes_received, fp);
            total_received += bytes_received;
        }

        fclose(fp);

        const char *result_msg = (total_received == filesize) ? "+OK Successful upload" : "-ERR Upload failed";
        send(client_sock, result_msg, strlen(result_msg), 0);
        log_activity(log_fp, client_addr, buffer, result_msg);

        close(client_sock);
    }

    fclose(log_fp);
    close(server_sock);
    return 0;
}
