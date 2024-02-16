#include "./headers/nucleus.h"

/* GLOBAL VARIABLES*/
int process_count = 0; // started but not terminated processes
int soft_block_count = 0; // processes waiting for a resource
// pcb_t *ready_queue = NULL; 
struct list_head ready_queue_head;// tail pointer to the ready state queue processes
pcb_t *current_process = NULL;
struct list_head blockedPCBs[SEMDEVLEN - 1]; // size (siam sicuri ..-1 ?)
struct list_head pseudoClockList; //time list
passupvector_t *passupvector = (passupvector_t *) PASSUPVECTOR; 
extern void test();

void initKernel(){

	// populate the passup vector
	passupvector->tlb_refill_handler = (memaddr) uTLB_RefillHandler; //TODO refil
	passupvector->tlb_refill_stackPtr = KERNELSTACK; // Stacks in µMPS3 grow down	
	passupvector->exception_handler = (memaddr) exceptionHandler; //TODO: our exception handler;
	passupvector->exception_stackPtr = KERNELSTACK;
	
	// initialize the nucleus data structures
	initPcbs(); 
	initMsgs();

	// Initialize other variables
	INIT_LIST_HEAD(&ready_queue_head);
	for(int i = 0; i < SEMDEVLEN; i++){
		INIT_LIST_HEAD(&blockedPCBs[i]);
	}
	INIT_LIST_HEAD(&pseudoClockList);

	// load the system wide interval timer
	int *interval_timer = (int *) INTERVALTMR;  //non si fa tipo LDIT(PSECOND) (visto da prog che mandò linux master sul gruppo)?
	*interval_timer = PSECOND; 

	// init the first process
	pcb_t *first_process = allocPcb();
	/*state_t*/
	/*s_entryHI: Represents the value of the EntryHi register, which holds high-order address bits of the virtual address being translated.
		s_cause: Corresponds to the Cause register, holding information about exceptions and interrupts.
		s_status: Associated with the Status register, containing control and status bits.
		s_pc: Represents the Program Counter (PC), holding the address of the next instruction to be executed.
		s_reg[STATEREGNUM]: An array representing general-purpose registers.*/

	/*init first process state */
    state_t initState;
	RAMTOP(initState.reg_sp); //Set SP to RAMTOP
	STST(&initState); //saving in pcb struct

	/*FROM MANUAL:
		IEc: bit 0 - The “current” global interrupt enable bit. When 0, regardless
		of the settings in Status.IM all interrupts are disabled. When 1, interrupt
		acceptance is controlled by Status.IM.
		• KUc: bit 1 - The “current” kernel-mode user-mode control bit. When Sta-
		tus.KUc=0 the processor is in kernel-mode.
		• IEp & KUp: bits 2-3 - the “previous” settings of the Status.IEc and Sta-
		tus.KUc.
		• IEo & KUo: bits 4-5 - the “previous” settings of the Status.IEp and Sta-
		tus.KUp - denoted the “old” bit settings.
		These six bits; IEc, KUc, IEp, KUp, IEo, and KUo act as 3-slot deep
		KU/IE bit stacks. Whenever an exception is raised the stack is pushed
		[Section 3.1] and whenever an interrupted execution stream is restarted, the
		stack is popped. [Section 7.4]*/
	
	initState.pc_epc = (memaddr) SSI_function_entry_point; //TODO
    initState.s_t9 = initState.pc_epc; //da erroe ma dovrebbe essere corretto (MANUALE pg.141 [maremmaputtana])
	initState.status = IECON | ALLOFF; //32 bit reg. -> 0x01 = Enable interrupts, kernel mode actv

	//TODO: pid unici?!
	first_process->p_s = initState;
	first_process->p_supportStruct = NULL;
	first_process->p_parent = NULL;
	first_process->p_time = 0;

	process_count++;

	//TODO: proc2

}

