#ifndef INC_TRAP_H
#define INC_TRAP_H

#include <stdint.h>

struct trapframe {
    /* TODO: Design your own trapframe layout here. */
    uint64_t elr_el1;
    uint64_t spsr_el1;
    uint64_t sp_el0;
    uint64_t r[31];
};

void trap(struct trapframe*);
void irq_init();
void irq_error();

#endif
