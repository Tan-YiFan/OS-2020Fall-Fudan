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
    sub sp, sp, #0x20
	stp	x29, x30, [sp, #-16]!
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x17, x18, [sp, #-16]!
	stp	x15, x16, [sp, #-16]!
	stp	x13, x14, [sp, #-16]!
	stp	x11, x12, [sp, #-16]!
	stp	x9, x10, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!
	add x1, sp, #0x110
	mrs	x2, spsr_el1
	mrs	x3, elr_el1
	stp	x1, x0, [sp, #0xf0]
	stp	x3, x2, [sp, #0x100]
    /*
     * Call trap(struct *trapframe).
     * Hint: The first argument is a stack pointer.
     */

    /* TODO: Your code here. */
    add x0, sp, #0
    bl trap
/* Return falls through to trapret. */
.global trapret
trapret:
    /*
     * Restore registers.
     * Hint: `ldp x1, x2, [sp], #16` is equivalent to first `pop x1`
     * and then `pop x2`.
     */

    /* TODO: Your code here. */
    ldp	x1, x2, [sp, #0x100]
	msr	elr_el1, x1
	msr	spsr_el1, x2
	ldr x1, [sp, #0xf0]
	msr sp_el0, x1
	mov x30, sp
	ldp	x0, x1, [x30], #16
	ldp	x2, x3, [x30], #16
	ldp	x4, x5, [x30], #16
	ldp	x6, x7, [x30], #16
	ldp	x8, x9, [x30], #16
	ldp	x10, x11, [x30], #16
	ldp	x12, x13, [x30], #16
	ldp	x14, x15, [x30], #16
	ldp	x16, x17, [x30], #16
	ldp	x18, x19, [x30], #16
	ldp	x20, x21, [x30], #16
	ldp	x22, x23, [x30], #16
	ldp	x24, x25, [x30], #16
	ldp	x26, x27, [x30], #16
	ldp x28, x29, [x30], #16
	ldr x30, [x30]
    eret
