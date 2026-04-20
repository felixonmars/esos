/*
 * Copyright (c) 2024-Present Spacemit Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Nell
 * 2024/03/23     Nell        First Spacemit RISC-V porting implementation
 */

#include <rtconfig.h>
#include <riscv-sleep.h>
#include <riscv_encoding.h>

.section    .bss
    .align 8
    .global __sleep_sp_store
__sleep_sp_store:
    .space 8

    .global _g_ap_sp_base
    .global _g_ap_sp_top
_g_ap_sp_base:
    .space 1024
_g_ap_sp_top:
.align 8

	.text

.align 2
.globl __cpu_suspend_enter
__cpu_suspend_enter:
	/* Save registers (except A0 and T0-T6) */
	STORE   ra, (SUSPEND_CONTEXT_REGS + PT_RA)(a0)
	STORE   sp, (SUSPEND_CONTEXT_REGS + PT_SP)(a0)
	STORE   gp, (SUSPEND_CONTEXT_REGS + PT_GP)(a0)
	STORE   tp, (SUSPEND_CONTEXT_REGS + PT_TP)(a0)
	STORE   s0, (SUSPEND_CONTEXT_REGS + PT_S0)(a0)
	STORE   s1, (SUSPEND_CONTEXT_REGS + PT_S1)(a0)
	STORE   a1, (SUSPEND_CONTEXT_REGS + PT_A1)(a0)
	STORE   a2, (SUSPEND_CONTEXT_REGS + PT_A2)(a0)
	STORE   a3, (SUSPEND_CONTEXT_REGS + PT_A3)(a0)
	STORE   a4, (SUSPEND_CONTEXT_REGS + PT_A4)(a0)
	STORE   a5, (SUSPEND_CONTEXT_REGS + PT_A5)(a0)
	STORE   a6, (SUSPEND_CONTEXT_REGS + PT_A6)(a0)
	STORE   a7, (SUSPEND_CONTEXT_REGS + PT_A7)(a0)
	STORE   s2, (SUSPEND_CONTEXT_REGS + PT_S2)(a0)
	STORE   s3, (SUSPEND_CONTEXT_REGS + PT_S3)(a0)
	STORE   s4, (SUSPEND_CONTEXT_REGS + PT_S4)(a0)
	STORE   s5, (SUSPEND_CONTEXT_REGS + PT_S5)(a0)
	STORE   s6, (SUSPEND_CONTEXT_REGS + PT_S6)(a0)
	STORE   s7, (SUSPEND_CONTEXT_REGS + PT_S7)(a0)
	STORE   s8, (SUSPEND_CONTEXT_REGS + PT_S8)(a0)
	STORE   s9, (SUSPEND_CONTEXT_REGS + PT_S9)(a0)
	STORE   s10, (SUSPEND_CONTEXT_REGS + PT_S10)(a0)
	STORE   s11, (SUSPEND_CONTEXT_REGS + PT_S11)(a0)

	/* Save CSRs */
	csrr    t0, CSR_MEPC
	STORE   t0, (SUSPEND_CONTEXT_REGS + PT_EPC)(a0)
	csrr    t0, CSR_MSTATUS
	STORE   t0, (SUSPEND_CONTEXT_REGS + PT_STATUS)(a0)
	csrr    t0, CSR_MTVAL
	STORE   t0, (SUSPEND_CONTEXT_REGS + PT_BADADDR)(a0)
	csrr    t0, CSR_MCAUSE
	STORE   t0, (SUSPEND_CONTEXT_REGS + PT_CAUSE)(a0)

	/* save the context to __sleep_sp_store */
	la t0, __sleep_sp_store
	STORE a0, 0*REGBYTES(t0)

	/* Return non-zero value */
	li      a0, 1

	/* Return to C code */
	ret

