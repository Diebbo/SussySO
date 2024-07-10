#ifndef SYSSUPPORT_H
#define SYSSUPPORT_H

#include "sst.h"

/*pussed up exception handling*/
void supportExceptionHandler(void);
/*function similar to the one in ssi but for sst msg handling*/
void UsysCallHandler(state_t* exception_state);

#endif