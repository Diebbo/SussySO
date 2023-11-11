/*********************************P1TEST.C*******************************
 *
 *	Test program for the modules ASL and pcbQueues (phase 1).
 *
 *	Produces progress messages on terminal 0 in addition
 *		to the array ``okbuf[]''
 *		Error messages will also appear on terminal 0 in
 *		addition to the array ``errbuf[]''.
 *
 *		Aborts as soon as an error is detected.
 *
 *      Modified by Michael Goldweber on May 15, 2004
 */

#include "../headers/const.h"
#include "../headers/types.h"

#include <uriscv/liburiscv.h>
#include "./headers/pcb.h"
#include "./headers/msg.h"

char   okbuf[2048]; /* sequence of progress messages */
char   errbuf[128]; /* contains reason for failing */
char   msgbuf[128]; /* nonrecoverable error message before shut down */
pcb_t *procp[MAXPROC], *p, *q, *firstproc, *lastproc, *midproc;
char  *mp = okbuf;
msg_t *msg[MAXMESSAGES], *m, *n, *firstmsg, *lastmsg, *midmsg;


#define TRANSMITTED 5
#define ACK         1
#define PRINTCHR    2
#define CHAROFFSET  8
#define STATUSMASK  0xFF
#define TERM0ADDR   0x10000254

typedef unsigned int devreg;

/* This function returns the terminal transmitter status value given its address */
devreg termstat(memaddr *stataddr) {
    return ((*stataddr) & STATUSMASK);
}

/* This function prints a string on specified terminal and returns TRUE if
 * print was successful, FALSE if not   */
unsigned int termprint(char *str, unsigned int term) {
    memaddr     *statusp;
    memaddr     *commandp;
    devreg       stat;
    devreg       cmd;
    unsigned int error = FALSE;

    if (term < DEVPERINT) {
        /* terminal is correct */
        /* compute device register field addresses */
        statusp  = (devreg *)(TERM0ADDR + (term * DEVREGSIZE) + (TRANSTATUS * DEVREGLEN));
        commandp = (devreg *)(TERM0ADDR + (term * DEVREGSIZE) + (TRANCOMMAND * DEVREGLEN));

        /* test device status */
        stat = termstat(statusp);
        if (stat == READY || stat == TRANSMITTED) {
            /* device is available */

            /* print cycle */
            while (*str != EOS && !error) {
                cmd       = (*str << CHAROFFSET) | PRINTCHR;
                *commandp = cmd;

                /* busy waiting */
                stat = termstat(statusp);
                while (stat == BUSY)
                    stat = termstat(statusp);

                /* end of wait */
                if (stat != TRANSMITTED)
                    error = TRUE;
                else
                    /* move to next char */
                    str++;
            }
        } else
            /* device is not available */
            error = TRUE;
    } else
        /* wrong terminal device number */
        error = TRUE;

    return (!error);
}


/* This function placess the specified character string in okbuf and
 *	causes the string to be written out to terminal0 */
void addokbuf(char *strp) {
    char *tstrp = strp;
    while ((*mp++ = *strp++) != '\0')
        ;
    mp--;
    termprint(tstrp, 0);
}


/* This function placess the specified character string in errbuf and
 *	causes the string to be written out to terminal0.  After this is done
 *	the system shuts down with a panic message */
void adderrbuf(char *strp) {
    char *ep    = errbuf;
    char *tstrp = strp;

    while ((*ep++ = *strp++) != '\0')
        ;

    termprint(tstrp, 0);

    PANIC();
}



