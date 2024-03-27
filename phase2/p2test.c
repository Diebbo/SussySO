#include "./headers/nucleus.h"

typedef unsigned int devregtr;

/* hardware constants */
#define PRINTCHR 2
#define RECVD 5

#define CLOCKINTERVAL 100000UL /* interval to V clock semaphore */

#define BUSERROR 7
#define ADDRERROR 5
#define USYSCALLEXCPT 8
#define MSYSCALLEXCPT 11
#define SELF NULL

#define QPAGE 1024

#define MINLOOPTIME 30000
#define LOOPNUM 10000

#define CLOCKLOOP 10
#define MINCLOCKLOOP 3000
#define TERMSTATMASK 0xFF

#define BADADDR 0xFFFFFFFF
#define TERM0ADDR 0x10000254

#define MSTATUS_MIE_MASK 0x8
#define MIE_MTIE_MASK 0x40
#define MIP_MTIP_MASK 0x40
#define MIE_ALL 0xFFFFFFFF

#define MSTATUS_MPIE_BIT 7
#define MSTATUS_MIE_BIT 3
#define MSTATUS_MPRV_BIT 17
#define MSTATUS_MPP_BIT 11
#define MSTATUS_MPP_M 0x1800
#define MSTATUS_MPP_U 0x0000
#define MSTATUS_MPP_MASK 0x1800

/* just to be clear */
#define NOLEAVES 4 /* number of leaves of p8 process tree */

#define START 0

state_t p2state, p3state, p4state, p5state, p6state, p7state, p8state,
    p8rootstate, child1state, child2state, gchild1state, gchild2state,
    gchild3state, gchild4state, p9state, p10state, hp_p1state, hp_p2state,
    printstate;

int p2pid, p3pid, p4pid, p8pid, p9pid;

/* support structure for p5 */
support_t pFiveSupport;
unsigned int p5Stack; /* so we can allocate new stack for 2nd p5 */

int p1p2sync = 0; /* to check on p1/p2 synchronization */
int p4inc = 1;    /* p4 incarnation number */

memaddr *p5MemLocation = 0; /* To cause a p5 trap */

void p2(), p3(), p4(), p5(), p5a(), p5b(), p6(), p7(), p5mm(), p5sys(), p5gen(),
    p5mm(), p8root(), child1();
void child2(), p8(), p8leaf1(), p8leaf2(), p8leaf3(), p8leaf4(), p9(), p10(),
    hp_p1(), hp_p2();

extern pcb_t *ssi_pcb;
extern pcb_t *current_process;
extern int process_count;
pcb_PTR test_pcb, print_pcb, p2_pcb, p3_pcb, p4_pcb_v1, p4_pcb_v2, p5_pcb,
    p6_pcb, p7_pcb, p8_pcb, p8root_pcb, child1_pcb, child2_pcb, gchild1_pcb,
    gchild2_pcb, gchild3_pcb, gchild4_pcb, p9_pcb, p10_pcb;

ssi_create_process_t p4_ssi_create_process = {
    .state = &p4state,
    .support = NULL,
};
ssi_payload_t p4_payload = {
    .service_code = CREATEPROCESS,
    .arg = &p4_ssi_create_process,
};

/* a procedure to print on terminal 0 */
void print() {
  while (1) {
    char *msg;
    unsigned int sender =
        SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)(&msg), 0);
    char *s = msg;
    devregtr *base = (devregtr *)(TERM0ADDR);
    devregtr *command = base + 3;
    devregtr status;

    while (*s != EOS) {
      devregtr value = PRINTCHR | (((devregtr)*s) << 8);
      ssi_do_io_t do_io = {
          .commandAddr = command,
          .commandValue = value,
      };
      ssi_payload_t payload = {
          .service_code = DOIO,
          .arg = &do_io,
      };
      SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
      SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status),
              0);

      if ((status & TERMSTATMASK) != RECVD)
        PANIC();

      s++;
    }
    SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
  }
}

void print_term0(char *s) {
  SYSCALL(SENDMESSAGE, (unsigned int)print_pcb, (unsigned int)s, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)print_pcb, 0, 0);
}

