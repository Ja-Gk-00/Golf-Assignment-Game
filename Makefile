# Makefile for Golf Assignment Project (fixed linker flags + benchmark)

# Compiler settings
CC       := gcc
CFLAGS   := -I src/include -Wall -Wextra -g
LDFLAGS  := -L src/lib -lmingw32 -lSDL3 -lcomdlg32 -lm

# GUI targets
SRC_MAIN    := main.c
TARGET_MAIN := main.exe

# Generator targets
SRC_GEN     := file_generator.c
TARGET_GEN  := file_generator.exe

# Benchmark targets
SRC_BENCH     := benchmark.c
TARGET_BENCH  := benchmark.exe

.PHONY: all generate clean

# Default target builds GUI, generator, and benchmark
all: $(TARGET_MAIN) $(TARGET_GEN) $(TARGET_BENCH)

# Build the main GUI executable
$(TARGET_MAIN): $(SRC_MAIN)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Build the input file generator
$(TARGET_GEN): $(SRC_GEN)
	$(CC) -o $@ $< -lm

# Build the benchmark executable
$(TARGET_BENCH): $(SRC_BENCH)
	$(CC) $(CFLAGS) -o $@ $< -lm

# Alias to only build the generator
generate: $(TARGET_GEN)

# Clean up binaries
clean:
	rm -f $(TARGET_MAIN) $(TARGET_GEN) $(TARGET_BENCH)

# Example:
#   make
#   benchmark.exe 100 50 timings.txt
