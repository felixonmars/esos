#ifndef __RISCV_SLEEP_H__
#define __RISCV_SLEEP_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include <rthw.h>
#include <rtthread.h>

#define PT_SIZE 288 /* sizeof(struct pt_regs) */
#define PT_EPC 0 /* offsetof(struct pt_regs, epc) */
#define PT_RA 8 /* offsetof(struct pt_regs, ra) */
#define PT_FP 64 /* offsetof(struct pt_regs, s0) */
#define PT_S0 64 /* offsetof(struct pt_regs, s0) */
#define PT_S1 72 /* offsetof(struct pt_regs, s1) */
#define PT_S2 144 /* offsetof(struct pt_regs, s2) */
#define PT_S3 152 /* offsetof(struct pt_regs, s3) */
#define PT_S4 160 /* offsetof(struct pt_regs, s4) */
#define PT_S5 168 /* offsetof(struct pt_regs, s5) */
#define PT_S6 176 /* offsetof(struct pt_regs, s6) */
#define PT_S7 184 /* offsetof(struct pt_regs, s7) */
#define PT_S8 192 /* offsetof(struct pt_regs, s8) */
#define PT_S9 200 /* offsetof(struct pt_regs, s9) */
#define PT_S10 208 /* offsetof(struct pt_regs, s10) */
#define PT_S11 216 /* offsetof(struct pt_regs, s11) */
#define PT_SP 16 /* offsetof(struct pt_regs, sp) */
#define PT_TP 32 /* offsetof(struct pt_regs, tp) */
#define PT_A0 80 /* offsetof(struct pt_regs, a0) */
#define PT_A1 88 /* offsetof(struct pt_regs, a1) */
#define PT_A2 96 /* offsetof(struct pt_regs, a2) */
#define PT_A3 104 /* offsetof(struct pt_regs, a3) */
#define PT_A4 112 /* offsetof(struct pt_regs, a4) */
#define PT_A5 120 /* offsetof(struct pt_regs, a5) */
#define PT_A6 128 /* offsetof(struct pt_regs, a6) */
#define PT_A7 136 /* offsetof(struct pt_regs, a7) */
#define PT_T0 40 /* offsetof(struct pt_regs, t0) */
#define PT_T1 48 /* offsetof(struct pt_regs, t1) */
#define PT_T2 56 /* offsetof(struct pt_regs, t2) */
#define PT_T3 224 /* offsetof(struct pt_regs, t3) */
#define PT_T4 232 /* offsetof(struct pt_regs, t4) */
#define PT_T5 240 /* offsetof(struct pt_regs, t5) */
#define PT_T6 248 /* offsetof(struct pt_regs, t6) */
#define PT_GP 24 /* offsetof(struct pt_regs, gp) */
#define PT_ORIG_A0 280 /* offsetof(struct pt_regs, orig_a0) */
#define PT_STATUS 256 /* offsetof(struct pt_regs, status) */
#define PT_BADADDR 264 /* offsetof(struct pt_regs, badaddr) */
#define PT_CAUSE 272 /* offsetof(struct pt_regs, cause) */
#define SUSPEND_CONTEXT_REGS 0 /* offsetof(struct suspend_context, regs) */

struct pt_regs {
	rt_ubase_t epc; /* 0 */
	rt_ubase_t ra; /* 8 */
	rt_ubase_t sp; /* 16 */
	rt_ubase_t gp; /* 24 */
	rt_ubase_t tp; /* 32 */
	rt_ubase_t t0; /* 40 */
	rt_ubase_t t1; /* 48 */
	rt_ubase_t t2; /* 56 */
	rt_ubase_t s0; /* 64 */
	rt_ubase_t s1; /* 72 */
	rt_ubase_t a0; /* 80 */
	rt_ubase_t a1; /* 88 */
	rt_ubase_t a2; /* 96 */
	rt_ubase_t a3; /* 104 */
	rt_ubase_t a4; /* 112 */
	rt_ubase_t a5; /* 120 */
	rt_ubase_t a6; /* 128 */
	rt_ubase_t a7; /* 136 */
	rt_ubase_t s2; /* 144 */
	rt_ubase_t s3; /* 152 */
	rt_ubase_t s4; /* 160 */
	rt_ubase_t s5; /* 168 */
	rt_ubase_t s6; /* 176 */
	rt_ubase_t s7; /* 184 */
	rt_ubase_t s8; /* 192 */
	rt_ubase_t s9; /* 200 */
	rt_ubase_t s10; /* 208 */
	rt_ubase_t s11; /* 216 */
	rt_ubase_t t3; /* 224 */
	rt_ubase_t t4; /* 232 */
	rt_ubase_t t5; /* 240 */
	rt_ubase_t t6; /* 248 */
	/* Supervisor/Machine CSRs */
	rt_ubase_t status; /* 256 */
	rt_ubase_t badaddr; /* 264 */
	rt_ubase_t cause; /* 272 */
	/* a0 value before the syscall */
	rt_ubase_t orig_a0; /* 280 */
} __attribute__((aligned(8)));

struct suspend_context {
	struct pt_regs stack_frame;
	rt_ubase_t mscratch;
	rt_ubase_t mie;

	/* interrupt handler */
	rt_ubase_t mmisc_ctl;
	rt_ubase_t mtvt;
	rt_ubase_t mtvt2;
	rt_ubase_t mtvec;
} __attribute__((aligned(8)));

#ifdef __cplusplus
}
#endif
#endif