void clockwait_process() {
  ssi_payload_t clock_wait_payload = {
      .service_code = CLOCKWAIT,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&clock_wait_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

void terminate_process(pcb_t *arg) {
  ssi_payload_t term_process_payload = {
      .service_code = TERMPROCESS,
      .arg = (void *)arg,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&term_process_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

pcb_t *create_process(state_t *s) {
  pcb_t *p;
  ssi_create_process_t ssi_create_process = {
      .state = s,
      .support = NULL,
  };
  ssi_payload_t payload = {
      .service_code = CREATEPROCESS,
      .arg = &ssi_create_process,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p), 0);
  return p;
}

/*********************************************************************/
/*                                                                   */
/*                 p1 test -- the root process                       */
/*                                                                   */
/*********************************************************************/
void test() {
  test_pcb = current_process;

  // test send and receive
  SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
  pcb_PTR sender = (pcb_PTR)SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);

  if (sender != test_pcb)
    PANIC();

  // init print process
  STST(&printstate);
  printstate.reg_sp = printstate.reg_sp - QPAGE;
  printstate.pc_epc = (memaddr)print;
  printstate.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  printstate.mie = MIE_ALL;

  // create print process
  print_pcb = create_process(&printstate);

  if ((int)print_pcb == NOPROC)
    PANIC();

  // test print process
  print_term0("Don't Panic.\n");

  /* set up states of the other processes */

  STST(&hp_p1state);
  hp_p1state.reg_sp = printstate.reg_sp - QPAGE;
  hp_p1state.pc_epc = (memaddr)hp_p1;
  hp_p1state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  hp_p1state.mie = MIE_ALL;

  STST(&hp_p2state);
  hp_p2state.reg_sp = hp_p1state.reg_sp - QPAGE;
  hp_p2state.pc_epc = (memaddr)hp_p2;
  hp_p2state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  hp_p2state.mie = MIE_ALL;

  STST(&p2state);
  p2state.reg_sp = hp_p2state.reg_sp - QPAGE;
  p2state.pc_epc = (memaddr)p2;
  p2state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p2state.mie = MIE_ALL;

  STST(&p3state);
  p3state.reg_sp = p2state.reg_sp - QPAGE;
  p3state.pc_epc = (memaddr)p3;
  p3state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p3state.mie = MIE_ALL;

  STST(&p4state);
  p4state.reg_sp = p3state.reg_sp - QPAGE;
  p4state.pc_epc = (memaddr)p4;
  p4state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p4state.mie = MIE_ALL;

  STST(&p5state);
  p5Stack = p5state.reg_sp =
      p4state.reg_sp - (2 * QPAGE); /* because there will 2 p4 running*/
  p5state.pc_epc = (memaddr)p5;
  p5state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p5state.mie = MIE_ALL;

  STST(&p6state);
  p6state.reg_sp = p5state.reg_sp - (2 * QPAGE);
  p6state.pc_epc = (memaddr)p6;
  p6state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p6state.mie = MIE_ALL;

  STST(&p7state);
  p7state.reg_sp = p6state.reg_sp - QPAGE;
  p7state.pc_epc = (memaddr)p7;
  p7state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p7state.mie = MIE_ALL;

  STST(&p8state);
  p8state.reg_sp = p7state.reg_sp - QPAGE;
  p8state.pc_epc = (memaddr)p8;
  p8state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p8state.mie = MIE_ALL;

  STST(&p8rootstate);
  p8rootstate.reg_sp = p8state.reg_sp - QPAGE;
  p8rootstate.pc_epc = (memaddr)p8root;
  p8rootstate.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p8rootstate.mie = MIE_ALL;

  STST(&child1state);
  child1state.reg_sp = p8rootstate.reg_sp - QPAGE;
  child1state.pc_epc = (memaddr)child1;
  child1state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  child1state.mie = MIE_ALL;

  STST(&child2state);
  child2state.reg_sp = child1state.reg_sp - QPAGE;
  child2state.pc_epc = (memaddr)child2;
  child2state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  child2state.mie = MIE_ALL;

  STST(&gchild1state);
  gchild1state.reg_sp = child2state.reg_sp - QPAGE;
  gchild1state.pc_epc = (memaddr)p8leaf1;
  gchild1state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  gchild1state.mie = MIE_ALL;

  STST(&gchild2state);
  gchild2state.reg_sp = gchild1state.reg_sp - QPAGE;
  gchild2state.pc_epc = (memaddr)p8leaf2;
  gchild2state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  gchild2state.mie = MIE_ALL;

  STST(&gchild3state);
  gchild3state.reg_sp = gchild2state.reg_sp - QPAGE;
  gchild3state.pc_epc = (memaddr)p8leaf3;
  gchild3state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  gchild3state.mie = MIE_ALL;

  STST(&gchild4state);
  gchild4state.reg_sp = gchild3state.reg_sp - QPAGE;
  gchild4state.pc_epc = (memaddr)p8leaf4;
  gchild4state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  gchild4state.mie = MIE_ALL;

  STST(&p9state);
  p9state.reg_sp = gchild4state.reg_sp - QPAGE;
  p9state.pc_epc = (memaddr)p9;
  p9state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p9state.mie = MIE_ALL;

  STST(&p10state);
  p10state.reg_sp = p9state.reg_sp - QPAGE;
  p10state.pc_epc = (memaddr)p10;
  p10state.status |= MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  p10state.mie = MIE_ALL;

  /* create process p2 */
  p2_pcb = create_process(&p2state);
  p2pid = p2_pcb->p_pid;

  /* check p2 pid */
  if (p2pid != 4)
    print_term0("ERROR: wrong p2 pid\n");

  SYSCALL(SENDMESSAGE, (unsigned int)p2_pcb, START, 0); /* start p2 */
  SYSCALL(RECEIVEMESSAGE, (unsigned int)p2_pcb, 0, 0);  /* wait p2 to end */

  /* check p2 sync */
  if (p1p2sync == 0)
    print_term0("ERROR: p1/p2 synchronization bad\n");
  else
    print_term0("p1 knows p2 ended\n");

  /* create p3 */
  p3_pcb = create_process(&p3state);
  p3pid = p3_pcb->p_pid;

  SYSCALL(SENDMESSAGE, (unsigned int)p3_pcb, START, 0); /* start p3 */
  SYSCALL(RECEIVEMESSAGE, (unsigned int)p3_pcb, 0, 0);  /* wait p3 to end */

  print_term0("p1 knows p3 ended\n");

  /* create two process without waiting the first to end */
  create_process(&hp_p1state);
  create_process(&hp_p2state);

  /* create p4 */
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p4_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p4_pcb_v1), 0);

  /* wait first incarnation of p4 to end */
  SYSCALL(RECEIVEMESSAGE, (unsigned int)p4_pcb_v1, 0, 0);

  print_term0("p1 knows p4's first incarnation ended\n");

  // try to reach p4's second incarnation
  int reply = SYSCALL(SENDMESSAGE, (unsigned int)p4_pcb_v2, 0, 0);

  if (reply == DEST_NOT_EXIST)
    print_term0("p1 knows p4's second incarnation ended\n");
  else
    print_term0("ERROR: p4's second incarnation still alive\n");

  /* create p5 with support struct */
  pFiveSupport.sup_exceptContext[GENERALEXCEPT].stackPtr = (int)p5Stack;
  pFiveSupport.sup_exceptContext[GENERALEXCEPT].status |=
      MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  pFiveSupport.sup_exceptContext[GENERALEXCEPT].pc = (memaddr)p5gen;
  pFiveSupport.sup_exceptContext[PGFAULTEXCEPT].stackPtr = p5Stack;
  pFiveSupport.sup_exceptContext[PGFAULTEXCEPT].status |=
      MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  pFiveSupport.sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr)p5mm;

  ssi_create_process_t p5_ssi_create_process = {
      .state = &p5state,
      .support = &pFiveSupport,
  };
  ssi_payload_t p5_payload = {
      .service_code = CREATEPROCESS,
      .arg = &p5_ssi_create_process,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p5_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p5_pcb), 0);

  SYSCALL(SENDMESSAGE, (unsigned int)p5_pcb, START, 0); // start p5

  /* start p6 */
  p6_pcb = create_process(&p6state);
  SYSCALL(SENDMESSAGE, (unsigned int)p6_pcb, START, 0);

  /* start p7 */
  p7_pcb = create_process(&p7state);
  SYSCALL(SENDMESSAGE, (unsigned int)p7_pcb, START, 0);

  /* start p8 */
  p8_pcb = create_process(&p8state);
  SYSCALL(SENDMESSAGE, (unsigned int)p8_pcb, START, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)p8_pcb, 0, 0); // wait p8 to end

  print_term0("check p8 root and leaf\n");
  /* now for a more rigorous check of process termination */
  for (int i = 0; i < 2; i++) {
    /* create p8root */
    p8root_pcb = create_process(&p8rootstate);
    p8pid = p8root_pcb->p_pid;

    SYSCALL(SENDMESSAGE, (unsigned int)p8root_pcb, START, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)p8root_pcb, 0, 0);

    // wait a bit for process termination
    for (int i = 0; i < 100; i++)
      clockwait_process();

    int ret = SYSCALL(SENDMESSAGE, (unsigned int)child1_pcb, 0, 0);
    if (ret != DEST_NOT_EXIST)
      print_term0("ERROR: first child should be dead\n");
    ret = SYSCALL(SENDMESSAGE, (unsigned int)child2_pcb, 0, 0);
    if (ret != DEST_NOT_EXIST)
      print_term0("ERROR: second child should be dead\n");
    ret = SYSCALL(SENDMESSAGE, (unsigned int)gchild1_pcb, 0, 0);
    if (ret != DEST_NOT_EXIST)
      print_term0("ERROR: first grandchild should be dead\n");
    ret = SYSCALL(SENDMESSAGE, (unsigned int)gchild2_pcb, 0, 0);
    if (ret != DEST_NOT_EXIST)
      print_term0("ERROR: second grandchild should be dead\n");
    ret = SYSCALL(SENDMESSAGE, (unsigned int)gchild3_pcb, 0, 0);
    if (ret != DEST_NOT_EXIST)
      print_term0("ERROR: third grandchild should be dead\n");
    ret = SYSCALL(SENDMESSAGE, (unsigned int)gchild4_pcb, 0, 0);
    if (ret != DEST_NOT_EXIST)
      print_term0("ERROR: fourth grandchild should be dead\n");
  }

  /* start p9 */
  p9_pcb = create_process(&p9state);
  p9pid = p9_pcb->p_pid;

  SYSCALL(SENDMESSAGE, (unsigned int)p9_pcb, START, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)p9_pcb, 0, 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)p10_pcb, 0, 0);

  terminate_process(p9_pcb);

  // check test_pcb child's length
  struct list_head *pos;
  int c = 0;
  list_for_each(pos, &test_pcb->p_child) c++;

  if (c > 1)
    print_term0("ERROR: all process child should be dead!\n");

  print_term0("A towel, it says, is about the most massively useful thing an "
              "interstellar hitchhiker can have.\n");

  terminate_process(SELF);

  /* should not reach this point, since p1 just got a program trap */
  print_term0("ERROR: p1 still alive after terminate\n");
  PANIC();
}

