#ifndef SYSSUPPORT_H
#define SYSSUPPORT_H

#include "../../phase2/headers/nucleus.h"

void supportExceptionHandler(void);
void TLBhandler();
void UsysCallHandler(state_t* exception_state);

#endif