/* Buffer cache.
 *
 * The buffer cache is a linked list of buf structures holding
 * cached copies of disk block contents.  Caching disk blocks
 * in memory reduces the number of disk reads and also provides
 * a synchronization point for disk blocks used by multiple processes.
 *
 * Interface:
 * * To get a buffer for a particular disk block, call bread.
 * * After changing buffer data, call bwrite to write it to disk.
 * * When done with the buffer, call brelse.
 * * Do not use the buffer after calling brelse.
 * * Only one process at a time can use a buffer,
 *     so do not keep them longer than necessary.
 *
 * The implementation uses two state flags internally:
 * * B_VALID: the buffer data has been read from the disk.
 * * B_DIRTY: the buffer data has been modified
 *     and needs to be written to disk.
 */

#include "spinlock.h"
#include "sleeplock.h"
#include "buf.h"
#include "console.h"
#include "sd.h"
#include "fs.h"

struct {
    struct spinlock lock;
    struct buf buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // head.next is most recently used.
    struct buf head;
} bcache;

/* Initialize the cache list and locks. */
void
binit()
{
    /* TODO: Your code here. */
    initlock(&bcache.lock, "bcache");
    init_buf_head(&bcache.head);
    
    for (struct buf* b = bcache.buf; b < bcache.buf + NBUF; b++) {
        b->dev = -1;
        buf_add(&bcache.head, b);
        initsleeplock(&b->lock, "buffer");
    }
}

/*
 * Look through buffer cache for block on device dev.
 * If not found, allocate a buffer.
 * In either case, return locked buffer.
 */
static struct buf *
bget(uint32_t dev, uint32_t blockno)
{
    /* TODO: Your code here. */
    acquire(&bcache.lock);
    
    loop:
    for (struct buf* b = bcache.head.next; b != &bcache.head; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            if (!holdingsleep(&b->lock)) {
                b->refcnt++;
                acquiresleep(&b->lock);
                release(&bcache.lock);
                return b;
            }
            sleep(b, &bcache.lock);
            goto loop;
        }
    }
    
    // not hit
    for (struct buf* b = bcache.head.prev; b != &bcache.head; b = b->prev) {
        if ((!holdingsleep(&b->lock)) && (b->flags & B_DIRTY) == 0 && b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            acquiresleep(&b->lock);
            b->flags = 0;
            b->refcnt = 1;
            release(&bcache.lock);
            return b;
        }
    }

    panic("bget: no buffers\n");
}

/* Return a locked buf with the contents of the indicated block. */
struct buf *
bread(uint32_t dev, uint32_t blockno)
{
    /* TODO: Your code here. */
    struct buf *b = bget(dev, blockno + MBR_BASE);

    if ((b->flags & B_VALID) == 0) {
        sdrw(b);
    }

    return b;
}

/* Write b's contents to disk. Must be locked. */
void
bwrite(struct buf *b)
{
    /* TODO: Your code here. */
    if (!holdingsleep(&b->lock)) {
        panic("bwrite");
    }

    b->flags |= B_DIRTY;
    sdrw(b);
}

/*
 * Release a locked buffer.
 * Move to the head of the MRU list.
 */
void
brelse(struct buf *b)
{
    /* TODO: Your code here. */
    acquire(&bcache.lock);
    if (!holdingsleep(&b->lock)) {
        panic("brelse\n");
    } 
    releasesleep(&b->lock);

    if (--b->refcnt == 0) {
        buf_del(b);
        buf_add(&bcache.head, b);
        wakeup(b);
    } 
    
    release(&bcache.lock);
}

void
bpin(struct buf *b) {
    acquire(&bcache.lock);
    b->refcnt++;
    release(&bcache.lock);
}

void
bunpin(struct buf *b) {
    acquire(&bcache.lock);
    b->refcnt--;
    release(&bcache.lock);
}