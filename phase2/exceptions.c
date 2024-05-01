#include "./headers/exceptions.h"

extern cpu_t time_interrupt_start;

void uTLB_RefillHandler() {
  setENTRYHI(0x80000000);
  setENTRYLO(0x00000000);
  TLBWR();
  LDST((state_t *)0x0FFFF000);
}


void exceptionHandler() {
  // get cpu time 
  STCK(time_interrupt_start);

  // error code from .ExcCode field of the Cause register
  unsigned status = getSTATUS();
  unsigned cause = getCAUSE();
  unsigned exception_code = cause & 0x7FFFFFFF; // 0311 1111 x 32

  unsigned is_interrupt_enabled = BIT_CHECKER(status, 7);
  unsigned is_interrupt = BIT_CHECKER(cause, 31);

  state_t *exception_state = (state_t *)BIOSDATAPAGE;

  // check if interrupts first
  if (is_interrupt_enabled && is_interrupt) {
    interruptHandler();
    return;
  }
  /*specific code for uRISCV*/
  if (exception_code >= 24 && exception_code <= 28) {
    TLBExceptionHandler(exception_state);
  } else if (exception_code >= 8 && exception_code <= 11) {
    if (BIT_CHECKER(exception_state->status, 2) == USERMODE) {
      exception_state->cause = PRIVINSTR;
      exceptionHandler();
    } else {
      SYSCALLExceptionHandler();
    }
  } else if ((exception_code >= 0 && exception_code <= 7) ||
             (exception_code >= 12 && exception_code <= 23)) {
    TrapExceptionHandler(exception_state);
  }
}

void SYSCALLExceptionHandler() {
  // finding if in user or kernel mode
  state_t *exception_state = (state_t *)BIOSDATAPAGE;

  /* syscall number */
  int a0_reg = exception_state->reg_a0;
  /* dest process */
  int a1_reg = exception_state->reg_a1;
  /* payload */
  int a2_reg = exception_state->reg_a2;
  msg_t *msg;

  if (a0_reg >= -2 && a0_reg <= -1) {
    // check if in current process is in kernel mode
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

      if (a1_reg == 0) {
        exception_state->reg_a0 = DEST_NOT_EXIST;
        break;
      }

      pcb_t *dest_process = (pcb_PTR)a1_reg;

      if (isFree(dest_process->p_pid)) { // error!
        exception_state->reg_a0 = DEST_NOT_EXIST;
        break;
      }

      // push message
      msg = allocMsg();
      if (msg == NULL) {
        exception_state->reg_a0 = MSGNOGOOD;
        break;
      }

      msg->m_payload = (unsigned)a2_reg;
      msg->m_sender = current_process;

      if (outProcQ(&msg_queue_list, dest_process) != NULL) {
        // process is blocked waiting for a message, i unblock it
        insertProcQ(&ready_queue_list, dest_process);
        soft_block_count--;
      }

      insertMessage(&dest_process->msg_inbox, msg);
      exception_state->reg_a0 = 0;
      /*on success returns/places 0 in the caller’s v0, otherwise
        MSGNOGOOD is used to provide a meaningful error condition 
        on return*/

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
      and put it in the Ready Queue.*/

      /*
       * NOTE:
       * 1. msg already in inbox -> give back msg
       * 2. msg not found -> blocck process
       *
       */
      //desired sender pid
      pcb_t *sender = (pcb_PTR)a1_reg;

      // if the sender is NULL, then the process is looking for the first
      msg = popMessage(&current_process->msg_inbox, sender);

      // there is no correct message in the inbox, need to be frozen.
      if (msg == NULL) {
        // i can assume the process is in running state
        insertProcQ(&msg_queue_list, current_process);
        soft_block_count++;
        // save the processor state
        copyState(exception_state, &(current_process->p_s));
        // update the accumulated CPU time for the Current Process
        current_process->p_time -= deltaInterruptTime();
        // get the next process
        Scheduler();
        return;
      }

      /*The saved processor state (located at the start of the BIOS Data
      Page [Section 3]) must be copied into the Current Process’s PCB
      (p_s)*/
      /*This system call provides as returning value (placed in caller’s v0 in
      µMPS3) the identifier of the process which sent the message extracted.
      +payload in stored in a2*/
      exception_state->reg_a0 = (unsigned)msg->m_sender;

      // write the message's payload in the location signaled in the a2
      // register.
      if (a2_reg != 0) {
        // has a payload
        *((unsigned *)a2_reg) = (unsigned)msg->m_payload;
      }
      
      break;
    }
    // Returning from SYSCALL1 or SYSCALL2 (no blocking)
    // Increment PC by 4 to avoid an infinite loop of SYSCALLs +//load back
    // updated interrupted state
    exception_state->pc_epc += WORDLEN;
    LDST(exception_state);
  } else {
    // invalid syscall
    killProgeny(current_process);
    Scheduler();
  }
}

void TrapExceptionHandler(state_t *exec_state) { passUpOrDie(GENERALEXCEPT, exec_state); }

void TLBExceptionHandler(state_t *exec_state) { passUpOrDie(PGFAULTEXCEPT, exec_state); }

void passUpOrDie(unsigned type, state_t *exec_state) {
  if (current_process == NULL || current_process->p_supportStruct == NULL) {
    // then the process and the progeny of the process must be terminated
    killProgeny(current_process);
    Scheduler();
    return;
  }
  // 1st Save the processor state
  copyState(exec_state, &current_process->p_supportStruct->sup_exceptState[type]);
  // 2nd Update the accumulated CPU time for the Current Process
  current_process->p_time -= deltaInterruptTime();
  current_process->p_time += deltaTime();
  // 3rd Pass up the exception
  context_t context_pass_to = current_process->p_supportStruct->sup_exceptContext[type];
  LDCXT(context_pass_to.stackPtr, context_pass_to.status, context_pass_to.pc);
}
