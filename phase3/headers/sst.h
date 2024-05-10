#ifndef SST_H
#define SST_H

#include "../../phase2/headers/nucleus.h"

extern pcb_PTR ith_sst_child;
extern pcb_PTR ith_sst_pcb;

/*initialize ith SST and its process*/
void initSST(void);
/*SST utility*/
void sstEntry(void);
void sstRequestHandler();
/*return TimeOfDay in microseconds*/
void getTOD(pcb_PTR sender);
/*kill SST and its child*/
void killSST(pcb_PTR sender);


#endif
