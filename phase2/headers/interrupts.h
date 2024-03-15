/*Implement the device/timer interrupt exception handler. Process device/timer interrupts and convert 
them into appropriate messages for blocked PCBs.*/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "scheduler.h"

void interruptHandler(pcb_PTR sender);
void interruptHandlerNonTimer(pcb_PTR sender, int IntlineNo);
void interruptHandlerPLT(pcb_PTR sender);








#endif
