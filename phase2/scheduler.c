#include "./headers/nucleus.h"

void Scheduler() {

  // 1. get the first prcess
  // 2. set the timer - generate an interrupt
  // 3. save the current process state
  // 4. get the next process
  // 5. restore the next process state
  // 6. set the timer - generate an interrupt
  // 7. repeat

    // ready_queue_head is a shared resource => disable interrupts
    if(current_process != NULL){
      current_process->p_time += deltaTime();
    }

    current_process = removeProcQ(&ready_queue_list);

    if (current_process == NULL) {
      // ready_queue_head is empty
      if(process_count == 0){
        // no process in the system
        HALT();
      }

      if (process_count > 0 && soft_block_count > 0) {
        // enable interrupts
        setTIMER(MAXINT);
                                              // dal generare interrupt, guardare sezione "important" del paragrafo 2 di spec
        setSTATUS(MSTATUS_MIE_MASK | MSTATUS_MPP_M); // enable interrupts
        setMIE(MIE_ALL);
        
        // wait for an interrupt
        WAIT();
      } else if(process_count > 0 && soft_block_count == 0) { // process count > 0 soft block count = 0
        PANIC();
      }

    } else {
      STCK(acc_cpu_time);
      // load in plt 5 seconds
      setTIMER(TIMESLICE);
      LDST(&current_process->p_s);
    }
  
}
