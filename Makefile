# Compiler and flags
CC	=gcc
CFLAGS	=-Wall	-g	-Iinclude

# Directories
SRC_DIR	=src
TEST_DIR	=test
BUILD_DIR	=build

# Source files
SRC_FILES	=$(SRC_DIR)/mn_thread.c	$(SRC_DIR)/scheduler.c
TEST_FILES	=$(TEST_DIR)/test_threads.c

# Output binary
OUTPUT	=$(BUILD_DIR)/test_threads

# Build rules
all:	$(OUTPUT)

$(OUTPUT):	$(SRC_FILES)	$(TEST_FILES)
	mkdir	-p	$(BUILD_DIR)
	$(CC)	$(CFLAGS)	$(SRC_FILES)	$(TEST_FILES)	-o	$(OUTPUT)

clean:
	rm	-rf	$(BUILD_DIR)

run:	all
	./$(OUTPUT)

TEST_HTTP = $(BUILD_DIR)/test_http_sim
SRCS += $(TEST_DIR)/test_http_sim.c

http_sim: $(TEST_HTTP)
	./$(TEST_HTTP)

$(TEST_HTTP): $(SRC_FILES) $(TEST_DIR)/test_http_sim.c
	$(CC) $(CFLAGS) $^ -o $@
