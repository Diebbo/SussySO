#ifndef STD_LIB_H

// general headers
#include "../../headers/const.h"
#include "../../headers/listx.h"
#include "../../headers/types.h"

// phase 1 headers
#include "../../phase1/headers/msg.h"
#include "../../phase1/headers/pcb.h"

// phase 2 headers
#include "../../phase2/headers/nucleus.h"

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

// internal global variables
extern pcb_PTR swap_mutex;
extern swap_t swap_pool[POOLSIZE];

// internal phase2 global variables & functions
extern pcb_PTR ssi_pcb;

// static support array
extern support_t support_arr[MAXSSTNUM];

#define OFFINTERRUPTS() setSTATUS(getSTATUS() & (~MSTATUS_MIE_MASK))
#define ONINTERRUPTS() setSTATUS(getSTATUS() | MSTATUS_MIE_MASK)

// init and fill the support page table with the correct values
void initUprocPageTable(pteEntry_t *tbl, int asid);

// initialization of a single user process
pcb_PTR initUProc(state_t *u_proc_state,support_t *sst_support);

/*function to get support struct (requested to SSI)*/
support_t *getSupportData(void);

/*function to request creation of a child to SSI*/
pcb_t *createChild(state_t *s, support_t *sup);

// gain mutual exclusion over the swap pool
void gainSwapMutex();

// release mutual exclusion over the swap pool
void releaseSwapMutex();

// check if is a SST pid
int isOneOfSSTPids(int pid);

// terminate a process
void terminateProcess(pcb_PTR);

// send message to parent to terminate
void terminateParent(void);

// send void message to the process
void notify(pcb_PTR);

// init default support struct
void defaultSupportData(support_t *, int);

// assign the current stack top and decrement it
memaddr getCurrentFreeStackTop(void);

// init the pcb of the print process
pcb_PTR initPrintProcess(state_t *,support_t *);

// entry function of each print process
void printEntry();

// print a string to the printer with the same number of the sender ASID
void writeOnPrinter(sst_print_PTR arg, unsigned asid);
// print a string to the terminal with the same number of the sender ASID
void writeOnTerminal(sst_print_PTR arg, unsigned asid);
// write a string to a device
void write(char *msg, int lenght, devreg_t *devAddrBase, enum writet write_to);

// init the free stack top
void initFreeStackTop(void);

// invalidate the page table of the current process, tbl and swap pool
void invalidateUProcPageTable(support_t *support);

// insert page into TLB, if not present
void updateTLB(pteEntry_t *page);

extern void pager();

extern void supportExceptionHandler(void);

extern void programTrapExceptionHandler();

#endif // !STD_LIB_H
