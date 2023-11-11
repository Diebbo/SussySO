# Cross toolchain variables
# If these are not in your path, you can make them absolute.
XT_PRG_PREFIX = riscv32-unknown-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld

URISCV_DIR_PREFIX = /usr/local

URISCV_DATA_DIR = $(URISCV_DIR_PREFIX)/share/uriscv
URISCV_INCLUDE_DIR = $(URISCV_DIR_PREFIX)/include

# Compiler options
CFLAGS_LANG = -ffreestanding -static -nostartfiles -nostdlib -Werror -ansi
CFLAGS = $(CFLAGS_LANG) -I$(URISCV_INCLUDE_DIR) -ggdb -Wall -O0 -std=gnu99 -march=rv32imfd -mabi=ilp32d

# Linker options
LDFLAGS = -G 0 -nostdlib -T $(URISCV_DATA_DIR)/uriscvcore.ldscript

# Add the location of crt*.S to the search path
VPATH = $(URISCV_DATA_DIR)

.PHONY : all clean

all : kernel.core.uriscv

kernel.core.uriscv : kernel
	uriscv-elf2uriscv -k $<

kernel : ./phase1/msg.o ./phase1/pcb.o ./phase1/p1test.o crtso.o liburiscv.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean :
	-rm -f *.o ./phase1/*.o kernel kernel.*.uriscv

# Pattern rule for assembly modules
%.o : %.S
	$(CC) $(CFLAGS) -c -o $@ $<
