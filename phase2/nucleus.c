/*Implement the main function and export the Nucleus's global variables, such as process count, 
soft-blocked count, blocked PCBs lists/pointers, etc.*/
#include "./headers/nucleus.h"
//necessaria x fare test
extern void test();

void initKernel() {
  passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
  // populate the passup vector
  passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler; // TODO refil
  passupvector->tlb_refill_stackPtr = (memaddr)KERNELSTACK; // Stacks in µMPS3 grow down
  passupvector->exception_stackPtr = (memaddr)KERNELSTACK;
  passupvector->exception_handler = (memaddr)exceptionHandler; // TODO: our exception handler;

  // initialize the nucleus data structures
  initPcbs();
  initMsgs();

  // Initialize other variables
  INIT_LIST_HEAD(&ready_queue_list);
  for (int i = 0; i < SEMDEVLEN; i++) {
    INIT_LIST_HEAD(&blockedPCBs[i]);
  }
  INIT_LIST_HEAD(&pseudoClockList);
  INIT_LIST_HEAD(&pcbFree_h);
  current_process = NULL;

  // load the system wide interval timer
  LDIT(PSECOND);

  // init the first process
  pcb_t *first_process = allocPcb();
  /*state_t*/
  /*s_entryHI: Represents the value of the EntryHi register, which holds
     high-order address bits of the virtual address being translated. s_cause:
     Corresponds to the Cause register, holding information about exceptions and
     interrupts. s_status: Associated with the Status register, containing
     control and status bits. s_pc: Represents the Program Counter (PC), holding
     the address of the next instruction to be executed. s_reg[STATEREGNUM]: An
     array representing general-purpose registers.*/

  /*init first process state */
  RAMTOP(first_process->p_s.reg_sp); // Set SP to RAMTOP
  STST(&first_process->p_s);         // saving in pcb struct

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

  first_process->p_s.pc_epc = (memaddr)SSI_function_entry_point; // TODO
  first_process->p_s.status =
      IECON |
      ALLOFF; // 32 bit reg. -> 0x01 = Enable interrupts, kernel mode actv

  // tree structure
  INIT_LIST_HEAD(&first_process->p_sib);
  INIT_LIST_HEAD(&first_process->p_child);
  first_process->p_parent = NULL;

  first_process->p_time = 0;

  first_process->p_supportStruct = NULL;

  list_add_tail(&first_process->p_list, &ready_queue_list);

  first_process->p_pid = generate_pid();

  process_count++;

  pcb_t *second_process = allocPcb();

  RAMTOP(second_process->p_s.reg_sp); // Set SP to RAMTOP - 2 * FRAME_SIZE
  second_process->p_s.reg_sp -= 2 * sizeof(pcb_t); // STST()???
  second_process->p_s.pc_epc = (memaddr)test; // TODO
  second_process->p_s.status = IECON | ALLOFF;

  INIT_LIST_HEAD(&second_process->p_sib);
  INIT_LIST_HEAD(&second_process->p_child);
  second_process->p_parent = NULL;

  second_process->p_time = 0;

  second_process->p_supportStruct = NULL;

  list_add_tail(&second_process->p_list, &ready_queue_list);

  second_process->p_pid = generate_pid();

  process_count++;
}

void uTLB_RefillHandler() {
  setENTRYHI(0x80000000);
  setENTRYLO(0x00000000);
  TLBWR();
  LDST((state_t *)0x0FFFF000);
}

void exceptionHandler() {
  // error code from .ExcCode field of the Cause register
  memaddr exception_error = getCAUSE();
  // performing a bitwise right shift operation
  // int exception_error = Cause >> CAUSESHIFT; // GETEXCODE?

  /*fare riferimento a sezione 12 delle slide 'phase2spec' x riscv*/
  if (exception_error >= 24 && exception_error <= 28)
    TLBExceptionHandler();                                              // TODO
  else if (exception_error >= 8 && exception_error <= 11)
    SYSCALLExceptionHandler();
  else if ((exception_error >= 0 && exception_error <= 7) ||
           (exception_error >= 12 && exception_error <= 23))
    TrapExceptionHandler();                                             // TODO
  else if (exception_error >= 17 && exception_error <= 21)
    InterruptExceptionHandler();                                        // TODO
}

