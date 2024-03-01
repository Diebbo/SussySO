/*Implement the System Service Interface (SSI) process. 
This involves handling various system service calls (SYS1, SYS3, SYS5, SYS6, SYS7, etc.).*/
#include <./headers/ssi.h>

void SSIRequest(pcb_t* sender, int service, void* arg){
    ssi_payload_PTR request;
    while(TRUE){
        //receive request
        SYSCALL(RECEIVEMESSAGE,sender->p_pid,service,arg); //?
        //satisfy request
    }

}