#include "./headers/sst.h"

pcb_PTR sst_pcb[MAXSSTNUM];
pcb_PTR child_pcb[MAXSSTNUM]; // debug purpose
memaddr current_stack_top;
state_t sst_st[MAXSSTNUM];
state_t u_proc_state[MAXSSTNUM];
state_t print_state[MAXSSTNUM];

void initSSTs() {
  // init of the 8 sst process
  for (int i = 0; i < 1; i++) {
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

  // init the print process
  print_pcb[sst_support->sup_asid - 1] = initPrintProcess(&print_state[sst_support->sup_asid - 1], sst_support);

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
    killSST(sender->p_supportStruct->sup_asid);
    break;
  case WRITEPRINTER:
    /* This service cause the print of a string of characters
     * to the printer with the same number of the sender
     * ASID.
     */
  case WRITETERMINAL:
    /* This service cause the print of a string of characters
     * to the terminal with the same number of the sender
     * ASID.
     */
    ssi_payload_t print_payload = {service, arg};
    SYSCALL(SENDMESSAGE,
            (unsigned int)print_pcb[sender->p_supportStruct->sup_asid - 1],
            (unsigned int)&print_payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned)print_pcb[sender->p_supportStruct->sup_asid - 1],
            0, 0);
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

void killSST(int asid) {
  notify(test_process);

  // invalidate the page table
  invalidateUProcPageTable(sst_pcb[asid]->p_supportStruct);

  // kill the sst and its child
  terminateProcess(SELF);
}
