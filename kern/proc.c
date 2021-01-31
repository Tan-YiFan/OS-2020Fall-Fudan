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

// int nextpid = 1;
struct {
    int nextpid;
    struct spinlock lock;
} nextpid = { 1 };
void forkret();
extern void trapret();
void swtch(struct context**, struct context*);

/*
 * Initialize the spinlock for ptable to serialize the access to ptable
 */
void
proc_init()
{
    initlock(&ptable.lock, "proc table");
    initlock(&nextpid.lock, "nextpid");
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
    // p->pgdir = pgdir_init();
    // memset(p->pgdir, 0, PGSIZE);

    // kstack
    char* sp = kalloc();
    if (sp == NULL) {
        panic("proc_alloc: cannot alloc kstack");
    }
    p->kstack = sp;
    sp += KSTACKSIZE;
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
    p->context->r30 = (uint64_t)forkret + 8;

    // other settings
    p->pid = alloc_pid();
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

    p = proc_alloc();

    if (p == NULL) {
        panic("user_init: cannot allocate a process");
    }
    if ((p->pgdir = pgdir_init()) == NULL) {
        panic("user_init: cannot allocate a pagetable");
    }

    initproc = p;

    uvm_init(p->pgdir, _binary_obj_user_initcode_start, (long)_binary_obj_user_initcode_size);

    // tf
    memset(p->tf, 0, sizeof(*(p->tf)));
    p->tf->spsr_el1 = 0;
    p->tf->sp_el0 = PGSIZE;
    p->tf->r30 = 0;
    p->tf->elr_el1 = 0;

    p->state = RUNNABLE;
    p->sz = PGSIZE;
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
        acquire(&ptable.lock);
        for (p = ptable.proc; p < ptable.proc + NPROC; p++) {
            if (p->state == RUNNABLE) {
                uvm_switch(p);
                c->proc = p;
                p->state = RUNNING;
                // cprintf("scheduler: process id %d takes the cpu %d\n", p->pid, cpuid());
                swtch(&c->scheduler, p->context);

                // back
                c->proc = NULL;
                break;
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
    struct proc* p = thiscpu->proc;
    if (!holding(&ptable.lock)) {
        panic("sched: not holding ptable lock");
    }

    if (p->state == RUNNING) {
        panic("sched: process running");
    }

    swtch(&p->context, thiscpu->scheduler);
}

/*
 * A fork child will first swtch here, and then "return" to user space.
 */
void
forkret()
{

    release(&ptable.lock);
#include "sd.h"
    if (thiscpu->proc->pid == 1) {
        sd_test();
    }

    // sd_test();
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
    /* if (p == initproc) {
        panic("exit: init process shall not exit!");
    } */

    acquire(&ptable.lock);
    p->state = ZOMBIE;
    sched();

    // never exit
    panic("exit: shall not return");
}

void
yield()
{
    acquire(&ptable.lock);
    struct proc* p = thiscpu->proc;
    p->state = RUNNABLE;
    // cprintf("yield: process id %d gives up the cpu %d\n", p->pid, cpuid());
    sched();
    release(&ptable.lock);
}

int
alloc_pid()
{
    acquire(&nextpid.lock);
    int pid = nextpid.nextpid;
    nextpid.nextpid++;
    release(&nextpid.lock);
    return pid;
}

/*
 * Atomically release lock and sleep on chan.
 * Reacquires lock when awakened.
 */
void
sleep(void* chan, struct spinlock* lk)
{
    if (!holding(lk)) {
        panic("sleep: lock not held");
    }

    // change the state of ptable, add lock
    if (lk != &ptable.lock) {
        acquire(&ptable.lock);
        release(lk);
    }

    thiscpu->proc->chan = chan;
    thiscpu->proc->state = SLEEPING;
    sched();

    // sched returns
    thiscpu->proc->chan = 0;

    if (lk != &ptable.lock) {
        release(&ptable.lock);
        acquire(lk);
    }
}

/* Wake up all processes sleeping on chan. */
void
wakeup(void* chan)
{
    acquire(&ptable.lock);
    for (struct proc* p = ptable.proc; p < ptable.proc + NPROC; p++) {
        if (p->state == SLEEPING && p->chan == chan) {
            p->state = RUNNABLE;
        }
    }
    release(&ptable.lock);
}

/* Give up CPU. */
void
yield()
{
    /* TODO: Your code here. */
}

/*
 * Create a new process copying p as the parent.
 * Sets up stack to return as if from system call.
 * Caller must set state of returned proc to RUNNABLE.
 */
int
fork()
{
    /* TODO: Your code here. */
}

/*
 * Wait for a child process to exit and return its pid.
 * Return -1 if this process has no children.
 */
int
wait()
{
    /* TODO: Your code here. */
}

/*
 * Print a process listing to console.  For debugging.
 * Runs when user types ^P on console.
 * No lock to avoid wedging a stuck machine further.
 */
void
procdump()
{
    panic("unimplemented");
}

