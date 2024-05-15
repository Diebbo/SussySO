#ifndef SST_H
#define SST_H

#include "../../phase2/headers/nucleus.h"

extern pcb_PTR ith_sst_child;
extern pcb_PTR ith_sst_pcb;
extern pcb_PTR ssi_pcb;

/*initialize ith SST and its process*/
void initSST(void);
/*SST utility*/
void sstEntry(void);
void sstRequestHandler();
/*return TimeOfDay in microseconds*/
void getTOD(pcb_PTR sender);
/*kill SST and its child*/
void killSST(pcb_PTR sender);
/*This service cause the print of a string of characters to the device 
with the same number of the sender ASID*/
void writeOnDevice(pcb_PTR sender, ssi_payload_PTR arg, unsigned int ip_line);

#endif
