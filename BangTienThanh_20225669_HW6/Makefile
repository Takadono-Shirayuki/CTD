
CC = gcc
CFLAGS = -Wall
LDFLAGS = -pthread

SRCS_SERVER = TCP_Server/server.c TCP_Server/server_lib.c
SRCS_CLIENT = TCP_Client/client.c TCP_Client/client_lib.c

TARGETS = server client

all: $(TARGETS)

server: $(SRCS_SERVER)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS_SERVER)

client: $(SRCS_CLIENT)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS_CLIENT)

clean:
	rm -f $(TARGETS)

.PHONY: all clean server client
