#include "./headers/p3test.h"

void test3() {
  /*While test was the name/external reference to a function that exercised the Level 3/Phase 2 code,
   * in Level 4/Phase 3 it will be used as the instantiator process (InstantiatorProcess).3
   * The InstantiatorProcess will perform the following tasks:
   *  • Initialize the Level 4/Phase 3 data structures. These are:
   *     – The Swap Pool table and a Swap Mutex process that must provide mutex access to the
   *        swap table using message passing [Section 4.1].
   *     – Each (potentially) sharable peripheral I/O device can have a process for it. These process
   *        will be used to receive complex requests (i.e. to write of a string to a terminal) and request
   *        the correct DoIO service to the SSI (this feature is optional and can be delegated directly
   *        to the SST processes to simplify the project).
   *  • Initialize and launch eight SST, one for each U-procs.
   *  • Terminate after all of its U-proc “children” processes conclude. This will drive Process Count
   *      to one, triggering the Nucleus to invoke HALT. Wait for 8 messages, that should be send when
   *      each SST is terminated.
   */

  // Init. of Swap Pool table and a Swap Mutex process
  initSwapPool();
  entrySwapFunction();

  // Init. sharable peripheral (done in initSSTs)
  /* Technical Point: A careful reading of the Level 4/Phase 3 specification reveals that there are
   * actually no purposefully shared peripheral devices. Each of the [1..8] U-procs has its own flash device
   * (backing store), printer, and terminal device(s). Hence, one does not actually need a process to
   * protect access to device registers. However, for purposes of correctness (or more appropriate: to
   * protect against erroneous behavior) and future phase compatibility, it is possible to have a process for
   * each device that waits for messages and requests the single DoIO to the SSI.
   */

  //Init 8 SST
  initSSTs();

  //Terminate after the 8 sst die
  terminate();
}



void terminateAll(){
  for(int i=0; i<8; i++){
    SYSCALL(RECEIVEMSG,ANYMESSAGE,0,0);
  }
  PANIC();
}