int main(void) {
    int i;

    initPcbs();
    addokbuf("Initialized process control blocks   \n");

    /* Check allocProc */
    for (i = 0; i < MAXPROC; i++) {
        if ((procp[i] = allocPcb()) == NULL)
            adderrbuf("allocPcb: unexpected NULL   ");
    }
    if (allocPcb() != NULL) {
        adderrbuf("allocPcb: allocated more than MAXPROC entries   ");
    }
    addokbuf("allocPcb ok   \n");

    /* return the last 10 entries back to free list */
    for (i = 10; i < MAXPROC; i++)
        freePcb(procp[i]);
    addokbuf("freed 10 entries   \n");

    /* create a 10-element process queue */
    struct list_head qa;
    mkEmptyProcQ(&qa);
    if (!emptyProcQ(&qa))
        adderrbuf("emptyProcQ: unexpected FALSE   ");
    addokbuf("Inserting...   \n");
    for (i = 0; i < 10; i++) {
        if ((q = allocPcb()) == NULL)
            adderrbuf("allocPcb: unexpected NULL while insert   ");
        switch (i) {
            case 0:
                firstproc = q;
                break;
            case 5:
                midproc = q;
                break;
            case 9:
                lastproc = q;
                break;
            default:
                break;
        }
        insertProcQ(&qa, q);
    }
    addokbuf("inserted 10 elements   \n");

    if (emptyProcQ(&qa))
        adderrbuf("emptyProcQ: unexpected TRUE");

    /* Check outProc and headProc */
    if (headProcQ(&qa) != firstproc)
        adderrbuf("headProcQ failed   ");
    q = outProcQ(&qa, firstproc);
    if (q == NULL || q != firstproc)
        adderrbuf("outProcQ failed on first entry   ");
    freePcb(q);
    q = outProcQ(&qa, midproc);
    if (q == NULL || q != midproc)
        adderrbuf("outProcQ failed on middle entry   ");
    freePcb(q);
    if (outProcQ(&qa, procp[0]) != NULL)
        adderrbuf("outProcQ failed on nonexistent entry   ");
    addokbuf("outProcQ ok   \n");

    /* Check if removeProc and insertProc remove in the correct order */
    addokbuf("Removing...   \n");
    for (i = 0; i < 8; i++) {
        if ((q = removeProcQ(&qa)) == NULL)
            adderrbuf("removeProcQ: unexpected NULL   ");
        freePcb(q);
    }
    if (q != lastproc)
        adderrbuf("removeProcQ: failed on last entry   ");
    if (removeProcQ(&qa) != NULL)
        adderrbuf("removeProcQ: removes too many entries   ");

    if (!emptyProcQ(&qa))
        adderrbuf("emptyProcQ: unexpected FALSE   ");

    addokbuf("insertProcQ, removeProcQ and emptyProcQ ok   \n");
    addokbuf("process queues module ok      \n");

    addokbuf("checking process trees...\n");

    if (!emptyChild(procp[2]))
        adderrbuf("emptyChild: unexpected FALSE   ");

    /* make procp[1] through procp[9] children of procp[0] */
    addokbuf("Inserting...   \n");
    for (i = 1; i < 10; i++) {
        insertChild(procp[0], procp[i]);
    }
    addokbuf("Inserted 9 children   \n");

    if (procp[0]->p_child.next != &(procp[1]->p_sib))
        adderrbuf("insertChild: p_child.next should point to the p_sib of the first child   ");

    if (emptyChild(procp[0]))
        adderrbuf("emptyChild: unexpected TRUE   ");

    /* Check outChild */
    q = outChild(procp[1]);
    if (q == NULL || q != procp[1])
        adderrbuf("outChild failed on first child   ");
    q = outChild(procp[4]);
    if (q == NULL || q != procp[4])
        adderrbuf("outChild failed on middle child   ");
    if (outChild(procp[0]) != NULL)
        adderrbuf("outChild failed on nonexistent child   ");
    addokbuf("outChild ok   \n");

    /* Check removeChild */
    addokbuf("Removing...   \n");
    for (i = 0; i < 7; i++) {
        if ((q = removeChild(procp[0])) == NULL)
            adderrbuf("removeChild: unexpected NULL   ");
    }
    if (removeChild(procp[0]) != NULL)
        adderrbuf("removeChild: removes too many children   ");

    if (!emptyChild(procp[0]))
        adderrbuf("emptyChild: unexpected FALSE   ");

    addokbuf("insertChild, removeChild and emptyChild ok   \n");
    addokbuf("process tree module ok      \n");

    for (i = 0; i < 10; i++)
        freePcb(procp[i]);

    initMsgs();
    addokbuf("Initialized messages   \n");

    /* Check allocMsg */
    for (i = 0; i < MAXMESSAGES; i++) {
        if ((msg[i] = allocMsg()) == NULL)
            adderrbuf("allocMsg: unexpected NULL   ");
    }
    if (allocMsg() != NULL) {
        adderrbuf("allocMsg: allocated more than MAXMESSAGES entries   ");
    }
    addokbuf("allocMsg ok   \n");

    /* return the last 15 entries back to free list */
    for (i = 5; i < MAXMESSAGES; i++)
        freeMsg(msg[i]);
    addokbuf("freed 15 entries   \n");

    struct list_head qm;
    mkEmptyMessageQ(&qm);
    if (!emptyMessageQ(&qm))
        adderrbuf("emptyMessageQ: unexpected FALSE   ");
    addokbuf("Inserting...   \n");
    for (i = 0; i < 10; i++) {
        if ((m = allocMsg()) == NULL)
            adderrbuf("allocMsg: unexpected NULL while insert   ");
        switch (i) {
            case 0:
                firstmsg = m;
                break;
            case 5:
                midmsg = m;
                break;
            case 9:
                lastmsg = m;
                break;
            default:
                break;
        }
        insertMessage(&qm, m);
    }
    addokbuf("inserted 10 elements   \n");

    if (emptyMessageQ(&qm))
        adderrbuf("emptyMessageQ: unexpected TRUE");

    /* Check outProc and headProc */
    if (headMessage(&qm) != firstmsg)
        adderrbuf("headMessage failed   ");
    addokbuf("headMessage ok   \n");
    n = popMessage(&qm, NULL);
    if (n == NULL || n != firstmsg)
        adderrbuf("popMessage failed with NULL pcb   ");
    freeMsg(n);
    addokbuf("popMessage with NULL pcb ok   \n");
    n = allocMsg();
    pushMessage(&qm, n);
    if (headMessage(&qm) != n)
        adderrbuf("pushMessage failed   ");
    freeMsg(n);
    addokbuf("pushMessage ok   \n");
    n = allocMsg();
    insertMessage(&qm, n);
    if (container_of(list_next(&qm), msg_t, m_list) != n)
        adderrbuf("insertMessage   ");
    freeMsg(n);
    addokbuf("insertMessage ok   \n");
    m = allocMsg();
    m->m_sender = p;
    insertMessage(&qm, m);
    n = allocMsg();
    n->m_sender = p;
    insertMessage(&qm, n);
    msg_t* o = popMessage(&qm, p);
    if (o == NULL || o != m)
        adderrbuf("popMessage failed with pcb   ");
    addokbuf("popMessage with pcb ok   \n");

    addokbuf("messages queue module ok      \n");

    addokbuf("So long, and thanks for all the fish\n");
    return 0;
}
