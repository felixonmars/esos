/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ____RISCV_SLEEP_H__
#define ____RISCV_SLEEP_H__

#ifdef __cplusplus
 extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
#endif
