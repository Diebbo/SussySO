#include "./headers/p3test.h"

// external global variables
extern pcb_PTR swap_mutex;
extern pcb_PTR sst_pcb[MAXSSTNUM];
extern memaddr current_stack_top;
extern support_t support_arr[MAXSSTNUM];

// internal global variables
pcb_PTR test_process;
state_t swap_st;
support_t swap_sup;

void test3() {
  test_process = current_process;
  initFreeStackTop();
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

  // Init array of support struct (so each will be used for every u-proc init. in initSSTs)
  initSupportArray();

  // alloc swap mutex process
  swap_mutex = allocSwapMutex();

  // Init. sharable peripheral (done in initSSTs)
  /* Technical Point: A careful reading of the Level 4/Phase 3 specification reveals that there are
   * actually no purposefully shared peripheral devices. Each of the [1..8] U-procs has its own flash device
   * (backing store), printer, and terminal device(s). Hence, one does not actually need a process to
   * protect access to device registers. However, for purposes of correctness (or more appropriate: to
   * protect against erroneous behavior) and future phase compatibility, it is possible to have a process for
   * each device that waits for messages and requests the single DoIO to the SSI.
   */

  //Init 8 SST
  initSSTs();

  //Terminate after the 8 sst die
  waitTermination(sst_pcb);

  // terminate the test process
  terminateProcess(SELF);
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
   * 
   * NOTE: (ASID 0 is reserved for kernel daemons, so the (up to) eight U-proc’s should be assigned ASID values from [1..8].)
   */
  INIT_LIST_HEAD(&free_supports);

  for(int i=0; i < MAXSSTNUM; i++){
    INIT_LIST_HEAD(&support_arr[i].s_list);
    deallocateSupport(&support_arr[i]);
  }
}

void waitTermination(pcb_PTR *ssts){
  for(int i=0; i < MAXSSTNUM; i++){
    SYSCALL(RECEIVEMESSAGE, (unsigned)ssts[i],0,0);
  }
}

pcb_PTR allocSwapMutex(void){
  STST(&swap_st);
  swap_st.reg_sp = getCurrentFreeStackTop();
  swap_st.status |= MSTATUS_MIE_MASK | MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
  swap_st.pc_epc = (memaddr) entrySwapFunction;
  swap_st.mie = MIE_ALL;

  pcb_PTR child = createChild(&swap_st, NULL);
  
  return child;
}

