#include "./headers/sst.h"
#include "headers/stdlib.h"

pcb_PTR sst_pcb[MAXSSTNUM];
pcb_PTR child_pcb[MAXSSTNUM]; // debug purpose
memaddr current_stack_top;
state_t sst_st[MAXSSTNUM];
state_t u_proc_state[MAXSSTNUM];


state_t print_state[MAXSSTNUM];
state_t term_state[MAXSSTNUM];

pcb_PTR print_pcb[MAXSSTNUM];
pcb_PTR term_pcb[MAXSSTNUM];

void initSSTs() {
  // init of the 8 sst process
  for (int i = 0; i < MAXSSTNUM; i++) {
    sst_pcb[i] = initHelper(&sst_st[i], &support_arr[i], sstEntry);
  }
}

void sstEntry() {
  // init the child
  support_t *sst_support = getSupportData();

  child_pcb[sst_support->sup_asid - 1] =
      initUProc(&u_proc_state[sst_support->sup_asid - 1], sst_support);

  // init the print process
  print_pcb[sst_support->sup_asid - 1] =
      initPrintProcess(&print_state[sst_support->sup_asid - 1], sst_support);

  // init the term process
  term_pcb[sst_support->sup_asid - 1] =
      initTermProcess(&term_state[sst_support->sup_asid - 1], sst_support);

  // get the message from someone - user process
  // handle
  // reply
  while (TRUE) {
    ssi_payload_PTR process_request_payload;
    pcb_PTR process_request_ptr = (pcb_PTR)SYSCALL(
        RECEIVEMESSAGE, ANYMESSAGE, (unsigned)(&process_request_payload), 0);
    sstRequestHandler(
        process_request_ptr, process_request_payload->service_code,
        process_request_payload->arg, print_pcb[sst_support->sup_asid - 1],
        term_pcb[sst_support->sup_asid - 1]);
  }
}

void sstRequestHandler(pcb_PTR sender, int service, void *arg,
                       pcb_PTR print_process, pcb_PTR term_process) {
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
    print(service, (sst_print_PTR)arg, print_process);
    has_to_reply = TRUE;
    break;
  case WRITETERMINAL:
    /* This service cause the print of a string of characters
     * to the terminal with the same number of the sender
     * ASID.
     */
    print(service, (sst_print_PTR)arg, term_process);
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

void print(unsigned code, sst_print_PTR arg, pcb_PTR print_process) {
  // unwrap the arg and send it to the print process
  int length = arg->length;
  char string[length];
  for (int i = 0; i < length; i++) {
    string[i] = arg->string[i];
  }
  sst_print_t printing = {
      .string = string,
      .length = length,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)print_process, (unsigned int)&printing, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned)print_process, 0, 0);
}

cpu_t getTOD() {
  cpu_t tod_time;
  STCK(tod_time);
  return tod_time;
}

void killSST(int asid) {
  notify(test_process);

  // invalidate the page table
  invalidateUProcPageTable(asid);

  // kill the sst and its child
  terminateProcess(SELF);
}
