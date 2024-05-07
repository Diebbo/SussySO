#include "./headers/sst.h"
#include <uriscv/liburiscv.h>

pcb_PTR sst_pcb;

void initSST(){
  // TODO:
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
  //ask service to ssi
  SYSCALL(SENDMSG, SSIPID, service, 0);
  unsigned *response = SYSCALL(RECEIVEMESSAGE, SSIPID, 0, 0);
  unsigned *blank = 0;
  switch (service){
  case GET_TOD:
    //sends back micro seconds received
    SYSCALL(SENDMSG, sender, response, 0);
    break;
  case TERMINATE:
    /*Send a message to the test process to communicate the termination of the SST.*/
    //SYSCALL(SENDMESSAGE, tester, blank, 0);
    break;
  case WRITEPRINTER:
    /*The sender must wait an empty response from the SST signaling the completion of the write.*/
    SYSCALL(SENDMSG, sender, blank, 0);
    break;
  case WRITETERMINAL:
    /*The sender must wait an empty response from the SST signaling the completion of the write.*/
    SYSCALL(SENDMSG, sender, blank, 0);
    break;      
  default:
    //error
    SYSCALL(SENDMSG, SSIPID, TERMINATE, 0);
    break;
  }
}

