/*Implement the TLB, Program Trap, and SYSCALL exception handlers. 
This module should also contain the provided skeleton TLB-Refill event handler.*/
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H


#include "nucleus.h"


void TLBExceptionHandler(void);
void InterruptExceptionHandler(void);
void SYSCALLExceptionHandler(void);

#endif

