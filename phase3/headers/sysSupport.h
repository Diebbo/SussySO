#ifndef SYSSUPPORT_H
#define SYSSUPPORT_H

#include "stdlib.h"

/*pussed up exception handling*/
void supportExceptionHandler(void);
void TLBhandler();
/*function similar to the one in ssi but for sst msg handling*/
void UsysCallHandler(state_t* exception_state);

#endif