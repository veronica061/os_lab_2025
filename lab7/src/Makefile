CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGETS = client_tcp server_tcp client_udp server_udp

all: $(TARGETS)

client_tcp: tcpclient.c
	$(CC) $(CFLAGS) -o client_tcp tcpclient.c

server_tcp: tcpserver.c
	$(CC) $(CFLAGS) -o server_tcp tcpserver.c

client_udp: udpclient.c
	$(CC) $(CFLAGS) -o client_udp udpclient.c

server_udp: udpserver.c
	$(CC) $(CFLAGS) -o server_udp udpserver.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean