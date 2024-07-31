#include "headers/sysSupport.h"

extern pcb_PTR current_process;

void supportExceptionHandler() {
  /*Assuming that the handling of the exception is to be passed up (non-NULL
   *Support Structure pointer) and the appropriate sup_exceptContext fields of
   *the Support Structure were correctly ini- tialized, execution continues with
   *the Support Level’s general exception handler, which should then pass
   *control to the Support Level’s SYSCALL exception handler. The processor
   *state at the time of the exception will be in the Support Structure’s
   *corresponding sup_exceptState field. (NOTE: already overwritten the one in
   *phase2)
   */
  support_t *current_support = getSupportData();
  // unsigned int context = current_support->sup_exceptContext->status;
  unsigned int status = current_support->sup_exceptState->cause;
  state_t *exception_state = (state_t *)BIOSDATAPAGE;
  unsigned exception_code = status & 0x7FFFFFFF;

  if ((exception_code >= 0 && exception_code <= 7) ||
      (exception_code >= 12 && exception_code <= 23)) {
    UsysCallHandler(exception_state);
  }
}

void UsysCallHandler(state_t *exception_state) {
  /* wrapper syscall number */
  int a0_reg = exception_state->reg_a0;
  /* dest process */
  int a1_reg = exception_state->reg_a1;
  /* payload */
  int a2_reg = exception_state->reg_a2;
  pcb_t *dest_process;

  // perform action only if is a SEND/RECEIVE request AND if it's an user
  // process
  if (a0_reg != SENDMSG || a0_reg != RECEIVEMSG ||
      !isOneOfSSTPids(current_process->p_parent->p_pid)) {
    // invalid syscall
    TrapExceptionHandler(exception_state);
  }
  switch (a0_reg) {
  case SENDMSG:
    /* This services cause the transmission of a message to a specified process.
     * The USYS1 service is essentially a user-mode “wrapper” for the
     * kernel-mode restricted SYS1 service. The USYS1 service is requested by
     * the calling process by placing the value 1 in a0, the destination process
     * PCB address or SST in a1, the payload of the message in a2 and then
     * executing the SYSCALL instruction. If a1 contains PARENT, then the
     * requesting process send the message to its SST [Section 6], that is its
     * parent.
     */

    dest_process =
        a1_reg == PARENT ? current_process->p_parent : (pcb_t *)a1_reg;

    SYSCALL(SENDMESSAGE, (unsigned)current_process, (unsigned)dest_process,
            a2_reg);

    break;
  case RECEIVEMSG:
    /* This system call is used by a process to extract a message from its inbox
     * or, if this one is empty, to wait for a message. The USYS2 service is
     * essentially a user-mode “wrapper” for the kernel-mode restricted SYS2
     * service. The USYS2 service is requested by the calling process by placing
     * the value 2 in a0, the sender process PCB address or ANYMESSAGE in a1, a
     * pointer to an area where the nucleus will store the payload of the
     * message in a2 (NULL if the payload should be ignored) and then executing
     * the SYSCALL instruction. If a1 contains a ANYMESSAGE pointer, then the
     * requesting process is looking for the first message in its inbox, without
     * any restriction about the sender. In this case it will be frozen only if
     * the queue is empty, and the first message sent to it will wake up it and
     * put it in the Ready Queue.
     */
    SYSCALL(RECEIVEMESSAGE, (unsigned)current_process, (unsigned)a1_reg,
            a2_reg);

    break;
  }
}
