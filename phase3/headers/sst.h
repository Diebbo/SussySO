#ifndef SST_H
#define SST_H

#include "stdlib.h"

extern pcb_PTR sst_pcb[MAXSSTNUM];
extern pcb_PTR test_process;
extern pcb_PTR print_pcb[MAXSSTNUM];
extern void programTrapExceptionHandler();

/*SST utility*/
void sstEntry(void);
/*SST server entry point*/
void sstRequestHandler(pcb_PTR sender, int service, void *arg, pcb_PTR print_process, pcb_PTR term_process);
/*initialize ith SST and its process*/
void initSSTs(void);
/*return TimeOfDay in microseconds*/
cpu_t getTOD(void);
/*kill SST and its child*/
void killSST(int asid);
/*print a string to the printer or terminal*/
void print(unsigned code, sst_print_PTR arg, pcb_PTR print_process);

#endif
