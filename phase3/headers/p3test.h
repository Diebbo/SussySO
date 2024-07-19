#ifndef P3TEST_H
#define P3TEST_H

// uRISCV lib.
#include "../headers/const.h"
#include "../headers/types.h"
#include <uriscv/arch.h>
#include <uriscv/liburiscv.h>

#include "vmManager.h"

// test phase 3 ('Istantiator process')
void test3(void);
// func. to panic after the 8 sst have been killed
void terminateAll(void);
// func. tu init. 8 support struct
void initSupportArray(void);

// extern ds and func.
extern pcb_t *current_process;

#endif