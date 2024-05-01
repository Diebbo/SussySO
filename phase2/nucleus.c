/*Implement the main function and export the Nucleus's global variables, such as
process count, soft-blocked count, blocked PCBs lists/pointers, etc.*/

#include "./headers/nucleus.h"

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
// accumulated CPU time
cpu_t acc_cpu_time;
// ssi
pcb_PTR ssi_pcb;

int main(void) {
  // 1. Initialize the nucleus
  initKernel();

  // 2. scheduler
  Scheduler();

  return 0;
}

void initKernel() {
  passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
  // populate the passup vector
  passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
  passupvector->tlb_refill_stackPtr = (memaddr)KERNELSTACK;
  passupvector->exception_handler = (memaddr)exceptionHandler;
  passupvector->exception_stackPtr = (memaddr)KERNELSTACK;

  // initialize the nucleus data structures
  initPcbs();
  initMsgs();

  // Initialize other variables
  soft_block_count = 0;
  process_count = 0;

  INIT_LIST_HEAD(&ready_queue_list);
  for (int i = 0; i < SEMDEVLEN - 1; i++) {
    INIT_LIST_HEAD(&blockedPCBs[i]);
  }
  INIT_LIST_HEAD(&pseudoClockList);
  current_process = NULL;

  INIT_LIST_HEAD(&msg_queue_list);

  // load the system wide interval timer
  LDIT(PSECOND);

  initSSI();

  process_count++;

  /*pcb_t *second_process = allocPcb();

  RAMTOP(second_process->p_s.reg_sp); // Set SP to RAMTOP - 2 * FRAME_SIZE
  second_process->p_s.reg_sp -= 2 * PAGESIZE;
  second_process->p_s.pc_epc = (memaddr)test;
  second_process->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
  second_process->p_s.mie = MIE_ALL;

  process_count++;

  insertProcQ(&ready_queue_list, second_process);
    */

  initSST();
  process_count++;

}

void copyState(state_t *source, state_t *dest) {
  dest->entry_hi = source->entry_hi;
  dest->cause = source->cause;
  dest->status = source->status;
  dest->pc_epc = source->pc_epc;
  dest->mie = source->mie;
  for (unsigned i = 0; i < REGISTERNUMBER; i++) {
    dest->gpr[i] = source->gpr[i];
  }
}

cpu_t deltaTime(void) {
  cpu_t current_time_TOD;
  return (STCK(current_time_TOD) - acc_cpu_time);
}
