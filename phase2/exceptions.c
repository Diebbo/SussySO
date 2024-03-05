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
  if (exception_error > 24 && exception_error <= 28)
    TLBExceptionHandler();										//TODO
  else if (exception_error >= 8 && exception_error < 11)
    SYSCALLExceptionHandler();
  else if ((exception_error >= 0 && exception_error < 7) ||
           (exception_error >= 11 && exception_error < 24))
    TrapExceptionHandler();										//TODO
  else if (exception_error >= 17 && exception_error <=21)
    InterruptExceptionHandler();								//TODO
}

void SYSCALLExceptionHandler() {
	// finding if in user or kernel mode
	state_t *exception_state = (state_t *)BIOSDATAPAGE;
	//pcb_t *current_process?

	int a0 = current_process->p_s.reg_a0, a1 = current_process->p_s.reg_a1,
		a2 = current_process->p_s.reg_a2, a3 = current_process->p_s.reg_a3,
		user_state = exception_state->status;

	/*Kup???*/
	if (a0 >= -2 && a0 <= -1) {
		if (user_state == 1) { // kernel state syscallexception_state
			switch (a0) {
				case SENDMESSAGE:
					/*If the target process is in the pcbFree_h list, set the return
					register (v0 in µMPS3) to DEST_NOT_EXIST. If the target process is in
					the ready queue, this message is just pushed in its inbox, otherwise
					if the process is awaiting for a message, then it has to be awakened
					and put into the Ready Queue.*/
					
					if(is_in_list(&pcbFree_h,a2))													//non trova pcbFree_h??
						current_process->p_s.reg_a0 = DEST_NOT_EXIST;
					
					//if(is_in_list(&ready_queue_list,a2))
					//	pushMessage(&current_process->msg_inbox,);									//push it inbox??

					/*on success returns/places 0 in the caller’s v0, otherwise
							MSGNOGOOD is used to provide a meaningful error condition
							on return*/
					current_process->p_s.reg_a0 = SYSCALL(SENDMESSAGE, a1, a2, 0);

					break;
				case RECEIVEMESSAGE:
					/*the sender process PCB address or ANYMESSAGE in a1, a pointer to an
					area where the nucleus will store the payload of the message in a2 (NULL
					if the payload should be ignored) and then executing the SYSCALL
					instruction. If a1 contains a ANYMESSAGE pointer, then the requesting
					process is looking for the first message in its inbox, without any
					restriction about the sender. In this case it will be frozen only if the
					queue is empty, and the first message sent to it will wake up it and put
					it in the Ready Queue.*/

					/*This system call provides as returning value (placed in caller’s v0 in µMPS3) 
					the identifier of the process which sent the message extracted.*/
					current_process->p_s.reg_a0 = SYSCALL(RECEIVEMESSAGE,a1,a2,0);
					//blocking call so 1st load state
					LDST(exception_state->status);
					//2nd Update the accumulated CPU time for the Current Process
					LDIT(current_process->p_time);																	
					//3rd call the scheduler
					scheduler();
					break;
				default:
					// Unhandled SYSCALL, simulate Program Trap exception in user-mode
					// Set Cause.ExcCode to RI (Reserved Instruction)
					current_process->p_s.cause = (current_process->p_s.cause & ~GETEXECCODE) 
												| (RI << CAUSESHIFT); //RI non esiste???
						/*exception_state->cause & ~GETEXECCODE: This part clears the bits related to the exception code 
						in the cause field. It uses the bitwise NOT (~) operator to create a bitmask where all bits are 
						set to 1 except for the bits in the GETEXECCODE position, and then performs a bitwise AND with the 
						existing cause field to clear those bits.
						| (RI << CAUSESHIFT): This part sets the exception code to RI (Reserved Instruction) by shifting the 
						value of RI left by CAUSESHIFT bits and performing a bitwise OR with the existing cause field.*/
							
					// Call Program Trap exception handler
					TrapExceptionHandler();
					break;
			}
				// Returning from SYSCALL
				// Increment PC by 4 to avoid an infinite loop of SYSCALLs +//load back updated interrupted state 
				current_process->p_s.pc_epc += WORDLEN;
				LDST(exception_state->status);
		}else{
			// Process is in user mode, simulate Program Trap exception
			// Set Cause.ExcCode to RI (Reserved Instruction)
			current_process->p_s.cause = (current_process->p_s.cause & ~GETEXECCODE) 
										| (RI << CAUSESHIFT); //RI non esiste???	
			TrapExceptionHandler();
		}
	}else{
		TrapExceptionHandler();
	}
}

int is_in_list(struct list_head *target_process, int pid){
	pcb_PTR tmp;
    list_for_each_entry(tmp,target_process,p_list){
        if(tmp->p_pid == pid) return 1;
    }
	return 0;
}