#ifndef SYSSUPPORT_H
#define SYSSUPPORT_H

#include "sst.h"

/*function to handle program trap exception*/
void programTrapExceptionHandler(state_t *exception_state);

/*pussed up exception handling*/
void supportExceptionHandler(void);
/*function similar to the one in ssi but for sst msg handling*/
void UsysCallHandler(state_t* exception_state, int asid);

#endif