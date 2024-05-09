#include "./headers/vmmenager.h"

pcb_PTR swap_mutex;

// swap pool
unsigned swap_pool[UPROCMAX][UPROCMAX];
/* processes from 1..8 can have up to 8 pages on memoriy at a time */

void entrySwapFunctio() {
  while (TRUE) {
    unsigned *req_payload, *res_payload;
    swap_t swap_message;
    unsigned *process_requesting_swap;
    // wait for a swap request
    process_requesting_swap = (unsigned *)SYSCALL(RECEIVEMSG, ANYMESSAGE, (unsigned *)(&swap_message), 0);
    
    // giving the process the swap mutex
    SYSCALL(SENDMSG, (unsigned)process_requesting_swap, 0, 0);
    // wait for the process to finish the swap
    SYSCALL(RECEIVEMSG, process_requesting_swap, 0, 0);
  }
}

void uTLB_RefillHandler() {
  /* This function will:
   * Locate the correct Page Table entry in the Current Process’s Page Table; a
   component of p_supportStruct.
   * Write the entry into the TLB using the TLBWR instruction
   [Section 6.4-pops].
   * Return control (LDST) to the Current Process to restart the address
   translation process. To accomplish this, a TLB-Refill event handler must:
    1. Determine the page number (denoted as p) of the missing TLB entry by
   inspecting EntryHi in the saved exception state located at the start of the
   BIOS Data Page.
    2. Get the Page Table entry for page number p for the Current Process. This
   will be located in the Current Process’s Page Table, which is part of its
   Support Structure.
    3. Write this Page Table entry into the TLB. This is a three-set process:
      (a) setENTRYHI
      (b) setENTRYLO
      (c) TLBWR
    4. Return control to the Current Process to retry the instruction that
   caused the TLB-Refill event: LDST on the saved exception state located at the
   start of the BIOS Data Page.
  */
  state_t *exception_state = (state_t *)BIOSDATAPAGE;
  // page number
  unsigned p = exception_state->entry_hi & 0xFFFFF000;
  // check if the page is in the TLB
  if () { // TODO:
  }
  // get the page table entry for the current process
  unsigned index = (p - KUSEG) >> VPNSHIFT;

  // write the entry into the TLB
  setENTRYHI(
      current_process->p_supportStruct->sup_privatePgTbl[index].pte_entryHI);
  setENTRYLO(
      current_process->p_supportStruct->sup_privatePgTbl[index].pte_entryLO);
  TLBWR();

  // return control to the current process
  LDST(&current_process->p_s);
}
