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
/* given an unsigned int and an integer representing the bit you want to check
 (0-indexed) and return 1 if the bit is 1, 0 otherwise */
#define BIT_CHECKER(n, bit) (((n) >> (bit)) & 1)
// started but not terminated processes
extern int process_count;
// processes waiting for a resource
extern int soft_block_count;
// tail pointer to the ready state queue processes
extern struct list_head ready_queue_list;
extern pcb_t *current_process;
extern struct list_head blockedPCBs[SEMDEVLEN - 1]; 
// waiting for a message
extern struct list_head msg_queue_list;
// pcb waiting for clock tick
extern struct list_head pseudoClockList;
// SSI process
extern pcb_PTR ssi_pcb;
//pid counter
extern int pid_counter_tracer;
//accumulated CPU time
extern cpu_t acc_cpu_time;

void initKernel(void);
void Scheduler(void);

// copy entry_hi, cause, status, pc_epc and mie from source to dest
void copyState(state_t *source, state_t *dest);
// time accumulated function
cpu_t deltaTime(void);


/* EXTERN FUNCTIONS */
extern void uTLB_RefillHandler(void);
extern void exceptionHandler(void);
extern void TrapExceptionHandler(state_t *);
extern void SSI_function_entry_point(void);
extern void passUpOrDie(unsigned , state_t *);
extern void initSSI(void);
extern void initSST(void);

/* defined in p2test.c */
extern void test(void);

int main(void);

#endif
