#include "./headers/ssi.h"

#define BIT_CHECKER(n, bit) (((n) >> (bit)) & 1)

void SSI_function_entry_point() {
  while (TRUE) {
    // find msg payload in modo sensato magari
    ssi_payload_PTR process_request_payload;

    pcb_PTR process_request_ptr = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned)(&process_request_payload), 0);

    // satysfy request and send back resoults(with a SYSYCALL in SSIRequest)
    SSI_Request(process_request_ptr, process_request_payload->service_code,
                process_request_payload->arg);
  }
}

void SSI_Request(pcb_PTR sender, int service, void *arg) {
  /*If service does not match any of those provided by the SSI, the SSI should
  terminate the process and its progeny. Also, when a process requires a service
  to the SSI, it must wait for the answer.*/

  // finding if in user or kernel mode
  // state_t *exception_state = (state_t *)BIOSDATAPAGE;
  // int user_state = exception_state->status;
  int is_user_mode = BIT_CHECKER(getSTATUS(), 1);
  void *syscall_response_arg = NULL;

  if (is_user_mode) {
    // Must be in kernel mode otherwise trap!
    // sbagliatissimo !!! TrapExceptionHandler(); -> cerco di uccidere l'ssi
    Terminate_Process(ssi_pcb, sender);
  } else {
    switch (service) {
    case CREATEPROCESS:
      syscall_response_arg = Create_Process(sender, (ssi_create_process_t *)arg); // giusta fare una roba de genere per 2
      break;
    case TERMPROCESS:
      Terminate_Process(sender, (pcb_t *)arg);
      // no reponse otherwise exception cannot find it 
      syscall_response_arg = (void *)NORESPONSE;  
      break;
    case DOIO:
      DoIO(sender, (ssi_do_io_PTR)arg);\
      syscall_response_arg = (void *)NORESPONSE;
      break;
    case GETTIME:
      syscall_response_arg = (void *)Get_CPU_Time(sender);
      break;
    case CLOCKWAIT:
      Wait_For_Clock(sender);
      syscall_response_arg = (void *)NORESPONSE;
      break;
    case GETSUPPORTPTR:
      syscall_response_arg = Get_Support_Data(sender);
      break;
    case GETPROCESSID:
      syscall_response_arg = (void *)Get_Process_ID(sender, (int)arg);
      break;
    default:
      // no match with services so must end process and progeny
      killProgeny(sender);
      syscall_response_arg = (void *)NORESPONSE;
      break;
    }
    // send back resoults - or just need to unblock sender
    if((unsigned)syscall_response_arg != NORESPONSE)
      SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned)(syscall_response_arg == NULL ? 0 : syscall_response_arg), 0);
    
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
  // ssi_create_process_PTR new_prole_support = arg;
  if (new_prole == NULL) // no more free PBCs
    return (pcb_PTR)NOPROC;
  else {
    // initialization of new prole
    copyState(arg->state,&new_prole->p_s);//non mi avete cagato e allora io inverto i parametri.
    new_prole->p_supportStruct = arg->support; // even if optional -> will be null
    new_prole->p_time = 0;
    process_count++;
    insertProcQ(&ready_queue_list, new_prole);
    insertChild(sender, new_prole);
    return new_prole;
  }
}

void Terminate_Process(pcb_t *sender, pcb_t *target) {
  /*
  This services causes the sender process or another process to cease to exist
  [Section 11]. In addition, recursively, all progeny of that process are
  terminated as well. Execution of this instruction does not complete until all
  progeny are terminated. The mnemonic constant TERMINATEPROCESS has the value
  of 2. This service terminates the sender process if arg is NULL. Otherwise,
  arg should be a pcb_t pointer
  */
  if (target == NULL) {
    outChild(sender);
    killProgeny(sender);
  } else {
    outChild(target);
    killProgeny(target);
  }
}