/* p2 -- sync and cputime-SYS test process */
void p2() {
  cpu_t now1, now2;     /* times of day        */
  cpu_t cpu_t1, cpu_t2; /* cpu time used       */

  // p2 wait until p1 sends a message
  SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);

  print_term0("p2 started correctly\n");

  int pid;
  ssi_payload_t get_process_payload = {
      .service_code = GETPROCESSID,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&get_process_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&pid), 0);
  if (pid != p2pid)
    print_term0("Inconsistent process id for p2!\n");
  else
    print_term0("p2 get pid successfully\n");

  /* test of GETTIME */

  STCK(now1);
  ssi_payload_t get_time_payload = {
      .service_code = GETTIME,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&get_time_payload),
          0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&cpu_t1), 0);

  /* delay for several milliseconds */
  for (int i = 1; i < LOOPNUM; i++)
    ;

  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&get_time_payload),
          0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&cpu_t2), 0);
  STCK(now2);

  if (((now2 - now1) >= (cpu_t2 - cpu_t1)) &&
      ((cpu_t2 - cpu_t1) >= (MINLOOPTIME / (*((cpu_t *)TIMESCALEADDR)))))
    print_term0("p2 is OK\n");
  else {
    if ((now2 - now1) < (cpu_t2 - cpu_t1))
      print_term0("ERROR: more cpu time than real time\n");
    if ((cpu_t2 - cpu_t1) < (MINLOOPTIME / (*((cpu_t *)TIMESCALEADDR))))
      print_term0("ERROR: not enough cpu time went by\n");
    print_term0("p2 blew it!\n");
  }

  p1p2sync = 1; /* p1 will check this */

  // notify p1 that p2 ended
  SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);

  terminate_process(SELF);

  /* just did a TERMPROCESS, so should not get to this point */
  print_term0("ERROR: p2 didn't terminate\n");
  PANIC();
}

