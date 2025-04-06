# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -Iinclude

# Directories
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

# Source files
SRC_FILES = $(SRC_DIR)/mn_thread.c $(SRC_DIR)/scheduler.c

# Test targets
TEST_HTTP = $(BUILD_DIR)/test_http_sim
TEST_FACT = $(BUILD_DIR)/test_fact

# Default target
all: $(TEST_HTTP) $(TEST_FACT)

# Build rules for test_http_sim
$(TEST_HTTP): $(SRC_FILES) $(TEST_DIR)/test_http_sim.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Build rules for test_fact
$(TEST_FACT): $(SRC_FILES) $(TEST_DIR)/test_fact.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

# Run targets
http_sim: $(TEST_HTTP)
	./$(TEST_HTTP)

test_fact: $(TEST_FACT)
	./$(TEST_FACT)

# Clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean http_sim test_fact
