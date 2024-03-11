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


//SSI id (the only process to have pid = 0)
#define ssi_id 0

/* GLOBAL VARIABLES*/
int process_count = 0;    // started but not terminated processes
int soft_block_count = 0; // processes waiting for a resource
struct list_head ready_queue_list; // tail pointer to the ready state queue processes
pcb_t *current_process;
struct list_head blockedPCBs[SEMDEVLEN - 1]; // size (siam sicuri ..-1 ?)
struct list_head pseudoClockList;            // pcb waiting for clock tick
struct list_head pcbFree_h; //pcb free list


// device list -> 6 devices you can find the list in const.h IL_*
pcb_PTR device_list[6][MAXPROC];
int device_free_index[6]; // 6 devices

/* GLOBAL FUNCTIONS */
void initKernel(void);
void uTLB_RefillHandler(void);

#endif
