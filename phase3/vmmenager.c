#include "./headers/vmmenager.h"
#include <uriscv/liburiscv.h>

pcb_PTR swap_mutex;

// swap pool
pteEntry_t *swap_pool[2 * UPROCMAX];

void entrySwapFunctio() {
  while (TRUE) {
    unsigned *req_payload, *res_payload;
    swap_t swap_message;
    // wait for a swap request
    unsigned int process_requesting_swap = SYSCALL(
        RECEIVEMSG, ANYMESSAGE, (unsigned int)(&swap_message), 0);

    // giving the process the swap mutex
    SYSCALL(SENDMSG, (unsigned int)process_requesting_swap, 0, 0);
    // wait for the process to finish the swap
    SYSCALL(RECEIVEMSG, (unsigned int)process_requesting_swap, 0, 0);
  }
}

support_t *getSupportData() {
  support_t *support_data;
  ssi_payload_t getsup_payload = {
      .service_code = GETSUPPORTPTR,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload),
          0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&getsup_payload), 0);
  return support_data;
}

void uTLB_RefillHandler() {
  /*
   * 1. Obtain the pointer to the Current Process’s Support Structure requesting
the GetSupportData service to the SSI. Important: Level 4/Phase 3 exception
handlers are limited in their interaction with the Nucleus and its data
structures to the functionality of SYSCALLs identified by negative numbers and
service requests to the SSI.
6
2. Determine the cause of the TLB exception. The saved exception state
responsible for this TLB exception should be found in the Current Process’s
Support Structure for TLB exceptions (sup_exceptState[0]’s Cause register).
3. If the Cause is a TLB-Modification exception, treat this exception as a
program trap [Section 9], otherwise continue.
4. Gain mutual exclusion over the Swap Pool table sending a message to the
swap_table PCB and waiting for a response.
5. Determine the missing page number (denoted as p): found in the saved
exception state’s EntryHi.
6. Pick a frame, i, from the Swap Pool. Which frame is selected is determined by
the µPandOS page replacement algorithm [Section 5.4].
7. Determine if frame i is occupied; examine entry i in the Swap Pool table.
8. If frame i is currently occupied, assume it is occupied by logical page
number k belonging to process x (ASID) and that it is “dirty” (i.e. been
modified): (a) Update process x’s Page Table: mark Page Table entry k as not
valid. This entry is easily accessible, since the Swap Pool table’s entry i
contains a pointer to this Page Table entry. (b) Update the TLB, if needed. The
TLB is a cache of the most recently executed process’s Page Table entries. If
process x’s page k’s Page Table entry is currently cached in the TLB it is
clearly out of date; it was just updated in the previous step. Important: This
step and the previous step must be accomplished atomically [Section 5.3]. (c)
Update process x’s backing store. Write the contents of frame i to the correct
location on process x’s backing store/flash device [Section 5.1]. Treat any
error status from the write operation as a program trap [Section 9].
9. Read the contents of the Current Process’s backing store/flash device logical
page p into frame i [Section 5.1]. Treat any error status from the read
operation as a program trap [Section 9].
10. Update the Swap Pool table’s entry i to reflect frame i’s new contents: page
p belonging to the Current Process’s ASID, and a pointer to the Current
Process’s Page Table entry for page p.
11. Update the Current Process’s Page Table entry for page p to indicate it is
now present (V bit) and occupying frame i (PFN field).
12. Update the TLB. The cached entry in the TLB for the Current Process’s page p
is clearly out of date; it was just updated in the previous step. Important:
This step and the previous step must be accomplished atomically [Section 5.3].
13. Release mutual exclusion over the Swap Pool table sending a message to the
swap_table PCB.
14. Return control to the Current Process to retry the instruction that caused
the page fault: LDST on the saved exception state.
*/
  // get the support data of the current process
  support_t *support_data = getSupportData();

  state_t *exception_state = &(support_data->sup_exceptState[0]);

  // check if the exception is a TLB-Modification exception
  if (exception_state->cause == TLBMOD) {
    // treat this exception as a program trap
    TrapExceptionHandler(exception_state);
  }

  // gain mutual exclusion over the swap pool
  SYSCALL(SENDMSG, (unsigned int)swap_mutex, 0, 0);
  SYSCALL(RECEIVEMSG, (unsigned int)swap_mutex, 0, 0);

  // get the missing page number
  unsigned missing_page = (getENTRYHI() & GETPAGENO) >> VPNSHIFT;
  
  // pick a frame from the swap pool
  unsigned frame = getFrameFromSwapPool();

  // check if the frame is occupied
  if (swap_pool[frame] != NULL) {
    // we assume the frame is occupied by a dirty page

    // mark the page pointed by the swap pool as not valid
    swap_pool[frame]->pte_entryLO |= !VALIDON;

    // update the TLB if needed
    // TODO: ? [Section 5.3]  -> TBLR()
    TLBWR();

    // update the backing store
    // TODO: ? [Section 5.1] & [Section 9]
  }

  // read the contents of the current process's backing store
  // TODO: ? [Section 5.1] & [Section 9]
  pteEntry_t *new_page = readBackingStore(missing_page);

  // update the swap pool table
  swap_pool[frame] = new_page;

  // update the current process's page table
  support_data->sup_privatePgTbl[missing_page].pte_entryLO |= VALIDON;
  support_data->sup_privatePgTbl[missing_page].pte_entryLO |= (frame << 0x0); // TODO: check this

  // update the TLB
  TLBWR();

  // release mutual exclusion over the swap pool
  SYSCALL(SENDMSG, (unsigned int)swap_mutex, 0, 0);

  // return control to the current process
  LDST(exception_state);
}

unsigned getFrameFromSwapPool(){
  // TODO: implement the page replacement algorithm
}