void *DoIO(pcb_t *sender, ssi_do_io_PTR arg) {
  /*Here is the step by step execution of the kernel when a generic DoIO is
    requested: • A process sends a request to the SSI to perform a DoIO; • the
    process will wait for a response from the SSI; • the SSI will eventually
    execute on the CPU to perform the DoIO; • given the device address, the SSI
    should save the waiting pcb_t on the corresponding device; • at last, the
    SSI will write the requested value on the device, i.e. *commandAddr =
    commandValue; Important: This call should rise an interrupt exception from a
    device; • the SSI will continue its execution until it blocks again; • every
    process should now be in a blocked state as everyone is waiting for a task
    to end, so the the scheduler should call the WAIT() function; Important: The
    current process must be set to NULL and all interrupts must be ON; • an
    interrupt exception should be raised by the CPU; • given the cause code, the
    interrupt handler should understand which device triggered the TRAP; • check
    the status and send to the device an acknowledge (setting the device command
    address to ACK); • the device sends to the SSI a message with the status of
    the device operation, i.e. setting the a3 parameter with the device address;
    now if the current process is NULL, return the control flow to the
    scheduler. Otherwise, load the current process state into the CPU; • the
    kernel will execute the send, that will free the SSI process, that will
    elaborate the request from the device; • given the device address, the SSI
    should free the process waiting the completion on the DoIO and finally,
    forwarding the status message to the original process.*/
  // unsigned dev_no = 7; // da specifiche per terminale
  unsigned dev_line = IL_TERMINAL - IL_OFFSET; // da specifiche per terminale  
  unsigned dev_no = (unsigned)arg->commandAddr - 0x10000054 - ((dev_line-3) * 0x80);
  dev_no = dev_no / 0x10;

  unsigned dev_index = (dev_line - 3) * 8 + dev_no;

  pcb_PTR blocked_for_message = outProcQ(&msg_queue_list, sender);
  
  if (blocked_for_message == NULL) {
    // no message to send
    return NULL;
  } 

  insertProcQ(&blockedPCBs[dev_index], sender);

  soft_block_count++;

  *arg->commandAddr = arg->commandValue;
 // !this should rise an interrupt exception from a device

  return NULL;
}

cpu_t Get_CPU_Time(pcb_t *sender) {
  /*This service should allow the sender to get back the accumulated processor
  time (in µseconds) used by the sender process. Hence, the Nucleus records (in
  the PCB: p_time) the amount of processor time used by each process*/
  return sender->p_time;
}

void Wait_For_Clock(pcb_t *sender) {
  /*One of the services the nucleus has to implement is the pseudo-clock, that
  is, a virtual device which sends out an interrupt (a tick) every 100
  milliseconds (constant PSECOND). This interrupt will be translated into a
  message to the SSI, as for other interrupts. This service should allow the
  sender to suspend its execution until the next pseudo-clock tick. You need to
  save the list of PCBs waiting for the tick.*/
  // il processo si trova ad aspettare risposta dal pseudo clock
  outProcQ(&msg_queue_list, sender); 
  // saving proc waiting for tick
  insertProcQ(&pseudoClockList, sender);
  soft_block_count++;
}

support_t *Get_Support_Data(pcb_t *sender) {
  /*This service should allow the sender to obtain the process’s Support
  Structure. Hence, this service returns the value of p_supportStruct from the
  sender process’s PCB. If no value for p_supportStruct was provided for the
  sender process when it was created, return NULL.*/
  if (sender == NULL)
    return NULL;
  else
    return sender->p_supportStruct;
}

int Get_Process_ID(pcb_t *sender, int arg) {
  /*This service should allow the sender to obtain the process identifier (PID)
  of the sender if argument is 0 or of the sender’s parent otherwise. It should
  return 0 as the parent identifier of the root process.*/
  if (arg == 0)
    return sender->p_pid;
  else {
    // sender parent is root
    if (sender->p_parent->p_parent == NULL)
      return 0;
    else {
      return sender->p_parent->p_pid;
    }
  }
}

void killProgeny(pcb_t *sender) {
  // check if process exists and is not ssi
  if (sender == NULL || sender->p_pid == SSI_PID || isFree(sender->p_pid)) {
    return;
  }

  // recurrsively kill childs
  pcb_PTR child = NULL;
  while ((child = headProcQ(&sender->p_child)) != NULL) {
    outProcQ(&sender->p_child, child);
    killProgeny(child);
  }

  pcb_PTR removed = outProcQ(&ready_queue_list, sender);

  if (outProcQ(&msg_queue_list, sender) != NULL || outProcQ(&pseudoClockList, sender) != NULL) {
    soft_block_count--;
  } else if (removed == NULL) { // pcb not found in the ready queue so it must be blocked for a device
    /*check if is blocked for device response*/
    for (int i = 0; i < SEMDEVLEN - 1; i++) {
      if (outProcQ(&blockedPCBs[i], sender) != NULL) {
        soft_block_count--;
        break;
      }
    }
  }

  // kill process
  freePcb(sender);
  process_count--;
}

