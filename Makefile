CC = gcc
CFLAGS = -std=c99 -ansi -pedantic -Wall
FILES = server.c utilities/pduudp.c utilities/logs.c utilities/controllers.c  utilities/server_conf.c

server: server.c
	$(CC) $(CFLAGS) -o server $(FILES)

clean:
	rm -f server
