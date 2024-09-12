/*Implement the TLB, Program Trap, and SYSCALL exception handlers. 
This module should also contain the provided skeleton TLB-Refill event handler.*/
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "nucleus.h"

void uTLB_RefillHandler(void);
// called if page fault
void TLBExceptionHandler(state_t *);
void TrapExceptionHandler(state_t *);
// Pass up of page fault (TLB exception) or general exception 
void passUpOrDie(unsigned , state_t *);
void exceptionHandler(void);
void SYSCALLExceptionHandler(void);

/* EXTERN FUNCTIONS */
extern void killProgeny(pcb_t *);
extern void interruptHandler(void);
extern cpu_t deltaInterruptTime(void);

#endif
