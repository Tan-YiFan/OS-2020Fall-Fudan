#ifndef INC_BUF_H
#define INC_BUF_H

#include <stdint.h>
#include "sleeplock.h"
#include "fs.h"

#define B_BUSY  0x1  // buffer is locked by some process
#define B_VALID 0x2     /* Buffer has been read from disk. */
#define B_DIRTY 0x4     /* Buffer needs to be written to disk. */

struct buf {
    int flags;
    uint32_t dev;
    uint32_t blockno;
    uint32_t refcnt;
    uint8_t data[BSIZE];
    struct sleeplock lock;

    /* TODO: Your code here. */
    struct buf* prev;
    struct buf* next;
};

void        binit();
void        bwrite(struct buf *b);
void        brelse(struct buf *b);
struct buf* bread(uint32_t dev, uint32_t blockno);
void bpin(struct buf* b);
void bunpin(struct buf* b);

static inline void init_buf_head(struct buf* head) {
    head->next = head;
    head->prev = head;
}

// insert at head
static inline void buf_add(struct buf* head, struct buf* new) {
    new->next = head->next;
    new->prev = head;
    head->next->prev = new;
    head->next = new;
}

// insert at tail
static inline void buf_append(struct buf* head, struct buf* new) {
    buf_add(head->prev, new);
}

static inline void buf_del(struct buf* b) {
    b->next->prev = b->prev;
    b->prev->next = b->next;
}

static inline int buf_isempty(struct buf* head) {
    return (head == head->next && head == head->prev);
}

#endif
