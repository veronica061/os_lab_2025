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

%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) -c -o $@ $< $(CFLAGS)

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

check: $(TARGET)
	@echo "Checking correctness..."
	@echo "Parallel version:"
	@./$(TARGET) --threads_num 4 --array_size 100000 --seed 123 | grep "Total sum"
	@echo "Expected: Run sequential version to compare"

# Очистка
clean:
	@echo "Cleaning..."
	rm -f $(TARGET) $(OBJECTS)
	@echo "Clean complete"


.PHONY: all run test check clean help