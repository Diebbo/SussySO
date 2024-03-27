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
int process_count;
// processes waiting for a resource
int soft_block_count;
// tail pointer to the ready state queue processes
struct list_head ready_queue_list;
pcb_t *current_process;
struct list_head blockedPCBs[SEMDEVLEN - 1]; // size (siam sicuri ..-1 ?)

// waiting for a message
struct list_head msg_queue_list;

// pcb waiting for clock tick
struct list_head pseudoClockList;
// SSI id (the only process to have pid = 0)
#define SSIPID 0
// SSI process
pcb_PTR ssi_pcb;
// last pid number assigned to a process
int last_used_pid;

/* GLOBAL FUNCTIONS */
void initKernel(void);
void uTLB_RefillHandler(void);

/* COMMON FUNCTIONS*/
void exceptionHandler(void);
void TrapExceptionHandler(void);
void Scheduler(void);
void SSI_function_entry_point(void);
void passUpOrDie(pcb_t *process, unsigned value);
// generate unique pid for processes
int generatePid(void);
// Return pcb_ptr of a process given the list where it is and his pid, NULL if
// not found
pcb_PTR findProcessPtr(struct list_head *target_process, int pid);

// defined in p2test.c
void test(void);
int main(void);

#endif
