#ifndef NUCL_H
#define NUCL_H

// general headers
#include "../../headers/const.h"
#include "../../headers/listx.h"
#include "../../headers/types.h"

// phase 1 headers
#include "../../phase1/headers/pcb.h"
#include "../../phase1/headers/msg.h"


// phase 2 headers
#include <uriscv/types.h>
#include <uriscv/bios.h>
#include <uriscv/liburiscv.h>

/* GLOBAL VARIABLES*/
int process_count = 0;    // started but not terminated processes
int soft_block_count = 0; // processes waiting for a resource
struct list_head ready_queue_list; // tail pointer to the ready state queue processes
pcb_t *current_process;
struct list_head blockedPCBs[SEMDEVLEN - 1]; // size (siam sicuri ..-1 ?)
struct list_head pseudoClockList;            // time list



/* GLOBAL FUNCTIONS */
void initKernel(void);
void uTLB_RefillHandler(void);

#endif
