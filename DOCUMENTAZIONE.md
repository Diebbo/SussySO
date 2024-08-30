# Documentation for Operating System Project - A.Y. 2023/2024
## Introduction

## Index

- [Virtual Memory Support](#virtual-memory-support)


## Virtual Memory Support
### Pager
The pager is a module that is responsible for managing the virtual memory of the system. It is responsible for loading and unloading pages from the disk to the memory, using the swap pool as a support struct for handling pages.

```c
void pager(void) {
  unsigned status;
  // get the support data of the current process
  support_t *support_data = getSupportData();

  state_t *exception_state = &(support_data->sup_exceptState[PGFAULTEXCEPT]);

  unsigned cause = exception_state->cause & 0x7FFFFFFF;

  // check if the exception is a TLB-Modification exception
  if (cause == TLBMOD) {
    // treat this exception as a program trap
    programTrapExceptionHandler(exception_state);
  }

  gainSwapMutex();

  /* enter the critical section */

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
    // we assume the frame is occupied by a dirty page

    // operations performed atomically
    OFFINTERRUPTS();

    // mark the page pointed by the swap pool as not valid
    swap_pool[victim_frame].sw_pte->pte_entryLO &= !VALIDON;

    // update the TLB if needed
    updateTLB(swap_pool[victim_frame].sw_pte);

    ONINTERRUPTS();

    // update the backing store
    status = writeBackingStore(victim_page_addr, swap_pool[victim_frame].sw_asid,
                               swap_pool[victim_frame].sw_pageNo);
    if (status != DEVRDY) {
      programTrapExceptionHandler(exception_state);
    }

  }

  // read the contents of the current process's backing store
  status =
      readBackingStoreFromPage(victim_page_addr, support_data->sup_asid, vpn);

  if (status != DEVRDY) // operation failed
  {
    programTrapExceptionHandler(exception_state);
  }

  // update the swap pool table
  swap_pool[victim_frame].sw_asid = support_data->sup_asid;
  swap_pool[victim_frame].sw_pageNo = vpn;
  swap_pool[victim_frame].sw_pte = &support_data->sup_privatePgTbl[vpn];

  // #region atomic operations
  OFFINTERRUPTS();

  // update the current process's page table
  support_data->sup_privatePgTbl[vpn].pte_entryLO |= DIRTYON;
  support_data->sup_privatePgTbl[vpn].pte_entryLO |= VALIDON;
  support_data->sup_privatePgTbl[vpn].pte_entryLO &= 0xfff;
  support_data->sup_privatePgTbl[vpn].pte_entryLO |= victim_page_addr & 0xfffff000;


  // update the TLB
  updateTLB(&support_data->sup_privatePgTbl[vpn]);

  ONINTERRUPTS();
  // #endregion atomic operations

  releaseSwapMutex();
  /* exit the critical section */

  // return control to the current process
  LDST(exception_state);
}

```

The pager is responsible for handling page faults. When a page fault occurs, the pager is invoked and it is responsible for loading the page from the backing store to the memory. The pager is also responsible for updating the TLB and the page table of the current process.
It's important to ensure that the operations performed by the pager are atomic, so we disable the interrupts before performing the operations and re-enable them after the operations are completed (see the `OFFINTERRUPTS` and `ONINTERRUPTS` macros).
```c
#define OFFINTERRUPTS() setSTATUS(getSTATUS() & (~MSTATUS_MIE_MASK))
#define ONINTERRUPTS() setSTATUS(getSTATUS() | MSTATUS_MIE_MASK)
```
We also need to ensure that the operations performed by the pager are thread-safe, so we use a mutex to ensure that only one pager can run at a time: that's accomplished by calling the `gainSwapMutex` and `releaseSwapMutex` functions, which acquire and release the mutex usign message passing.

### Swap Pool
The swap pool is a data structure that is used to store the pages that are swapped out of the memory. It is used by the pager to store the pages that are evicted from the memory when a page fault occurs. The swap pool is implemented as an array of swap pool entries, where each entry contains the address of the page in the backing store, the ASID of the process that owns the page, and the page number of the page in the backing store.

```c
typedef struct swap_t
{
    int sw_asid;        /* ASID number			*/
    int sw_pageNo;      /* page's virt page no.	*/
    pteEntry_t *sw_pte; /* page's PTE entry.	*/
} swap_t;
```

### Page Tables
Are part of the process control block (support struct) and are used to store the mapping between the virtual pages and the physical frames. The page table is implemented as an array of page table entries, where each entry contains the physical frame number, the ASID of the process that owns the page and the process register entriHI and entryLO.

```c
typedef struct pteEntry_t
{
    unsigned int pte_entryHI;
    unsigned int pte_entryLO;
} pteEntry_t;
```

### Page Replacement Algorithms
The pager uses a page replacement algorithm to decide which page to evict from the memory when a page fault occurs. The algorithm looks for a free frame in the swap pool and if there are no free frames, it selects one in a round-robin fashion. The algorithm is implemented in the `getFrameFromSwapPool` function. 
```c
unsigned getFrameFromSwapPool() {
  static unsigned frame = 0;
  // find a free frame in the swap pool
  for (unsigned i = 0; i < POOLSIZE; i++) {
    if (isSwapPoolFrameFree(i)) {
      frame = i;
      break;
    }
  }
  // otherwise implement the page replacement algorithm FIFO
  return frame++ % POOLSIZE;
}
```
### Read/Write to Backing Store
We use the `readBackingStoreFromPage` and `writeBackingStore` functions to read and write to the backing store. The backing store is a disk that is used to store the pages that are swapped out of the memory. The backing store is implemented as a file that is stored on the disk. We use the ssi apis to read and write to the backing store using the mnemonics `DOIO` operation. Is important to note which how this operation is performed:
1. We set the page address in the flash device register
2. We set the command value in the flash device register
3. We send a message to the ssi device to perform the operation
4. We receive the status of the operation

```c
unsigned flashOperation(unsigned command, unsigned page_addr, unsigned asid,
                        unsigned page_number) {
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
```
The function two functions:
- `readBackingStoreFromPage`
- `writeBackingStore`

act as a wrapper around the `flashOperation` function, they are used to read and write to the backing store.

### TLB Update
This is one of the most important parts of the pager, the TLB is a cache that is used to store the mappings between the virtual pages and the physical frames. As the TLB is implemented as an array of TLB entries, we can access it using built in macros. The *TLB update* is by performing a check if the page is already in the TLB, if it is, we update the entry, otherwise we add a new entry to the TLB. 
```c
void updateTLB(pteEntry_t *page) {
  // place the new page in the Data0 register
  setENTRYHI(page->pte_entryHI);
  TLBP();
  // check if the page is already in the TLB
  unsigned is_present = getINDEX() & PRESENTFLAG;
  if (is_present == FALSE) {
    // the page is not in the TLB, so we need to insert it
    setENTRYHI(page->pte_entryHI);
    setENTRYLO(page->pte_entryLO);
    TLBWI();
  }
}
```

