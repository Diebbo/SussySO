/*Implement the device/timer interrupt exception handler. Process device/timer interrupts and convert 
them into appropriate messages for blocked PCBs.*/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "nucleus.h"
#include <uriscv/arch.h>

// time interrupt started
cpu_t time_interrupt_start;

cpu_t deltaInterruptTime();
void interruptHandler(void);
void interruptHandlerNonTimer(int IntlineNo);
void interruptHandlerPLT();
void pseudoClockHandler();

#endif
