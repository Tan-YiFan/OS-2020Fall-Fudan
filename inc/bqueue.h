#ifndef BQUEUE_H
#define BQUEUE_H
#include "spinlock.h"
#include "buf.h"
#include "proc.h"
#include "string.h"

#define MAX_Q 0x1000
struct {
    struct spinlock lock;
    // struct buf* head;
    // struct buf* tail;
    int head, tail;
    struct buf q[MAX_Q];
} bqueue;
// struct buf initbuf;
static void bqueue_init()
{
    initlock(&bqueue.lock, "buf queue");
    // bqueue.head = &initbuf;
    // bqueue.tail = &initbuf;
    bqueue.head = 0;
    bqueue.tail = 0;
}

static struct buf* bqueue_push(struct buf* buf)
{
    // acquire(&bqueue.lock);
    if (buf == 0) {
        // release(&bqueue.lock);
        return 0;
    }
    // bqueue.tail->next = buf;
    // bqueue.tail = buf;
    // if ((bqueue.tail + 1) % MAX_Q == bqueue.head) {
    //     sleep(&bqueue.q[bqueue.tail], &bqueue.lock);
    // }
    bqueue.tail++;
    bqueue.tail %= MAX_Q;
    // bqueue.q[bqueue.tail] = *buf;
    if (buf->flags & B_DIRTY) {
        memcpy(&bqueue.q[bqueue.tail].data, buf->data, sizeof(buf->data));
        bqueue.q[bqueue.tail].blockno = buf->blockno;
        bqueue.q[bqueue.tail].flags = buf->flags;
    }
    else if (!buf->flags) {
        bqueue.q[bqueue.tail].blockno = buf->blockno;
        bqueue.q[bqueue.tail].flags = buf->flags;
    }

    // sleep(buf, &bqueue.lock);
    // release(&bqueue.lock);
    return &bqueue.q[bqueue.tail];
}

static struct buf* bqueue_pop()
{
    // acquire(&bqueue.lock);
    if (bqueue.head == bqueue.tail) {
        // release(&bqueue.lock);
        return 0;
    }
    struct buf* ret;
    // ret = bqueue.head->next;
    // bqueue.head = bqueue.head->next;
    bqueue.head++;
    bqueue.head %= MAX_Q;
    ret = &bqueue.q[bqueue.head];
    if (bqueue.head == bqueue.tail) {
        // bqueue.head = &initbuf;
        // bqueue.tail = &initbuf;
        // bqueue.head = 0;
        // bqueue.tail = 0;
    }
    // wakeup((void*)ret);
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
    // ret = bqueue.head->next;
    ret = &bqueue.q[(bqueue.head + 1) % MAX_Q];
    // release(&bqueue.lock);
    return ret;
}

static int bqueue_empty()
{
    return bqueue.head == bqueue.tail;
}

#endif