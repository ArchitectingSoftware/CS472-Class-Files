
HEADERS = cs472-proto.h
CFLAGS = -g -Wall
CC = gcc

all: client server

client: client.o cs472-proto.o
	$(CC) $(CFLAGS) client.o cs472-proto.o -o client

client.o: client.c client.h cs472-proto.h
	$(CC) $(CFLAGS) -c client.c -o client.o

server: server.o cs472-proto.o
	$(CC) $(CFLAGS) server.o cs472-proto.o -o server

server.o: server.c server.h cs472-proto.h
	$(CC) $(CFLAGS) -c server.c -o server.o

cs472-proto: cs472-proto.c cs472-proto.h
	$(CC) $(CFLAGS) -c cs472-proto.c -o cs472-proto.o

clean:
	rm *.o
	rm ./client
	rm ./server