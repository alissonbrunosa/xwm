# Compiler and flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude $(shell pkg-config --cflags xcb xcb-icccm xcb-ewmh xcb-keysyms x11)
LDFLAGS = $(shell pkg-config --libs xcb xcb-icccm xcb-ewmh xcb-keysyms x11)

# Source and build files
SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:src/%.c=build/%.o)
TARGET  = xwm

# Default target
all: $(TARGET)

# Build target binary
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Build object files
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# Create build dir if missing
build:
	mkdir -p build

# Clean all build artifacts
clean:
	rm -rf build $(TARGET)

.PHONY: all clean
