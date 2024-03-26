/*Implement the System Service Interface (SSI) process.
This involves handling various system service calls (SYS1, SYS3, SYS5, SYS6,
SYS7, etc.).*/
#include "./headers/ssi.h"

void SSI_function_entry_point() {
  pcb_PTR process_request_ptr;
  msg_PTR process_request_msg;
  while (TRUE) {
    // receive request (asked from ssi proc; payload is temporaly not important)
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);

    unsigned int process_request_id = current_process->p_s.reg_a0;

    process_request_ptr = findProcessPtr(
        &ready_queue_list, process_request_id); // situato in ready queue?
    // find msg payload
    process_request_msg =
        popMessageByPid(&process_request_ptr->msg_inbox, process_request_id);
    // satysfy request and send back resoults(with a SYSYCALL in SSIRequest)
    SSI_Request(process_request_ptr, process_request_ptr->p_s.reg_a2,
                (void *)process_request_msg->m_payload);
  }
}

void SSI_Request(pcb_t *sender, int service, void *arg) {
  /*If service does not match any of those provided by the SSI, the SSI should
  terminate the process and its progeny. Also, when a process requires a service
  to the SSI, it must wait for the answer.*/

  // finding if in user or kernel mode
  // state_t *exception_state = (state_t *)BIOSDATAPAGE;
  // int user_state = exception_state->status;
  memaddr kernel_user_state = getSTATUS() << 1;

  if (kernel_user_state != 1) {
    // Must be in kernel mode otherwise trap!
    TrapExceptionHandler();
  } else {
    switch (service) {
    case CREATEPROCESS:
      arg = Create_Process(
          sender,
          (ssi_create_process_t *)arg); // giusta fare una roba de genere per 2
          break;
    case TERMPROCESS:
      Terminate_Process(sender, (pcb_t *)arg);
      break;
    case DOIO:
      arg = DoIO(sender, (ssi_payload_t *)arg);
      break;
    case GETTIME:
      arg = (void *)Get_CPU_Time(sender);
      break;
    case CLOCKWAIT:
      Wait_For_Clock(sender);
      arg = NULL;
      break;
    case GETSUPPORTPTR:
      arg = Get_Support_Data(sender);
      break;
    case GETPROCESSID:
      arg = (void *)Get_Process_ID(sender, (int)arg);
      break;
    default:
      // no match with services so must end process and progeny
      kill_progeny(sender);
      arg = NULL;
      break;
    }
    // send back resoults
    if (arg != NULL) {
      /*
       * some functions (eg. DOIO & default) don't need to send back a message
       * */
      SYSCALL(SENDMESSAGE, sender->p_pid, (unsigned)arg, 0);
    }
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
    new_prole->p_pid = generatePid();
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
  /*if (target == NULL) {                               //sistemerà vitto per ora provo a sistemarlo io
    // terminate sender process but not the progeny!
    removeChild(sender->p_parent);
    outChild(sender);
    outProcQ(sender->p_list, sender);
    // delete sender???
  } else {
    list_for_each_entry(target, &target->p_child, p_child) {
      Terminate_Process(target,
                        container_of(target->p_child.next, pcb_t, p_child));
      removeChild(target->p_parent); // TODO: check if it's correct, serve che
      outChild(target);
      outProcQ(target->p_list, target);
      // delete target???
    }
  }*/
  if(target == NULL){
    outChild(sender);
    removeProcQ(&sender->p_list);
  }else{
    kill_progeny(sender);
  }
}

void *DoIO(pcb_t *sender, ssi_payload_t *arg) {
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
  ssi_do_io_PTR do_io = arg->arg;
  unsigned int_line_no = 7; // da specifiche per terminale
  unsigned device =
      (unsigned)(*do_io->commandAddr - 0x10000054 - (int_line_no - 3) * 0x80) >>
      0x10; // formula inversa dell'indirizzamento interrupt
  list_add_tail(&sender->p_list, &blockedPCBs[device]);
  soft_block_count++;

  *do_io->commandAddr =
      do_io->commandValue; // !IMPORTANT: this rise an interrupt exception from
                           // a device

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
  int tick = PSECOND;
  cpu_t current_time;
  cpu_t last_time;
  // macro to read TimeOfDay clock
  STCK(last_time);
  STCK(current_time);
  // send interrupt
  SYSCALL(SENDMESSAGE, sender->p_pid, sender->p_s.reg_a2, 0);
  // saving proc waiting for tick
  insertProcQ(&pseudoClockList, sender);
  // waiting till tick
  while (TRUE) {
    if ((current_time - last_time) % tick == 0)
      break;
    // refresh TOD time till tick passed
    STCK(current_time);
  }
  // removing from clock list cause tick passed
  removeProcQ(&pseudoClockList);
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

void kill_progeny(pcb_t *sender) {
  // check if process has children
  if (headProcQ(&sender->p_child) == NULL) {
    // check if has sib
    if (headProcQ(&sender->p_sib) != NULL) {
      struct list_head *iter;
      // iteration on all sib to recursevely kill progeny
      list_for_each(iter, &sender->p_sib) {
        pcb_t *item = container_of(iter, pcb_t, p_sib);
        process_count--;
        outChild(sender);
        removeProcQ(&sender);
        pcb_PTR son = headProcQ(&sender->p_child);
        kill_progeny(son);
      }
    }
    process_count--;
    outChild(sender);
    removeProcQ(&sender);
  } else {
    if (headProcQ(&sender->p_sib) != NULL) {
      struct list_head *iter;
      list_for_each(iter, &sender->p_sib) {
        pcb_t *item = container_of(iter, pcb_t, p_sib);
        process_count--;
        outChild(sender);
        removeProcQ(&sender);
        pcb_PTR son = headProcQ(&sender->p_child);
        kill_progeny(son);
      }
    } else {
      process_count--;
      outChild(sender);
      removeProcQ(&sender);
        pcb_PTR son = headProcQ(&sender->p_child);
        kill_progeny(son);
    }
  }
}
