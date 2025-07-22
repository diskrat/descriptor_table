CC = gcc
CFLAGS = -Wall -pthread
SRC_DIR = src
TEST_DIR = tests
OBJ = $(SRC_DIR)/descriptor_table.o

all: run_tests

# Compile the descriptor_table library
$(OBJ): $(SRC_DIR)/descriptor_table.c $(SRC_DIR)/descriptor_table.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/descriptor_table.c -o $(OBJ)

# Compile each test case
test_basic: $(OBJ) $(TEST_DIR)/test_basic.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/test_basic.c -o test_basic

test_dup: $(OBJ) $(TEST_DIR)/test_dup.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/test_dup.c -o test_dup

test_dup2: $(OBJ) $(TEST_DIR)/test_dup2.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/test_dup2.c -o test_dup2

test_saturation: $(OBJ) $(TEST_DIR)/test_saturation.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/test_saturation.c -o test_saturation

test_concurrency: $(OBJ) $(TEST_DIR)/test_concurrency.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/test_concurrency.c -o test_concurrency

test_error_handling: $(OBJ) $(TEST_DIR)/test_error_handling.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/test_error_handling.c -o test_error_handling

test_large_files: $(OBJ) $(TEST_DIR)/test_large_files.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/test_large_files.c -o test_large_files

test_open_close: $(OBJ) $(TEST_DIR)/test_open_close.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/test_open_close.c -o test_open_close


# Run all tests
run_tests: test_basic test_dup test_saturation test_concurrency test_error_handling test_large_files test_open_close test_dup2
	./test_basic
	./test_dup
	./test_saturation
	./test_concurrency
	./test_error_handling
	./test_large_files
	./test_open_close
	./test_dup2

# Clean up generated files
clean:
	rm -f $(OBJ) test_basic test_dup test_saturation test_concurrency test_error_handling test_large_files test_open_close test_dup2 test_dup2_overwrite
