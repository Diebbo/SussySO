/*Implement the main function and export the Nucleus's global variables, such as
process count, soft-blocked count, blocked PCBs lists/pointers, etc.*/

#include "./headers/nucleus.h"
/* GLOBAL VARIABLES*/
// started but not terminated processes
int process_count;
// processes waiting for a resource
int soft_block_count;
// tail pointer to the ready state queue processes
struct list_head ready_queue_list;
pcb_t *current_process;
struct list_head blockedPCBs[SEMDEVLEN - 1]; 
// waiting for a message
struct list_head msg_queue_list;
// pcb waiting for clock tick
struct list_head pseudoClockList;
// SSI process
pcb_PTR ssi_pcb;
//accumulated CPU time
cpu_t acc_cpu_time;

int main(int argc, char *argv[])
{
    // 1. Initialize the nucleus
    initKernel();

    // 2. scheduler
    Scheduler();

    return 0;
}

void initKernel() {
  // block interrupts
  //setSTATUS(ALLOFF);

  passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
  // populate the passup vector
  passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler; 
  passupvector->tlb_refill_stackPtr = (memaddr)KERNELSTACK;
  passupvector->exception_handler =
      (memaddr)exceptionHandler; 
  passupvector->exception_stackPtr = (memaddr)KERNELSTACK;
  
  // initialize the nucleus data structures
  initPcbs();
  initMsgs();

  // Initialize other variables
  soft_block_count = 0;
  process_count = 0;

  INIT_LIST_HEAD(&ready_queue_list);
  for (int i = 0; i < SEMDEVLEN-1; i++) {
    INIT_LIST_HEAD(&blockedPCBs[i]);
  }
  INIT_LIST_HEAD(&pseudoClockList);
  current_process = NULL;

  INIT_LIST_HEAD(&msg_queue_list);
  
  // load the system wide interval timer
  LDIT(PSECOND);

  // init the first process
  pcb_t *first_process = allocPcb();
  /*state_t*/
  /*s_entryHI: Represents the value of the EntryHi register, which holds
     high-order address bits of the virtual address being translated. s_cause:
     Corresponds to the Cause register, holding information about exceptions and
     interrupts. s_status: Associated with the Status register, containing
     control and status bits. s_pc: Represents the Program Counter (PC), holding
     the address of the next instruction to be executed. s_reg[STATEREGNUM]: An
     array representing general-purpose registers.*/

  /*init first process state */
  RAMTOP(first_process->p_s.reg_sp); // Set SP to RAMTOP

  /*FROM MANUAL:
          IEc: bit 0 - The “current” global interrupt enable bit. When 0,
     regardless of the settings in Status.IM all interrupts are disabled. When
     1, interrupt acceptance is controlled by Status.IM. • KUc: bit 1 - The
     “current” kernel-mode user-mode control bit. When Sta- tus.KUc=0 the
     processor is in kernel-mode. • IEp & KUp: bits 2-3 - the “previous”
     settings of the Status.IEc and Sta- tus.KUc. • IEo & KUo: bits 4-5 - the
     “previous” settings of the Status.IEp and Sta- tus.KUp - denoted the “old”
     bit settings. These six bits; IEc, KUc, IEp, KUp, IEo, and KUo act as
     3-slot deep KU/IE bit stacks. Whenever an exception is raised the stack is
     pushed [Section 3.1] and whenever an interrupted execution stream is
     restarted, the stack is popped. [Section 7.4]*/

  first_process->p_s.pc_epc = (memaddr)SSI_function_entry_point; 
  first_process->p_s.status = IMON | IEPON | IECON | TEBITON; //| ALLOFF;

  list_add_tail(&first_process->p_list, &ready_queue_list);

  first_process->p_pid = SSIPID;

  ssi_pcb = first_process;

  process_count++;

  pcb_t *second_process = allocPcb();

  RAMTOP(second_process->p_s.reg_sp); // Set SP to RAMTOP - 2 * FRAME_SIZE
  second_process->p_s.reg_sp -= 2 * PAGESIZE; // STST()???
  second_process->p_s.pc_epc = (memaddr)test; 
  second_process->p_s.status = IMON | IEPON | IECON | TEBITON;// | ALLOFF;

  process_count++;

  list_add_tail(&second_process->p_list, &ready_queue_list);

  // setSTATUS(IMON | IEPON | IECON);
}

pcb_PTR findProcessPtr(struct list_head *target_process, int pid) {
  pcb_PTR tmp;
  list_for_each_entry(tmp, target_process, p_list) {
    if (tmp->p_pid == pid)
      return tmp;
  }
  return NULL;
}

void copyState(state_t *source, state_t *dest) {
    dest->entry_hi = source->entry_hi;
    dest->cause = source->cause;
    dest->status = source->status;
    dest->pc_epc = source->pc_epc;
    dest->mie = source->mie;
    for (unsigned i = 0; i < 32; i++)
    {
        dest->gpr[i] = source->gpr[i];
    }
    
}

cpu_t deltaTime(void) {
  cpu_t current_time;
  STCK(current_time);
  return current_time - acc_cpu_time;
}