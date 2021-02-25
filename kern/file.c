/* File descriptors */

#include "types.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "console.h"
#include "log.h"

struct devsw devsw[NDEV];
struct {
    struct spinlock lock;
    struct file file[NFILE];
} ftable;

/* Optional since BSS is zero-initialized. */
void
fileinit()
{
    /* TODO: Your code here. */
    initlock(&ftable.lock, "ftable");
}

/* Allocate a file structure. */
struct file *
filealloc()
{
    /* TODO: Your code here. */
    acquire(&ftable.lock);
    for (struct file* f = ftable.file; f < ftable.file + NFILE; f++) {
        if (f->ref == 0) { // no reference
            f->ref = 1;
            release(&ftable.lock);
            return f;
        } 
    }
    
    release(&ftable.lock);
    return 0;
}

/* Increment ref count for file f. */
struct file *
filedup(struct file *f)
{
    /* TODO: Your code here. */
    acquire(&ftable.lock);
    if (f->ref <= 0) {
        panic("filedup: file %x no ref\n", f);
    } 
    f->ref++;
    
    release(&ftable.lock);
    return f;
}

/* Close file f. (Decrement ref count, close when reaches 0.) */
void
fileclose(struct file *f)
{
    /* TODO: Your code here. */
    acquire(&ftable.lock);
    if (f->ref <= 0) {
        panic("fileclose: ref %d\n", f->ref);
    } 
    
    f->ref--;
    if (f->ref) {
        release(&ftable.lock);
        return;
    } 
    
    assert(f->ref == 0);
    
    struct file f_copy = *f;
    f->type = FD_NONE;
    release(&ftable.lock);
    
    switch (f_copy.type) {
    // case FD_PIPE:
    //     break;
    case FD_INODE:
        begin_op();
        iput(f_copy.ip);
        end_op();
        break;
    default:
        panic("fileclose: unsupported file type %d\n", f->type);
    }
    
}

/* Get metadata about file f. */
int
filestat(struct file *f, struct stat *st)
{
    /* TODO: Your code here. */
    acquire(&ftable.lock);
    if (f->type == FD_INODE) {
        ilock(f->ip);
        stati(f->ip, st);
        iunlock(f->ip);
        release(&ftable.lock);
        return 0;
    } 
    release(&ftable.lock);
    return -1;
}

/* Read from file f. */
ssize_t
fileread(struct file *f, char *addr, ssize_t n)
{
    /* TODO: Your code here. */
    acquire(&ftable.lock);
    if (f->readable == 0) {
        return -1;
    } 
    switch (f->type) {
    // case FD_PIPE:
    //     release(&ftable.lock);
    //     // return piperead(f->pipe, addr, n);
    //     break;
    case FD_INODE:
        ilock(f->ip);
        ssize_t sz = readi(f->ip, addr, f->off, n);
        if (sz > 0) {
            f->off += sz;
        } 
        
        iunlock(f->ip);
        release(&ftable.lock);
        return sz;
    default:
        panic("fileread: unsupported file type %d\n", f->type);
    }
    return 0;
}

/* Write to file f. */
ssize_t
filewrite(struct file *f, char *addr, ssize_t n)
{
    /* TODO: Your code here. */
    acquire(&ftable.lock);
    if (f->writable == 0) {
        release(&ftable.lock);
        return -1;
    }
    int mx;
    switch (f->type) {
    // case FD_PIPE:
    //     release(&ftable.lock);
    //     break;
    case FD_INODE:
        mx = ((LOGSIZE - 4) >> 1) * 512;
        int i;
        for (i = 0; i < n;) {
            ssize_t s = MIN(mx, n - i);
            begin_op();
            ilock(f->ip);
            
            ssize_t sz = writei(f->ip, addr + i, f->off, s);
            if (sz < 0) {
                f->off += sz;
            } 
            
            iunlock(f->ip);
            end_op();
            
            if (sz < 0) {
                panic("filewrite: writei returns %lld\n", sz);
            } 
            
            if (sz < s) {
                panic("filewrite: writei fails: expected %lld got %lld\n", s, sz);
            }
            i += s;
        }
        release(&ftable.lock);
        return i == n ? n : -1;
        break;
    default:
        panic("filewrite: unsupported file type %d\n", f->type);
    }
    return -1;
}

