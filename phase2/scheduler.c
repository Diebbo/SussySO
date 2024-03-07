#include "headers/scheduler.h"
#include "headers/ssi.h"
#include "headers/nucleus.h"
#include <uriscv/liburiscv.h>

void scheduler() {

  // 1. get the first prcess
  // 2. set the timer - generate an interrupt
  // 3. save the current process state
  // 4. get the next process
  // 5. restore the next process state
  // 6. set the timer - generate an interrupt
  // 7. repeat

  while (TRUE) {
    // ready_queue_head is a shared resource => disable interrupts
    current_process = removeProcQ(&ready_queue_list);

    if (current_process == NULL) { // ready_queue_head is empty
      if (process_count == 1 && current_process->p_pid == ssi_id) { 
          HALT();
      } else if (process_count > 0 && soft_block_count > 0) {
          WAIT();
      } else { // PC > 0 soft block count = 0
          PANIC();
      }

    } else {
      // load in plt 5 seconds
      setTIMER(TIMESLICE);
      LDST(&current_process->p_s);
    }
  }
}
