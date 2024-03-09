/*Implement the System Service Interface (SSI) process.
This involves handling various system service calls (SYS1, SYS3, SYS5, SYS6,
SYS7, etc.).*/
#include "headers/ssi.h"

int generate_pid() {
  // 49 = num max of pcb
  if (last_used_pid == 40) {
    last_used_pid = 1;
  } else {
    last_used_pid++;
  }
  return last_used_pid + 1;
}

pcb_PTR find_process_ptr(struct list_head *target_process, int pid) {
  pcb_PTR tmp;
  list_for_each_entry(tmp, target_process, p_list) {
    if (tmp->p_pid == pid)
      return tmp;
  }
}

void SSI_function_entry_point() {
  pcb_PTR process_request_ptr;
  while (TRUE) {
    // receive request
    int process_id_request = SYSCALL(RECEIVEMESSAGE, ssi_id, ANYMESSAGE, 0);
    process_request_ptr = find_process_ptr(&ready_queue_list, process_id_request); // situato in ready queue?
    // satysfy request and send back resoults(with a SYSYCALL in SSIRequest)
    SSI_Request(process_request_ptr, process_request_ptr->p_s.reg_a2, (void *)process_request_ptr->p_s.reg_a3);
  }
}

void SSI_Request(pcb_t *sender, int service, void *arg) {
  /*If service does not match any of those provided by the SSI, the SSI should
  terminate the process and its progeny. Also, when a process requires a service
  to the SSI, it must wait for the answer.*/

  // finding if in user or kernel mode
  state_t *exception_state = (state_t *)BIOSDATAPAGE;
  int user_state = exception_state->status;

  if (user_state != 1) {
    // Must be in kernel mode otherwise trap!
    TrapExceptionHandler();
  } else {
    switch (service) {
    case CREATEPROCESS:
      arg = Create_Process(sender, arg); // giusta fare una roba de genere per 2 tipi diversi di ritorno?
      break;
    case TERMPROCESS:
      terminate_process(sender,sender); //Vitto devi inserire tu cosa ci devi immettere nella funzione, non so se sia corretto     
      break;
    case DOIO:
      do_io((ssi_payload_t *)arg);
      break;
    case GETTIME:
      arg = Get_CPU_Time(sender);
      break;
    case CLOCKWAIT:
      Wait_For_Clock(sender);
      break;
    case GETSUPPORTPTR:

      break;
    case GETPROCESSID:

      break;
    default:
      // no match with services so must end process and progeny

      break;
    }
    // send back resoults
    SYSCALL(SENDMESSAGE, sender->p_pid, service, (unsigned)arg);
  }
}

pcb_PTR Create_Process(pcb_t *sender, struct ssi_create_process_t *arg) {
  /*When requested, this service causes a new process, said to be a progeny of
  the sender, to be created. arg should contain a pointer to a struct
  ssi_create_process_t (ssi_create_process_t *). Inside this struct, state
  should contain a pointer to a processor state (state_t *). This processor
  state is to be used as the initial state for the newly created process. The
  process requesting the this service continues to exist and to execute. If the
  new process cannot be created due to lack of resources (e.g. no more free
  PBCs), an error code of -1 (constant NOPROC) will be returned, otherwise, the
  pointer to the new PCB will be returned.*/
  pcb_t *new_prole = allocPcb();
  ssi_create_process_PTR new_prole_support = arg;
  if (process_count == MAXPROC || new_prole == NULL)
    return (pcb_PTR)NOPROC;
  else {
    // initialization of new prole
    RAMTOP(new_prole->p_s.reg_sp);
    new_prole->p_s.status = arg->state->status;
    STST(&new_prole->p_s); // loading arg state in new_prole p_s??
    if (arg == NULL)
      new_prole->p_supportStruct = NULL;
    else
      new_prole->p_supportStruct = arg->support;
    new_prole->p_time = 0;
    new_prole->p_pid = generate_pid();
    process_count++;
    insertProcQ(new_prole, &ready_queue_list);
    insertChild(sender, &new_prole);
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

/*typedef struct ssi_payload_t
{
    int service_code;
    void *arg;
} ssi_payload_t, *ssi_payload_PTR;


typedef struct ssi_do_io_t
{
    memaddr* commandAddr;
    unsigned int commandValue; <----
} ssi_do_io_t, *ssi_do_io_PTR;
*/
void do_io(ssi_payload_PTR payload){
    // 1. device address -> save pcb_t on the device
    // 2. perform *commandAddr = commandValue; 
    // 3. -> rise an interrupt exception from device

    // during this phase only the print process will request the do_io service:

}

cpu_t* Get_CPU_Time(pcb_t* sender){
  /*This service should allow the sender to get back the accumulated processor time (in µseconds) used
  by the sender process. Hence, the Nucleus records (in the PCB: p_time) the amount of processor time
  used by each process*/
  return sender->p_time;
}

void Wait_For_Clock(pcb_t* sender){
  /*One of the services the nucleus has to implement is the pseudo-clock, that is, a virtual device which
  sends out an interrupt (a tick) every 100 milliseconds (constant PSECOND). This interrupt will be
  translated into a message to the SSI, as for other interrupts.
  This service should allow the sender to suspend its execution until the next pseudo-clock tick. You
  need to save the list of PCBs waiting for the tick.*/
  int tick = PSECOND;
  cpu_t current_time;
  cpu_t last_time;
  //macro to read TimeOfDay clock
  STCK(last_time);
  STCK(current_time);
  //send interrupt
  SYSCALL(SENDMESSAGE,ssi_id,sender->p_pid,sender->p_s.reg_a3); //3o argomento corretto?
  //saving proc waiting for tick
  insertProcQ(&pseudoClockList,sender);
  while ((current_time-last_time)%tick == 0)
  {
    //refresh TOD time till tick passed
    STCK(current_time);
  }
  //removing from clock list cause tick passed
  removeProcQ(&pseudoClockList);
}

