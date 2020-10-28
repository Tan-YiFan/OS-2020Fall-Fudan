/* vectors.S send all traps here. */
.global alltraps
alltraps:
    /*
     * Build your trap frame.
     * Hint:
     * 1. Use stp/ldp in favor of str/ldr to maintain stack alignment
          For example, `stp x1, x2, [sp, #-16]!` is equivalent to 
          first `push x2` and then `push x1`. Be careful about the order.
     * 2. Use mrs/msr to move from/to system registers such as
     *    ELR_EL1, SPSR_EL1, and SP_EL0.
     */

    /* TODO: Your code here. */
    sub sp, sp, #0x20        /* reserverd for LR, SP, SPSR and ELR */
	stp	x28, x29, [sp, #-16]!
	stp	x26, x27, [sp, #-16]!
	stp	x24, x25, [sp, #-16]!
	stp	x22, x23, [sp, #-16]!
	stp	x20, x21, [sp, #-16]!
	stp	x18, x19, [sp, #-16]!
	stp	x16, x17, [sp, #-16]!
	stp	x14, x15, [sp, #-16]!
	stp	x12, x13, [sp, #-16]!
	stp	x10, x11, [sp, #-16]!
	stp	x8, x9, [sp, #-16]!
	stp	x6, x7, [sp, #-16]!
	stp	x4, x5, [sp, #-16]!
	stp	x2, x3, [sp, #-16]!
	stp	x0, x1, [sp, #-16]!
	add x21, sp, #0x110

	mrs	x22, elr_el1             /* ELR */
	mrs	x23, spsr_el1            /* SPSR */

	stp	x30, x21, [sp, #0xf0]    /* LR, SP */
	stp	x22, x23, [sp, #0x100]   /* ELR, SPSR */
    /*
     * Call trap(struct *trapframe).
     * Hint: The first argument is a stack pointer.
     */

    /* TODO: Your code here. */
    b trap
/* Return falls through to trapret. */
.global trapret
trapret:
    /*
     * Restore registers.
     * Hint: `ldp x1, x2, [sp], #16` is equivalent to first `pop x1`
     * and then `pop x2`.
     */

    /* TODO: Your code here. */
    ldp	x22, x23, [sp, #0x100]   /* ELR, SPSR */
	ldp	x30, x28, [sp, #0xf0]    /* LR, SP */

	msr	elr_el1, x22             /* ELR */
	msr	spsr_el1, x23
	msr sp_el0, x28

	mov x29, sp

	ldp	x0, x1, [x29], #16
	ldp	x2, x3, [x29], #16
	ldp	x4, x5, [x29], #16
	ldp	x6, x7, [x29], #16
	ldp	x8, x9, [x29], #16
	ldp	x10, x11, [x29], #16
	ldp	x12, x13, [x29], #16
	ldp	x14, x15, [x29], #16
	ldp	x16, x17, [x29], #16
	ldp	x18, x19, [x29], #16
	ldp	x20, x21, [x29], #16
	ldp	x22, x23, [x29], #16
	ldp	x24, x25, [x29], #16
	ldp	x26, x27, [x29], #16
	ldr x28, [x29], #8
	ldr x29, [x29]
    eret
