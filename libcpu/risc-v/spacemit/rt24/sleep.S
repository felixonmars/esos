/*
 * Copyright (c) 2024-Present Spacemit Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Nell
 * 2024/03/23     Nell        First Spacemit RISC-V porting implementation
 */

#include <riscv-sleep.h>
#include <riscv_encoding.h>

.section    .bss
    .global __sleep_sp_store
__sleep_sp_store:
    .space 4

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
	sw a0, 0*REGBYTES(t0)

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

	/* invalidate icache */
	csrw CSR_CCM_MCOMMAND, 0xd

	/* invalidate dcache */
	csrw CSR_CCM_MCOMMAND, 0x17

	/* get the saved context */
	la a1, __sleep_sp_store
	lw a0, 0*REGBYTES(a1)

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
