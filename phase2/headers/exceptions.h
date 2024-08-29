/*Implement the TLB, Program Trap, and SYSCALL exception handlers. 
This module should also contain the provided skeleton TLB-Refill event handler.*/
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "nucleus.h"


void uTLB_RefillHandler();
void TLBExceptionHandler(state_t *);
void SYSCALLExceptionHandler(void);
void passUpOrDie(unsigned , state_t *);
void exceptionHandler(void);
void TrapExceptionHandler(state_t *);
extern void killProgeny(pcb_t *);
extern void interruptHandler(void);
extern cpu_t deltaInterruptTime(void);
#endif
