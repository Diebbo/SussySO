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
    // satysfy request and send back resoults(with a SYSYCALL in SSIRequest)
    unsigned *response = sstRequestHandler(process_request_ptr, process_request_payload->service_code,process_request_payload->arg);

    SYSCALL(SENDMSG, process_request_ptr, response, 0);
  }
}

void sstRequestHandler(){

}

