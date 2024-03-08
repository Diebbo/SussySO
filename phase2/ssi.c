/*Implement the System Service Interface (SSI) process. 
This involves handling various system service calls (SYS1, SYS3, SYS5, SYS6, SYS7, etc.).*/
#include "headers/ssi.h"

int generate_pid(){
    //49 = num max of pcb
    if(last_used_pid == 40){
        last_used_pid = 1;
    }else{
        last_used_pid++;
    }
    return last_used_pid + 1;
}

pcb_PTR find_process_ptr(struct list_head *target_process, int pid){
    pcb_PTR tmp;
    list_for_each_entry(tmp,target_process,p_list){
        if(tmp->p_pid == pid) return tmp;
    }
}

void SSI_function_entry_point(){
    pcb_PTR process_request_ptr;
    while (TRUE){
        //receive request
        int process_id_request = SYSCALL(RECEIVEMESSAGE,ssi_id,ANYMESSAGE,0);
        process_request_ptr = find_process_ptr(&ready_queue_list,process_id_request);                   //situato in ready queue?
        //satysfy request and send back resoults(with a SYSYCALL in SSIRequest)
        SSIRequest(process_request_ptr,process_request_ptr->p_s.reg_a2,process_request_ptr->p_s.reg_a3);
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
            arg = CreateProcess(sender,arg);                            //giusta fare una roba de genere per 2 tipi diversi di ritorno?
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
        //send back resoults
        SYSCALL(SENDMESSAGE,sender->p_pid,service,(unsigned)arg);
    }
}

pcb_PTR CreateProcess(pcb_t *sender, struct ssi_create_process_t *arg){
    /*When requested, this service causes a new process, said to be a progeny of the sender, to be created.
    arg should contain a pointer to a struct ssi_create_process_t (ssi_create_process_t *). Inside
    this struct, state should contain a pointer to a processor state (state_t *). This processor state is
    to be used as the initial state for the newly created process. The process requesting the this service
    continues to exist and to execute. If the new process cannot be created due to lack of resources (e.g.
    no more free PBCs), an error code of -1 (constant NOPROC) will be returned, otherwise, the pointer to
    the new PCB will be returned.*/
    pcb_t *new_prole = allocPcb();
    ssi_create_process_PTR new_prole_support = arg;
    if(process_count == MAXPROC || new_prole == NULL)
        return (pcb_PTR)NOPROC;
    else{
        //initialization of new prole
        RAMTOP(new_prole->p_s.reg_sp);
        new_prole->p_s.status = arg->state->status;
        STST(&new_prole->p_s);                      //loading arg state in new_prole p_s??
        if(arg == NULL)
            new_prole->p_supportStruct = NULL;
        else
            new_prole->p_supportStruct = arg->support;
        new_prole->p_time = 0;
        new_prole->p_pid = generate_pid();
        process_count++;
        insertProcQ(new_prole,&ready_queue_list);
        insertChild(sender,&new_prole);
        return new_prole;
    }
}

void terminate_process(pcb_t *sender, pcb_t *target){
    /*
    This services causes the sender process or another process to cease to exist [Section 11]. In addition,
    recursively, all progeny of that process are terminated as well. Execution of this instruction does not
    complete until all progeny are terminated.
    The mnemonic constant TERMINATEPROCESS has the value of 2.
    This service terminates the sender process if arg is NULL. Otherwise, arg should be a pcb_t
    pointer
    */
    if (target == NULL)
    {
        //terminate sender process but not the progeny!
        removeChild(sender->p_parent);
        removeProcQ(sender);
        //delete sender???
    }
    else if (sender == target->p_parent) {
        terminate_process(sender,container_of(sender->p_child.next,pcb_t, p_sib));
        removeChild(sender); 
        removeProcQ(target);
        //delete target???
    }
    else {
        //target is not a progeny of sender
        terminate_process(target,container_of(target->p_child.next,pcb_t,p_sib));
        removeChild(target->p_parent);//TODO: check if it's correct, serve che rimuova un figlio specifico
        removeProcQ(target);
        //delete target???
    }
}

void do_io(){

}
