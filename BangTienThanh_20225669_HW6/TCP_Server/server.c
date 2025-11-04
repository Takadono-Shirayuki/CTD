#include <stdio.h>
#include <stdlib.h>

// server.c
// Main program for the server: load accounts then start listening service.
// Compile: gcc -Wall -pthread server.c server_lib.c -o server
#include <stdio.h>
#include <stdlib.h>
int load_accounts(const char *filename);
int server_listen_and_serve(int port);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Port_Number>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port\n");
        return 1;
    }

    if (load_accounts("account.txt") != 0) {
        fprintf(stderr, "Warning: cannot open account.txt. Continue with empty account list\n");
    }

    return server_listen_and_serve(port);
}
