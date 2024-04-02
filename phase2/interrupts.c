/*Implement the device/timer interrupt exception handler. Process device/timer
interrupts and convert them into appropriate messages for blocked PCBs.*/

#include "./headers/interrupts.h"

/*int getInterruptLines(){
    // 1. Read the interrupt lines from the interrupting devices
    // 2. Return the interrupt lines
    return getCAUSE() & IOINTERRUPTS & TIMERINTERRUPT & DISKINTERRUPT &
FLASHINTERRUPT & PRINTINTERRUPT & TERMINTERRUPT;
}*/

void interruptHandler(void)
{
  pcb_PTR caller = current_process;
  unsigned exce_mie = getMIE();
  unsigned exce_mip = getMIP();
  unsigned ip = exce_mie & exce_mip; // interrupt pending
  if (BIT_CHECKER(ip, 7))
  {
    interruptHandlerPLT(caller);
  }
  if (BIT_CHECKER(ip, 3))
  {
    pseudoClockHandler(caller);
  }
  for (int i = 16; i <= 21; i++)
  {
    if (BIT_CHECKER(ip, i))
    {
      interruptHandlerNonTimer(i);
    }
  }
}

void interruptHandlerNonTimer(int IntlineNo)
{
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
  IntlineNo -= 14;
  int dev_no = 0;
  // unsigned *devices_bit_map = (unsigned *)0x10000040 + 0x04 * (IntlineNo - 3);
  unsigned *devices_bit_map = (unsigned *)(0x10000040 + 0x10);
  switch (*devices_bit_map)
  {
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
    // Error
    // TrapExceptionHandler();
    break;
  }

  // Interrupt line number da calcolare
  unsigned dev_addr_base =
      (unsigned)0x10000054 + ((IntlineNo - 3) * 0x80) + (dev_no * 0x10);

  termreg_t *term = (termreg_t *)dev_addr_base;
  unsigned status = term->recv_status;
  term->recv_command = ACK;

  // 2. Save off the status code from the device’s device register
  unsigned dev_index = (IntlineNo - 3) * 7 + dev_no;

  pcb_PTR caller = removeProcQ(&blockedPCBs[dev_index]);

  if (caller != NULL)
  {
    // no process is blocked -> pass control to the scheduler
    soft_block_count--;
    // send ack to device & unlock process SYS2
    msg_t *ack_msg = (msg_t *)allocMsg();

    // 3. Acknowledge the outstanding interrupt
    ack_msg->m_sender = ssi_pcb;
    ack_msg->m_payload = (unsigned)status;

    pushMessage(&caller->msg_inbox, ack_msg);

    caller->p_s.reg_a0 = status;
    insertProcQ(&ready_queue_list, caller);
  }

  // 7. Return control to the Current Process
  if (current_process == NULL)
    Scheduler();
  else 
    LDST((state_t *)BIOSDATAPAGE);
}

void interruptHandlerPLT(pcb_PTR caller)
{
  /* - Acknowledge the PLT interrupt by loading the timer with a new valu
      [Section 4.1.4-pops].
    - Copy into the Current Process’s PCB (p_s).
    - Place the
     Current Process on the Ready Queue; transitioning the Current Process from
     the “running” state to the “ready” state.
    - Call the Scheduler.
  */
  setTIMER(TIMESLICE);
  // STST(&caller->p_s);
  state_t *exception_state = (state_t *)BIOSDATAPAGE;

  copyState(exception_state, &caller->p_s);
  insertProcQ(&ready_queue_list, caller);
  Scheduler();
}

void pseudoClockHandler(pcb_PTR caller)
{
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
  pcb_PTR pcb;
  while ((pcb = outProcQ(&pseudoClockList, caller)) !=
         NULL)
  { // non proprio sicuro della correttezza del outProcQ, da
    // ricontrollare
    insertProcQ(&ready_queue_list, pcb);
  }
  LDST(&caller->p_s);
}
