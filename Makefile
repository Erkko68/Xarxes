CC = gcc
CFLAGS = -std=c99 -ansi -pedantic -Wall -pthread
FILES = server.c utilities/pdu/udp.c utilities/pdu/tcp.c utilities/logs.c utilities/server/controllers.c  utilities/server/conf.c utilities/server/subs_process.c utilities/server/commands.c
server: server.c
	$(CC) $(CFLAGS) -o server $(FILES)

clean:
	rm -f server