/* p3 -- gettime and clockwait test process */
void p3() {
  cpu_t time1, time2;
  cpu_t cpu_t1, cpu_t2;

  time1 = 0;
  time2 = 0;

  // p3 wait until p1 sends a message
  SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);
  print_term0("p3 started correctly\n");

  /* loop until we are delayed at least half of clock V interval */
  while (time2 - time1 < (CLOCKINTERVAL >> 1)) {
    STCK(time1);
    clockwait_process();
    STCK(time2);
  }

  print_term0("p3 - CLOCKWAIT OK\n");

  /* now let's check to see if we're really charge for CPU time correctly */
  ssi_payload_t get_time_payload = {
      .service_code = GETTIME,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&get_time_payload),
          0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&cpu_t1), 0);

  for (int i = 0; i < CLOCKLOOP; i++)
    clockwait_process();

  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&get_time_payload),
          0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&cpu_t2), 0);

  if (cpu_t2 - cpu_t1 < (MINCLOCKLOOP / (*((cpu_t *)TIMESCALEADDR))))
    print_term0("ERROR: p3 - CPU time incorrectly maintained\n");
  else
    print_term0("p3 - CPU time correctly maintained\n");

  int pid;
  ssi_payload_t get_process_payload = {
      .service_code = GETPROCESSID,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&get_process_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&pid), 0);
  if (pid != p3pid)
    print_term0("Inconsistent process id for p3!\n");

  // notify p1 that p3 ended
  SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);

  terminate_process(SELF);

  /* just did a TERMPROCESS, so should not get to this point */
  print_term0("ERROR: p3 didn't terminate\n");
  PANIC();
}

