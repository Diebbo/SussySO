/*Implement the TLB, Program Trap, and SYSCALL exception handlers. 
This module should also contain the provided skeleton TLB-Refill event handler.*/

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H


#include <uriscv/types.h>
#include <uriscv/const.h>
#include <uriscv/bios.h>
#include <uriscv/liburiscv.h>
#include "../../headers/const.h"
#include "../../headers/listx.h"
#include "../../headers/types.h"

// phase 1 headers
#include "../../phase1/headers/pcb.h"
#include "../../phase1/headers/msg.h"


void TLBExceptionHandler();
void exceptionHandler();
void InterruptExceptionHandler();
void SYSCALLExceptionHandler();
void TrapExceptionHandler();
//this function serves to find if a crocess is in a specific list based on his pid
int is_in_list(struct list_head *target_process, int pid);

#endif

