# XzalgoChain - 320-bit Cryptographic Hash Function
# Makefile - Test suite
# Part of XzalgoChain project (Apache 2.0 License)

# Compiler and flags
CC = gcc
CFLAGS = -O3 -march=native -mtune=native -flto -fopenmp -lm

# Folder output
BIN_DIR = bin

# Source files
SRCS = \
    avalanche_test.c \
    bic_test.c \
    bit_bias_analyzer.c \
    consistent_test.c \
    cross_correlation_test.c \
    differential_test.c \
    dot_test.c \
    entropy_test.c \
    hash_counter.c \
    linear_correlation_test.c \
    permutation_compression_test.c \
    sac_test.c \
    benchmark.c

# Output binaries
BINS = $(patsubst %.c,$(BIN_DIR)/%,$(SRCS))

# Default target
all: $(BIN_DIR) $(BINS)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Rule to compile each binary
$(BIN_DIR)/%: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Clean
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
