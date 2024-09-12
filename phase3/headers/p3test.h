#ifndef P3TEST_H
#define P3TEST_H

// uRISCV lib.
#include "../../headers/const.h"
#include "../../headers/types.h"
#include <uriscv/arch.h>
#include <uriscv/liburiscv.h>

#include "vmManager.h"

// extern ds and func.
extern unsigned current_stack_top;

// test phase 3 ('Istantiator process')
void test3(void);
// func. to initialize 8 support struct
void initSupportArray(void);
// func. to allocate the swap mutex process
pcb_PTR allocSwapMutex(void);
// func. that waits for the termination of the 8 SST
void waitTermination(pcb_PTR *ssts);

#endif
