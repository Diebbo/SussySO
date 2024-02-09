#include "./headers/nucleus.h"
#include <uriscv/types.h>
#include <uriscv/bios.h>
#include <uriscv/liburiscv.h>

/* GLOBAL VARIABLES*/
int process_count; // started but not terminated processes
int soft_block_count; // processes waiting for a resource
// pcb_t *ready_queue = NULL; 
LIST_HEAD(ready_queue_head);// tail pointer to the ready state queue processes
pcb_t *current_process;
LIST_HEAD(blocked_pbs); // size -> [SEMDEVLEN - 1];
LIST_HEAD(support_blocked_pbs); //TODO: cambiare nome, non so a che serva

void initKernel(){
	// populate the passup vector
	passupvector_t *passupvector = (passupvector_t *) PASSUPVECTOR;
	passupvector->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
	passupvector->tlb_refill_stackPtr = KERNELSTACK; // Stacks in µMPS3 grow down
	
	passupvector->exception_handler = (memaddr) exceptionHandler; //TODO: our exception handler;
	
	// TODO: set the stack pointer for the exception handler
	
	// initialize the nucleus data structures
	initPcbs();
	initMsgs();

	process_count = 0;
	soft_block_count = 0;
	INIT_LIST_HEAD(&ready_queue_head);
	current_process = NULL;
	INIT_LIST_HEAD(&blocked_pbs);
	INIT_LIST_HEAD(&support_blocked_pbs);

	// load the system wide interval timer
	int *interval_timer = (int *) INTERVALTMR;
	*interval_timer = PSECOND; 
}

void uTLB_RefillHandler() {
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}
