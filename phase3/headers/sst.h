#ifndef SST_H
#define SST_H

#include "stdlib.h"

extern pcb_PTR sst_pcb[MAXSSTNUM];
extern pcb_PTR test_process;

/*SST utility*/
void sstEntry(void);
/*SST server entry point*/
void sstRequestHandler();
/*initialize ith SST and its process*/
void initSSTs(void);
/*return TimeOfDay in microseconds*/
void getTOD(pcb_PTR sender);
/*kill SST and its child*/
void killSST(pcb_PTR sender);
// print a string to the printer with the same number of the sender ASID
void writeOnPrinter(pcb_PTR sender, sst_print_PTR arg, unsigned asid);
// print a string to the terminal with the same number of the sender ASID
void writeOnTerminal(pcb_PTR sender, sst_print_PTR arg, unsigned asid);
// write a string to a device
void write(char *msg, int lenght, devreg_t *devAddrBase);

#endif
