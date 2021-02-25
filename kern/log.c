#include "types.h"
#include "console.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "string.h"
#include "file.h"

/* Simple logging that allows concurrent FS system calls.
 *
 * A log transaction contains the updates of multiple FS system
 * calls. The logging system only commits when there are
 * no FS system calls active. Thus there is never
 * any reasoning required about whether a commit might
 * write an uncommitted system call's updates to disk.
 *
 * A system call should call begin_op()/end_op() to mark
 * its start and end. Usually begin_op() just increments
 * the count of in-progress FS system calls and returns.
 * But if it thinks the log is close to running out, it
 * sleeps until the last outstanding end_op() commits.
 *
 * The log is a physical re-do log containing disk blocks.
 * The on-disk log format:
 *   header block, containing block #s for block A, B, C, ...
 *   block A
 *   block B
 *   block C
 *   ...
 * Log appends are synchronous.
 */

/*
 * Contents of the header block, used for both the on-disk header block
 * and to keep track in memory of logged block# before commit.
 */
struct logheader {
    int n;
    int block[LOGSIZE];
};

struct log {
    struct spinlock lock;
    int start;
    int size;
    int outstanding;    // How many FS sys calls are executing.
    int committing;     // In commit(), please wait.
    int dev;
    struct logheader lh;
};
struct log log;

static void recover_from_log();
static void commit();

void
initlog(int dev)
{
    /* TODO: Your code here. */
    if (sizeof(struct logheader) > BSIZE) {
        panic("initlog: too big logheader\n");
    }
    initlock(&log.lock, "log");
    struct superblock sb;
    readsb(dev, &sb);
    log.start = sb.size - sb.nlog;
    log.size = sb.nlog;
    log.dev = dev;
    recover_from_log();
}

/* Copy committed blocks from log to their home location. */
static void
install_trans()
{
    /* TODO: Your code here. */
    for (int tail = 0; tail < log.lh.n; tail++) {
        struct buf* lbuf = bread(log.dev, log.start + tail + 1);
        struct buf* dbuf = bread(log.dev, log.lh.block[tail]);

        memmove(dbuf->data, lbuf->data, BSIZE);
        bwrite(dbuf);
        brelse(lbuf);
        brelse(dbuf);
    }
}

/* Read the log header from disk into the in-memory log header. */
static void
read_head()
{
    /* TODO: Your code here. */
    struct buf* b = bread(log.dev, log.start);
    struct logheader* lh = (struct logheader*)b->data;
    log.lh.n = lh->n;

    for (int i = 0; i < log.lh.n; i++) {
        log.lh.block[i] = lh->block[i];
    }
    brelse(b);
}

/*
 * Write in-memory log header to disk.
 * This is the true point at which the
 * current transaction commits.
 */
static void
write_head()
{
    struct buf *buf = bread(log.dev, log.start);
    struct logheader *hb = (struct logheader *) (buf->data);
    int i;
    hb->n = log.lh.n;
    for (i = 0; i < log.lh.n; i++) {
        hb->block[i] = log.lh.block[i];
    }
    bwrite(buf);
    brelse(buf);
}

static void
recover_from_log()
{
    /* TODO: Your code here. */
    read_head();
    install_trans();
    log.lh.n = 0;
    write_head();
}

/* Called at the start of each FS system call. */
void
begin_op()
{
    /* TODO: Your code here. */
    acquire(&log.lock);

    while (1) {
        if (log.committing) {
            sleep(&log, &log.lock);
        } else if (log.lh.n + (log.outstanding + 1) * MAXOPBLOCKS > LOGSIZE) {
            sleep(&log, &log.lock);
        } else {
            log.outstanding++;
            break;
        }
    }
    release(&log.lock);
}

/*
 * Called at the end of each FS system call.
 * Commits if this was the last outstanding operation.
 */
void
end_op()
{
    /* TODO: Your code here. */
    acquire(&log.lock);

    if (log.committing) {
        panic("end_op: log.commiting\n");
    }
    log.outstanding--;
    int do_commit;
    if (log.outstanding == 0) {
        do_commit = 1;
        log.committing = 1;
    } else {
        wakeup(&log);
    }
    release(&log.lock);

    if (do_commit) {
        commit();
        acquire(&log.lock);
        log.committing = 0;
        wakeup(&log);
        release(&log.lock);
    } 
}

/* Copy modified blocks from cache to log. */
static void
write_log()
{
    /* TODO: Your code here. */
    for (int tail = 0; tail < log.lh.n; tail++) {
        struct buf* to = bread(log.dev, log.start + tail + 1);
        struct buf* from = bread(log.dev, log.lh.block[tail]);

        memmove(to->data, from->data, BSIZE);
        bwrite(to);
        brelse(from);
        brelse(to);
    }
}

static void
commit()
{
    /* TODO: Your code here. */
    if (log.lh.n > 0) {
        write_log();
        write_head();
        install_trans();
        log.lh.n = 0;
        write_head();
    } 
}

/* Caller has modified b->data and is done with the buffer.
 * Record the block number and pin in the cache with B_DIRTY.
 * commit()/write_log() will do the disk write.
 *
 * log_write() replaces bwrite(); a typical use is:
 *   bp = bread(...)
 *   modify bp->data[]
 *   log_write(bp)
 *   brelse(bp)
 */
void
log_write(struct buf *b)
{
    /* TODO: Your code here. */

    acquire(&log.lock);
    int i;
    for (i = 0; i < log.lh.n; i++) {
        if (log.lh.block[i] == b->blockno - MBR_BASE)   // log absorbtion
            break;
    }
    log.lh.block[i] = b->blockno - MBR_BASE;
    // struct buf* bp = bread(b->dev, log.start + i + 1);
    // memmove(bp->data, b->data, BSIZE);
    // bwrite(bp);
    // brelse(bp);
    if (i == log.lh.n) {  // Add new block to log?
        // bpin(b);
        log.lh.n++;
    }
    release(&log.lock);
    b->flags |= B_DIRTY;
}

