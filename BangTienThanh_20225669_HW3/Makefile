CC = gcc
CFLAGS = -Wall

all: server client

server: UDP_Server/server.c resolver_lib.c
    $(CC) $(CFLAGS) -o server UDP_Server/server.c resolver_lib.c

client: UDP_Client/client.c
    $(CC) $(CFLAGS) -o client UDP_Client/client.c

clean:
    rm -f server client