.align 2
.globl __cpu_resume_enter
__cpu_resume_enter:
        /* Load the global pointer */
        .option push
        .option norelax
                la gp, __global_pointer$
        .option pop

	/* Disable Global Interrupt */
	csrc CSR_MSTATUS, MSTATUS_MIE

	/* get the saved context */
	la a1, __sleep_sp_store
	LOAD a0, 0*REGBYTES(a1)

        /* Restore CSRs */
        LOAD	t0, (SUSPEND_CONTEXT_REGS + PT_EPC)(a0)
        csrw	CSR_MEPC, t0
        LOAD	t0, (SUSPEND_CONTEXT_REGS + PT_STATUS)(a0)
        csrw	CSR_MSTATUS, t0
        LOAD	t0, (SUSPEND_CONTEXT_REGS + PT_BADADDR)(a0)
        csrw	CSR_MTVAL, t0
        LOAD	t0, (SUSPEND_CONTEXT_REGS + PT_CAUSE)(a0)
        csrw	CSR_MCAUSE, t0

        /* Restore registers (except A0 and T0-T6) */
        LOAD   ra, (SUSPEND_CONTEXT_REGS + PT_RA)(a0)
        LOAD   sp, (SUSPEND_CONTEXT_REGS + PT_SP)(a0)
        LOAD   gp, (SUSPEND_CONTEXT_REGS + PT_GP)(a0)
        LOAD   tp, (SUSPEND_CONTEXT_REGS + PT_TP)(a0)
        LOAD   s0, (SUSPEND_CONTEXT_REGS + PT_S0)(a0)
        LOAD   s1, (SUSPEND_CONTEXT_REGS + PT_S1)(a0)
        LOAD   a1, (SUSPEND_CONTEXT_REGS + PT_A1)(a0)
        LOAD   a2, (SUSPEND_CONTEXT_REGS + PT_A2)(a0)
        LOAD   a3, (SUSPEND_CONTEXT_REGS + PT_A3)(a0)
        LOAD   a4, (SUSPEND_CONTEXT_REGS + PT_A4)(a0)
        LOAD   a5, (SUSPEND_CONTEXT_REGS + PT_A5)(a0)
        LOAD   a6, (SUSPEND_CONTEXT_REGS + PT_A6)(a0)
        LOAD   a7, (SUSPEND_CONTEXT_REGS + PT_A7)(a0)
        LOAD   s2, (SUSPEND_CONTEXT_REGS + PT_S2)(a0)
        LOAD   s3, (SUSPEND_CONTEXT_REGS + PT_S3)(a0)
        LOAD   s4, (SUSPEND_CONTEXT_REGS + PT_S4)(a0)
        LOAD   s5, (SUSPEND_CONTEXT_REGS + PT_S5)(a0)
        LOAD   s6, (SUSPEND_CONTEXT_REGS + PT_S6)(a0)
        LOAD   s7, (SUSPEND_CONTEXT_REGS + PT_S7)(a0)
        LOAD   s8, (SUSPEND_CONTEXT_REGS + PT_S8)(a0)
        LOAD   s9, (SUSPEND_CONTEXT_REGS + PT_S9)(a0)
        LOAD   s10, (SUSPEND_CONTEXT_REGS + PT_S10)(a0)
        LOAD   s11, (SUSPEND_CONTEXT_REGS + PT_S11)(a0)

	/* Enable mcycle and minstret counter */
	csrci CSR_MCOUNTINHIBIT, 0x5

        /* Return zero value */
        add     a0, zero, zero

        /* Return to C code */
        ret

#ifdef SOC_SPACEMIT
        .align 3
        .globl _reset_regs
_reset_regs:
	/* flush the instruction cache */
	fence.i
	/* Reset all registers except ra, a0, a1, a2, a3 and a4 */
	li sp, 0
	li gp, 0
	li tp, 0
	li t0, 0
	li t1, 0
	li t2, 0
	li s0, 0
	li s1, 0
	li a5, 0
	li a6, 0
	li a7, 0
	li s2, 0
	li s3, 0
	li s4, 0
	li s5, 0
	li s6, 0
	li s7, 0
	li s8, 0
	li s9, 0
	li s10, 0
	li s11, 0
	li t3, 0
	li t4, 0
	li t5, 0
	li t6, 0
	csrw CSR_MSCRATCH, 0

	ret

        .align 3
        .globl _start_warm_dummy
_start_warm_dummy:
        /* Reset all registers except ra, a0, a1, a2, a3 and a4 for non-boot HART */
        li      ra, 0
        call    _reset_regs

	/* reset nic400 */
	/* re-set 0xd84402cc */
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 556
	li      a5, 0
	sw      a5, 0(t0)
	fence   w, o

	/* delay 1ms */
	rdtime a5
	li     t1, 240000
	add    t0, a5, t1
.Lloop:
	rdtime t2
	blt    t2, t0, .Lloop

	li      a5, 1
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 556
	sw      a5, 0(t0)
	fence   w, o

	/* re-set 0xd8440234 */
	li	a5, 0
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 564
	sw      a5, 0(t0)
	fence   w,o

	/* delay 1ms */
	rdtime a5
	li     t1, 240000
	add    t0, a5, t1
