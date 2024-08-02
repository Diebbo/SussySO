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
extern support_t *support_arr[8];

#define OFFINTERRUPTS() setSTATUS(getSTATUS() & ~IECON)
#define ONINTERRUPTS() setSTATUS(getSTATUS() | IECON)

int getASID(void);

// init and fill the support page table with the correct values
void initUprocPageTable(pcb_PTR p);

// initialization of the support struct of the user process
void initSupportStruct(pcb_PTR u_proc);

// initialization of a single user process
pcb_PTR initUProc(pcb_PTR sst_father);

/*function to get support struct (requested to SSI)*/
support_t *getSupportData();

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

// send void message to the process
void notify(pcb_PTR);

// init default support struct
void defaultSupportData(support_t *, int);

void pager();

void supportExceptionHandler();
#endif // !STD_LIB_H
