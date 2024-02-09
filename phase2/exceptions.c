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

}