void SYSCALLExceptionHandler() {
  // finding if in user or kernel mode
  state_t *exception_state = (state_t *)BIOSDATAPAGE;
  // pcb_t *current_process?

  int a0 = current_process->p_s.reg_a0, a1 = current_process->p_s.reg_a1,
      a2 = current_process->p_s.reg_a2, a3 = current_process->p_s.reg_a3,
      user_state = exception_state->status;

  /*Kup???*/
  if (a0 >= -2 && a0 <= -1) {
    if (user_state == 1) { // kernel state syscallexception_state
      switch (a0) {
      case SENDMESSAGE:
        /*This system call cause the transmission of a message to a specified process. This is an asynchronous
          operation (sender doesn’t wait for receiver to perform a ReceiveMessage [Section 5.2]) and on success
          returns/places 0 in the caller’s v0, otherwise MSGNOGOOD is used to provide a meaningful error condition
          on return. This system call doesn’t require the process to lose its remaining time slice (it depends on
          scheduler policy). If the target process is in the pcbFree_h list, set the return register (v0 in µMPS3)
          to DEST_NOT_EXIST. If the target process is in the ready queue, this message is just pushed in its
          inbox, otherwise if the process is awaiting for a message, then it has to be awakened and put into the
          Ready Queue.
          The SYS1 service is requested by the calling process by placing the value -1 in a0, the destina-
          tion process PCB address in a1, the payload of the message in a2 and then executing the SYSCALL
          instruction.*/

        if (is_in_list(&pcbFree_h, a2))
          current_process->p_s.reg_a0 = DEST_NOT_EXIST;
        else{
          /*on success returns/places 0 in the caller’s v0, otherwise
                          MSGNOGOOD is used to provide a meaningful error
            condition on return*/
          current_process->p_s.reg_a0 = SYSCALL(SENDMESSAGE, a1, a2, 0);
        }
        
        if(is_in_list(&ready_queue_list,a2))
          pushMessage(&current_process->msg_inbox,current_process->p_s.reg_a0);
        else if(is_in_list(&blockedPCBs,a2)){
          //wakeup process                                                      //TODO
          outProcQ(&blockedPCBs,current_process);
          insertProcQ(&ready_queue_list,current_process);
        }

        break;
      case RECEIVEMESSAGE:
        /*This system call is used by a process to extract a message from its inbox or, if this one is empty, to
          wait for a message. This is a synchronous operation since the requesting process will be frozen until a
          message matching the required characteristics doesn’t arrive. This system call provides as returning
          value (placed in caller’s v0 in µMPS3) the identifier of the process which sent the message extracted.
          This system call may cause the process to lose its remaining time slice, since if its inbox is empty it
          has to be frozen.
          The SYS2 service is requested by the calling process by placing the value -2 in a0, the sender
          process PCB address or ANYMESSAGE in a1, a pointer to an area where the nucleus will store the
          payload of the message in a2 (NULL if the payload should be ignored) and then executing the SYSCALL
          instruction. If a1 contains a ANYMESSAGE pointer, then the requesting process is looking for the first
          message in its inbox, without any restriction about the sender. In this case it will be frozen only if
          the queue is empty, and the first message sent to it will wake up it and put it in the Ready Queue.*/

        /*This system call provides as returning value (placed in caller’s v0 in
        µMPS3) the identifier of the process which sent the message extracted.
        +payload in stored in a2*/
        current_process->p_s.reg_a0 = SYSCALL(RECEIVEMESSAGE, a1, a2, 0);
        /*The saved processor state (located at the start of the BIOS Data Page [Section 3]) must be
        copied into the Current Process’s PCB (p_s)*/
        current_process->p_s.cause = exception_state->cause;
        current_process->p_s.entry_hi = exception_state->entry_hi;
        //current_process->p_s.gpr = exception_state->gpr;              //err 'l'espressione deve essere un lvalue modificabile'
        current_process->p_s.mie = exception_state->mie;
        current_process->p_s.pc_epc = exception_state->pc_epc;
        current_process->p_s.status = exception_state->status;
        LDST((void*)current_process->p_s.status);
        // 2nd Update the accumulated CPU time for the Current Process
        //LDIT(current_process->p_time);                                  //TODO = sez 10
        // 3rd call the scheduler
        Scheduler();
        break;
      default:
        TrapExceptionHandler();
        break;
      }
      // Returning from SYSCALL
      // Increment PC by 4 to avoid an infinite loop of SYSCALLs +//load back
      // updated interrupted state
      current_process->p_s.pc_epc += WORDLEN;
      LDST(exception_state->status);
    } else {
      /*The above two Nucleus services are considered privileged services and are only available to processes
        executing in kernel-mode. Any attempt to request one of these services while in user-mode should
        trigger a Program Trap exception response.
        In particular the Nucleus should simulate a Program Trap exception when a privileged service is
        requested in user-mode. This is done by setting Cause.ExcCode in the stored exception state to RI
        (Reserved Instruction), and calling one’s Program Trap exception handler.
        Technical Point: As described above [Section 4], the saved exception state (for Processor 0) is
        stored at the start of the BIOS Data Page (0x0FFF.F000)*/

      // Process is in user mode, simulate Program Trap exception
      // Set Cause.ExcCode to RI (Reserved Instruction)
      exception_state->cause = PRIVINSTR; //Reserved Instruction Exception, exc code = 10
      TrapExceptionHandler();
    }
  } else {
    TrapExceptionHandler();
  }
}

int is_in_list(struct list_head *target_process, int pid) {
  pcb_PTR tmp;
  list_for_each_entry(tmp, target_process, p_list) {
    if (tmp->p_pid == pid)
      return 1;
  }
  return 0;
}
