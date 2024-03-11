/*Implement the device/timer interrupt exception handler. Process device/timer interrupts and convert 
them into appropriate messages for blocked PCBs.*/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "nucleus.h"

void interrupt_handler(pcb_PTR sender);
void non_timer_interrupt_handler(pcb_PTR sender, int IntlineNo);
void PLT_interrupt_handler(pcb_PTR sender);








#endif