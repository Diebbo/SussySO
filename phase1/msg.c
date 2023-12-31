#include "./headers/msg.h"

static msg_t msgTable[MAXMESSAGES];
LIST_HEAD(msgFree_h);

void initMsgs() {
    // Initialize list head for each message and ad it to the free list
    for (int i = 0; i < MAXMESSAGES; ++i) {
        INIT_LIST_HEAD(&msgTable[i].m_list); 
        msgTable[i].m_payload = 0;                                                           
        msgTable[i].m_sender = NULL;                                                         
        list_add(&msgTable[i].m_list, &msgFree_h); 
    }    
}

void freeMsg(msg_t *m) {
    if (m) {
        list_add(&m->m_list, &msgFree_h); 
    }    
}

msg_t *allocMsg() {
    if (list_empty(&msgFree_h)) {
        return NULL; // No available messages
    } else {
        // Get the first entry from the list and obtain its address                         
        struct list_head *entry = msgFree_h.next;
        list_del(entry); // Remove message from msgFree list

        // Retrieve the pointer to the actual message structure
        msg_t *m = container_of(entry, msg_t, m_list);

        // Initialize message fields
        m->m_sender = NULL;
        m->m_payload = 0;

        return m;
    }    
}

void mkEmptyMessageQ(struct list_head *head) {
    INIT_LIST_HEAD(head);                                                                   
}

int emptyMessageQ(struct list_head *head) {
    return list_empty(head);                                                                
}

void insertMessage(struct list_head *head, msg_t *m) {
    list_add_tail(&m->m_list, head);                                                        
}

void pushMessage(struct list_head *head, msg_t *m) {
    list_add(&m->m_list, head);                                                             
}

msg_t *popMessage(struct list_head *head, pcb_t *p_ptr) {
    struct list_head *pos;
    msg_t *msg = NULL;

    if (list_empty(head)) {
        return NULL; // Empty queue
    }

    list_for_each(pos, head) {
        msg = container_of(pos, msg_t, m_list);
        if (p_ptr == NULL || msg->m_sender == p_ptr) {                                      
            list_del(pos); // Remove the message from the list                              
            return msg;
        }
    }

    return NULL; // No message found for the given sender
}

msg_t *headMessage(struct list_head *head) {
    if (list_empty(head)) {
        return NULL; // Empty queue
    }

    struct list_head *first = head->next;                                                  
    return container_of(first, msg_t, m_list);                                             
}
