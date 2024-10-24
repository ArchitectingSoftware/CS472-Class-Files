CC=gcc
CFLAGS=-g

all: client server server2 server3

client: client.c
	$(CC) $(CFLAGS) -o client client.c

server: server.c
	$(CC) $(CFLAGS) -o server server.c

server2: server2.c
	$(CC) $(CFLAGS) -o server2 server2.c

server3: server3.c
	$(CC) $(CFLAGS) -o server3 server3.c

clean:
	rm -f client server server2 server3