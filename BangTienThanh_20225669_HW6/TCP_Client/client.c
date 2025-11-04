#include <stdio.h>
#include <stdlib.h>

// client.c
#include <stdio.h>
#include <stdlib.h>

int client_connect_and_run(const char *ip, int port);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP_Addr> <Port_Number>\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port\n");
        return 1;
    }
    return client_connect_and_run(ip, port);
}