/* p4 -- termination test process */
void p4() {
  switch (p4inc) {
  case 1:
    print_term0("first incarnation of p4 starts\n");
    p4inc++;
    break;

  case 2:
    SYSCALL(RECEIVEMESSAGE, (unsigned int)p4_pcb_v1, 0, 0);
    print_term0("second incarnation of p4 starts\n");

    SYSCALL(SENDMESSAGE, (unsigned int)p4_pcb_v1, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ANYMESSAGE, 0, 0);

    print_term0("ERROR: second incarnation of p4 didn't terminate\n");
    PANIC();
    break;
  }

  // create second incarnation of p4
  p4state.reg_sp -= QPAGE; /* give another page  */

  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p4_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p4_pcb_v2), 0);

  SYSCALL(SENDMESSAGE, (unsigned int)p4_pcb_v2, 0, 0);    // start
  SYSCALL(RECEIVEMESSAGE, (unsigned int)p4_pcb_v2, 0, 0); // wait wake up

  print_term0("p4 is OK\n");

  // notify p4 ended
  SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);

  // this TERMPROCESS should terminate p4 seecond incarnation
  terminate_process(SELF);

  /* just did a TERMPROCESS, so should not get to this point */
  print_term0("ERROR: first incarnation of p4 didn't terminate\n");
  PANIC();
}

