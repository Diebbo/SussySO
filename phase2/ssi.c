#include "./headers/ssi.h"

void initSSI(){
    // init the first process
  ssi_pcb = allocPcb();
  /*init first process state */
  RAMTOP(ssi_pcb->p_s.reg_sp); // Set SP to RAMTOP

  /*FROM MANUAL:
          IEc: bit 0 - The “current” global interrupt enable bit. When 0,
     regardless of the settings in Status.IM all interrupts are disabled. When
     1, interrupt acceptance is controlled by Status.IM. • KUc: bit 1 - The
     “current” kernel-mode user-mode control bit. When Sta- tus.KUc=0 the
     processor is in kernel-mode. • IEp & KUp: bits 2-3 - the “previous”
     settings of the Status.IEc and Sta- tus.KUc. • IEo & KUo: bits 4-5 - the
     “previous” settings of the Status.IEp and Sta- tus.KUp - denoted the “old”
     bit settings. These six bits; IEc, KUc, IEp, KUp, IEo, and KUo act as
     3-slot deep KU/IE bit stacks. Whenever an exception is raised the stack is
     pushed [Section 3.1] and whenever an interrupted execution stream is
     restarted, the stack is popped. [Section 7.4]*/

  ssi_pcb->p_s.pc_epc = (memaddr)SSI_function_entry_point;
  ssi_pcb->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M | MSTATUS_MIE_MASK;
  ssi_pcb->p_s.mie = MIE_ALL;

  insertProcQ(&ready_queue_list, ssi_pcb);

  ssi_pcb->p_pid = SSIPID;
}

void SSI_function_entry_point() {
  while (TRUE) {
    ssi_payload_PTR process_request_payload;
    pcb_PTR process_request_ptr = (pcb_PTR) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned)(&process_request_payload), 0);
    // satysfy request and send back resoults(with a SYSYCALL in SSIRequest)
    SSI_Request(process_request_ptr, process_request_payload->service_code,process_request_payload->arg);
  }
}

void SSI_Request(pcb_PTR sender, int service, void *arg) {
  /*If service does not match any of those provided by the SSI, the SSI should
  terminate the process and its progeny. Also, when a process requires a service
  to the SSI, it must wait for the answer.*/

  unsigned syscall_response_arg = 0;
  unsigned has_response = TRUE;

  switch (service)
  {
  case CREATEPROCESS:
    syscall_response_arg = (unsigned)Create_Process(sender, (ssi_create_process_t *)arg);
    break;
  case TERMPROCESS:
    has_response = Terminate_Process(sender, (pcb_t *)arg);
    // no reponse otherwise exception cannot find it
    break;
  case DOIO:
    DoIO(sender, (ssi_do_io_PTR)arg);
    has_response = FALSE;
    break;
  case GETTIME:
    syscall_response_arg = (unsigned)Get_CPU_Time(sender);
    break;
  case CLOCKWAIT:
    Wait_For_Clock(sender);
    has_response = FALSE;
    break;
  case GETSUPPORTPTR:
    syscall_response_arg = (unsigned)Get_Support_Data(sender);
    break;
  case GETPROCESSID:
    syscall_response_arg = (unsigned)Get_Process_ID(sender, (int)arg);
    break;
  default:
    // no match with services so must end process and progeny
    Terminate_Process(sender, NULL);
    has_response = FALSE;
    break;
  }
  // send back resoults - or just need to unblock sender
  if (has_response)
    SYSCALL(SENDMESSAGE, (unsigned int)sender, syscall_response_arg, 0);
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
  
  // initialization of new prole
  copyState(arg->state,&new_prole->p_s);
  new_prole->p_supportStruct = arg->support; // even if optional -> will be null
  new_prole->p_time = 0;
  // enalbe interrupts
  process_count++;
  insertProcQ(&ready_queue_list, new_prole);
  insertChild(sender, new_prole);
  return new_prole;
}

unsigned Terminate_Process(pcb_t *sender, pcb_t *target) {
  /*
  This services causes the sender process or another process to cease to exist
  [Section 11]. In addition, recursively, all progeny of that process are
  terminated as well. Execution of this instruction does not complete until all
  progeny are terminated. The mnemonic constant TERMINATEPROCESS has the value
  of 2. This service terminates the sender process if arg is NULL. Otherwise,
  arg should be a pcb_t pointer
  */
  if (target == NULL) {
    killProgeny(sender);
    return FALSE;
  } else {
    killProgeny(target);
    return TRUE;
  }
}

void getDevLineAndNumber(unsigned command_address, unsigned *dev_line, unsigned *dev_no){
  for (int j = 0; j < N_DEV_PER_IL; j++) {
        termreg_t *base_address = (termreg_t *)DEV_REG_ADDR(IL_TERMINAL, j);
        if (command_address == (memaddr)&(base_address->recv_command)) {
            *dev_line = IL_TERMINAL;
            *dev_no = j;
            return;
        } else if (command_address == (memaddr)&(base_address->transm_command)) {
            *dev_line = IL_TERMINAL;
            *dev_no = j;
            return;
        }
    }

  for (int i = DEV_IL_START; i < DEV_IL_START + 7; i++) {
        for (int j = 0; j < N_DEV_PER_IL; j++) {
            dtpreg_t *base_address = (dtpreg_t *)DEV_REG_ADDR(i, j);
            if (command_address == (memaddr)&(base_address->command)) {
                *dev_line = i;
                *dev_no = j;
                return;
            }
        }
    }
}

unsigned DoIO(pcb_t *sender, ssi_do_io_PTR arg) {
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
  unsigned dev_line, dev_no;
  getDevLineAndNumber((unsigned)arg->commandAddr, &dev_line, &dev_no);

  unsigned dev_index = DEVINDEX(dev_line, dev_no);

  pcb_PTR blocked_for_message = outProcQ(&msg_queue_list, sender);
  
  // in case the process has been eliminated in the meantime
  if (blocked_for_message == NULL) {
    // need to check if for some reasons it's been unblocked in the meantime too
    if ((blocked_for_message = outProcQ(&ready_queue_list, sender)) == NULL)
      return NORESPONSE;
    else
      soft_block_count++;
  }
   

  insertProcQ(&blockedPCBs[dev_index], sender);

  // !NO, device is already blocked (need msg), soft_block_count++;

  // !this should rise an interrupt exception from a device
  *arg->commandAddr = arg->commandValue;

  return NORESPONSE;
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
  // il processo si trova ad aspettare risposta dalla sys call per pseudo clock -> lo blocco
  insertProcQ(&pseudoClockList, outProcQ(&msg_queue_list, sender));
  // !NO, e' already waiting mdg from ssi, soft_block_count++;
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
  if (sender == NULL || sender->p_pid == SSIPID || isFree(sender->p_pid)) {
    return;
  }
  
  outChild(sender);

  // recurrsively kill childs
  pcb_PTR child = NULL;
  while ((child = removeChild(sender)) != NULL) {
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

