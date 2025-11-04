// client_lib.c
// Client helper functions and interactive loop.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAXLINE 2048

/* Send a single line to the socket.
 * Appends CRLF ("\r\n") to the provided string before sending.
 * Returns number of bytes sent, or -1 on error.
 */
static ssize_t send_line(int fd, const char *s) {
    char buf[MAXLINE];
    int n = snprintf(buf, sizeof(buf), "%s\r\n", s);
    if (n < 0) return -1;
    return send(fd, buf, n, 0);
}

/* Remove trailing newline characters (\n and \r) from a string.
 * Modifies the input string in-place.
 */
static void trim_line(char *s) {
    int n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

/* Read a line from the socket into buf (until '\n').
 * buf will be null-terminated when >0 bytes are read.
 * Returns number of bytes read (excluding the terminating null),
 * 0 if connection closed, or -1 on error.
 */
static ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    size_t idx = 0;
    char c;
    ssize_t n;
    while (idx + 1 < maxlen) {
        n = recv(fd, &c, 1, 0);
        if (n == 1) {
            buf[idx++] = c;
            if (c == '\n') break;
        } else if (n == 0) {
            if (idx == 0) return 0;
            break;
        } else {
            if (errno == EINTR) continue;
            return -1;
        }
    }
    buf[idx] = '\0';
    return (ssize_t)idx;
}

/* Connect to server and run interactive client loop.
 * Parameters:
 *   ip   - server IP address (dotted-quad)
 *   port - server port number
 * Returns 0 on normal exit, 1 on error.
 */
int client_connect_and_run(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address\n");
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    char rbuf[MAXLINE];
    ssize_t rc = recv_line(sockfd, rbuf, sizeof(rbuf));
    if (rc <= 0) {
        fprintf(stderr, "Server closed connection\n");
        close(sockfd);
        return 1;
    }
    trim_line(rbuf);
    printf("Server: %s\n", rbuf);

    char input[MAXLINE];
    while (1) {
        printf("Enter command (USER <username> | POST <article> | BYE | QUIT):\n> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("Input EOF, exiting\n");
            break;
        }
        trim_line(input);
        if (strlen(input) == 0) continue;

        if (strcasecmp(input, "QUIT") == 0) {
            printf("Close client without BYE\n");
            break;
        }

        if (send_line(sockfd, input) < 0) {
            perror("send");
            break;
        }

        rc = recv_line(sockfd, rbuf, sizeof(rbuf));
        if (rc == 0) {
            printf("Server closed connection\n");
            break;
        } else if (rc < 0) {
            perror("recv");
            break;
        }
        trim_line(rbuf);
        printf("Server: %s\n", rbuf);

        if (strcmp(rbuf, "130") == 0) {
            printf("Logged out, closing connection\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}