/* p5's program trap handler */
void p5gen() {
  unsigned int exeCode = pFiveSupport.sup_exceptState[GENERALEXCEPT].cause;
  switch (exeCode) {
  // store access fault
  case BUSERROR:
    print_term0("Bus Error (as expected): Access non-existent memory\n");
    pFiveSupport.sup_exceptState[GENERALEXCEPT].pc_epc =
        (memaddr)p5a; /* Continue with p5a() */
    break;

  // user mode syscall
  case ADDRERROR:
    print_term0("Address Error (as expected): non-kuseg access w/KU=1\n");
    /* return in kernel mode */
    pFiveSupport.sup_exceptState[GENERALEXCEPT].status |= MSTATUS_MPP_M;
    pFiveSupport.sup_exceptState[GENERALEXCEPT].pc_epc =
        (memaddr)p5b; /* Continue with p5b() */
    break;

  // machine/kernel mode syscall
  case MSYSCALLEXCPT:
    p5sys();
    break;

  default:
    print_term0("ERROR: other program trap\n");
    PANIC(); // to avoid sys call looping just exit the program
  }

  LDST(&(pFiveSupport.sup_exceptState[GENERALEXCEPT]));
}

/* p5's memory management trap handler */
void p5mm() {
  print_term0("memory management trap\n");

  support_t *pFiveSupAddr;
  ssi_payload_t getsup_payload = {
      .service_code = GETSUPPORTPTR,
      .arg = NULL,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload),
          0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&pFiveSupAddr),
          0);

  if (pFiveSupAddr != &pFiveSupport)
    print_term0("ERROR: support structure addresses are not the same\n");
  else
    print_term0("Correct Support Structure Address\n");

  // turn off kernel mode
  pFiveSupport.sup_exceptState[PGFAULTEXCEPT].status &= (~MSTATUS_MPP_M);
  // pFiveSupport.sup_exceptState[PGFAULTEXCEPT].status &= (~0x800);
  pFiveSupport.sup_exceptState[PGFAULTEXCEPT].pc_epc =
      (memaddr)p5b; /* return to p5b()  */

  LDST(&(pFiveSupport.sup_exceptState[PGFAULTEXCEPT]));
}

/* p5's SYS trap handler */
void p5sys() {
  print_term0("p5sys enter\n");
  unsigned int p5status = pFiveSupport.sup_exceptState[GENERALEXCEPT].status;
  p5status &= MSTATUS_MPP_MASK;
  switch (p5status) {
  case MSTATUS_MPP_U:
    print_term0("ERROR: high level SYS call from USER mode process (should be "
                "in kernel mode)\n");
    break;

  case MSTATUS_MPP_M:
    print_term0("High level SYS call from kernel mode process\n");
    break;
  default:
    print_term0("ERROR: p5sys cannot recognise the status\n");
  }
  pFiveSupport.sup_exceptState[GENERALEXCEPT].pc_epc +=
      4; /* to avoid SYS looping */

  LDST(&(pFiveSupport.sup_exceptState[GENERALEXCEPT]));
}

/* p5 -- SYS5 test process */
void p5() {
  SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);

  print_term0("p5 starts\n");
  /* cause a pgm trap access some non-existent memory */
  *p5MemLocation = *p5MemLocation + 1; /* Should cause a program trap */
}

void p5a() {
  print_term0("p5a starts\n");
  /* generage a TLB exception after a TLB-Refill event */
  p5MemLocation = (memaddr *)0x80000000;
  *p5MemLocation = 42;
}

/* second part of p5 - should be entered in user mode first time through */
/* should generate a program trap (Address error) */
void p5b() {
  // should trigger a program trap bacause process it's in user mode
  SYSCALL(1, 0, 0, 0);

  print_term0("p5 ok\n");
  terminate_process(SELF);

  /* should have terminated, so should not get to this point */
  print_term0("ERROR: p5 didn't terminate\n");
  PANIC();
}

/*p6 -- high level syscall without initializing passup vector */
void p6() {
  SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);
  print_term0("p6 starts\n");

  SYSCALL(1, 0, 0,
          0); /* should cause termination because p6 has no trap vector */

  print_term0("ERROR: p6 alive after SYS9() with no trap vector\n");
  PANIC();
}

/*p7 -- program trap without initializing passup vector */
void p7() {
  SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);
  print_term0("p7 starts\n");

  *((memaddr *)BADADDR) = 0;

  print_term0("ERROR: p7 alive after program trap with no trap vector\n");
  PANIC();
}

