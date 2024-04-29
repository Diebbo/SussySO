#$Id: Makefile,v 1.2 2004/05/01 14:53:48 morsiani Exp morsiani $
# Makefile for mipsel-linux
#
XT_PRG_PREFIX = riscv32-unknown-linux-gnu-
CC = $(XT_PRG_PREFIX)gcc
LD = $(XT_PRG_PREFIX)ld
AS = $(XT_PRG_PREFIX)as
OBJCOPY = $(XT_PRG_PREFIX)objcopy
OBJDUMP = $(XT_PRG_PREFIX)objdump

# uRISCV-related paths
URISCV_DIR_PREFIX = /usr/local

URISCV_DATA_DIR = $(URISCV_DIR_PREFIX)/share/uriscv
URISCV_INCLUDE_DIR = $(URISCV_DIR_PREFIX)/include
URISCV_LIB_DIR = $(URISCV_DIR_PREFIX)/lib/uriscv

TDEFS = test/print.h test/tconst.h $(URISCV_INCLUDE_DIR)/uriscv/liburiscv.e Makefile

# CFLAGS = -ffreestanding -ansi -c -mips1 -mabi=32 -mfp32 -mno-gpopt -G 0 -I$(URISCV_INCLUDE_DIR) -std=gnu99 -fno-pic -mno-abicalls
CFLAGS_LANG = -ffreestanding -static -c -nostdlib
CFLAGS = $(CFLAGS_LANG) -I$(URISCV_INCLUDE_DIR) -Wall -O0 -march=rv32imfd -mabi=ilp32d
# -Wall

LDAOUTFLAGS = -G 0 -nostdlib -T $(URISCV_DATA_DIR)/uriscvaout.ldscript
LDCOREFLAGS = -G 0 -nostdlib -T $(URISCV_DATA_DIR)/uriscvcore.ldscript

# AS = mipsel-linux-gnu-as -KPIC

EF = uriscv-elf2uriscv
UDEV = uriscv-mkdev

# main target
all: todTest.uriscv terminalTest1.uriscv terminalTest2.uriscv terminalTest3.uriscv terminalTest4.uriscv fibEight.uriscv fibEleven.uriscv printerTest.uriscv

%.o: %.c $(TDEFS)
	$(CC) $(CFLAGS) $<

%.t: %.o print.o $(URISCV_LIB_DIR)/crti.o
	$(LD) $(LDAOUTFLAGS) $(URISCV_LIB_DIR)/crti.o $< print.o $(URISCV_LIB_DIR)/liburiscv.o -o $@

%.t.aout.uriscv: %.t
	$(EF) -a $<

%.uriscv: %.t.aout.uriscv
	$(UDEV) -f $@ $<

clean:
	rm -f *.o *.t *.uriscv

