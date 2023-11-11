#ifndef PANDOS_TYPES_H_INCLUDED
#define PANDOS_TYPES_H_INCLUDED

/****************************************************************************
 *
 * This header file contains utility types definitions.
 *
 ****************************************************************************/

#include <uriscv/types.h>
#include "./const.h"
#include "./listx.h"

typedef signed int cpu_t;
typedef unsigned int memaddr;

/* Page Table Entry descriptor */
typedef struct pteEntry_t {
    unsigned int pte_entryHI;
    unsigned int pte_entryLO;
} pteEntry_t;


/* Support level context */
typedef struct context_t {
    unsigned int stackPtr;
    unsigned int status;
    unsigned int pc;
} context_t;


/* Support level descriptor */
typedef struct support_t {
    int        sup_asid;                        /* process ID					*/
    state_t    sup_exceptState[2];              /* old state exceptions			*/
    context_t  sup_exceptContext[2];            /* new contexts for passing up	*/
    pteEntry_t sup_privatePgTbl[USERPGTBLSIZE]; /* user page table				*/
    struct list_head s_list;
} support_t;

/* process table entry type */
typedef struct pcb_t
{
    /* process queue  */
    struct list_head p_list;

    /* process tree fields */
    struct pcb_t *p_parent;   /* ptr to parent	*/
    struct list_head p_child; /* children list */
    struct list_head p_sib;   /* sibling list  */

    /* process status information */
    state_t p_s;  /* processor state */
    cpu_t p_time; /* cpu time used by proc */

    /* First message in the message queue */
    struct list_head msg_inbox;

    /* Pointer to the support struct */
    support_t *p_supportStruct;

    /* process id */
    int p_pid;
} pcb_t, *pcb_PTR;

/* message entry type */
typedef struct msg_t
{
    /* message queue */
    struct list_head m_list;

    /* thread that sent this message */
    struct pcb_t *m_sender;

    /* the payload of the message */
	unsigned int m_payload;
} msg_t, *msg_PTR;

typedef struct ssi_payload_t
{
    int service_code;
    void *arg;
} ssi_payload_t, *ssi_payload_PTR;

typedef struct ssi_create_process_t
{
    state_t *state;
    support_t *support;
} ssi_create_process_t, *ssi_create_process_PTR;

typedef struct ssi_do_io_t
{
    memaddr* commandAddr;
    unsigned int commandValue;
} ssi_do_io_t, *ssi_do_io_PTR;

#endif
