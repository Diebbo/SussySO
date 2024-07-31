#include "./headers/sst.h"

pcb_PTR sst_pcb[MAXSSTNUM];

void initSSTs() {
  // init of the 8 sst process
  for (int i = 0; i < MAXSSTNUM; i++) {
    sst_pcb[i] = allocPcb();
    RAMTOP(sst_pcb[i]->p_s.reg_sp);
    // Non mi interessa il pid sst_pcb[i]->p_pid = SSTPIDS - 10 + i;
    sst_pcb[i]->p_supportStruct->sup_asid = i;
    process_count++;
    sst_pcb[i]->p_s.pc_epc = (memaddr)sstEntry;
    sst_pcb[i]->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
    sst_pcb[i]->p_s.mie = MIE_ALL;
    insertProcQ(&ready_queue_list, sst_pcb[i]);
    // init the uProc (sst child)
    initUProc(sst_pcb[i]);
  }
}

void sstEntry() {
  // get the message from someone - user process
  // handle
  // reply
  while (TRUE) {
    ssi_payload_PTR process_request_payload;
    pcb_PTR process_request_ptr = (pcb_PTR)SYSCALL(
        RECEIVEMSG, ANYMESSAGE, (unsigned)(&process_request_payload), 0);
    sstRequestHandler(process_request_ptr,
                      process_request_payload->service_code,
                      process_request_payload->arg);
  }
}

void sstRequestHandler(pcb_PTR sender, int service, void *arg) {
  switch (service) {
  case GET_TOD:
    /* This service should allow the sender to get back the
     * number of microseconds since the system was last
     * booted/reset.
     */
    getTOD(sender);
    break;
  case TERMINATE:
    /* This service causes the sender U-proc and its SST
     * (its parent) to cease to exist. It is essentially a SST
     * “wrapper” for the SSI service TerminateProcess.
     * Remember to send a message to the test process to
     * communicate the termination of the SST.
     */
    killSST(sender);
    break;
  case WRITEPRINTER:
    /* This service cause the print of a string of characters
     * to the printer with the same number of the sender
     * ASID.
     */
    writeOnPrinter(sender, (ssi_payload_PTR)arg, IL_PRINTER);
    break;
  case WRITETERMINAL:
    /* This service cause the print of a string of characters
     * to the terminal with the same number of the sender
     * ASID.
     */
    writeOnTerminal(sender, (ssi_payload_PTR)arg, IL_TERMINAL);
    break;
  default:
    // error
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, TERMINATE, 0);
    break;
  }
}

void getTOD(pcb_PTR sender) {
  cpu_t tod_time;
  STCK(tod_time);
  SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned)(&tod_time), 0);
}

void killSST(pcb_PTR sender) {
  ssi_payload_t input_payload = {
      .service_code = TERMPROCESS,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned)(&input_payload), 0);

  /*Send a message to the test process to communicate the termination of the
   * SST.*/
  SYSCALL(SENDMSG, (unsigned int)sender, (unsigned)NULL, 0);
}

void writeOnPrinter(pcb_PTR sender, ssi_payload_PTR pcb_payload,
                    unsigned int i_line) {
  // obtain string and lenght of arg
  sst_print_PTR print_payload = (sst_print_PTR)pcb_payload;
  int lenght = (int)print_payload->length;
  char *string = (char *)print_payload->string;
  unsigned int IntlineNo = i_line;
  unsigned int DevNo = sender->p_supportStruct->sup_asid;
  // indexes to check
  int i = 0;
  char *msg = string;
  // obtain other info
  devreg_t *status = (devreg_t *)sender->p_supportStruct->sup_exceptState;
  devreg_t *devAddrBase = (devreg_t *)DEV_REG_ADDR(IntlineNo, DevNo);

  while (TRUE) {
    if ((*msg == EOS) || (i >= lenght)) {
      break;
    }

    //  device not ready nor busy -> error!
    if (((unsigned int)status & STATMASK) != (DEVRDY | DEVBSY)) {
      // there can be a better way to signal it
      PANIC();
    }

    unsigned int value = PRINTCHR | (((unsigned int)*msg) << 8);

    ssi_do_io_t do_io = {
        .commandAddr = &devAddrBase->dtp.data0,
        .commandValue = value,
    };
    ssi_payload_t payload = {
        .service_code = DOIO,
        .arg = &do_io,
    };

    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);

    // device not ready -> error!
    if (((unsigned int)status & STATMASK) != DEVRDY) {
      PANIC();
    }

    msg++;
    i++;
  }
  /*The sender must wait an empty response from the SST signaling the
   * completion of the write.*/
  SYSCALL(SENDMSG, (unsigned int)sender, 0, 0);
}

void writeOnTerminal(pcb_PTR sender, ssi_payload_PTR pcb_payload,
                     unsigned int i_line) {
  // obtain string and lenght of arg
  sst_print_PTR print_payload = (sst_print_PTR)pcb_payload;
  int lenght = (int)print_payload->length;
  char *string = (char *)print_payload->string;
  unsigned int IntlineNo = i_line;
  unsigned int DevNo = sender->p_supportStruct->sup_asid;
  // indexes to check
  int i = 0;
  char *msg = string;
  // obtain other info
  devreg_t *status = (devreg_t *)sender->p_supportStruct->sup_exceptState;
  devreg_t *devAddrBase = (devreg_t *)DEV_REG_ADDR(DevNo, IntlineNo);

  while (TRUE) {
    if ((*msg == EOS) || (i >= lenght)) {
      break;
    }

    //  device not ready nor busy -> error!
    if (((unsigned int)status & STATMASK) != (DEVRDY | DEVBSY)) {
      PANIC();
    }

    unsigned int value = PRINTCHR | (((unsigned int)*msg) << 8);

    ssi_do_io_t do_io = {
        .commandAddr = &devAddrBase->term.transm_command,
        .commandValue = value,
    };
    ssi_payload_t payload = {
        .service_code = DOIO,
        .arg = &do_io,
    };

    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);

    // error on reviving or transimtting char
    if ((((unsigned int)devAddrBase->term.recv_status & STATMASK) != RECVD) &&
        (((unsigned int)devAddrBase->term.transm_status & STATMASK) != RECVD)) {
      PANIC();
    }

    msg++;
    i++;
  }
  /*The sender must wait an empty response from the SST signaling the
   * completion of the write.*/
  SYSCALL(SENDMSG, (unsigned int)sender, 0, 0);
}
