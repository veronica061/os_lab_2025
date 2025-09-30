CC = gcc
CFLAGS = -I. -Wall -Wextra -pthread
TARGET = parallel_sum

SOURCES = parallel_sum.c sum_lib.c array_utils.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = sum_lib.h array_utils.h

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) -o $@ $(OBJECTS) $(CFLAGS)
	@echo "$(TARGET) built successfully"

parallel_sum.o: parallel_sum.c $(HEADERS)
	@echo "Compiling parallel_sum.c..."
	$(CC) -c -o $@ parallel_sum.c $(CFLAGS)

sum_lib.o: sum_lib.c sum_lib.h
	@echo "Compiling sum_lib.c..."
	$(CC) -c -o $@ sum_lib.c $(CFLAGS)

array_utils.o: array_utils.c array_utils.h
	@echo "Compiling array_utils.c..."
	$(CC) -c -o $@ array_utils.c $(CFLAGS)

run: $(TARGET)
	@echo "Running $(TARGET) with test parameters..."
	./$(TARGET) --threads_num 4 --array_size 1000000 --seed 42

test: $(TARGET)
	@echo "Testing with different thread counts..."
	@echo "=== 1 thread ==="
	./$(TARGET) --threads_num 1 --array_size 1000000 --seed 42
	@echo ""
	@echo "=== 2 threads ==="
	./$(TARGET) --threads_num 2 --array_size 1000000 --seed 42
	@echo ""
	@echo "=== 4 threads ==="
	./$(TARGET) --threads_num 4 --array_size 1000000 --seed 42
	@echo ""
	@echo "=== 8 threads ==="
	./$(TARGET) --threads_num 8 --array_size 1000000 --seed 42

clean:
	@echo "Cleaning..."
	rm -f $(TARGET) $(OBJECTS)
	@echo "Clean complete"


.PHONY: all run test clean help