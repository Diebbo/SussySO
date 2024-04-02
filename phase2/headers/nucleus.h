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
// SSI id (the only process to have pid = 0)
#define SSIPID 0
// SSI process
extern pcb_PTR ssi_pcb;
//pid counter
extern int pid_counter_tracer;
//accumulated CPU time
extern cpu_t acc_cpu_time;

void initKernel(void);


/* GLOBAL FUNCTIONS */
void uTLB_RefillHandler(void);

void exceptionHandler(void);
void TrapExceptionHandler(void);
void Scheduler(void);
void SSI_function_entry_point(void);
void passUpOrDie(pcb_t *process, unsigned value);
// Return pcb_ptr of a process given the list where it is and his pid, NULL if not found
pcb_PTR findProcessPtr(struct list_head *target_process, int pid);
/*copy entry_hi, cause, status, pc_epc and mie from source to dest*/
void copyState(state_t *source, state_t *dest);
cpu_t deltaTime(void);

/* defined in p2test.c */
extern void test(void);


int main(int, char **);

#endif
