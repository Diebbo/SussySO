#ifndef VM_MANAGER_H
#define VM_MANAGER_H

#include "../../phase2/headers/nucleus.h"

extern pcb_PTR swap_mutex;

void uTLB_RefillHandler();

void mutexSwap();

#endif
