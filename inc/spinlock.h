#ifndef INC_SPINLOCK_H
#define INC_SPINLOCK_H

struct spinlock {
    volatile int locked;
};
void acquire(struct spinlock*);
void release(struct spinlock*);

struct cpu {
    int lock_num;
    unsigned char prev_int_enabled;
};

static struct cpu cpus[64] = { 0 };

#endif
