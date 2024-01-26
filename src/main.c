// general headers
#include "../headers/const.h"
#include "../headers/listx.h"
#include "../headers/types.h"

// phase 1 headers
#include "../phase1/headers/pcb.h"
#include "../phase1/headers/msg.h"


// phase 2 headers
//TODO: #include <../phase2/headers/>


/* GLOBAL VARIABLES*/
int process_count = 0; // started but not terminated processes
int soft_block_count = 0; // processes waiting for a resource
pcb_t *ready_queue = NULL; // tail pointer to the ready state queue processes
pcb_t *current_process = NULL;
pcb_t blocked_pbs[SEMDEVLEN - 1];
pcb_t support_blocked_pbs[SEMDEVLEN - 1]; //TODO: cambiare nome, non so a che serva


int main(int argc, char *argv[])
{
    return 0;
}
