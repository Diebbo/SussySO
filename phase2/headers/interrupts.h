/*Implement the device/timer interrupt exception handler. Process device/timer interrupts and convert 
them into appropriate messages for blocked PCBs.*/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "nucleus.h"

void interruptHandler(void);
void interruptHandlerNonTimer(int IntlineNo);
void interruptHandlerPLT(pcb_PTR sender);
void pseudoClockHandler(pcb_PTR sender);

#endif
