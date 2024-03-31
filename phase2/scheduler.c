#include "./headers/nucleus.h"

void Scheduler() {

  // 1. get the first prcess
  // 2. set the timer - generate an interrupt
  // 3. save the current process state
  // 4. get the next process
  // 5. restore the next process state
  // 6. set the timer - generate an interrupt
  // 7. repeat

  while (TRUE) {
    // ready_queue_head is a shared resource => disable interrupts
    if(current_process != NULL){
      current_process->p_time += deltaTime();
    }

    current_process = removeProcQ(&ready_queue_list);

    if (current_process == NULL) {
      // ready_queue_head is empty

      if (process_count > 1 && soft_block_count > 0) {
        // enable interrupts
            setTIMER(MAXINT);
            setSTATUS(IECON | IMON);
                                              // dal generare interrupt, guardare sezione "important" del paragrafo 2 di spec
        // wait for an interrupt
        WAIT();
      } else { // process count > 0 soft block count = 0
        PANIC();
      }

    } else if (process_count == 1 && current_process->p_pid == SSIPID) {
      // only one process in the system and it is the System Idle Process
      HALT();
    } else {
      // load in plt 5 seconds
      setTIMER(TIMESLICE);
      STCK(acc_cpu_time);
      LDST(&current_process->p_s);
    }
  }
}
