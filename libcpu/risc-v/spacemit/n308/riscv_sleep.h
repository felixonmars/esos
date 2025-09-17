/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RISCV_SLEEP_H__
#define __RISCV_SLEEP_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include <rthw.h>
#include <rtthread.h>

#define PT_SIZE 144 /* sizeof(struct pt_regs) */
#define PT_EPC 0 /* offsetof(struct pt_regs, epc) */
#define PT_RA 4 /* offsetof(struct pt_regs, ra) */
#define PT_FP 32 /* offsetof(struct pt_regs, s0) */
#define PT_S0 32 /* offsetof(struct pt_regs, s0) */
#define PT_S1 36 /* offsetof(struct pt_regs, s1) */
#define PT_S2 72 /* offsetof(struct pt_regs, s2) */
#define PT_S3 76 /* offsetof(struct pt_regs, s3) */
#define PT_S4 80 /* offsetof(struct pt_regs, s4) */
#define PT_S5 84 /* offsetof(struct pt_regs, s5) */
#define PT_S6 86 /* offsetof(struct pt_regs, s6) */
#define PT_S7 92 /* offsetof(struct pt_regs, s7) */
#define PT_S8 96 /* offsetof(struct pt_regs, s8) */
#define PT_S9 100 /* offsetof(struct pt_regs, s9) */
#define PT_S10 104 /* offsetof(struct pt_regs, s10) */
#define PT_S11 108 /* offsetof(struct pt_regs, s11) */
#define PT_SP 8 /* offsetof(struct pt_regs, sp) */
#define PT_TP 16 /* offsetof(struct pt_regs, tp) */
#define PT_A0 40 /* offsetof(struct pt_regs, a0) */
#define PT_A1 44 /* offsetof(struct pt_regs, a1) */
#define PT_A2 48 /* offsetof(struct pt_regs, a2) */
#define PT_A3 52 /* offsetof(struct pt_regs, a3) */
#define PT_A4 56 /* offsetof(struct pt_regs, a4) */
#define PT_A5 60 /* offsetof(struct pt_regs, a5) */
#define PT_A6 64 /* offsetof(struct pt_regs, a6) */
#define PT_A7 68 /* offsetof(struct pt_regs, a7) */
#define PT_T0 20 /* offsetof(struct pt_regs, t0) */
#define PT_T1 24 /* offsetof(struct pt_regs, t1) */
#define PT_T2 28 /* offsetof(struct pt_regs, t2) */
#define PT_T3 112 /* offsetof(struct pt_regs, t3) */
#define PT_T4 116 /* offsetof(struct pt_regs, t4) */
#define PT_T5 120 /* offsetof(struct pt_regs, t5) */
#define PT_T6 124 /* offsetof(struct pt_regs, t6) */
#define PT_GP 12 /* offsetof(struct pt_regs, gp) */
#define PT_ORIG_A0 140 /* offsetof(struct pt_regs, orig_a0) */
#define PT_STATUS 128 /* offsetof(struct pt_regs, status) */
#define PT_BADADDR 132 /* offsetof(struct pt_regs, badaddr) */
#define PT_CAUSE 136 /* offsetof(struct pt_regs, cause) */
#define SUSPEND_CONTEXT_REGS 0 /* offsetof(struct suspend_context, regs) */

struct pt_regs {
	rt_ubase_t epc;
	rt_ubase_t ra;
	rt_ubase_t sp;
	rt_ubase_t gp;
	rt_ubase_t tp;
	rt_ubase_t t0;
	rt_ubase_t t1;
	rt_ubase_t t2;
	rt_ubase_t s0;
	rt_ubase_t s1;
	rt_ubase_t a0;
	rt_ubase_t a1;
	rt_ubase_t a2;
	rt_ubase_t a3;
	rt_ubase_t a4;
	rt_ubase_t a5;
	rt_ubase_t a6;
	rt_ubase_t a7;
	rt_ubase_t s2;
	rt_ubase_t s3;
	rt_ubase_t s4;
	rt_ubase_t s5;
	rt_ubase_t s6;
	rt_ubase_t s7;
	rt_ubase_t s8;
	rt_ubase_t s9;
	rt_ubase_t s10;
	rt_ubase_t s11;
	rt_ubase_t t3;
	rt_ubase_t t4;
	rt_ubase_t t5;
	rt_ubase_t t6;
	/* Supervisor/Machine CSRs */
	rt_ubase_t status;
	rt_ubase_t badaddr;
	rt_ubase_t cause;
	/* a0 value before the syscall */
	rt_ubase_t orig_a0;
};

struct suspend_context {
	struct pt_regs stack_frame;
	rt_ubase_t mscratch;
	rt_ubase_t mie;

	/* interrupt handler */
	rt_ubase_t mmisc_ctl;
	rt_ubase_t mtvt;
	rt_ubase_t mtvt2;
	rt_ubase_t mtvec;
};

#ifdef __cplusplus
}
#endif
#endif
