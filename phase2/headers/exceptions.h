/*Implement the TLB, Program Trap, and SYSCALL exception handlers. 
This module should also contain the provided skeleton TLB-Refill event handler.*/
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H


#include "ssi.h"

void TLBExceptionHandler(void);
void SYSCALLExceptionHandler(void);
extern void Terminate_Process(pcb_t *sender, pcb_t *target);
extern void interruptHandler(void);
#endif

