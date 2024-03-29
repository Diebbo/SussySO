#include "./headers/exceptions.h"
/*
gimmie an unsigned int and an integer representing the bit you want to check
(0-indexed) and i'll return 1 if the bit is 1, 0 otherwise (Alex anglofono)
*/
int bitChecker(unsigned int n, int bit) { return (n >> bit) % 2; }

/*
gimmie an unsigned int and an integer representing the bit you want to check
(0-indexed) and i'll return 1 if the bit is 1, 0 otherwise (Alex anglofono)
*/
#define BIT_CHECKER(n, bit) (((n) >> (bit)) & 1)

void uTLB_RefillHandler() {
  setENTRYHI(0x80000000);
  setENTRYLO(0x00000000);
  TLBWR();
  LDST((state_t *)0x0FFFF000);
}

void exceptionHandler() {
  // error code from .ExcCode field of the Cause register
  unsigned int operation_start_timer = getTIMER(); // get timer serve a ?
  unsigned int exception_error = getCAUSE();
  // performing a bitwise right shift operation
  // int exception_error = Cause >> CAUSESHIFT; // GETEXCODE?

  /*fare riferimento a sezione 12 delle slide 'phase2spec' x riscv*/
  if (exception_error >= 24 && exception_error <= 28)
    TLBExceptionHandler();
  else if (exception_error >= 8 && exception_error <= 11)
    SYSCALLExceptionHandler(operation_start_timer);
  else if ((exception_error >= 0 && exception_error <= 7) ||
           (exception_error >= 12 && exception_error <= 23))
    TrapExceptionHandler();
  else if (exception_error >= 17 && exception_error <= 21)
    interruptHandler();
}

