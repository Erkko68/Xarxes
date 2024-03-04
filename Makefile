CC = gcc
CFLAGS = -ansi -pedantic -Wall
FILES = server.c utilities/pduudp.c utilities/logs.c utilities/clientsDB.c 

server: server.c
	$(CC) $(CFLAGS) -o server $(FILES)

clean:
	rm -f server
