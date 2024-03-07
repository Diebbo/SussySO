/*Implement the System Service Interface (SSI) process. 
This involves handling various system service calls (SYS1, SYS3, SYS5, SYS6, SYS7, etc.).*/

#ifndef SSI_H
#define SSI_H
#include "../../headers/const.h"
#include "../../headers/listx.h"
#include "../../headers/types.h"
#include "../../phase1/headers/pcb.h"
#include "../../phase1/headers/msg.h"
#include "../../phase2/headers/nucleus.h"
#include <uriscv/types.h>
#include <uriscv/bios.h>
#include <uriscv/liburiscv.h>
#include "exceptions.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 

//last pid number assign to a process 
int last_used_pid = 0;
//SSI id (the only process to have pid = 0)
int ssi_id = 0;   
void SSI_function_entry_point(void);
void SSIRequest(pcb_t* sender, int service, void* arg);
//generate unique pid for processes
int generate_pid();
//When requested, this service causes a new process, said to be a progeny of the sender, to be created.
pcb_PTR CreateProcess(pcb_t* sender, struct ssi_create_process_t * arg);
//Return pcb_ptr of a process given the list where it is and his pid
pcb_PTR find_process_ptr(struct list_head *target_process, int pid);

#endif
/*ROBE DA IMPLEMENTARE:
    [fun. name - identifiers for different system service calls (services provided by the operating system) - explaination]

TerminateProcess (SYS1): This service is responsible for terminating a process and all its progeny. 
                        When a process is terminated, the entire subtree of processes rooted at that process must be terminated.

GetTimeOfDay (SYS3): This service allows a process to obtain the current time of day from the operating system.

WaitForClock (SYS5): This service is used by a process to wait for the next Pseudo-clock Tick. 
                    The operating system is expected to unblock all processes that are waiting for this clock tick.

Delay (SYS6): This service enables a process to delay its execution for a specified amount of time.

GetPid (SYS7): This service allows a process to obtain its process identifier (PID).

GetSupportData (SYS6): This service enables a process to obtain the process's Support Structure.

GetProcessID (SYS7): This service allows a process to obtain the process identifier (PID) of itself or its parent.
*/