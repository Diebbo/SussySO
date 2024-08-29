#include "./headers/sst.h"

pcb_PTR sst_pcb[MAXSSTNUM];
pcb_PTR child_pcb[MAXSSTNUM]; // debug purpose
memaddr current_stack_top;
state_t sst_st[MAXSSTNUM];
state_t u_proc_state[MAXSSTNUM];

void initSSTs() {
  // init of the 8 sst process
  // for (int i = 0; i < MAXSSTNUM; i++) {
  for (int i = 0; i < 8; i++) {
    STST(&sst_st[i]);
    sst_st[i].entry_hi = (i + 1) << ASIDSHIFT;
    sst_st[i].reg_sp = getCurrentFreeStackTop();
    sst_st[i].pc_epc = (memaddr)sstEntry;
    sst_st[i].status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M | MSTATUS_MIE_MASK;
    sst_st[i].mie = MIE_ALL;
    sst_pcb[i] = createChild(&sst_st[i], &support_arr[i]);
    // init the uProc (sst child)
  }
}

void sstEntry() {
  // init the child
  support_t *sst_support = getSupportData();

  child_pcb[sst_support->sup_asid - 1] =
      initUProc(&u_proc_state[sst_support->sup_asid - 1], sst_support);
  // get the message from someone - user process
  // handle
  // reply
  while (TRUE) {
    ssi_payload_PTR process_request_payload;
    pcb_PTR process_request_ptr = (pcb_PTR)SYSCALL(
        RECEIVEMESSAGE, ANYMESSAGE, (unsigned)(&process_request_payload), 0);
    sstRequestHandler(process_request_ptr,
                      process_request_payload->service_code,
                      process_request_payload->arg);
  }
}

void sstRequestHandler(pcb_PTR sender, int service, void *arg) {
  void *res_payload = NULL;
  unsigned has_to_reply = FALSE;
  switch (service) {
  case GET_TOD:
    /* This service should allow the sender to get back the
     * number of microseconds since the system was last
     * booted/reset.
     */
    res_payload = (void *)getTOD();
    has_to_reply = TRUE;
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
    writeOnPrinter((sst_print_PTR)arg, sender->p_supportStruct->sup_asid);
    has_to_reply = TRUE;
    break;
  case WRITETERMINAL:
    /* This service cause the print of a string of characters
     * to the terminal with the same number of the sender
     * ASID.
     */
    writeOnTerminal((sst_print_PTR)arg, sender->p_supportStruct->sup_asid);
    has_to_reply = TRUE;
    break;
  default:
    // error
    terminateProcess(SELF); // terminate the SST and child
    break;
  }

  // ack the sender
  if (has_to_reply) {
    SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)res_payload, 0);
  }
}

cpu_t getTOD() {
  cpu_t tod_time;
  STCK(tod_time);
  return tod_time;
}

void killSST(pcb_PTR sender) {
  if (sender != NULL) {
    // terminate the sender
    terminateProcess(sender);
  }
  notify(test_process);

  terminateProcess(SELF);
}

void writeOnPrinter(sst_print_PTR arg, unsigned asid) {
  // write the string on the printer
  write(arg->string, arg->length, (devreg_t *)DEV_REG_ADDR(IL_PRINTER, asid-1),
        PRINTER);
}

void writeOnTerminal(sst_print_PTR arg, unsigned int asid) {
  // write the string on t RECEIVEMSG, he printer
  write(arg->string, arg->length, (devreg_t *)DEV_REG_ADDR(IL_TERMINAL, asid-1),
        TERMINAL);
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
      programTrapExceptionHandler(&(ssi_pcb->p_supportStruct->sup_exceptState[GENERALEXCEPT]));
    } else if (write_to == PRINTER && status != DEVRDY) {
      programTrapExceptionHandler(&(ssi_pcb->p_supportStruct->sup_exceptState[GENERALEXCEPT]));
    }

    msg++;
    i++;
  }
}
