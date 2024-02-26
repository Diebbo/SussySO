#include "headers/scheduler.h"

void scheduler() {

  // 1. get the first prcess
  // 2. set the timer - generate an interrupt
  // 3. save the current process state
  // 4. get the next process
  // 5. restore the next process state
  // 6. set the timer - generate an interrupt
  // 7. repeat

  while (1){
		pcb_t* current_process = removeProcQ(&ready_queue_head);
		if (current_process == NULL){
			// no more processes
			break;
		}

		// load in plt 5 seconds
		LDST(&current_process->p_s);


  }
}
