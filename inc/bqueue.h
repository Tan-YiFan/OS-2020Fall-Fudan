#ifndef BQUEUE_H
#define BQUEUE_H
#include "spinlock.h"
#include "buf.h"
struct {
    struct spinlock lock;
    struct buf* head;
    struct buf* tail;
} bqueue;

static void bqueue_init()
{
    initlock(&bqueue.lock, "buf queue");
    bqueue.head = 0;
    bqueue.tail = 0;
}

static void bqueue_push(struct buf* buf)
{
    // acquire(&bqueue.lock);
    if (buf == 0) {
        // release(&bqueue.lock);
        return;
    }
    bqueue.tail->next = buf;
    bqueue.tail = buf;
    // release(&bqueue.lock);
    return;
}

static struct buf* bqueue_pop()
{
    // acquire(&bqueue.lock);
    if (bqueue.head == bqueue.tail) {
        // release(&bqueue.lock);
        return 0;
    }
    struct buf* ret;
    ret = bqueue.head;
    bqueue.head = bqueue.head->next;
    // release(&bqueue.lock);
    return ret;
}

static struct buf* bqueue_front()
{
    // acquire(&bqueue.lock);
    if (bqueue.head == bqueue.tail) {
        // release(&bqueue.lock);
        return 0;
    }
    struct buf* ret;
    ret = bqueue.head;
    // release(&bqueue.lock);
    return ret;
}

static int bqueue_empty()
{
    return bqueue.head == bqueue.tail;
}

#endif