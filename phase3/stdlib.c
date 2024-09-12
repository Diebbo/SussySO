#include "headers/stdlib.h"

extern memaddr current_stack_top;

// init and fill the support page table with the correct values
void initUprocPageTable(pteEntry_t *tbl, int asid) {
  for (int i = 0; i < MAXPAGES; i++) {
    tbl[i].pte_entryHI = KUSEG | (i << VPNSHIFT) | (asid << ASIDSHIFT);
    tbl[i].pte_entryLO = DIRTYON;
  }
  tbl[31].pte_entryHI = (0xbffff << VPNSHIFT) | (asid << ASIDSHIFT);
}

void initFreeStackTop(void) {
  RAMTOP(current_stack_top);
  current_stack_top -= 3 * PAGESIZE;
}

void defaultSupportData(support_t *support_data, int asid) {
  /*
   * Only the sup_asid, sup_exceptContext[2], and sup_privatePgTbl[32]
   * [Section 2.1] require initialization prior to request the CreateProcess
   * service. To initialize a processor context area one performs the following:
   *   • Set the two PC fields. One of them (0 - PGFAULTEXCEPT) should be set to
   * the address of the Support Level’s TLB handler, while the other one (1 -
   * GENERALEXCEPT) should be set to the address of the Support Level’s general
   * exception handler. • Set the two Status registers to: kernel-mode with all
   * interrupts and the Processor Local Timer enabled. • Set the two SP fields
   * to utilize the two stack spaces allocated in the Support Structure. Stacks
   *      grow “down” so set the SP fields to the address of the end of these
   * areas. E.g. ... = &(...sup_stackGen[499]).
   */
  support_data->sup_asid = asid;

  support_data->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr)pager;
  support_data->sup_exceptContext[PGFAULTEXCEPT].stackPtr =
      getCurrentFreeStackTop();
  support_data->sup_exceptContext[PGFAULTEXCEPT].status =
      MSTATUS_MIE_MASK | MSTATUS_MPP_M | MSTATUS_MPIE_MASK;

  support_data->sup_exceptContext[GENERALEXCEPT].pc =
      (memaddr)supportExceptionHandler;
  support_data->sup_exceptContext[GENERALEXCEPT].stackPtr =
      getCurrentFreeStackTop();
  support_data->sup_exceptContext[GENERALEXCEPT].status =
      MSTATUS_MIE_MASK | MSTATUS_MPIE_MASK | MSTATUS_MPP_M;

  initUprocPageTable(support_data->sup_privatePgTbl, asid);
}

memaddr getCurrentFreeStackTop(void) {
  unsigned tmp_stack_top = current_stack_top;
  current_stack_top -= PAGESIZE;
  return tmp_stack_top;
}

pcb_PTR initPrintProcess(state_t *print_state, support_t *sst_support) {
  return initHelper(print_state, sst_support, printEntry);
}

pcb_PTR initTermProcess(state_t *term_state, support_t *sst_support) {
  return initHelper(term_state, sst_support, termEntry);
}

pcb_PTR initHelper(state_t *helper_state, support_t *sst_support, void *entry) {
  STST(helper_state);
  helper_state->entry_hi = sst_support->sup_asid << ASIDSHIFT;
  helper_state->pc_epc = (memaddr)entry;
  helper_state->reg_sp = getCurrentFreeStackTop();
  helper_state->status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M | MSTATUS_MIE_MASK;
  helper_state->mie = MIE_ALL;

  return createChild(helper_state, sst_support);
}

void termEntry() {
  support_t *support = getSupportData();
  unsigned asid = support->sup_asid;

  while (TRUE) {
    sst_print_PTR print_payload;
    pcb_PTR sender =
        (pcb_PTR)SYSCALL(RECEIVEMESSAGE, (unsigned)sst_pcb[asid - 1],
                         (unsigned int)(&print_payload), 0);

    writeOnTerminal(print_payload, asid);

    // notify the sender that the print is done
    SYSCALL(SENDMESSAGE, (unsigned)sender, 0, 0);
  }
}

void printEntry() {
  support_t *support = getSupportData();
  unsigned asid = support->sup_asid;

  while (TRUE) {
    sst_print_PTR print_payload;
    pcb_PTR sender =
        (pcb_PTR)SYSCALL(RECEIVEMESSAGE, (unsigned)sst_pcb[asid - 1],
                         (unsigned int)(&print_payload), 0);

    writeOnPrinter(print_payload, asid);

    // notify the sender that the print is done
    SYSCALL(SENDMESSAGE, (unsigned)sender, 0, 0);
  }
}

