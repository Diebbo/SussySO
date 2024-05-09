#ifndef VM_MANAGER_H
#define VM_MANAGER_H

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

// internal global variables
extern pcb_PTR swap_mutex;
extern unsigned swap_pool[UPROCMAX][UPROCMAX];

// internal phase2 global variables
extern pcb_PTR ssi_pcb;

// support level TLB handler
void sTLB_RefillHandler(void);
// get support data of current process
support_t *getSupportData(void);

// entry point function for mutex swap process
void entrySwapFunction();


#endif 
