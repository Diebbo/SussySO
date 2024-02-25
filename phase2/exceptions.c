/*Implement the TLB, Program Trap, and SYSCALL exception handlers. 
This module should also contain the provided skeleton TLB-Refill event handler.*/
#include "./headers/exceptions.h"

void uTLB_RefillHandler() {
	setENTRYHI(0x80000000);
	setENTRYLO(0x00000000);
	TLBWR();
	LDST((state_t*) 0x0FFFF000);
}

void exceptionHandler(){
	//error code from .ExcCode field of the Cause register
	memaddr Cause = getCAUSE();
	int exception_error = GETEXECCODE >> CAUSESHIFT;
 
	/*fare riferimento a sezione 12 delle slide 'phase2spec' x riscv*/
    if(exception_error > 24 && exception_error <= 28) 
		TLBExceptionHandler();
    else if(exception_error >= 8 && exception_error < 11) 
		SYSCALLExceptionHandler();
    else if (exception_error >= 0 && exception_error < 7 && 
				exception_error >= 11 && exception_error < 24) 
		TrapExceptionHandler();
	else	
		InterruptExceptionHandler();
}

void SYSCALLExceptionHandler(){
	//finding if in user or kernel mode 
	state_t* exception_state = (state_t*) BIOSDATAPAGE;
    int a0 = exception_state->reg_a0,
        a1 = exception_state->reg_a1,
        a2 = exception_state->reg_a2,
        a3 = exception_state->reg_a3,
		user_state = exception_state->status;

	if(a0 >= -2 && a0 <= -1){
		if(user_state == 1){ //kernel state syscall
			switch (a0)
			{
			case SENDMESSAGE:
				SYSCALL(SENDMESSAGE,a1,a2,0);
				break;
			case RECEIVEMESSAGE:
				SYSCALL(RECEIVEMESSAGE,ANYMESSAGE,a2,0);
			default:
				//metto trap
				break;
			}

		}else{

		}
	}else{

	}
}


