#include "./headers/vmManager.h"

pcb_PTR swap_mutex;
int gained_mutex_asid = NOPROC; // debug purpose

/* Swap Pool (allows the system to extend its virtual memory beyond the physical RAM
by using the disk as a temporary holding place for data.)*/
swap_t swap_pool[POOLSIZE];

/**
 * From the μMPS3 documentation:
 * 
 *                    Entry High               |                  Entry Low          
 * +---------------------+---------------------+---------------------+---------------------+
 * |         VPN         |         ASID        |         PFN         |  N  |  D  | V |  G  |
 * +---------------------+---------------------+---------------------+---------------------+
 * 
 * VPN: Virtual Page Number. This is simply the higher order 20-bits of
 *      a logical address. The lower order 12-bits of the address are the offset into
 *      a 4 KB (212) page.
 * ASID: Address Space Identifier, a.k.a. process ID for this VPN.
 * PFN: Physical Frame Number, where the VPN for the specified ASID
 *      can be found in RAM.
 * N: Non-cacheable bit: Not used in μMPS3.
 * D: Dirty bit, This bit is used to implement memory protection mechanisms.
 *    When set to zero (off) a write access to a location in the physical frame will
 *    cause a TLB-Modification exception to be raised. This “write protection”
 *    bit allows for the realization of memory protection schemes and/or sophis-
 *    ticated page replacement algorithms.
 * V: Valid bit, If set to 1 (on), this TLB entry is considered valid. A valid
 *    entry is one where the PFN actually holds the ASID/virtual page number
 *    pairing. If 0 (off), the indicated ASID/virtual page number pairing is not
 *    actually in the PFN and any access to this page will cause a TLB-Invalid
 *    exception to be raised. In practical terms, a TLB-Invalid exception is what
 *    is generically called a “page fault.”
 * G: Global bit, If set to 1 (on), the TLB entry will match any ASID with the
 *    corresponding VPN. This bit allows for memory sharing schemes.
 * 
 */

void initSwapPool(void) {
  /**
   * Technical Point: Since all valid ASID values are positive numbers, one can indicate that a frame
   * is unoccupied with an entry of -1 in that frame’s ASID entry in the Swap Pool table.
   */
  for (int i = 0; i < POOLSIZE; i++) {
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

    pcb_PTR process = (pcb_PTR)process_requesting_swap;

    gained_mutex_asid = process->p_supportStruct->sup_asid;

    // giving the process the swap mutex
    SYSCALL(SENDMESSAGE, (unsigned int)process_requesting_swap, 0, 0);
    // wait for the process to finish the swap
    SYSCALL(RECEIVEMESSAGE, (unsigned int)process_requesting_swap, 0, 0);
    
    // released the swap mutex
    gained_mutex_asid = NOPROC;
  }
}

void pager(void) {
  unsigned status;
  // get the support data of the current process
  support_t *support_data = getSupportData();
  state_t *exception_state = &(support_data->sup_exceptState[PGFAULTEXCEPT]);
  unsigned cause = exception_state->cause & 0x7FFFFFFF;

  // check if the exception is a TLB-Modification exception
  if (cause == TLBMOD) {
    // treat this exception as a program trap
    programTrapExceptionHandler(support_data);
  }

  /**
   * ##################################
   * |   enter the critical section   |
   * ##################################
   */ 
  gainSwapMutex();

  // it's not the actual vpn, but the page index in the backing store
  int vpn = ENTRYHI_GET_VPN(exception_state->entry_hi);
  if (vpn >= MAXPAGES) {
    vpn = MAXPAGES - 1;
  }

  // pick a frame from the swap pool
  unsigned victim_frame = getFrameFromSwapPool();
  memaddr victim_page_addr = SWAPPOOLADDR + (victim_frame * PAGESIZE);

  // check if the frame is occupied
  if (isSwapPoolFrameFree(victim_frame) == FALSE) {
    /**
     *  we assume the frame is occupied by a dirty page
     */
  
    /** 
     * /----------------------------------\
     * | startregion of atomic operations |
     * \----------------------------------/
     */
    OFFINTERRUPTS();

    // mark the page pointed by the swap pool as not valid
    swap_pool[victim_frame].sw_pte->pte_entryLO &= ~VALIDON;

    // update the TLB if needed
    updateTLB(swap_pool[victim_frame].sw_pte);

    ONINTERRUPTS();
    /** 
     * /----------------------------------\
     * |  endregion of atomic operations  |
     * \----------------------------------/
     */

    // update the backing store
    status = writeBackingStore(victim_page_addr, swap_pool[victim_frame].sw_asid, swap_pool[victim_frame].sw_pageNo);
    if (status != DEVRDY) {
      programTrapExceptionHandler(support_data);
    }

  }

  // read the contents of the current process's backing store
  status =
      readBackingStoreFromPage(victim_page_addr, support_data->sup_asid, vpn);

  if (status != DEVRDY) // operation failed
  {
    programTrapExceptionHandler(support_data);
  }

  // update the swap pool table
  swap_pool[victim_frame].sw_asid = support_data->sup_asid;
  swap_pool[victim_frame].sw_pageNo = vpn;
  swap_pool[victim_frame].sw_pte = &support_data->sup_privatePgTbl[vpn];

  /** 
   * /----------------------------------\
   * | startregion of atomic operations |
   * \----------------------------------/
   */

  OFFINTERRUPTS();

  // update the current process's page table
  support_data->sup_privatePgTbl[vpn].pte_entryLO |= DIRTYON;
  support_data->sup_privatePgTbl[vpn].pte_entryLO |= VALIDON;
  support_data->sup_privatePgTbl[vpn].pte_entryLO &= 0xfff;
  support_data->sup_privatePgTbl[vpn].pte_entryLO |= victim_page_addr & 0xfffff000;


  // update the TLB
  updateTLB(&support_data->sup_privatePgTbl[vpn]);

  ONINTERRUPTS();
  /** 
   * /----------------------------------\
   * |  endregion of atomic operations  |
   * \----------------------------------/
   */

  releaseSwapMutex();
  /**
   * ##################################
   * |    exit the critical section   |
   * ##################################
   */

  // return control to the current process
  LDST(exception_state);
}

unsigned flashOperation(unsigned command, unsigned page_addr, unsigned asid, unsigned page_number) {
  dtpreg_t *flash_dev_addr = (dtpreg_t *)DEV_REG_ADDR(IL_FLASH, asid - 1);
  flash_dev_addr->data0 = page_addr;

  unsigned value = (page_number << 8) | command;
  unsigned status = 0;
  ssi_do_io_t do_io = {
    .commandAddr = &(flash_dev_addr->command),
    .commandValue = value,
  };
  ssi_payload_t payload = {
    .service_code = DOIO,
    .arg = &do_io,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
  return status;
}

unsigned readBackingStoreFromPage(memaddr missing_page_addr, unsigned asid, unsigned page_number) {
  return flashOperation(FLASHREAD, missing_page_addr, asid, page_number);
}

unsigned writeBackingStore(memaddr updating_page_addr, unsigned asid, unsigned page_number) {
  return flashOperation(FLASHWRITE, updating_page_addr, asid, page_number);
}

unsigned getFrameFromSwapPool() {
  static unsigned frame = 0;
  // find a free frame in the swap pool
  for (unsigned i = 0; i < POOLSIZE; i++) {
    if (isSwapPoolFrameFree(i)) {
      frame = i;
      break;
    }
  }
  // otherwise implement the page replacement algorithm RR
  return frame++ % POOLSIZE;
}
