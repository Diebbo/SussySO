#ifndef NUCL_H
#define NUCL_H

// general headers
#include "../../headers/const.h"
#include "../../headers/listx.h"
#include "../../headers/types.h"

// phase 1 headers
#include "../../phase1/headers/pcb.h"
#include "../../phase1/headers/msg.h"


// phase 2 headers
//TODO: #include <../phase2/headers/>


/* GLOBAL FUNCTIONS */
void initKernel(void);
void uTLB_RefillHandler(void);

#endif
