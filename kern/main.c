#include <stdint.h>

#include "arm.h"
#include "string.h"
#include "console.h"
#include "kalloc.h"
#include "trap.h"
#include "timer.h"
#include "spinlock.h"
#include "proc.h"
#include "sd.h"
#include "file.h"
#include "fs.h"

struct cpu cpus[NCPU];

struct check_once {
    int count;
    struct spinlock lock;
};
struct check_once alloc_once = { 0 };
struct check_once memset_once = { 0 };
struct check_once initproc_once = { 0 };
void
main()
{
    /*
     * Before doing anything else, we need to ensure that all
     * static/global variables start out zero.
     */

    extern char edata[], end[], vectors[];

    /*
     * Determine which functions in main can only be
     * called once, and use lock to guarantee this.
     */
     /* TODO: Your code here. */

    // cprintf("main: [CPU%d] is init kernel\n", cpuid());

    /* TODO: Use `memset` to clear the BSS section of our program. */

    acquire(&memset_once.lock);
    if (!memset_once.count) {
        memset_once.count = 1;
        memset(edata, 0, end - edata);
    }
    release(&memset_once.lock);
    /* TODO: Use `cprintf` to print "hello, world\n" */
    console_init();
    cprintf("main: [CPU%d] is init kernel\n", cpuid());
    acquire(&alloc_once.lock);
    if (!alloc_once.count) {
        alloc_once.count = 1;
        alloc_init();
        cprintf("Allocator: Init success.\n");
        check_free_list();
    }
    release(&alloc_once.lock);

    irq_init();

    acquire(&initproc_once.lock);
    if (!initproc_once.count) {
        initproc_once.count = 1;
        proc_init();
        user_init();
        user_idle_init();
        user_idle_init();
        user_idle_init();
        user_idle_init();
        // user_init();
        // user_init();
        // user_init();
        // user_init();
        sd_init();
        binit();
        fileinit();
    }
    release(&initproc_once.lock);
    lvbar(vectors);
    timer_init();

    cprintf("main: [CPU%d] Init success.\n", cpuid());
    scheduler();
    while (1);
}
