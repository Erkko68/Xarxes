CC = gcc
CFLAGS = -ansi -std=c99 -pedantic -Wall

server: server.c
	$(CC) $(CFLAGS) -o server server.c

clean:
	rm -f server
