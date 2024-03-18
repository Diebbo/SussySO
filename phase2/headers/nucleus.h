#ifndef NUCL_H
#define NUCL_H

// general headers
#include "../../headers/const.h"
#include "../../headers/listx.h"
#include "../../headers/types.h"

// phase 1 headers
#include "../../phase1/headers/msg.h"
#include "../../phase1/headers/pcb.h"

// uriscv headers
#include <uriscv/aout.h>
#include <uriscv/arch.h>
#include <uriscv/bios.h>
#include <uriscv/const.h>
#include <uriscv/cpu.h>
#include <uriscv/csr.h> //ucode.h <--- da errore 'impossibile aprire il codice errore "uriscv/ucode.h" del file origine',
#include <uriscv/liburiscv.h>
#include <uriscv/regdef.h>
#include <uriscv/types.h>

/* GLOBAL VARIABLES*/
int process_count = 0;    // started but not terminated processes
int soft_block_count = 0; // processes waiting for a resource
struct list_head
    ready_queue_list; // tail pointer to the ready state queue processes
pcb_t *current_process;
struct list_head blockedPCBs[SEMDEVLEN - 1]; // size (siam sicuri ..-1 ?)
struct list_head pseudoClockList;            // pcb waiting for clock tick
struct list_head pcbFree_h;                  // pcb free list

// last pid number assigned to a process
int last_used_pid = 0;

/* GLOBAL FUNCTIONS */
void initKernel(void);
void uTLB_RefillHandler(void);

/* COMMON FUNCTIONS*/
void exceptionHandler();
void TrapExceptionHandler();
void Scheduler();
void SSI_function_entry_point(void);
void passUpOrDie(pcb_t *, unsigned);

// generate unique pid for processes
int generate_pid();

extern void test(); //defined in p2test.c

#endif
