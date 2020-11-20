#include "proc.h"
#include "spinlock.h"
#include "console.h"
#include "kalloc.h"
#include "trap.h"
#include "string.h"
#include "vm.h"
#include "mmu.h"

struct {
    struct proc proc[NPROC];
    struct spinlock lock;
} ptable;

static struct proc* initproc;

int nextpid = 1;
void forkret();
extern void trapret();
void swtch(struct context**, struct context*);

/*
 * Initialize the spinlock for ptable to serialize the access to ptable
 */
void
proc_init()
{
    /* TODO: Your code here. */
    initlock(&ptable.lock, "proc table");
}

/*
 * Look through the process table for an UNUSED proc.
 * If found, change state to EMBRYO and initialize
 * state (allocate stack, clear trapframe, set context for switch...)
 * required to run in the kernel. Otherwise return 0.
 */
static struct proc*
proc_alloc()
{
    struct proc* p;
    /* TODO: Your code here. */
    int found = 0;
    acquire(&ptable.lock);
    for (p = ptable.proc;p < ptable.proc + NPROC;p++) {
        if (p->state == UNUSED) {
            found = 1;
            break;
        }
    }
    if (found == 0) {
        release(&ptable.lock);
        return NULL;
    }
    // found

    // pagetable
    p->pgdir = (uint64_t*)kalloc();
    if (p->pgdir == NULL) {
        panic("proc_alloc: cannot alloc pagetable");
    }
    memset(p->pgdir, 0, PGSIZE);

    // kstack
    char* sp = kalloc();
    if (sp == NULL) {
        panic("proc_alloc: cannot alloc kstack");
    }
    p->kstack = sp + KSTACKSIZE;
    // trapframe
    sp -= sizeof(*(p->tf));
    p->tf = (struct trapframe*)sp;
    memset(p->tf, 0, sizeof(*(p->tf)));
    // trapret
    sp -= 8;
    *(uint64_t*)sp = (uint64_t)trapret;
    // sp
    sp -= 8;
    *(uint64_t*)sp = (uint64_t)p->kstack + KSTACKSIZE;
    // context
    sp -= sizeof(*(p->context));
    p->context = (struct context*)sp;
    memset(p->context, 0, sizeof(*(p->context)));
    p->context->r30 = (uint64_t)forkret;

    // other settings
    p->pid = nextpid++;
    p->state = EMBRYO;

    release(&ptable.lock);

    return p;
}

/*
 * Set up first user process(Only used once).
 * Set trapframe for the new process to run
 * from the beginning of the user process determined
 * by uvm_init
 */
void
user_init()
{
    struct proc* p;
    /* for why our symbols differ from xv6, please refer https://stackoverflow.com/questions/10486116/what-does-this-gcc-error-relocation-truncated-to-fit-mean */
    extern char _binary_obj_user_initcode_start[], _binary_obj_user_initcode_size[];

    /* TODO: Your code here. */
    p = proc_alloc();
    if (p == NULL) {
        panic("user_init: cannot allocate a process");
    }
    initproc = p;

    uvm_init(p->pgdir, _binary_obj_user_initcode_start, (long)_binary_obj_user_initcode_size);

    // tf
    p->tf->spsr_el1 = 0;
    p->tf->sp_el0 = PGSIZE;
    p->tf->r30 = 0;
    p->tf->elr_el1 = 0;

    p->state = RUNNABLE;
}

/*
 * Per-CPU process scheduler
 * Each CPU calls scheduler() after setting itself up.
 * Scheduler never returns.  It loops, doing:
 *  - choose a process to run
 *  - swtch to start running that process
 *  - eventually that process transfers control
 *        via swtch back to the scheduler.
 */
void
scheduler()
{
    struct proc* p;
    struct cpu* c = thiscpu;
    c->proc = NULL;

    for (;;) {
        /* Loop over process table looking for process to run. */
        /* TODO: Your code here. */
        acquire(&ptable.lock);
        for (p = ptable.proc; p < ptable.proc + NPROC; p++) {
            if (p->state == RUNNABLE) {
                uvm_switch(p);
                c->proc = p;
                p->state = RUNNING;
                swtch(&c->scheduler, p->context);

                // back
                c->proc = NULL;
            }

        }
        release(&ptable.lock);
    }
}

/*
 * Enter scheduler.  Must hold only ptable.lock
 */
void
sched()
{
    /* TODO: Your code here. */
}

/*
 * A fork child will first swtch here, and then "return" to user space.
 */
void
forkret()
{
    /* TODO: Your code here. */
    release(&ptable.lock);
    return;
}

/*
 * Exit the current process.  Does not return.
 * An exited process remains in the zombie state
 * until its parent calls wait() to find out it exited.
 */
void
exit()
{
    struct proc* p = thiscpu->proc;
    /* TODO: Your code here. */
}