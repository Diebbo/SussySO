#include "./headers/nucleus.h"

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


	// create the first process
	pcb_t *first_process = allocPcb();
	first_process->p_s.status = (1 << 7) | ; // ?
	first_process->p_supportStruct = NULL;
	first_process->p_parent = NULL;
	first_process->p_time = 0;

}