void p8() {
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ANYMESSAGE, 0, 0);
  print_term0("p8 starts\n");

  SYSCALL(SENDMESSAGE, (unsigned int)p8_pcb, 0, 0);
  pcb_PTR sender =
      (pcb_PTR)SYSCALL(RECEIVEMESSAGE, (unsigned int)ANYMESSAGE, 0, 0);

  if (sender != p8_pcb)
    print_term0("ERROR: sender not equal to p8\n");

  print_term0("p8 ok\n");

  SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
  terminate_process(SELF);

  /* just did a TERMPROCESS, so should not get to this point */
  print_term0("ERROR: p8 didn't terminate\n");
  PANIC();
}

/* p8root -- test of termination of subtree of processes              */
/* create a subtree of processes, wait for the leaves to block, signal*/
/* the root process, and then terminate                               */
void p8root() {
  SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);

  print_term0("p8root starts\n");

  // create two child
  child1_pcb = create_process(&child1state);
  child2_pcb = create_process(&child2state);

  // wait a message from every grandchild
  for (int i = 0; i < NOLEAVES; i++)
    SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);

  // notify test_pcb
  SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
  terminate_process(SELF);

  print_term0("ERROR: p8root didn't terminate\n");
  PANIC();
}

/*child1 & child2 -- create two sub-processes each*/

void child1() {
  print_term0("child1 starts\n");

  int pidc1;
  ssi_payload_t get_process_payload = {
      .service_code = GETPROCESSID,
      .arg = (void *)1,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&get_process_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&pidc1), 0);

  if (pidc1 != p8pid)
    print_term0("Inconsistent (parent) process id for p8's first child\n");

  gchild1_pcb = create_process(&gchild1state);
  gchild2_pcb = create_process(&gchild2state);

  SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
}

void child2() {
  print_term0("child2 starts\n");

  int pidc2;
  ssi_payload_t get_process_payload = {
      .service_code = GETPROCESSID,
      .arg = (void *)1,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&get_process_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&pidc2), 0);

  if (pidc2 != p8pid)
    print_term0("Inconsistent (parent) process id for p8's first child\n");

  gchild3_pcb = create_process(&gchild3state);
  gchild4_pcb = create_process(&gchild4state);

  SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
}

/*p8leaf -- code for leaf processes*/

void p8leaf1() {
  print_term0("leaf process (1) starts\n");
  SYSCALL(SENDMESSAGE, (unsigned int)p8root_pcb, 0, 0);
  SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
}

void p8leaf2() {
  print_term0("leaf process (2) starts\n");
  SYSCALL(SENDMESSAGE, (unsigned int)p8root_pcb, 0, 0);
  SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
}

void p8leaf3() {
  print_term0("leaf process (3) starts\n");
  SYSCALL(SENDMESSAGE, (unsigned int)p8root_pcb, 0, 0);
  SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
}

void p8leaf4() {
  print_term0("leaf process (4) starts\n");
  SYSCALL(SENDMESSAGE, (unsigned int)p8root_pcb, 0, 0);
  SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
}

void p9() {
  SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
  print_term0("p9 starts\n");
  /* create p10 */
  p10_pcb = create_process(&p10state);
  SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
  SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
}

void p10() {
  print_term0("p10 starts\n");
  int pid10;
  ssi_payload_t get_process_payload = {
      .service_code = GETPROCESSID,
      .arg = (void *)1,
  };
  SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb,
          (unsigned int)(&get_process_payload), 0);
  SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&pid10), 0);

  if (pid10 != p9pid)
    print_term0("Inconsistent process id for p9!\n");

  print_term0("p9 and p10 ok\n");
  SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
  terminate_process(SELF);

  print_term0("ERROR: p10 didn't die with its parent!\n");
  PANIC();
}

void hp_p1() {
  print_term0("hp_p1 starts\n");

  terminate_process(SELF);

  print_term0("ERROR: hp_p1 didn't die!\n");
  PANIC();
}

void hp_p2() {
  print_term0("hp_p2 starts\n");

  for (int i = 0; i < 100; i++)
    clockwait_process();

  terminate_process(SELF);

  print_term0("ERROR: hp_p2 didn't die!\n");
  PANIC();
}
