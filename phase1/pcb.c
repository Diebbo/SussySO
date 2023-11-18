#include "./headers/pcb.h"

static pcb_t pcbTable[MAXPROC];  /* PCB array with maximum size 'MAXPROC' */
LIST_HEAD(pcbFree_h);            /* List of free PCBs                     */
static int next_pid = 1;

void initPcbs() {
    
}

void freePcb(pcb_t *p) {
    // aggiungiamo alla lista dei processi eliminati
    if(!list_empty(pcbFree_h))
        INIT_LIST_HEAD(p);
    else
        __list_add_tail(p, pcbFree_h);

}

pcb_t *allocPcb() {
    if(list_empty(pcbFree_h))
        return NULL;

    pcb_t *p = container_of(pcbFree_h->next);

    list_del(pcbFree_h);

    // ri-inizializzo l'elemento
    p->p_list = NULL;
    p->p_parent = NULL;
    p->p_child = NULL;
    p->p_sib = NULL;

    p->p_s = 0;
    p->p_time = 0;

    p->msg_inbox = NULL;
    p->p_supportStruct = NULL;
    p->p_pid = 0;
}

void mkEmptyProcQ(struct list_head *head) {
}

int emptyProcQ(struct list_head *head) {
}

void insertProcQ(struct list_head* head, pcb_t* p) {
}

pcb_t* headProcQ(struct list_head* head) {
}

pcb_t* removeProcQ(struct list_head* head) {
}

pcb_t* outProcQ(struct list_head* head, pcb_t* p) {
}

int emptyChild(pcb_t *p) {
}

void insertChild(pcb_t *prnt, pcb_t *p) {
}

pcb_t* removeChild(pcb_t *p) {
}

pcb_t* outChild(pcb_t *p) {
}
