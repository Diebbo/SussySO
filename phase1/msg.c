#include "./headers/msg.h"

static msg_t msgTable[MAXMESSAGES];
LIST_HEAD(msgFree_h);

void initMsgs() {
}

void freeMsg(msg_t *m) {
}

msg_t *allocMsg() {
}

void mkEmptyMessageQ(struct list_head *head) {
}

int emptyMessageQ(struct list_head *head) {
}

void insertMessage(struct list_head *head, msg_t *m) {
}

void pushMessage(struct list_head *head, msg_t *m) {
}

msg_t *popMessage(struct list_head *head, pcb_t *p_ptr) {
}

msg_t *headMessage(struct list_head *head) {
}
