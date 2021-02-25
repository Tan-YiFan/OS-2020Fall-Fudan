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
	# add x11, sp, #0xf0
	mrs x11, sp_el0
	mrs	x12, spsr_el1
	mrs	x13, elr_el1
	stp	x11, x0, [sp, #-16]!
	stp	x13, x12, [sp, #-16]!
    /*
     * Save Q0 and TPIDR_EL0 to placate musl such as
     * - `str q0, [sp, #-16]!`
     * - `mrs x12, tpidr_el0` and then push to stack
     */

    /* TODO: Your code here. */
	mrs x12, tpidr_el0
	stp x12, x12, [sp, #-16]!
	str q0, [sp, #-16]!
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
	ldr q0, [sp], #16
	ldp x12, x12, [sp], #16
	msr	tpidr_el0, x12
	
    ldp	x11, x12, [sp], #16
	msr	elr_el1, x11
	msr	spsr_el1, x12
	ldp x11, x0, [sp], #16
	msr sp_el0, x11
	ldp	x1, x2, [sp], #16
	ldp	x3, x4, [sp], #16
	ldp	x5, x6, [sp], #16
	ldp	x7, x8, [sp], #16
	ldp	x9, x10, [sp], #16
	ldp	x11, x12, [sp], #16
	ldp	x13, x14, [sp], #16
	ldp	x15, x16, [sp], #16
	ldp	x17, x18, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
	ldp x29, x30, [sp], #16

    /* Restore Q0 and TPIDR_EL0. */

    /* TODO: Your code here. */

    eret
