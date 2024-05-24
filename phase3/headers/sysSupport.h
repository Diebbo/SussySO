#ifndef SYSSUPPORT_H
#define SYSSUPPORT_H

#include "../../phase2/headers/nucleus.h"

/*pussed up exception handling*/
void supportExceptionHandler(void);
void TLBhandler();
/*function similar to the one in ssi but for sst msg handling*/
void UsysCallHandler(state_t* exception_state);
/*function to get support struct (requested to SSI)*/
pcb_t *getSupport(void);

#endif