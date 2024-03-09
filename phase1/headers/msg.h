#ifndef MSG_H_INCLUDED
#define MSG_H_INCLUDED

#include "../../headers/const.h"
#include "../../headers/types.h"
#include "../../headers/listx.h"

/*Initialize the msgFree list to contain all the elements of the static array of MAXMESSAGES
messages. This method will be called only once during data structure initialization.*/
void initMsgs();
/*Messages which are no longer in use can be returned to the mesgFree_h list by using the method*/
void freeMsg(msg_t *m);
/*Return NULL if the msgFree list is empty. Otherwise, remove an element from the msgFree
list, provide initial values for ALL of the messages fields and then return a pointer to the 
removed element.*/
msg_t *allocMsg();
/*Used to initialize a variable to be head pointer to a message queue; returns a pointer to the head
of an empty message queue, i.e. NULL.*/
void mkEmptyMessageQ(struct list_head *head);
/*Returns TRUE if the queue whose tail is pointed to by head is empty, FALSE otherwise.*/
int emptyMessageQ(struct list_head *head);
/*Insert the message pointed to by m at the end of the queue whose head pointer is pointed to by
head.*/
void insertMessage(struct list_head *head, msg_t *m);
/*Insert the message pointed to by m at the head of the queue whose head pointer is pointed to by
head.*/
void pushMessage(struct list_head *head, msg_t *m);
/*Remove the first element (starting by the head) from the message queue accessed via head whose
sender is p_ptr.
If p_ptr is NULL, return the first message in the queue. Return NULL if the message queue
was empty or if no message from p_ptr was found; otherwise return the pointer to the removed
message.*/
msg_t *popMessage(struct list_head *head, pcb_t *p_ptr);
/*Return a pointer to the first message from the queue whose head is pointed to by head. Do not
remove the message from the queue. Return NULL if the queue is empty.*/
msg_t *headMessage(struct list_head *head);

#endif
