#include <stdint.h>

#include "proc.h"
#include "trap.h"
#include "console.h"

int
sys_yield()
{
    yield();
    return 0;
}

size_t
sys_brk()
{
    /* TODO: Your code here. */
    uint64_t n;
    if(argint(0, &n) < 0) {
        return -1;
    }
    size_t sz = thisproc()->sz;
    if(growproc(n) < 0) {
        return -1;
    }
    return sz;
}

int
sys_clone()
{
    void *childstk;
    uint64_t flag;
    if (argint(0, &flag) < 0 || argint(1, &childstk) < 0)
        return -1;
    if (flag != 17) {
        cprintf("sys_clone: flags other than SIGCHLD are not supported.\n");
        return -1;
    }
    return fork();
}


int
sys_wait4()
{
    int64_t pid, opt;
    int *wstatus;
    void *rusage;
    if (argint(0, &pid) < 0 ||
        argint(1, &wstatus) < 0 ||
        argint(2, &opt) < 0 ||
        argint(3, &rusage) < 0)
        return -1;

    if (pid != -1 || wstatus != 0 || opt != 0 || rusage != 0) {
        cprintf("sys_wait4: unimplemented. pid %d, wstatus 0x%p, opt 0x%x, rusage 0x%p\n", pid, wstatus, opt, rusage);
        while (1) {
            
        }
        return -1;
    }

    return wait();
}

int sys_exit()
{
    exit();
}