void SYSCALLExceptionHandler(unsigned int operation_start_timer) {
  // finding if in user or kernel mode
  state_t *exception_state = (state_t *)BIOSDATAPAGE;
  // the 1st bit of the status register is the 'user mode' bit
  // but i need to shift to the KUp(revious) bit (3rd bit)
  // 0 = kernel mode, 1 = user mode
  memaddr kernel_user_state = getSTATUS();
  int kernel_mode = BIT_CHECKER(kernel_user_state, 3); // se il bit e' a zero allora sono in kernel mode
  if (kernel_mode){
    kernel_mode = 0;
  }
  else{
    kernel_mode = 1;
  }//ora kernelmode = 1 se sono in kernel mode, 0 altrimenti

  int a0_reg = current_process->p_s.reg_a0, /* syscall number */
      a1_reg = current_process->p_s.reg_a1, /* dest process */
      a2_reg = current_process->p_s.reg_a2; /* payload */

  msg_t *msg;
  if (a0_reg >= -2 && a0_reg <= -1) {
    // check if in current process is in kernel mode
    if (kernel_mode) {
      switch (a0_reg) {
      case SENDMESSAGE:
        /*This system call cause the transmission of a message to a specified
          process. This is an asynchronous operation (sender doesn’t wait for
          receiver to perform a ReceiveMessage [Section 5.2]) and on success
          returns/places 0 in the caller’s v0, otherwise MSGNOGOOD is used to
          provide a meaningful error condition on return. This system call
          doesn’t require the process to lose its remaining time slice (it
          depends on scheduler policy). If the target process is in the
          pcbFree_h list, set the return register (v0 in µMPS3) to
          DEST_NOT_EXIST. If the target process is in the ready queue, this
          message is just pushed in its inbox, otherwise if the process is
          awaiting for a message, then it has to be awakened and put into the
          Ready Queue.
          The SYS1 service is requested by the calling process by placing the
          value -1 in a0, the destina- tion process PCB address in a1, the
          payload of the message in a2 and then executing the SYSCALL
          instruction.*/

        if (isFree(a2_reg)) { // error!
          current_process->p_s.reg_a0 = DEST_NOT_EXIST;
          return;
        }

        // push message
        msg = allocMsg();
        if (msg == NULL) {
          current_process->p_s.reg_a0 = MSGNOGOOD;
          return;
        }

        msg->m_payload = (unsigned)a2_reg;
        msg->m_sender = current_process;

        int dest_process_pid = a1_reg;
        pcb_t *dest_process = NULL;

        if (isInList(&msg_queue_list, dest_process_pid) == TRUE) {
          // process is blocked waiting for a message
          dest_process = findProcessPtr(&msg_queue_list, dest_process_pid);
          outProcQ(&msg_queue_list, dest_process);
          insertProcQ(&ready_queue_list, dest_process);
          soft_block_count--;
          break;
        }

        // process not found in blockedPCBs, so check ready_queue_list
        if (dest_process == NULL) {
          dest_process = findProcessPtr(&ready_queue_list, dest_process_pid);

          if (dest_process == NULL) { // not found even in ready queue
            current_process->p_s.reg_a0 = DEST_NOT_EXIST;
            return;
          }
        }

        pushMessage(&dest_process->msg_inbox, msg);
        current_process->p_s.reg_a0 = 0;
        /*on success returns/places 0 in the caller’s v0, otherwise
                        MSGNOGOOD is used to provide a meaningful error
          condition on return*/

        break;
      case RECEIVEMESSAGE:
        /*This system call is used by a process to extract a message from its
        inbox or, if this one is empty, to wait for a message. This is a
        synchronous operation since the requesting process will be frozen
        until a message matching the required characteristics doesn’t arrive.
        This system call provides as returning value (placed in caller’s v0 in
        µMPS3) the identifier of the process which sent the message extracted.
        This system call may cause the process to lose its remaining time
        slice, since if its inbox is empty it has to be frozen. The SYS2
        service is requested by the calling process by placing the value -2 in
        a0, the sender process PCB address or ANYMESSAGE in a1, a pointer to
        an area where the nucleus will store the payload of the message in a2
        (NULL if the payload should be ignored) and then executing the SYSCALL
        instruction. If a1 contains a ANYMESSAGE pointer, then the requesting
        process is looking for the first message in its inbox, without any
        restriction about the sender. In this case it will be frozen only if
        the queue is empty, and the first message sent to it will wake up it
        and put it in the Ready Queue.
      */

        /*
         * NOTA implementativa:
         * 1. messaggio gia' in inbox -> restituire il messaggio
         * 2. messaggio non presente -> bloccare il processo
         * */
        unsigned int sender_pid = a1_reg;

        msg = popMessageByPid(&current_process->msg_inbox, sender_pid);

        if (msg == NULL) { // i'll wait
          soft_block_count++;
          // no need to remove from ready_queue_list -> already done in
          // Scheduler
          insertProcQ(&msg_queue_list, current_process);
          /*The saved processor state (located at the start of the BIOS Data
          Page [Section 3]) must be copied into the Current Process’s PCB
          (p_s)*/

          current_process->p_s.cause = exception_state->cause;
          current_process->p_s.entry_hi = exception_state->entry_hi;
          // current_process->p_s.gpr = exception_state->gpr;              //err
          // 'l'espressione deve essere un lvalue modificabile'
          current_process->p_s.mie = exception_state->mie;
          current_process->p_s.pc_epc = exception_state->pc_epc;
          current_process->p_s.status = exception_state->status;
          // 2nd Update the accumulated CPU time for the Current Process
          LDIT(getTIMER() - operation_start_timer);
          // 3rd call the scheduler
          Scheduler();
        }

        /*This system call provides as returning value (placed in caller’s v0 in
        µMPS3) the identifier of the process which sent the message extracted.
        +payload in stored in a2*/

        current_process->p_s.reg_a0 = msg->m_sender->p_pid;
        int *payload = (int *)a2_reg;
        *payload = (int)msg->m_payload;

        break;
      default:
        // Process is in kernel mode, simulate Program Trap exception
        // same behavior
        TrapExceptionHandler();
        break;
      }
      // Returning from SYSCALL1 or SYSCALL2 (no blocking)
      // Increment PC by 4 to avoid an infinite loop of SYSCALLs +//load back
      // updated interrupted state
      current_process->p_s.pc_epc += WORDLEN;
      LDST(&current_process->p_s);
    } else {
      /*The above two Nucleus services are considered privileged services and
        are only available to processes executing in kernel-mode. Any attempt to
        request one of these services while in user-mode should trigger a
        Program Trap exception response. In particular the Nucleus should
        simulate a Program Trap exception when a privileged service is requested
        in user-mode. This is done by setting Cause.ExcCode in the stored
        exception state to RI (Reserved Instruction), and calling one’s Program
        Trap exception handler. Technical Point: As described above [Section 4],
        the saved exception state (for Processor 0) is stored at the start of
        the BIOS Data Page (0x0FFF.F000)*/

      // Process is in user mode, simulate Program Trap exception
      // Set Cause.ExcCode to RI (Reserved Instruction)
      exception_state->cause =
          PRIVINSTR; // Reserved Instruction Exception, exc code = 10
      TrapExceptionHandler();
    }
  } else {
    TrapExceptionHandler();
  }
}

void TrapExceptionHandler() { passUpOrDie(current_process, GENERALEXCEPT); }

void passUpOrDie(pcb_t *p, unsigned type) {
  if (p->p_supportStruct == NULL) {
    // then the process and the progeny of the process must be terminated
    killProgeny(p);
    return;
  }

  // 1st Save the processor state
  state_t *exception_state = (state_t *)BIOSDATAPAGE;
  p->p_supportStruct->sup_exceptState[type].cause = exception_state->cause;
  p->p_supportStruct->sup_exceptState[type].entry_hi =
      exception_state->entry_hi;
  p->p_supportStruct->sup_exceptState[type].mie = exception_state->mie;
  p->p_supportStruct->sup_exceptState[type].pc_epc = exception_state->pc_epc;
  p->p_supportStruct->sup_exceptState[type].status = exception_state->status;

  // 2nd Update the accumulated CPU time for the Current Process
  LDCXT(p->p_supportStruct->sup_exceptContext[type].stackPtr,
        p->p_supportStruct->sup_exceptContext[type].status,
        p->p_supportStruct->sup_exceptContext[type].pc);
}

void TLBExceptionHandler() { passUpOrDie(current_process, PGFAULTEXCEPT); }
