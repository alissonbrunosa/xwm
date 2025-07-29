# Compiler and flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude $(shell pkg-config --cflags xcb xcb-icccm xcb-ewmh xcb-keysyms x11)
LDFLAGS = $(shell pkg-config --libs xcb xcb-icccm xcb-ewmh xcb-keysyms x11)

# Source and build files (recursive)
SRC     := $(shell find src -name '*.c')
OBJ     := $(patsubst src/%.c,build/%.o,$(SRC))
TARGET  = xwm

# Default target
all: $(TARGET)

# Build target binary
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Build object files, create build subdirs as needed
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean all build artifacts
clean:
	rm -rf build $(TARGET)

.PHONY: all clean
