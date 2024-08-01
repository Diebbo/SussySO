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
    writeOnPrinter(sender, (sst_print_PTR)arg, sender->p_supportStruct->sup_asid);
    break;
  case WRITETERMINAL:
    /* This service cause the print of a string of characters
     * to the terminal with the same number of the sender
     * ASID.
     */
    writeOnTerminal(sender, (sst_print_PTR)arg, sender->p_supportStruct->sup_asid);
    break;
  default:
    // error
    // what the hell !!! SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, TERMINATE,
    // 0);
    PANIC();
    break;
  }
}

void getTOD(pcb_PTR sender) {
  cpu_t tod_time;
  STCK(tod_time);
  SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned)(&tod_time), 0);
}

void killSST(pcb_PTR sender) {
  terminateProcess(sender);
  terminateProcess(SELF);
  // do i need to notify the test process? 
}

void writeOnPrinter(pcb_PTR sender, sst_print_PTR arg, unsigned asid) {
  // write the string on the printer
  write(arg->string, arg->length, (devreg_t *)DEV_REG_ADDR(IL_PRINTER, asid));

  // ack the sender
  SYSCALL(SENDMSG, (unsigned int)sender, 0, 0);
}

void writeOnTerminal(pcb_PTR sender, sst_print_PTR arg, unsigned int asid) {
  // write the string on the printer
  write(arg->string, arg->length, (devreg_t *)DEV_REG_ADDR(IL_TERMINAL, asid));

  // ack the sender
  SYSCALL(SENDMSG, (unsigned int)sender, 0, 0);
}

void write(char *msg, int lenght, devreg_t *devAddrBase) {
  int i = 0;
  unsigned status;

  while (TRUE) {
    if ((*msg == EOS) || (i >= lenght)) {
      break;
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
    if (status != OKCHARTRANS) {
      PANIC();
    }

    msg++;
    i++;
  }
}

