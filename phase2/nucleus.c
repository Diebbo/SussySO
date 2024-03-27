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
struct list_head blockedPCBs[SEMDEVLEN - 1]; // size (siam sicuri ..-1 ?)

// waiting for a message
struct list_head msg_queue_list;

// pcb waiting for clock tick
struct list_head pseudoClockList;
// SSI id (the only process to have pid = 0)
#define SSIPID 0
// SSI process
pcb_PTR ssi_pcb;
// last pid number assigned to a process
int last_used_pid;

int main()
{
    // 1. Initialize the nucleus
    initKernel();

    // 2. scheduler
    Scheduler();

    return 0;
}

void initKernel() {
  passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
  // populate the passup vector
  passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler; // TODO refil
  passupvector->tlb_refill_stackPtr =
      (memaddr)KERNELSTACK; // Stacks in µMPS3 grow down
  passupvector->exception_stackPtr = (memaddr)KERNELSTACK;
  passupvector->exception_handler =
      (memaddr)exceptionHandler; 

  // initialize the nucleus data structures
  initPcbs();
  initMsgs();

  // Initialize other variables
  soft_block_count = 0;
  process_count = 0;
  last_used_pid = 0;

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
  STST(&first_process->p_s);         // saving in pcb struct

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
  first_process->p_s.status =
      IEPON |
      IMON |
      ALLOFF; // 32 bit reg. -> 0x01 = Enable interrupts, kernel mode actv

  // tree structure
  INIT_LIST_HEAD(&first_process->p_sib);
  INIT_LIST_HEAD(&first_process->p_child);
  first_process->p_parent = NULL;

  first_process->p_time = 0;

  first_process->p_supportStruct = NULL;

  list_add_tail(&first_process->p_list, &ready_queue_list);

  first_process->p_pid = SSIPID;

  ssi_pcb = first_process;

  process_count++;

  pcb_t *second_process = allocPcb();

  RAMTOP(second_process->p_s.reg_sp); // Set SP to RAMTOP - 2 * FRAME_SIZE
  second_process->p_s.reg_sp -= 2 * PAGESIZE; // STST()???
  second_process->p_s.pc_epc = (memaddr)test; // TODO
  second_process->p_s.status = IECON | ALLOFF;

  INIT_LIST_HEAD(&second_process->p_sib);
  INIT_LIST_HEAD(&second_process->p_child);
  second_process->p_parent = NULL;

  second_process->p_time = 0;

  second_process->p_supportStruct = NULL;

  list_add_tail(&second_process->p_list, &ready_queue_list);

  second_process->p_pid = generatePid();

  process_count++;
}

int generatePid(void) {
  // 40 = num max of pcb
  if (last_used_pid == MAXPROC) {
    last_used_pid = 0;
  }
  last_used_pid++;
  return last_used_pid;
}

pcb_PTR findProcessPtr(struct list_head *target_process, int pid) {
  pcb_PTR tmp;
  list_for_each_entry(tmp, target_process, p_list) {
    if (tmp->p_pid == pid)
      return tmp;
  }
  return NULL;
}

