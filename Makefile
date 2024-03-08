CC = gcc
CFLAGS = -std=c99 -ansi -pedantic -Wall
FILES = server.c utilities/pdu/udp.c utilities/logs.c utilities/server/controllers.c  utilities/server/conf.c utilities/server/subs_process.c

server: server.c
	$(CC) $(CFLAGS) -o server $(FILES)

clean:
	rm -f server
