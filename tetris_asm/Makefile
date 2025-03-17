# Compiler and flags
CC = gcc
ASM = nasm
LDFLAGS = -lm -lSDL2
CFLAGS = --std=gnu11 -Wall -Wextra -pedantic -Ofast -g -fPIE
ASFLAGS = -f elf64  # Use ELF format for 64-bit assembly

# Files
TARGET = tetris
C_SOURCES = main.c
ASM_SOURCES = tetris.asm
OBJECTS = main.o tetris.o

# Default rule
all: $(TARGET)

# Linking step
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly source files
%.o: %.asm
	$(ASM) $(ASFLAGS) $< -o $@

# Clean up build files
clean:
	rm -f $(TARGET) $(OBJECTS)

