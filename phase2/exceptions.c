/*Implement the TLB, Program Trap, and SYSCALL exception handlers.
This module should also contain the provided skeleton TLB-Refill event
handler.*/
#include "./headers/exceptions.h"
#include "./headers/nucleus.h"
#include "./headers/scheduler.h"

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
