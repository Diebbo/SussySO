#ifndef PCB_H_INCLUDED
#define PCB_H_INCLUDED

#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../headers/listx.h"
/*Initialize the pcbFree list to contain all the elements of the static array of MAXPROC PCBs.
This method will be called only once during data structure initialization.*/
void initPcbs();
//Insert the element pointed to by p onto the pcbFree list.
void freePcb(pcb_t *p);
/*Return NULL if the pcbFree list is empty. Otherwise, remove an element from the pcbFree
list, provide initial values for ALL of the PCBs fields and then return a pointer to the removed element.*/
pcb_t *allocPcb();
//This method is used to initialize a variable to be head pointer to a process queue.
void mkEmptyProcQ(struct list_head *head);
//Return TRUE if the queue whose head is pointed to by head is empty. Return FALSE otherwise.
int emptyProcQ(struct list_head *head);
//Insert the PCB pointed by p into the process queue whose head pointer is pointed to by head.
void insertProcQ(struct list_head *head, pcb_t *p);
/*Return a pointer to the first PCB from the process queue whose head is pointed to by head. Do
not remove this PCB from the process queue. Return NULL if the process queue is empty.*/
pcb_t *headProcQ(struct list_head *head);
/*Remove the first (i.e. head) element from the process queue whose head pointer is pointed to
by head. Return NULL if the process queue was initially empty; otherwise return the pointer
to the removed element.*/
pcb_t *removeProcQ(struct list_head *head);
/*Remove the PCB pointed to by p from the process queue whose head pointer is pointed to by
head. If the desired entry is not in the indicated queue (an error condition), return NULL;
otherwise, return p.*/
pcb_t *outProcQ(struct list_head *head, pcb_t *p);
//Return TRUE if the PCB pointed to by p has no children. Return FALSE otherwise.
int emptyChild(pcb_t *p);
//Make the PCB pointed to by p a child of the PCB pointed to by prnt.
void insertChild(pcb_t *prnt, pcb_t *p);
/*Make the first child of the PCB pointed to by p no longer a child of p. Return NULL if initially
there were no children of p. Otherwise, return a pointer to this removed first child PCB.*/
pcb_t *removeChild(pcb_t *p);
/*Make the PCB pointed to by p no longer the child of its parent. If the PCB pointed to by p has
no parent, return NULL; otherwise, return p.*/
pcb_t *outChild(pcb_t *p);

#endif
