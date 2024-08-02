#include "./headers/vmManager.h"

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
    // wait for a swap request
    unsigned int process_requesting_swap = SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);

    // giving the process the swap mutex
    SYSCALL(SENDMESSAGE, (unsigned int)process_requesting_swap, 0, 0);
    // wait for the process to finish the swap
    SYSCALL(RECEIVEMESSAGE, (unsigned int)process_requesting_swap, 0, 0);
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

  gainSwapMutex();

  /* enter the critical section */

  // get the missing page number
  unsigned missing_page = (exception_state->entry_hi & GETPAGENO) >> VPNSHIFT;
  // pick a frame from the swap pool
  unsigned victim_frame = getFrameFromSwapPool();
  memaddr victim_page_addr = SWAPPOOLADDR + (victim_frame * PAGESIZE);

  // check if the frame is occupied
  if (!isSwapPoolFrameFree(victim_frame)) {
    // we assume the frame is occupied by a dirty page

    // operations performed atomically
    OFFINTERRUPTS();

    // mark the page pointed by the swap pool as not valid
    swap_pool[victim_frame].sw_pte->pte_entryLO &= !VALIDON;

    // update the TLB if needed
    // TODO: now we are not updating the TLB, we are just invalidating it
    TLBCLR();

    // update the backing store
    status = writeBackingStore(victim_page_addr, support_data->sup_asid,
                               swap_pool[victim_frame].sw_pageNo);
    // TODO: check status
    status = status; // to avoid warning

    ONINTERRUPTS();
  }

  // read the contents of the current process's backing store

  int vpn = ENTRYHI_GET_VPN(exception_state->entry_hi);

  status =
      readBackingStoreFromPage(victim_page_addr, support_data->sup_asid, vpn);

  // TODO: check status

  // update the swap pool table
  swap_pool[victim_frame].sw_asid = support_data->sup_asid;
  swap_pool[victim_frame].sw_pageNo = missing_page;
  swap_pool[victim_frame].sw_pte =
      &support_data->sup_privatePgTbl[missing_page];

  // #region atomic operations
  OFFINTERRUPTS();

  // update the current process's page table
  support_data->sup_privatePgTbl[missing_page].pte_entryLO =
      ((unsigned)victim_page_addr << PFNSHIFT) | DIRTYON | VALIDON;

  // place the new page in the CP0

  // update the TLB
  updateTLB(&support_data->sup_privatePgTbl[missing_page]);

  ONINTERRUPTS();
  // #endregion atomic operations

  releaseSwapMutex();

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
    setENTRYHI(page->pte_entryHI);
    setENTRYLO(page->pte_entryLO);
    TLBWI();
  }
}

unsigned flashOperation(unsigned command, unsigned page_addr, unsigned asid,
                        unsigned page_number) {
  dtpreg_t *flash_dev_addr = (dtpreg_t *)DEV_REG_ADDR(IL_FLASH, asid - 1);
  flash_dev_addr->data0 = page_addr;

  unsigned status = 0;
  ssi_do_io_t do_io = {
      .commandAddr = &flash_dev_addr->command,
      .commandValue = (page_number << 8) | command,
  };
  ssi_payload_t payload = {
      .service_code = DOIO,
      .arg = &do_io,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
  return status;
}

unsigned readBackingStoreFromPage(memaddr missing_page_addr, unsigned asid,
                                  unsigned page_number) {
  return flashOperation(FLASHREAD, missing_page_addr, asid, page_number);
}

unsigned writeBackingStore(memaddr missing_page_addr, unsigned asid,
                           unsigned page_number) {
  return flashOperation(FLASHWRITE, missing_page_addr, asid, page_number);
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
