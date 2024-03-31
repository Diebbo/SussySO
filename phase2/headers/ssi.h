/*Implement the System Service Interface (SSI) process. This involves handling
various system service calls (SYS1, SYS3, SYS5, SYS6, SYS7, etc.).*/

#ifndef SSI_H
#define SSI_H
#include "nucleus.h"


void SSI_Request(pcb_t *sender, int service, void *arg);
// When requested, this service causes a new process, said to be a progeny of
// the sender, to be created.
pcb_PTR Create_Process(pcb_t *sender, struct ssi_create_process_t *arg);
// When requested terminate a process and his progeny
void Terminate_Process(pcb_t *sender, pcb_t *target);
// I/O operation
void *DoIO(pcb_t *sender, ssi_do_io_PTR arg);
// This service should allow the sender to get back the accumulated processor
// time (in µseconds) used by the sender process.
cpu_t Get_CPU_Time(pcb_t *sender);
// virtual device which sends out an interrupt (a tick) every 100 milliseconds
void Wait_For_Clock(pcb_t *sender);
/*This service should allow the sender to obtain the process’s Support
Structure. Hence, this service returns the value of p_supportStruct from the
sender process’s PCB. If no value for p_supportStruct was provided for the
sender process when it was created, return NULL.*/
support_t *Get_Support_Data(pcb_t *sender);
/*This service should allow the sender to obtain the process identifier (PID) of
the sender if argument is 0 or of the sender’s parent otherwise. It should
return 0 as the parent identifier of the root process.*/
int Get_Process_ID(pcb_t *sender, int arg);
/*This service is called if there is no match with ssi service*/
void killProgeny(pcb_t *sender);

extern void interruptHandlerNonTimer(int IntlineNo);

#endif
