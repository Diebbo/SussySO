#include "./headers/sst.h"

pcb_PTR ith_sst_pcb;
pcb_PTR ith_sst_child;

void initSST() {
  // init of ith sst process
  ith_sst_pcb = allocPcb();
  RAMTOP(ith_sst_pcb->p_s.reg_sp);
  ith_sst_pcb->p_s.pc_epc = (memaddr)sstEntry;
  ith_sst_pcb->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
  ith_sst_pcb->p_s.mie = MIE_ALL;
  insertProcQ(&ready_queue_list, ith_sst_pcb);
  ith_sst_pcb->p_pid = SSTPID;
  // init of sst child
  ssi_payload_PTR ith_sst_child_payload;
  ith_sst_child_payload->service_code = CREATEPROCESS;
  ith_sst_child = (pcb_PTR)SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned)(&ith_sst_child_payload), 0);
  // same ASID!
  ith_sst_child->p_supportStruct->sup_asid =
      ith_sst_pcb->p_supportStruct->sup_asid;
  ith_sst_child->p_time;
  insertProcQ(&ready_queue_list, ith_sst_child);
}

void sstEntry() {
  // get the message from someone - user process
  // handle
  // responde
  while (TRUE) {
    ssi_payload_PTR process_request_payload;
    pcb_PTR process_request_ptr = (pcb_PTR)SYSCALL(RECEIVEMSG, ANYMESSAGE, (unsigned)(&process_request_payload), 0);
    sstRequestHandler(process_request_ptr, process_request_payload->service_code, process_request_payload->arg);
  }
}

void sstRequestHandler(pcb_PTR sender, int service, void *arg) {
  unsigned *blank = 0, *response;
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
    writePrinter(sender,(ssi_payload_PTR)arg);
    
    break;
  case WRITETERMINAL:
    /* This service cause the print of a string of characters
     * to the terminal with the same number of the sender
     * ASID.
     */

    // TODO:

    /*The sender must wait an empty response from the SST signaling the
     * completion of the write.*/
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
  ssi_payload_PTR response_payload, input_payload;
  unsigned *response;

  input_payload->arg = NULL;
  input_payload->service_code = TERMPROCESS;
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned)(&input_payload), 0);
  response_payload->arg = (unsigned)"SST TERMINATED!\n";
  response = response_payload->arg;

  /*Send a message to the test process to communicate the termination of the
   * SST.*/
  SYSCALL(SENDMSG, (unsigned int)sender, (unsigned)(&response), 0);
}

void writePrinter(pcb_PTR sender, ssi_payload_PTR pcb_payload){
  //obtain string and lenght of arg
  sst_print_PTR print_payload  = (sst_print_PTR) pcb_payload;
  int lenght = (int) print_payload->length;
  char *string = (char*) print_payload->string;
  int i = 0;
  char *msg = string;
  //obtain other info
  devreg_t *status = (devreg_t*) sender->p_supportStruct->sup_exceptState;
  devreg_t *base = (devreg_t*) sender->p_supportStruct->sup_asid;          //COME OTTENGO base (o device in generle)??
  devreg_t *command = base + 3;

  while(TRUE){
    if((*msg == EOS) || (i >= lenght)){
      break;
    }
    unsigned int value = PRINTCHR | (((unsigned int)*msg) << 8);           //8?
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

    /*if (((unsigned int)status & TERMSTATMASK) != RECVD)                   //NECESSITO della mask dei printer!
        PANIC();
    */
    msg++;
    i++;
  }
  /*The sender must wait an empty response from the SST signaling the
   * completion of the write.*/
  SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
}