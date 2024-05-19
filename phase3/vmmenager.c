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
    unsigned int process_requesting_swap =
        SYSCALL(RECEIVEMSG, ANYMESSAGE, (unsigned int)(&swap_message), 0);

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

  /* enter the critical section */

  // get the missing page number
  unsigned missing_page = (getENTRYHI() & GETPAGENO) >> VPNSHIFT;

  // pick a frame from the swap pool
  unsigned frame = getFrameFromSwapPool();

  // check if the frame is occupied
  if (swap_pool[frame] != NULL) {
    // we assume the frame is occupied by a dirty page

    // operations performed atomically
    unsigned status = getSTATUS();
    setSTATUS(status & ~IECON); // disable interrupts

    // mark the page pointed by the swap pool as not valid
    swap_pool[frame]->pte_entryLO &= !VALIDON;

    // update the TLB if needed
    // TODO: now we are not updating the TLB, we are just invalidating it
    TLBCLR();

    // update the backing store
    writeBackingStore(swap_pool[frame]);

    // restore interrupt state
    setSTATUS(status);
  }

  // read the contents of the current process's backing store
  // TODO: ? [Section 5.1] & [Section 9]
  pteEntry_t *new_page = readBackingStore(missing_page);

  // update the swap pool table
  swap_pool[frame] = new_page;

  // update the current process's page table
  support_data->sup_privatePgTbl[missing_page].pte_entryLO |= VALIDON;
  support_data->sup_privatePgTbl[missing_page].pte_entryLO |=
      (frame << 0x0); // TODO: check this

  // update the TLB
  TLBWR();

  // release mutual exclusion over the swap pool
  SYSCALL(SENDMSG, (unsigned int)swap_mutex, 0, 0);

  /* exit the critical section */

  // return control to the current process
  LDST(exception_state);
}

void writeBackingStore(pteEntry_t *page) {
  unsigned asid = (page->pte_entryHI >> ASIDSHIFT) & 0x0F;
  unsigned missing_page = (page->pte_entryHI >> VPNSHIFT) & 0x0F;
  unsigned value = page->pte_entryLO;
  unsigned command = 0;
  unsigned status;

  ssi_do_io_t do_io = {
      .commandAddr = command,
      .commandValue = value,
  };
  ssi_payload_t payload = {
      .service_code = DOIO,
      .arg = &do_io,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
}

pteEntry_t *readBackingStore(unsigned missing_page, unsigned asid) {}

unsigned getFrameFromSwapPool() {
  // implement the page replacement algorithm FIFO
  static unsigned frame = 0;
  frame = (frame + 1) % (2 * UPROCMAX);
  return frame;
}
