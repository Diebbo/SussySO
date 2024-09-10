#include "headers/stdlib.h"

extern memaddr current_stack_top;

// init and fill the support page table with the correct values
void initUprocPageTable(pteEntry_t *tbl, int asid) {
  for (int i = 0; i < MAXPAGES; i++) {
    tbl[i].pte_entryHI =
        KUSEG | (i << VPNSHIFT) | (asid << ASIDSHIFT);
    tbl[i].pte_entryLO = DIRTYON;
  }
  tbl[31].pte_entryHI =
      (0xbffff << VPNSHIFT) | (asid << ASIDSHIFT);
}

void initFreeStackTop(void){
  RAMTOP(current_stack_top);
  current_stack_top -= 3 * PAGESIZE;
}

void defaultSupportData(support_t *support_data, int asid){
  /*
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
  support_data->sup_asid = asid;


  support_data->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr) pager;
  support_data->sup_exceptContext[PGFAULTEXCEPT].stackPtr = getCurrentFreeStackTop();
  support_data->sup_exceptContext[PGFAULTEXCEPT].status = MSTATUS_MIE_MASK | MSTATUS_MPP_M | MSTATUS_MPIE_MASK;

  support_data->sup_exceptContext[GENERALEXCEPT].pc = (memaddr) supportExceptionHandler;
  support_data->sup_exceptContext[GENERALEXCEPT].stackPtr = getCurrentFreeStackTop();
  support_data->sup_exceptContext[GENERALEXCEPT].status = MSTATUS_MIE_MASK | MSTATUS_MPIE_MASK | MSTATUS_MPP_M;

  initUprocPageTable(support_data->sup_privatePgTbl, asid);
}

memaddr getCurrentFreeStackTop(void){
  unsigned tmp_stack_top = current_stack_top;
  current_stack_top -= PAGESIZE;
  return tmp_stack_top;
} 

// initialization of a single user process
pcb_PTR initUProc(state_t *u_proc_state, support_t *sst_support){
  /*To launch a U-proc, one simply requests a CreateProcess to the SSI. The ssi_create_process_t
   * that two parameters:
   *  • A pointer to the initial processor state for the U-proc.
   *  • A pointer to an initialized Support Structure for the U-proc.
   * Initial Processor State for a U-proc; Each U-proc’s initial processor state should have its:
   *  • PC (and s_t9) set to 0x8000.00B0; the address of the start of the .text section [Section 10.3.1-pops].
   *  • SP set to 0xC000.0000 [Section 2].
   *  • Status set for user-mode with all interrupts and the processor Local Timer enabled.
   *  • EntryHi.ASID set to the process’s unique ID; an integer from [1..8]
   * Important: Each U-proc MUST be assigned a unique, non-zero ASID.
   */
  STST(u_proc_state);

  u_proc_state->entry_hi = sst_support->sup_asid << ASIDSHIFT;
  u_proc_state->pc_epc = (memaddr) UPROCSTARTADDR;
  u_proc_state->reg_sp = (memaddr) USERSTACKTOP;
  u_proc_state->status |= MSTATUS_MIE_MASK | MSTATUS_MPIE_MASK;
  u_proc_state->status &= ~MSTATUS_MPP_MASK; // user mode
  u_proc_state->mie = MIE_ALL;

  return createChild(u_proc_state, sst_support);
}

/*function to get support struct (requested to SSI)*/
support_t *getSupportData(void) {
  support_t *support_data;
  ssi_payload_t getsup_payload = {
      .service_code = GETSUPPORTPTR,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload),0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&support_data), 0);
  return support_data;
}

/*function to request creation of a child to SSI*/
pcb_t *createChild(state_t *s, support_t *sup) {
    pcb_t *p;
    ssi_create_process_t ssi_create_process = {
        .state = s,
        .support = sup,
    };
    ssi_payload_t payload = {
        .service_code = CREATEPROCESS,
        .arg = &ssi_create_process,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p), 0);
    return p;
}

// gain mutual exclusion over the swap pool
void gainSwapMutex(){
  SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex, 0, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_mutex, 0, 0);
}

// release mutual exclusion over the swap pool
void releaseSwapMutex(){
  SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex, 0, 0);
}

// check if is a SST pid
int isOneOfSSTPids(int pid){
  return pid >= SSTPIDS && pid < SSTPIDS + MAXSSTNUM;
}

void terminateProcess(pcb_PTR arg){
    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = (void *)arg,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

void notify(pcb_PTR process){
  SYSCALL(SENDMESSAGE, (unsigned int)process, 0, 0);
}