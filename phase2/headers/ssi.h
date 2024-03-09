/*Implement the System Service Interface (SSI) process. 
This involves handling various system service calls (SYS1, SYS3, SYS5, SYS6, SYS7, etc.).*/

#ifndef SSI_H
#define SSI_H
#include "nucleus.h"

//last pid number assigned to a process 
int last_used_pid = 0;
void SSI_function_entry_point(void);
void SSI_Request(pcb_t* sender, int service, void* arg);
//generate unique pid for processes
int generate_pid();
//When requested, this service causes a new process, said to be a progeny of the sender, to be created.
pcb_PTR Create_Process(pcb_t* sender, struct ssi_create_process_t * arg);
//Return pcb_ptr of a process given the list where it is and his pid
pcb_PTR find_process_ptr(struct list_head *target_process, int pid);
//When requested terminate a process and his progeny
void terminate_process(pcb_t* sender, pcb_t* );
//I/O operation
void do_io(ssi_payload_PTR);
//This service should allow the sender to get back the accumulated processor time (in µseconds) used by the sender process.
cpu_t* Get_CPU_Time(pcb_t* sender);
//virtual device which sends out an interrupt (a tick) every 100 milliseconds
void Wait_For_Clock(pcb_t* sender);

#endif