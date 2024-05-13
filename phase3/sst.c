#include "./headers/sst.h"
#include <uriscv/liburiscv.h>

pcb_PTR ith_sst_pcb;
pcb_PTR ith_sst_child;

void initSST(){
  //init of ith sst process
  ith_sst_pcb = allocPcb();
  RAMTOP(ith_sst_pcb->p_s.reg_sp);
  ith_sst_pcb->p_s.pc_epc = (memaddr)sstEntry;
  ith_sst_pcb->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
  ith_sst_pcb->p_s.mie = MIE_ALL;
  insertProcQ(&ready_queue_list, ith_sst_pcb);
  ith_sst_pcb->p_pid = sstPID;  
  //init of sst child
  ssi_payload_PTR ith_sst_child_payload;
  ith_sst_child_payload->service_code = CREATEPROCESS;
  ith_sst_child = (pcb_PTR)SYSCALL(SENDMSG, SSIPID, (unsigned)(&ith_sst_child_payload), 0);
  //same ASID!
  ith_sst_child->p_supportStruct->sup_asid = ith_sst_pcb->p_supportStruct->sup_asid;
  ith_sst_child->p_time;
  insertProcQ(&ready_queue_list, ith_sst_child);
}

void sstEntry(){
  // get the message from someone - user process
  // handle
  // responde
  while (TRUE) {
    ssi_payload_PTR process_request_payload;
    pcb_PTR process_request_ptr = (pcb_PTR) SYSCALL(RECEIVEMSG, ANYMESSAGE, (unsigned)(&process_request_payload), 0);
    sstRequestHandler(process_request_ptr, process_request_payload->service_code,process_request_payload->arg);
  }
}

void sstRequestHandler(pcb_PTR sender, int service, void* arg){
  unsigned *blank = 0, *response;
  switch (service){
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

    //TODO:

    /*The sender must wait an empty response from the SST signaling the completion of the write.*/
    break;
  case WRITETERMINAL:
    /* This service cause the print of a string of characters
     * to the terminal with the same number of the sender
     * ASID.
     */

    //TODO:
    
    /*The sender must wait an empty response from the SST signaling the completion of the write.*/
    break;      
  default:
    //error
    SYSCALL(SENDMSG, SSIPID, TERMINATE, 0);
    break;
  }
}

void getTOD(pcb_PTR sender){
  unsigned *response;
  cpu_t TOD;
  response = (unsigned) STCK(TOD);
  SYSCALL(SENDMSG, sender, (unsigned)(&response), 0);
}

void killSST(pcb_PTR sender){
  ssi_payload_PTR response_payload, input_payload;
  unsigned *response;

  input_payload->arg = NULL;
  input_payload->service_code = TERMPROCESS;
  SYSCALL(SENDMSG, SSIPID, (unsigned)(&input_payload), 0);
  response_payload->arg = (unsigned) "SST TERMINATED!\n";
  response = response_payload->arg;

  /*Send a message to the test process to communicate the termination of the SST.*/
  SYSCALL(SENDMSG, sender, (unsigned)(&response), 0);
}
