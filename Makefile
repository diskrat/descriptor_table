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
TESTS = test_basic test_concurrency test_dup2 test_error_handling test_saturation

$(TESTS): %: $(OBJ) $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) $(OBJ) $(TEST_DIR)/$*.c -o $@

# Run all tests
run_tests: $(TESTS)
	@for t in $(TESTS); do ./$$t; done

# Clean up generated files
clean:
	rm -f $(OBJ) $(TESTS)
