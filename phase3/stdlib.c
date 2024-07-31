#include "headers/stdlib.h"

// internal global variables
support_t *support_arr[8];

int getASID(void) {
  static unsigned next_asid = 1; // asid 0 is reserved for nucleus
  if (next_asid >= 8) {
    /*function cannot be called more than 8 time*/
    next_asid = 1;
    return NOPROC;
  }
  return next_asid++;
}

// init and fill the support page table with the correct values
void initUprocPageTable(pcb_PTR p) {
  // nucleus proces has asid 0
  if (p->p_supportStruct->sup_asid == NOPROC)
    p->p_supportStruct->sup_asid = getASID();

  for (int i = 0; i < MAXPAGES; i++) {
    p->p_supportStruct->sup_privatePgTbl[i].pte_entryHI =
        (KUSEG + (i << VPNSHIFT)) | (p->p_supportStruct->sup_asid << ASIDSHIFT);
    p->p_supportStruct->sup_privatePgTbl[i].pte_entryLO = DIRTYON;
  }
  p->p_supportStruct->sup_privatePgTbl[31].pte_entryHI =
      (0xbffff << VPNSHIFT) | (p->p_supportStruct->sup_asid << ASIDSHIFT);
}

// initialization of the support struct of the user process
void initSupportStruct(pcb_PTR u_proc){
  static unsigned support_index = 0;
  u_proc->p_supportStruct = support_arr[support_index++];
  u_proc->p_s.entry_hi= u_proc->p_supportStruct->sup_asid << ASIDSHIFT;
}

// initialization of a single user process
void initUProc(pcb_PTR sst_father){
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
  state_t u_proc_state;
  STST(&u_proc_state);

  pcb_PTR u_proc = CreateChild(&u_proc_state);
  u_proc->p_s.pc_epc = (memaddr) UPROCSTARTADDR;
  u_proc->p_s.reg_sp = (memaddr) USERSTACKTOP;
  u_proc->p_s.mie = ALLOFF | USERPON | IEPON | IMON | TEBITON;

  initSupportStruct(u_proc);
}

/*function to get support struct (requested to SSI)*/
support_t *getSupportData() {
  support_t *support_data;
  ssi_payload_t getsup_payload = {
      .service_code = GETSUPPORTPTR,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload),0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload), 0);
  return support_data;
}

/*function to request creation of a child to SSI*/
pcb_t *CreateChild(state_t *s){
    pcb_t *p;
    ssi_create_process_t ssi_create_process = {
        .state = s,
        .support = (support_t *)NULL,
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
  SYSCALL(SENDMSG, (unsigned int)swap_mutex, 0, 0);
  SYSCALL(RECEIVEMSG, (unsigned int)swap_mutex, 0, 0);
}

// release mutual exclusion over the swap pool
void releaseSwapMutex(){
  SYSCALL(SENDMSG, (unsigned int)swap_mutex, 0, 0);
}

// check if is a SST pid
int isOneOfSSTPids(int pid){
  for(int i=0; i<8; i++){
    if(pid == (SSTPIDS -i)){
      return TRUE;
    }
  }
  return FALSE;
}

