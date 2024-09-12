/*Implement the device/timer interrupt exception handler. Process device/timer interrupts and convert 
them into appropriate messages for blocked PCBs.*/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "nucleus.h"

// Starting time of the interrupt
extern cpu_t time_interrupt_start;


// Accumulated time during interrupt
cpu_t deltaInterruptTime(void);
void interruptHandler(void);
// Interrupt handler for devices
void interruptHandlerNonTimer(unsigned);
/*Processor Local Time is the internal clock or timing mechanism specific to each processor, 
useful for managing time-based operations at the local level in multi-processor environments.*/
void interruptHandlerPLT(void);
// Clock interrupt for Round Robin alghorithm
void pseudoClockHandler(void);

#endif
