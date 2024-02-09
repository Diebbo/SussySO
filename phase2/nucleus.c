#include "./headers/nucleus.h"
#include <uriscv/types.h>

/* GLOBAL VARIABLES*/
int process_count = 0; // started but not terminated processes
int soft_block_count = 0; // processes waiting for a resource
// pcb_t *ready_queue = NULL; 
LIST_HEAD(ready_queue_head);// tail pointer to the ready state queue processes
pcb_t *current_process = NULL;
pcb_t blocked_pbs[SEMDEVLEN - 1];
pcb_t support_blocked_pbs[SEMDEVLEN - 1]; //TODO: cambiare nome, non so a che serva

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

}

void uTLB_RefillHandler() {
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}
