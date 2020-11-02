#include <stdint.h>
#include "types.h"
#include "string.h"
#include "mmu.h"
#include "memlayout.h"
#include "console.h"
#include "kalloc.h"
#include "spinlock.h"

extern char end[];

/*
 * Free page's list element struct.
 * We store each free page's run structure in the free page itself.
 */
struct run {
    struct run* next;
};

struct {
    struct run* free_list; /* Free list of physical pages */
    struct spinlock lock;
} kmem;

void
alloc_init()
{
    kmem.lock.locked = 0;
    free_range(end, P2V(PHYSTOP));
}

/* Free the page of physical memory pointed at by v. */
void
kfree(char* v)
{
    struct run* r;

    if ((uint64_t)v % PGSIZE // not aligned
        || v < end // below bss
        || V2P(v) >= PHYSTOP // above top physical memory
        )
        panic("kfree");

    /* Fill with junk to catch dangling refs. */
    memset(v, 0x11, PGSIZE);

    /* TODO: Your code here. */
    r = (struct run*)v;

    // lock kmem
    acquire(&kmem.lock);
    r->next = kmem.free_list;
    kmem.free_list = r;
    // unlock
    release(&kmem.lock);

    // another version
    /* if (kmem.free_list == NULL) {
        kmem.free_list = (struct run*)v;
        kmem.free_list->next = NULL;
    }
    else {
        struct run* k;
        k = kmem.free_list;
        kmem.free_list = (struct run*)v;
        kmem.free_list->next = k;
        // for (r = kmem.free_list; r; r = r->next) {
        //     // the page has already been free
        //     if (r == (struct run*)v) {
        //         break;
        //     }

        //     if (r->next == NULL) {
        //         r->next = (struct run*)v;
        //         r->next->next = NULL;
        //         break;
        //     }
        // }
    } */


}

void
free_range(void* vstart, void* vend)
{
    char* p;
    p = ROUNDUP((char*)vstart, PGSIZE);
    for (; p + PGSIZE <= (char*)vend; p += PGSIZE)
        kfree(p);
}

/*
 * Allocate one 4096-byte page of physical memory.
 * Returns a pointer that the kernel can use.
 * Returns 0 if the memory cannot be allocated.
 */
char*
kalloc()
{
    /* TODO: Your code here. */
    struct run* r;

    // lock kmem
    acquire(&kmem.lock);
    r = kmem.free_list;
    if (r) {
        kmem.free_list = r->next;
    }
    // unlock
    release(&kmem.lock);

    if (r) {
        memset((char*)r, 0x11, PGSIZE);
    }

    return (char*)r;

}

void
check_free_list()
{
    struct run* p;
    if (!kmem.free_list)
        panic("'kmem.free_list' is a null pointer!");

    for (p = kmem.free_list; p; p = p->next) {
        assert((void*)p > (void*)end);
    }
}
