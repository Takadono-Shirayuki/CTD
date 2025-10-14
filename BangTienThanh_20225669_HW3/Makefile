CC = gcc
CFLAGS = -Wall

all: server client

server: UDP_Server/server.c UDP_Server/resolver_lib.c
	$(CC) $(CFLAGS) -o server UDP_Server/server.c UDP_Server/resolver_lib.c

client: UDP_Client/client.c UDP_Client/utils.c
    $(CC) $(CFLAGS) -o client UDP_Client/client.c UDP_Client/utils.c

clean:
    rm -f server client
