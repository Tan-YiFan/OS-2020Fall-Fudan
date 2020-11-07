#ifndef INC_ARM_H
#define INC_ARM_H

#include <stdint.h>

static inline void
delay(int32_t count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n":
    "=r"(count) : [count] "0"(count) : "cc");
}

static inline void
put32(uint64_t p, uint32_t x)
{
    *(volatile uint32_t*)p = x;
}

static inline uint32_t
get32(uint64_t p)
{
    return *(volatile uint32_t*)p;
}

/* Unmask DAIF to start interrupt. */
static inline void
sti()
{
    asm volatile("msr daif, %[x]" : : [x] "r"(0));
}

/* Mask DAIF to close interrupt. */
static inline void
cli()
{
    asm volatile("msr daif, %[x]" : : [x] "r"(0xF << 6));
}

/* Brute-force data and instruction synchronization barrier. */
static inline void
disb()
{
    asm volatile("dsb sy; isb");
}

/* Read Exception Syndrome Register (EL1). */
static inline uint64_t
resr()
{
    uint64_t r;
    asm volatile("mrs %[x], esr_el1" : [x] "=r"(r));
    return r;
}

/* Read Exception Link Register (EL1). */
static inline uint64_t
relr()
{
    uint64_t r;
    asm volatile("mrs %[x], elr_el1" : [x] "=r"(r));
    return r;
}

/* Load Exception Syndrome Register (EL1). */
static inline void
lesr()
{
    asm volatile("msr esr_el1, %[x]" : : [x] "r"(0));
}

/* Load vector base (virtual) address register (EL1). */
static inline void
lvbar(void* p)
{
    disb();
    asm volatile("msr vbar_el1, %[x]" : : [x] "r"(p));
    disb();
}

/* Load Translation Table Base Register 0 (EL1). */
static inline void
lttbr0(uint64_t p)
{
    asm volatile("msr ttbr0_el1, %[x]" : : [x] "r"(p));
    disb();
    asm volatile("tlbi vmalle1");
    disb();
}

/* Load Translation Table Base Register 1 (EL1). */
static inline void
lttbr1(uint64_t p)
{
    asm volatile("msr ttbr1_el1, %[x]" : : [x] "r"(p));
    disb();
    asm volatile("tlbi vmalle1");
    disb();
}

static inline int
cpuid()
{
    int64_t id;
    asm volatile("mrs %[x], mpidr_el1" : [x] "=r"(id));
    return id & 0xFF;
}

static inline int
interrput_enabled()
{
    uint32_t ret;
    asm volatile("MRS %[v], DAIF": [v] "=r" (ret)::);
    return !(ret & 0x80);
}

#include "spinlock.h"
#include "console.h"
static inline void
cli_req()
{
    int enabled = interrput_enabled();
    cli();
    int id = cpuid();
    if (cpus[id].lock_num++ == 0) {
        cpus[id].prev_int_enabled = enabled;
    }

}

static inline void
cli_resp()
{
    if (interrput_enabled()) {
        panic("No corresponding req for resp!\n");
    }
    int id = cpuid();
    if (--cpus[id].lock_num < 0) {
        panic("No corresponding req for resp!\n");
    }

    if ((cpus[id].lock_num == 0) && cpus[id].prev_int_enabled) {
        sti();
    }
}
#endif
