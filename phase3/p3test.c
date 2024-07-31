#include "./headers/p3test.h"

extern pcb_PTR swap_mutex;

void test3() {
  /*While test was the name/external reference to a function that exercised the Level 3/Phase 2 code,
   * in Level 4/Phase 3 it will be used as the instantiator process (InstantiatorProcess).3
   * The InstantiatorProcess will perform the following tasks:
   *  • Initialize the Level 4/Phase 3 data structures. These are:
   *     – The Swap Pool table and a Swap Mutex process that must provide mutex access to the
   *        swap table using message passing [Section 4.1].
   *     – Each (potentially) sharable peripheral I/O device can have a process for it. These process
   *        will be used to receive complex requests (i.e. to write of a string to a terminal) and request
   *        the correct DoIO service to the SSI (this feature is optional and can be delegated directly
   *        to the SST processes to simplify the project).
   *  • Initialize and launch eight SST, one for each U-procs.
   *  • Terminate after all of its U-proc “children” processes conclude. This will drive Process Count
   *      to one, triggering the Nucleus to invoke HALT. Wait for 8 messages, that should be send when
   *      each SST is terminated.
   */

  // alloc swap mutex process
  allocSwapMutex();

  // Init. sharable peripheral (done in initSSTs)
  /* Technical Point: A careful reading of the Level 4/Phase 3 specification reveals that there are
   * actually no purposefully shared peripheral devices. Each of the [1..8] U-procs has its own flash device
   * (backing store), printer, and terminal device(s). Hence, one does not actually need a process to
   * protect access to device registers. However, for purposes of correctness (or more appropriate: to
   * protect against erroneous behavior) and future phase compatibility, it is possible to have a process for
   * each device that waits for messages and requests the single DoIO to the SSI.
   */

  // Init array of support struct (so each will be used for every u-proc init. in initSSTs)
  initSupportArray();

  //Init 8 SST
  initSSTs();

  //Terminate after the 8 sst die
  terminateAll();
}

void initSupportArray(){
  /* A Support Structure must contain all the fields necessary for the Support Level to support both
   * paging and passed up SYSCALL services. This includes:
   *   • sup_asid: The process’s ASID.
   *   • sup_exceptState[2]: The two processor state (state_t) areas where the processor state at the
   *      time of the exception is placed by the Nucleus for passing up exception handling to the Support
   *      Level.
   *   • sup_exceptContext[2]: The two processor context (context_t) sets. Each context is a PC
   *      / SP / Status combination. These are the two processor contexts which the Nucleus uses for
   *      passing up exception handling to the Support Level.
   *   • sup_privatePgTbl[32]: The process’s Page Table.
   *   • sup_stackTLB[500]: The stack area for the process’s TLB exception handler. An integer array
   *      of 500 is a 2Kb area.
   *   • sup_stackGen[500]: The stack area for the process’s Support Level general exception handler.
   * 
   * Only the sup_asid, sup_exceptContext[2], and sup_privatePgTbl[32] [Section 2.1] require
   * initialization prior to request the CreateProcess service.
   * To initialize a processor context area one performs the following:
   *   • Set the two PC fields. One of them (0 - PGFAULTEXCEPT) should be set to the address of the
   *      Support Level’s TLB handler, while the other one (1 - GENERALEXCEPT) should be set to the
   *      address of the Support Level’s general exception handler.
   *   • Set the two Status registers to: kernel-mode with all interrupts and the Processor Local Timer
   *      enabled.
   *   • Set the two SP fields to utilize the two stack spaces allocated in the Support Structure. Stacks
   *      grow “down” so set the SP fields to the address of the end of these areas.
   *      E.g. ... = &(...sup_stackGen[499]).
   */
  for(int asid=0; asid<8; asid++){
    memaddr maxaddr;
    support_arr[asid]->sup_asid = asid;

    support_arr[asid]->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pager;
    support_arr[asid]->sup_exceptContext[PGFAULTEXCEPT].stackPtr = RAMTOP(maxaddr) - (2 * asid * PAGESIZE);
    support_arr[asid]->sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;

    support_arr[asid]->sup_exceptContext[GENERALEXCEPT].pc = (memaddr) supportExceptionHandler;
    support_arr[asid]->sup_exceptContext[GENERALEXCEPT].stackPtr = RAMTOP(maxaddr) - (2 * asid * PAGESIZE) + PAGESIZE;
    support_arr[asid]->sup_exceptContext[GENERALEXCEPT].status = ALLOFF | IEPON | IMON | TEBITON;
  }
}

void terminateAll(){
  for(int i=0; i<8; i++){
    SYSCALL(RECEIVEMSG,ANYMESSAGE,0,0);
  }
  PANIC();
}

void allocSwapMutex(void){
  swap_mutex = allocPcb();
  RAMTOP(swap_mutex->p_s.reg_sp);
  swap_mutex->p_supportStruct->sup_asid = 0;
  process_count++;
  swap_mutex->p_s.pc_epc = (memaddr)entrySwapFunction;
  swap_mutex->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
  swap_mutex->p_s.mie = MIE_ALL;
  insertProcQ(&ready_queue_list, swap_mutex);
  initUProc(swap_mutex);
}

