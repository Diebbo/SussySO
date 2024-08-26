#ifndef VM_MANAGER_H

#include "sysSupport.h"

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

// entry point function for mutex swap process
void entrySwapFunction();

// get frame from swap pool
unsigned getFrameFromSwapPool();

// flash operation
unsigned flashOperation(unsigned command, unsigned page_addr, unsigned asid, unsigned page_number);

unsigned readBackingStoreFromPage(memaddr missing_page_addr, unsigned asid, unsigned page_number);

unsigned writeBackingStore(memaddr missing_page_addr, unsigned asid, unsigned page_no);

// insert page into TLB, if not present
void updateTLB(pteEntry_t *page);
#endif
