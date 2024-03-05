/*Implement the System Service Interface (SSI) process. 
This involves handling various system service calls (SYS1, SYS3, SYS5, SYS6, SYS7, etc.).*/
#include <./headers/ssi.h>
#include <./headers/exceptions.h>
ssi_payload_PTR ssi_p;                                      //probabile da buttare
int ssi_id = magic_number; //SSI id                         //TODO

void SSI_function_entry_point(){                            //da sistemare ma strutt logica più o meno corretta
    state_t *p_state = (state_t *)BIOSDATAPAGE;
    int ssi_service = ssi_p->service_code;
    void *ssi_arg = ssi_p->arg;
    while (TRUE){
        //receive request
        int process_id_request = SYSCALL(RECEIVEMESSAGE,ssi_id,ssi_service,ssi_arg);
        SSIRequest(current_process,ssi_service,ssi_arg);
        //satysfy request and send back resoults
        SYSCALL(SENDMESSAGE,process_id_request,ssi_service,ssi_arg);

    }
}

void SSIRequest(pcb_t* sender, int service, void* arg){
    /*If service does not match any of those provided by the SSI, the SSI should terminate the process
    and its progeny. Also, when a process requires a service to the SSI, it must wait for the answer.*/

    // finding if in user or kernel mode
	state_t *exception_state = (state_t *)BIOSDATAPAGE;
	int user_state = exception_state->status;

    if(user_state != 1){
        //Must be in kernel mode otherwise trap!
    	TrapExceptionHandler();
    }else{
        switch (service)
        {
        case CREATEPROCESS:
            
            break;
        case TERMPROCESS:

            break;
        case DOIO:

            break;
        case GETTIME:

            break;
        case CLOCKWAIT:

            break;
        case GETSUPPORTPTR:

            break;
        case GETPROCESSID:

            break;
        default:
            //no match with services so must end process and progeny
            
            break;
        }
    }




}