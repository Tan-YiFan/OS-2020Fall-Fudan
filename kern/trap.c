#include "trap.h"

#include "arm.h"
#include "sysregs.h"
#include "mmu.h"
#include <bits/syscall.h>
#include "peripherals/irq.h"

#include "uart.h"
#include "console.h"
#include "clock.h"
#include "timer.h"
#include "proc.h"
#include "sd.h"

void
irq_init()
{
    cprintf("irq_init: - irq init\n");
    clock_init();
    put32(ENABLE_IRQS_1, AUX_INT);
    put32(ENABLE_IRQS_2, VC_ARASANSDIO_INT);
    put32(GPU_INT_ROUTE, GPU_IRQ2CORE(0));
}

void
interrupt(struct trapframe *tf)
{
    int src = get32(IRQ_SRC_CORE(cpuid()));
    if (src & IRQ_CNTPNSIRQ) {
        timer_reset();
        // timer();
        yield();
    } else if (src & IRQ_TIMER) {
        clock_reset();
        // clock();
    } else if (src & IRQ_GPU) {
        int p1 = get32(IRQ_PENDING_1), p2 = get32(IRQ_PENDING_2);
        if (p1 & AUX_INT) {
            uart_intr();
        } else if (p2 & VC_ARASANSDIO_INT) {
            sd_intr();
        } else {
            cprintf("unexpected gpu intr p1 %x, p2 %x, sd %d, omitted\n", p1, p2, p2 & VC_ARASANSDIO_INT);
        }
    } else {
        cprintf("unexpected interrupt %x at cpu %d\n", src, cpuid());
        while (1) {
            
        }
    }
}

void
trap(struct trapframe *tf)
{
    int ec = resr() >> EC_SHIFT, iss = resr() & ISS_MASK;
    lesr(0);  /* Clear esr. */
    uint64_t fa;
    switch (ec) {
    case EC_UNKNOWN:
        interrupt(tf);
        break;

    case EC_SVC64:
        if (iss == 0) {
            /* Jump to syscall to handle the system call from user process */
            /* TODO: Your code here. */
            syscall1(tf);
        } else {
            cprintf("unexpected svc iss 0x%x\n", iss);
        }
        break;
    case EC_DABORT:
        asm("MRS %[r], FAR_EL1": [r]"=r" (fa)::);
    
        cprintf ("data abort: instruction 0x%x, fault addr 0x%llx\n",
            tf->elr_el1, fa);
        while (1);
    default:
        panic("trap: unexpected irq.\n");
    }
}

void
irq_error(uint64_t type)
{
    cprintf("irq_error: - irq_error\n");
    panic("irq_error: irq of type %d unimplemented. \n", type);
}
