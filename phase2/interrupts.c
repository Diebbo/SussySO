#include "./headers/interrupts.h"

cpu_t time_interrupt_start;

void interruptHandler(void) {
  unsigned exce_mie = getMIE();
  unsigned exce_mip = getMIP();
  // pending interrupt
  unsigned ip = exce_mie & exce_mip;
  if (BIT_CHECKER(ip, 7)) {
    interruptHandlerPLT();
  }
  if (BIT_CHECKER(ip, 3)) {
    pseudoClockHandler();
  }
  for (int i = 16; i <= 21; i++) {
    if (BIT_CHECKER(ip, i)) {
      interruptHandlerNonTimer(i);
    }
  }
}

void interruptHandlerNonTimer(unsigned ip_line) {
  /*  1. Calculate the address for this device’s device register
      2. Save off the status code from the device’s device register
      3. Acknowledge the outstanding interrupt
      4. Send a message and unblock the PCB waiting the status
          response from this (sub)device
      5. Place the stored off status code in the newly unblocked PCB’s
          v0 register
      6. Insert the newly unblocked PCB on the Ready Queue,
          transitioning this process from the “blocked” state to the
          “ready” state
      7. Return control to the Current Process
    */

  // 1. Calculate the address for this device’s device register
  // Tip: to calculate the device snumber you can use a switch among constants
  // DEVxON
  int dev_no = 0;

  unsigned *devices_bit_map = (unsigned *)CDEV_BITMAP_ADDR(ip_line);
  switch (*devices_bit_map) {
  case DEV0ON:
    dev_no = 0;
    break;
  case DEV1ON:
    dev_no = 1;
    break;
  case DEV2ON:
    dev_no = 2;
    break;
  case DEV3ON:
    dev_no = 3;
    break;
  case DEV4ON:
    dev_no = 4;
    break;
  case DEV5ON:
    dev_no = 5;
    break;
  case DEV6ON:
    dev_no = 6;
    break;
  case DEV7ON:
    dev_no = 7;
    break;
  default:
    break;
  }

  // Interrupt line number
  unsigned dev_addr_base = (unsigned)DEV_REG_ADDR(ip_line, dev_no);
  unsigned dev_index = 0, status = 0;

  // diffrent device interrupts
  switch (ip_line) {
  case IL_TERMINAL:
    termreg_t *term = (termreg_t *)dev_addr_base;


    // 2. Save off the status code from the device’s device register
    // 3. Acknowledge the outstanding interrupt
    if ((term->transm_status & 0xff) == OKCHARTRANS) {
      status = term->transm_status & 0xff;
      term->transm_command = ACK;
    } else {
      status = term->recv_status;
      term->recv_command = ACK;
    }

    // 4. Send a message and unblock the PCB waiting the status
    dev_index = DEVINDEX(ip_line, dev_no);

    break;

  case IL_FLASH:
  default:
    dtpreg_t *flash = (dtpreg_t *)dev_addr_base;

    status = flash->status;

    flash->command = ACK;
    
    dev_index = DEVINDEX(ip_line, dev_no);
    break;
  }

  pcb_PTR caller = removeProcQ(&blockedPCBs[dev_index]);

  if (caller != NULL) {
    // Simulating a SYS2 call to unblock the ssi
    msg_PTR ack = allocMsg();
    ack->m_sender = ssi_pcb;
    ack->m_payload = status;
    pushMessage(&caller->msg_inbox, ack);
    insertProcQ(&ready_queue_list, caller);
    soft_block_count--;
  }

  // 7. Return control to the Current Process
  if (current_process == NULL)
    Scheduler();
  else {
    // decrement the time that takes to the process to be interrupted
    current_process->p_time -= deltaInterruptTime();
    LDST((state_t *)BIOSDATAPAGE);
  }
}

void interruptHandlerPLT() {
  /* - Acknowledge the PLT interrupt by loading the timer with a new valu
      [Section 4.1.4-pops].
    - Copy into the Current Process’s PCB (p_s).
    - Place the
     Current Process on the Ready Queue; transitioning the Current Process from
     the “running” state to the “ready” state.
    - Call the Scheduler.
  */
  setTIMER(TIMESLICE);
  state_t *exception_state = (state_t *)BIOSDATAPAGE;

  if (current_process != NULL) {
    copyState(exception_state, &current_process->p_s);
    // ! WARNING: process already running
    insertProcQ(&ready_queue_list, current_process);

    // decrement the time that takes to the process to be interrupted
    current_process->p_time -= deltaInterruptTime();
  }
  Scheduler();
}

void pseudoClockHandler() {
  /*
    The Interval Timer portion of the interrupt exception handler should
    therefore:
      1. Acknowledge the interrupt by loading the Interval Timer with a new
    value: 100 milliseconds (constant PSECOND) [Section 4.1.3-pops].
      2. Unblock all PCBs blocked waiting a Pseudo-clock tick.
      3. Return control to the Current Process: perform a LDST on the saved
    exception state (located at the start of the BIOS Data Page [Section 4]).
  */
  LDIT(PSECOND);
  pcb_PTR pcb = NULL;
  while ((pcb = removeProcQ(&pseudoClockList)) != NULL) {
    // unlock process - SYS2
    insertProcQ(&ready_queue_list, pcb);

    msg_PTR msg = allocMsg();
    msg->m_sender = ssi_pcb;
    msg->m_payload = 0;
    insertMessage(&pcb->msg_inbox, msg);
    soft_block_count--;
  }

  if (current_process == NULL) {
    Scheduler();
    return;
  }

  // decrement the time that takes to the process to be interrupted
  current_process->p_time -= deltaInterruptTime();

  LDST((state_t *)BIOSDATAPAGE);
}

cpu_t deltaInterruptTime() {
  cpu_t current_time;
  STCK(current_time);
  return current_time - time_interrupt_start;
}