.Lloop0:
	rdtime t2
	blt    t2, t0, .Lloop0

	li	a5, 1
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 564
	sw      a5, 0(t0)
	fence   w,o

	/* re-set 0xd8440244 */
	li	a5, 1
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 580
	sw      a5, 0(t0)
	fence   w,o

	/* delay 1ms */
	rdtime a5
	li     t1, 240000
	add    t0, a5, t1
.Lloop1:
	rdtime t2
	blt    t2, t0, .Lloop1

	li	a5, 0
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 580
	sw      a5, 0(t0)
	fence   w,o

	/* re-set 0xd8440248 */
	li	a5, 1
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 584
	sw      a5, 0(t0)
	fence   w,o

	/* delay 1ms */
	rdtime a5
	li     t1, 240000
	add    t0, a5, t1
.Lloop2:
	rdtime t2
	blt    t2, t0, .Lloop2

	li	a5, 0
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 584
	sw      a5, 0(t0)
	fence   w,o

        /* Disable all interrupts */
        csrw    CSR_MIE, zero

	/* disable l2 cache preftch */
//	li	a5, 768
//	csrs	0x7d1, a5

        /* set audio buffer to cacheable */
        li      t0, 0xff
        slli    t0, t0, 48
        csrc    0xbc0, t0
        li      t0, 0x20
        slli    t0, t0, 48
        csrs    0xbc0, t0
        sfence.vma

        /* set XIP address to IO attribute */
        lui     t0, 0xff0
        csrc    0xbc0, t0
        lui     t0, 0x220
        csrs    0xbc0, t0
        sfence.vma

	/* get the hartid */
	csrr    a0, CSR_MHARTID

	/* set the sp */
	la sp, _g_ap_sp_top
	call boot_entry_dummy

	.align 3
	.globl _c0start_warm_dummy
_c0start_warm_dummy:
        /* Reset all registers except ra, a0, a1, a2, a3 and a4 for non-boot HART */
        li      ra, 0
        call    _reset_regs

	/* reset nic400 */
	/* re-set 0xd84402cc */
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 556
	li      a5, 0
	sw      a5, 0(t0)
	fence   w, o

	/* delay 1ms */
	rdtime a5
	li     t1, 240000
	add    t0, a5, t1
.Lloop3:
	rdtime t2
	blt    t2, t0, .Lloop3

	li      a5, 1
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 556
	sw      a5, 0(t0)
	fence   w, o

	/* re-set 0xd8440234 */
	li	a5, 0
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 564
	sw      a5, 0(t0)
	fence   w,o

	/* delay 1ms */
	rdtime a5
	li     t1, 240000
	add    t0, a5, t1
.Lloop4:
	rdtime t2
	blt    t2, t0, .Lloop4

	li	a5, 1
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 564
	sw      a5, 0(t0)
	fence   w,o

	/* re-set 0xd8440244 */
	li	a5, 1
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 580
	sw      a5, 0(t0)
	fence   w,o

	/* delay 1ms */
	rdtime a5
	li     t1, 240000
	add    t0, a5, t1
.Lloop5:
	rdtime t2
	blt    t2, t0, .Lloop5

	li	a5, 0
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 580
	sw      a5, 0(t0)
	fence   w,o

	/* re-set 0xd8440248 */
	li	a5, 1
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 584
	sw      a5, 0(t0)
	fence   w,o

	/* delay 1ms */
	rdtime a5
	li     t1, 240000
	add    t0, a5, t1
.Lloop6:
	rdtime t2
	blt    t2, t0, .Lloop6

	li	a5, 0
	lui     t0, 0x3611
	slli    t0, t0, 0x6
	addi    t0, t0, 584
	sw      a5, 0(t0)
	fence   w,o

        /* Disable all interrupts */
        csrw    CSR_MIE, zero

	/* disable l2 cache preftch */
//	li	a5, 768
//	csrs	0x7d1, a5

        /* set audio buffer to cacheable */
        li      t0, 0xff
        slli    t0, t0, 48
        csrc    0xbc0, t0
        li      t0, 0x20
        slli    t0, t0, 48
        csrs    0xbc0, t0
        sfence.vma

        /* set XIP address to IO attribute */
        lui     t0, 0xff0
        csrc    0xbc0, t0
        lui     t0, 0x220
        csrs    0xbc0, t0
        sfence.vma

	/* get the hartid */
	csrr    a0, CSR_MHARTID

	/* set the sp */
	la sp, _g_ap_sp_top
	call c0boot_entry_dummy
#endif
