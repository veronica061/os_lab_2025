CC = gcc
CFLAGS = -I. -Wall -Wextra
TARGETS = parallel_min_max process_memory

# Цвета для вывода
GREEN = \033[0;32m
YELLOW = \033[0;33m
RED = \033[0;31m
NC = \033[0m 

LAB3_SRC = lab3/src
LAB4_SRC = lab4/src

CHECK_DIRS = $(LAB3_SRC) $(LAB4_SRC)

all: check_dirs $(TARGETS)
	@echo "$(GREEN)All programs built successfully!$(NC)"

check_dirs:
	@for dir in $(CHECK_DIRS); do \
		if [ ! -d "$$dir" ]; then \
			echo "$(RED)Directory $$dir not found!$(NC)"; \
			exit 1; \
		fi; \
	done

parallel_min_max: $(LAB3_SRC)/parallel_min_max.o $(LAB3_SRC)/find_min_max.o $(LAB3_SRC)/utils.o
	@echo "$(YELLOW)Linking parallel_min_max...$(NC)"
	$(CC) -o $@ $^ $(CFLAGS)
	@echo "$(GREEN)arallel_min_max ready$(NC)"

process_memory: $(LAB4_SRC)/process_memory.o
	@echo "$(YELLOW)Linking process_memory...$(NC)"
	$(CC) -o $@ $^ $(CFLAGS)
	@echo "$(GREEN)process_memory ready$(NC)"

$(LAB3_SRC)/parallel_min_max.o: $(LAB3_SRC)/parallel_min_max.c
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) -c -o $@ $< $(CFLAGS)

$(LAB3_SRC)/find_min_max.o: $(LAB3_SRC)/find_min_max.c
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) -c -o $@ $< $(CFLAGS)

$(LAB3_SRC)/utils.o: $(LAB3_SRC)/utils.c
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) -c -o $@ $< $(CFLAGS)

$(LAB4_SRC)/process_memory.o: $(LAB4_SRC)/process_memory.c
	@echo "$(YELLOW)Compiling $<...$(NC)"
	$(CC) -c -o $@ $< $(CFLAGS)

run_parallel: parallel_min_max
	@echo "$(GREEN)Running parallel_min_max...$(NC)"
	./parallel_min_max --seed 42 --array_size 100000 --pnum 4

run_memory: process_memory
	@echo "$(GREEN)Running process_memory...$(NC)"
	./process_memory

clean:
	@echo "$(YELLOW)Cleaning...$(NC)"
	rm -f $(TARGETS)
	rm -f $(LAB3_SRC)/*.o
	rm -f $(LAB4_SRC)/*.o
	@echo "$(GREEN)Clean complete$(NC)"

.PHONY: all clean check_dirs run_parallel run_memory