#ifndef VM_MANAGER_H
#define VM_MANAGER_H

#include "stdlib.h"

// internal global variables
extern pcb_PTR swap_mutex;
extern swap_t swap_pool[2 * UPROCMAX];

// internal phase2 global variables & functions
extern pcb_PTR ssi_pcb;
extern void TrapExceptionHandler(state_t *);


void initSwapPool(void);

// retrun true if the frame is free
unsigned isSwapPoolFrameFree(unsigned);

// support level TLB handler
void sTLB_RefillHandler(void);

// support level PGM handler
void pager(void);

// get support data of current process
support_t *getSupportData(void);

// entry point function for mutex swap process
void entrySwapFunction();

// get frame from swap pool
unsigned getFrameFromSwapPool();

// get page from device
pteEntry_t *readBackingStore(unsigned , unsigned );

// write page into device
void writeBackingStore(pteEntry_t *);

#endif 
