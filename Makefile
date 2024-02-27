CC = gcc
CFLAGS = -ansi -pedantic -Wall

server: server.c
	$(CC) $(CFLAGS) -o server server.c

clean:
	rm -f server
