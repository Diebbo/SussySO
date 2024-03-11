/*Implement the device/timer interrupt exception handler. Process device/timer interrupts and convert 
them into appropriate messages for blocked PCBs.*/

#include "headers/interrupts.h"

/*int getInterruptLines(){
    // 1. Read the interrupt lines from the interrupting devices
    // 2. Return the interrupt lines
    return getCAUSE() & IOINTERRUPTS & TIMERINTERRUPT & DISKINTERRUPT & FLASHINTERRUPT & PRINTINTERRUPT & TERMINTERRUPT;
}*/

void interrupt_handler(pcb_PTR sender){
    int *pendingInterrupts;
    for (int i = 0; i < 8; i++) {
        pendingInterrupts[i] = CAUSE_IP_GET(getCAUSE(), i);
    } //potrebbe non essere la cosa corretta, da controllare

    for (int i = 0; i < 8; i++) {
        if (pendingInterrupts[i] == 1) {
            non_timer_interrupt_handler(sender, i);
        }
    }
}// stessa roba di sopra, da controllare

void non_timer_interrupt_handler(pcb_PTR sender, int IntlineNo){
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
    // Tip: to calculate the device number you can use a switch among constants DEVxON
    int DevNo; // Device number da calcolare
    switch (getCAUSE() & IOINTERRUPTS & DISKINTERRUPT & FLASHINTERRUPT & PRINTINTERRUPT & TERMINTERRUPT)
    {
    case DEV0ON:
        DevNo = 0;
        break;
    case DEV1ON:
        DevNo = 1;
        break;
    case DEV2ON:
        DevNo = 2;
        break;
    case DEV3ON:
        DevNo = 3;
        break;
    case DEV4ON:
        DevNo = 4;
        break;
    case DEV5ON:    
        DevNo = 5;
        break;
    case DEV6ON:
        DevNo = 6;
        break;
    case DEV7ON:
        DevNo = 7;
        break;
    default:
        break;
    }
    // Interrupt line number da calcolare
    int devAddrBase = 0x10000054 + ((IntlineNo - 3) * 0x80) + (DevNo * 0x10);

    // 2. Save off the status code from the device’s device register
    state_t* state = 

}

void PLT_interrupt_handler(pcb_PTR sender){
    /* The PLT portion of the interrupt exception handler should therefore:
        •Acknowledge the PLT interrupt by loading the timer with a new value [Section 4.1.4-pops].
        •Copy the processor state at the time of the exception (located at the start of the BIOS Data
            Page [Section 3.2.2-pops]) into the Current Process’s PCB (p_s).
        •Place the Current Process on the Ready Queue; transitioning the Current Process from the
            “running” state to the “ready” state.
        •Call the Scheduler.
    */
   
}