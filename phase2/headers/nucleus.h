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
// started but not terminated processes
int process_count = 0;    
// processes waiting for a resource
int soft_block_count = 0; 
// tail pointer to the ready state queue processes
struct list_head ready_queue_list; 
pcb_t *current_process;
struct list_head blockedPCBs[SEMDEVLEN - 1]; // size (siam sicuri ..-1 ?)
// pcb waiting for clock tick
struct list_head pseudoClockList;        
// pcb free list    
struct list_head pcbFree_h;                  
// SSI id (the only process to have pid = 0)
#define ssi_id = 0;
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

//defined in p2test.c
extern void test(); 

#endif
