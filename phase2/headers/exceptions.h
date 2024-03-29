/*Implement the TLB, Program Trap, and SYSCALL exception handlers. 
This module should also contain the provided skeleton TLB-Refill event handler.*/
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H


#include "interrupts.h"

void TLBExceptionHandler(void);
void SYSCALLExceptionHandler(unsigned);

#endif

