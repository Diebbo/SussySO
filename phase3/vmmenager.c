#include "./headers/vmmenager.h"

pcb_PTR swap_mutex;

void mutexSwap(){
  while (TRUE) {
    unsigned *req_payload, *res_payload;
    req_payload = (unsigned*)SYSCALL(RECEIVEMESSAGE, (unsigned) swap_mutex, ANYMESSAGE, 0);
    pcb_PTR process_requesting_swap; // = ?
    // swap
    SYSCALL(RECEIVEMESSAGE, (unsigned) process_requesting_swap, (unsigned)res_payload, 0);
  }

}

void uTLB_RefillHandler() {
  /* This function will:
   * Locate the correct Page Table entry in the Current Process’s Page Table; a component of p_supportStruct.
   * Write the entry into the TLB using the TLBWR instruction [Section 6.4-pops].
   * Return control (LDST) to the Current Process to restart the address translation process. To accomplish this, a TLB-Refill event handler must:
    1. Determine the page number (denoted as p) of the missing TLB entry by inspecting EntryHi in the saved exception state located at the start of the BIOS Data Page.
    2. Get the Page Table entry for page number p for the Current Process. This will be located in the Current Process’s Page Table, which is part of its Support Structure.
    3. Write this Page Table entry into the TLB. This is a three-set process:
      (a) setENTRYHI
      (b) setENTRYLO
      (c) TLBWR
    4. Return control to the Current Process to retry the instruction that caused the TLB-Refill event: LDST on the saved exception state located at the start of the BIOS Data Page.
  */
}
