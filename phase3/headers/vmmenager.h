#ifndef VM_MANAGER_H
#define VM_MANAGER_H

#include "../../phase2/headers/nucleus.h"

extern pcb_PTR swap_mutex;

extern unsigned swap_pool[UPROCMAX][UPROCMAX];

// support level TLB handler
void sTLB_RefillHandler();

// entry point function for mutex swap process
void entrySwapFunction();

// TODO: giuca
void pager();


#endif 
