#include "./headers/pcb.h"

static pcb_t pcbTable[MAXPROC];  /* PCB array with maximum size 'MAXPROC' */
LIST_HEAD(pcbFree_h); /* List head for the free PCBs */
int next_pid = 1; /* Next PID to be assigned */

int isInList(struct list_head *target_process, int pid) {
  pcb_PTR tmp;
  list_for_each_entry(tmp, target_process, p_list) {
    if (tmp->p_pid == pid)
      return TRUE;
  }
  return FALSE;
}

void initPcb(pcb_PTR p){
    // Reset PCB members if needed
    p->p_parent = NULL;
    // Initialize p_child, p_sib, msg_inbox as an empty list
    INIT_LIST_HEAD(&p->p_child); 
    INIT_LIST_HEAD(&p->p_sib);   
    p->p_time = 0;
    p->p_time = 0;                                                          
    INIT_LIST_HEAD(&p->msg_inbox); 
    p->p_supportStruct = NULL;

    // Update p_list for the allocated PCB
    INIT_LIST_HEAD(&p->p_list);                                            

    // Reset the processor state
    p->p_s.entry_hi = 0;
    p->p_s.cause = 0;
    p->p_s.status = 0;
    p->p_s.pc_epc = 0;
    p->p_s.mie = 0;

    p->p_pid = 1;
}

void initPcbs() {
    // Initialize the list head for the free 
    // assum to be already initialized

    for (int i = 0; i < MAXPROC; ++i) {
        // Initialize the PCB array
        initPcb(&pcbTable[i]);
        list_add(&pcbTable[i].p_list, &pcbFree_h);
    }

}

int isFree(int p_pid){
    return isInList(&pcbFree_h, p_pid);
}

void freePcb(pcb_t *p) {
    if (list_empty(&pcbFree_h)) {                                           
        // If the free list is empty, initialize it
        INIT_LIST_HEAD(&pcbFree_h);
    }
    
    // Add the PCB to the tail of the existing free list
    list_add_tail(&p->p_list, &pcbFree_h);
}

pcb_t *allocPcb() {                                                         
    if (list_empty(&pcbFree_h))
        return NULL;

    struct list_head *head = pcbFree_h.next; // Get the first element
    list_del(head); // Remove the PCB from the free list                    

    // Convert the list_head to the PCB type
    pcb_t *p = container_of(head, pcb_t, p_list);                           

    // Initialize the PCB fields
    initPcb(p);

    // Assign the next PID to the PCB
    p->p_pid = next_pid++;

    return p;
}

void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD(head);     
}

int emptyProcQ(struct list_head *head) {
    return list_empty(head) ? 1 : 0;                                       
}

void insertProcQ(struct list_head* head, pcb_t* p) {
    list_add_tail(&p->p_list, head);                                       
}

pcb_t* headProcQ(struct list_head* head) {
    if (emptyProcQ(head)) {
        return NULL; // Queue is empty, return NULL
    } 
    // Traverse to the first element and return its pointer
    return container_of(head->next, pcb_t, p_list);                    
        
}

pcb_t* removeProcQ(struct list_head* head) {
    if (list_empty(head)) {
        return NULL; // Queue is empty, return NULL
    } 
    struct list_head *removed = head->next; 
    list_del(removed); // Remove the element from the queue       
    return container_of(removed, pcb_t, p_list); 
}

pcb_t* outProcQ(struct list_head* head, pcb_t* p) {
    if (!p || list_empty(head)) {
        return NULL; // Invalid PCB pointer or empty queue, return NULL
    }

    struct list_head *pos;                                                 
    list_for_each(pos, head) {                                           
        pcb_t *current_pcb = container_of(pos, pcb_t, p_list);
        if (current_pcb->p_pid == p->p_pid) {
            // Found the PCB to be removed, delete it from the queue
            list_del(pos);
            return p;
        }
    }

    return NULL; // 'p' not found in the queue, return NULL    
}

int emptyChild(pcb_t *p) {
    return list_empty(&p->p_child);                                         
}

void insertChild(pcb_t *prnt, pcb_t *p) {
    if (prnt == NULL || p == NULL) {
        // Invalid pointers, cannot perform insertion
        return;
    }

    if (!list_empty(&prnt->p_child)) {
        // Parent already has children, add this as a sibling
        list_add_tail(&p->p_sib, &prnt->p_child);
    } else {
        // This is the first child, add it to the parent's child list
        INIT_LIST_HEAD(&prnt->p_child);                                    
        list_add(&p->p_sib, &prnt->p_child);                               
    }
    
    // Set the parent of the child
    p->p_parent = prnt;    
}

pcb_t* removeChild(pcb_t *p) {
    if (list_empty(&p->p_child)) {
        // No children to remove
        return NULL;
    }

    struct list_head *first_child = p->p_child.next;                            
    pcb_t *removed_child = container_of(first_child, pcb_t, p_sib);

    list_del(first_child);
    // Reinitialize the removed child's sibling pointer
    INIT_LIST_HEAD(&removed_child->p_sib);                                 

    removed_child->p_parent = NULL;

    return removed_child;   
}

pcb_t* outChild(pcb_t *p) {
    if (!p->p_parent) {
        // No parent, return NULL
        return NULL;
    }

    struct list_head *pos;
    pcb_t *current_child;
    list_for_each(pos, &p->p_parent->p_child) {                      
        current_child = container_of(pos, pcb_t, p_sib);
        if (current_child->p_pid == p->p_pid) {
            list_del(pos);
            // Reinitialize the removed child's sibling pointer and child's list head
            INIT_LIST_HEAD(&p->p_sib); 
            p->p_parent = NULL;
            return p;
        }
    }

    return NULL; // If p is not found in the parent's children list    
}
