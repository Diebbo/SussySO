# Cross toolchain variables
# If these are not in your path, you can make them absolute.
XT_PRG_PREFIX = riscv32-unknown-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld

URISCV_DIR_PREFIX = /usr/local

URISCV_DATA_DIR = $(URISCV_DIR_PREFIX)/share/uriscv
URISCV_INCLUDE_DIR = $(URISCV_DIR_PREFIX)/include

# HEADER files
PROJECT_HEADERS = \
	-I./headers \
	-I./phase1/headers \
	-I./phase2/headers

# Compiler options
CFLAGS_LANG = -ffreestanding -static -nostartfiles -nostdlib -Werror -ansi
CFLAGS = $(CFLAGS_LANG) -I $(URISCV_INCLUDE_DIR)  -ggdb -Wall -O0 -std=gnu99 -march=rv32imfd -mabi=ilp32d

# Linker options
LDFLAGS = -G 0 -nostdlib -T $(URISCV_DATA_DIR)/uriscvcore.ldscript

# Add the location of crt*.S to the search path
VPATH = $(URISCV_DATA_DIR)

# Source files
SOURCE = src/main.c src/p2test.c $(wildcard phase1/*.c)	$(wildcard phase2/*.c) 

# OBJ files
OBJ = $(patsubst %.c,%.o,$(SOURCE))


.PHONY : all clean

all : kernel.core.uriscv

kernel.core.uriscv : kernel
	uriscv-elf2uriscv -k $<

kernel : $(OBJ) rtso.o liburiscv.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	-rm -f *.o ./src/*.o ./phase2/*.o ./phase1/*.o kernel kernel.*.uriscv

# Pattern rule for assembly modules
%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<

# Pattern rule for C modules
%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $< 
