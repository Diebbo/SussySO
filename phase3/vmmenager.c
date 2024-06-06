#include "./headers/vmmenager.h"

pcb_PTR swap_mutex;

// swap pool
swap_t swap_pool[POOLSIZE];

void initSwapPool(void) {
  // initialize the swap pool
  for (int i = 0; i < 2 * UPROCMAX; i++) {
    swap_pool[i].sw_asid = NOPROC;
  }
}

unsigned isSwapPoolFrameFree(unsigned frame) {
  return swap_pool[frame].sw_asid == NOPROC;
}

void entrySwapFunction() {
  // initialize the swap pool
  initSwapPool();

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

void pager(void) {
  unsigned status;
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
  unsigned missing_page = (exception_state->entry_hi & GETPAGENO) >> VPNSHIFT;
  // pick a frame from the swap pool
  unsigned frame = getFrameFromSwapPool();
  memaddr page_addr = SWAPPOOLADDR + (frame * PAGESIZE);

  // check if the frame is occupied
  if (!isSwapPoolFrameFree(frame)) {
    // we assume the frame is occupied by a dirty page

    // operations performed atomically
    status = getSTATUS();
    setSTATUS(status & ~IECON); // disable interrupts

    // mark the page pointed by the swap pool as not valid
    swap_pool[frame].sw_pte->pte_entryLO &= !VALIDON;

    // update the TLB if needed
    // TODO: now we are not updating the TLB, we are just invalidating it
    TLBCLR();

    // update the backing store
    writeBackingStore(frame);

    // restore interrupt state
    setSTATUS(status);
  }

  // read the contents of the current process's backing store
  // TODO: ? [Section 5.1] & [Section 9]
  pteEntry_t *new_page = readBackingStore(missing_page);

  // update the swap pool table
  swap_pool[frame].sw_asid = support_data->sup_asid;
  swap_pool[frame].sw_pageNo = missing_page;
  swap_pool[frame].sw_pte = support_data->sup_privatePgTbl[missing_page];

  // operations performed atomically
  status = getSTATUS();
  setSTATUS(status & ~IECON); // disable interrupts

  // update the current process's page table
  support_data->sup_privatePgTbl[missing_page].pte_entryLO =
      page_addr | DIRTYON | VALIDON;

  // place the new page in the CP0

  // update the TLB
  updateTLB(new_page);

  // restore interrupt state
  setSTATUS(status);

  // release mutual exclusion over the swap pool
  SYSCALL(SENDMSG, (unsigned int)swap_mutex, 0, 0);

  /* exit the critical section */

  // return control to the current process
  LDST(exception_state);
}

void updateTLB(pteEntry_t *page) {
  // place the new page in the Data0 register
  setENTRYHI(page->pte_entryHI);
  TLBP();
  // check if the page is already in the TLB
  if ((getINDEX() & PRESENTFLAG) == 0) {
    // the page is not in the TLB
    setENTRYHI(p.pte_entryHI);
    setENTRYLO(p.pte_entryLO);
    TLBWI();
  }
}

void writeBackingStore(unsigned frame_number) {
  unsigned status, command;
  unsigned value = (unsigned)swap_pool[frame_number].sw_pte;
  command = START_DEVREG + (swap_pool[frame_number].sw_asid << 4) +
            0x0; // i actually don't know

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

pteEntry_t *readBackingStore(unsigned missing_page, unsigned asid) {
  unsigned command = START_DEVREG + (asid << 4) + 0x1; // i actually don't know
  unsigned status;
  unsigned value;

  ssi_do_io_t do_io = {
      .commandAddr = command,
      .commandValue = missing_page,
  };
  ssi_payload_t payload = {
      .service_code = DOIO,
      .arg = &do_io,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&value), 0);

  pteEntry_t *new_page = (pteEntry_t *)value;
  new_page->pte_entryHI = (asid << ASIDSHIFT) | (missing_page << VPNSHIFT);
  return new_page;
}

unsigned getFrameFromSwapPool() {
  // implement the page replacement algorithm FIFO
  static unsigned frame = 0;
  for (unsigned i = 0; i < POOLSIZE; i++) {
    if (isSwapPoolFrameFree(i)) {
      frame = i;
      break;
    }
  }
  return frame++ % POOLSIZE;
}