void writeOnPrinter(sst_print_PTR arg, unsigned asid) {
  write(arg->string, arg->length,
        (devreg_t *)DEV_REG_ADDR(IL_PRINTER, asid - 1), PRINTER);
}

void writeOnTerminal(sst_print_PTR arg, unsigned asid) {
  write(arg->string, arg->length,
        (devreg_t *)DEV_REG_ADDR(IL_TERMINAL, asid - 1), TERMINAL);
}

void write(char *msg, int lenght, devreg_t *devAddrBase, enum writet write_to) {
  int i = 0;
  unsigned status;
  // check if it's a terminal or a printer
  unsigned *command = write_to == TERMINAL ? &(devAddrBase->term.transm_command)
                                           : &(devAddrBase->dtp.command);

  while (TRUE) {
    if ((*msg == EOS) || (i >= lenght)) {
      break;
    }

    unsigned int value;
    status = 0;

    if (write_to == TERMINAL) {
      value = PRINTCHR | (((unsigned int)*msg) << 8);
    } else {
      value = PRINTCHR;
      devAddrBase->dtp.data0 = *msg;
    }

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

    // device not ready -> error!
    if (write_to == TERMINAL && status != OKCHARTRANS) {
      terminateParent();
    } else if (write_to == PRINTER && status != DEVRDY) {
      terminateParent();
    }

    msg++;
    i++;
  }
}

void terminateParent(void) {
  ssi_payload_t term_payload = {
      .service_code = TERMPROCESS,
      .arg = (void *)NULL,
  };
  SYSCALL(SENDMSG, PARENT, (unsigned)&term_payload, 0);
  SYSCALL(RECEIVEMSG, PARENT, 0, 0);
}

// initialization of a single user process
pcb_PTR initUProc(state_t *u_proc_state, support_t *sst_support) {
  /*To launch a U-proc, one simply requests a CreateProcess to the SSI. The
   * ssi_create_process_t that two parameters: • A pointer to the initial
   * processor state for the U-proc. • A pointer to an initialized Support
   * Structure for the U-proc. Initial Processor State for a U-proc; Each
   * U-proc’s initial processor state should have its: • PC (and s_t9) set to
   * 0x8000.00B0; the address of the start of the .text section
   * [Section 10.3.1-pops]. • SP set to 0xC000.0000 [Section 2]. • Status set
   * for user-mode with all interrupts and the processor Local Timer enabled. •
   * EntryHi.ASID set to the process’s unique ID; an integer from [1..8]
   * Important: Each U-proc MUST be assigned a unique, non-zero ASID.
   */
  STST(u_proc_state);

  u_proc_state->entry_hi = sst_support->sup_asid << ASIDSHIFT;
  u_proc_state->pc_epc = (memaddr)UPROCSTARTADDR;
  u_proc_state->reg_sp = (memaddr)USERSTACKTOP;
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
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload),
          0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&support_data),
          0);
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
void gainSwapMutex() {
  SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex, 0, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_mutex, 0, 0);
}

// release mutual exclusion over the swap pool
void releaseSwapMutex() {
  SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex, 0, 0);
}

// check if is a SST pid
int isOneOfSSTPids(int pid) {
  return pid >= SSTPIDS && pid < SSTPIDS + MAXSSTNUM;
}

void terminateProcess(pcb_PTR arg) {
  ssi_payload_t term_process_payload = {
      .service_code = TERMPROCESS,
      .arg = (void *)arg,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&term_process_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

void notify(pcb_PTR process) {
  SYSCALL(SENDMESSAGE, (unsigned int)process, 0, 0);
}

void invalidateUProcPageTable(int asid) {
  gainSwapMutex();
  OFFINTERRUPTS();

  // invalidate the swap pool
  for (int i = 0; i < POOLSIZE; i++) {
    if (swap_pool[i].sw_asid == asid) {
      swap_pool[i].sw_asid = NOPROC;
    }
  }

  ONINTERRUPTS();
  releaseSwapMutex();
}

void updateTLB(pteEntry_t *page) {
  // place the new page in the Data0 register
  setENTRYHI(page->pte_entryHI);
  TLBP();
  // check if the page is already in the TLB
  unsigned not_present = getINDEX() & PRESENTFLAG;
  // if the variable is 1, the page is not in the TLB
  if (not_present == FALSE) {
    // the page is in the TLB, so we update it
    setENTRYHI(page->pte_entryHI);
    setENTRYLO(page->pte_entryLO);
    TLBWI();
  }
}
