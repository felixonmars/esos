#include "riscv_encoding.h"

	.global         _start
	.section ".text"
_start:
	/* clear Interrupt Registers */
	csrw mie, 0
	csrw mip, 0
	/* set Trap Vector Base Address Register */
	la t0, trap_entry
	csrw mtvec, t0

	li x1, 0
	li x2, 0
	li x3, 0
	li x4, 0
	li x5, 0
	li x6, 0
	li x7, 0
	li x8, 0
	li x9, 0
	li x10,0
	li x11,0
	li x12,0
	li x13,0
	li x14,0
	li x15,0
	li x16,0
	li x17,0
	li x18,0
	li x19,0
	li x20,0
	li x21,0
	li x22,0
	li x23,0
	li x24,0
	li x25,0
	li x26,0
	li x27,0
	li x28,0
	li x29,0
	li x30,0
	li x31,0

	/* set to disable FPU */
	li t0, MSTATUS_FS
	csrs mstatus, t0

.option push
.option norelax
	la gp, __global_pointer$
.option pop

	/* Set correct sp for current cpu */
	la sp, __StackTop

	/* Clear bss section */
	la a0, __bss_start__
	la a1, __bss_end__
	bgeu a0, a1, 2f
1:
	sw      zero, (a0)
	addi    a0, a0, 4
	bltu    a0, a1, 1b
2:
	jal SystemInit

	jal entry

__exit:
    j      __exit
