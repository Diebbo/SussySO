#include "./headers/sysSupport.h"

extern pcb_PTR current_process;

void supportExceptionHandler(){
    /*Assuming that the handling of the exception is to be passed up (non-NULL Support Structure
     *pointer) and the appropriate sup_exceptContext fields of the Support Structure were correctly ini-
     *tialized, execution continues with the Support Level’s general exception handler, which should then
     *pass control to the Support Level’s SYSCALL exception handler. The processor state at the time of
     *the exception will be in the Support Structure’s corresponding sup_exceptState field.
     */
    support_t* current_support = current_process->p_supportStruct;
    unsigned int context = current_support->sup_exceptContext->status;
    unsigned int status = current_support->sup_exceptState->cause;
    state_t *exception_state = (state_t *)BIOSDATAPAGE;
    unsigned exception_code =  status & 0x7FFFFFFF;

    if (exception_code >= 24 && exception_code <= 28) {
        TLBhandler();
    }else if ((exception_code >= 0 && exception_code <= 7) ||
             (exception_code >= 12 && exception_code <= 23)){
        UsysCallHandler(exception_state);
    }
}

