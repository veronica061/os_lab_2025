CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lpthread

# Общая библиотека
COMMON_SRC = common.c
COMMON_OBJ = common.o
COMMON_H = common.h

all: client server

# Компиляция общей библиотеки
$(COMMON_OBJ): $(COMMON_SRC) $(COMMON_H)
	$(CC) $(CFLAGS) -c $(COMMON_SRC) -o $(COMMON_OBJ)

# Клиент с общей библиотекой
client: client.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o client client.c $(COMMON_OBJ) $(LDFLAGS)

# Сервер с общей библиотекой
server: server.c $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o server server.c $(COMMON_OBJ) $(LDFLAGS)

# Терминал 1: два сервера в фоне
run-servers:
	./server --port 20001 --tnum 4 &
	./server --port 20002 --tnum 4 &
	@echo "Servers started on ports 20001 and 20002"

# Терминал 2: клиент с двумя серверами
run-client:
	echo "127.0.0.1:20001" > servers.txt
	echo "127.0.0.1:20002" >> servers.txt
	./client --k 20 --mod 1000000007 --servers servers.txt

clean:
	rm -f client server servers.txt $(COMMON_OBJ)
	pkill server

.PHONY: all run-servers run-client clean