/*
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "riscv_encoding.h"


.macro DECLARE_INT_HANDLER  INT_HDL_NAME
#if defined(__riscv_xlen) && (__riscv_xlen == 32)
    .word \INT_HDL_NAME
#else
    .dword \INT_HDL_NAME
#endif
.endm

    .text
    .align  2

    .weak eclic_msip_handler
    .weak eclic_mtip_handler
    .weak eclic_inter_core_int_handler
    .globl vector_base
    .type vector_base, @object
vector_base:
    jal __cpu_resume_enter
    DECLARE_INT_HANDLER     default_intexc_handler          /* 1: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 2: Reserved */
    DECLARE_INT_HANDLER     eclic_msip_handler              /* 3: Machine software interrupt */

    DECLARE_INT_HANDLER     default_intexc_handler          /* 4: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 5: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 6: Reserved */
    DECLARE_INT_HANDLER     eclic_mtip_handler              /* 7: Machine timer interrupt */

    DECLARE_INT_HANDLER     default_intexc_handler          /* 8: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 9: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 10: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 11: Reserved */

    DECLARE_INT_HANDLER     default_intexc_handler          /* 12: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 13: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 14: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 15: Reserved */

    DECLARE_INT_HANDLER     eclic_inter_core_int_handler    /* 16: CIDU Inter Core Interrupt */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 17: Reserved */
    DECLARE_INT_HANDLER     default_intexc_handler          /* 18: Reserved */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler   /* 19: Interrupt num 0 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 20: Interrupt num 1 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 21: Interrupt num 2 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 22: Interrupt num 3 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 23: Interrupt num 4 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 24: Interrupt num 5 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 25: Interrupt num 6 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 26: Interrupt num 7 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 27: Interrupt num 8 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 28: Interrupt num 9 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 29: Interrupt num 10 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 30: Interrupt num 11 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 31: Interrupt num 12 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 32: Interrupt num 13 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 33: Interrupt num 14 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 34: Interrupt num 15 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 35: Interrupt num 16 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 36: Interrupt num 17 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 37: Interrupt num 18 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 38: Interrupt num 19 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 39: Interrupt num 20 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 40: Interrupt num 21 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 41: Interrupt num 22 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 42: Interrupt num 23 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 43: Interrupt num 24 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 44: Interrupt num 25 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 45: Interrupt num 26 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 46: Interrupt num 27 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 47: Interrupt num 28 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 48: Interrupt num 29 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 49: Interrupt num 30 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 50: Interrupt num 31 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 51: Interrupt num 32 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 52: Interrupt num 33 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 53: Interrupt num 34 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 54: Interrupt num 35 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 55: Interrupt num 36 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 56: Interrupt num 37 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 57: Interrupt num 28 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 58: Interrupt num 39 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 59: Interrupt num 40 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 60: Interrupt num 41 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 61: Interrupt num 42 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 62: Interrupt num 43 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 63: Interrupt num 44 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 64: Interrupt num 45 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 65: Interrupt num 46 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 66: Interrupt num 47 */
    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 67: Interrupt num 48 */

    DECLARE_INT_HANDLER     Spacemit_External_IRQHandler  /* 68: Interrupt num 49 */

    .text
    .align  2
_start:
    .text
    .align  2
    .globl  Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
/**
 * Reset Handler called on controller reset
 */
    /* ===== Startup Stage 1 ===== */
    /* Disable Global Interrupt */
    csrc CSR_MSTATUS, MSTATUS_MIE

    /* Initialize GP and TP */
    .option push
    .option norelax
    la gp, __global_pointer$
    /* la tp, __tls_base */
    .option pop

    /* Set correct sp for current cpu */
    la sp, __StackTop

    /*
     * Set the the NMI base mnvec to share
     * with mtvec by setting CSR_MMISC_CTL
     * bit 9 NMI_CAUSE_FFF to 1
     */
    li t0, MMISC_CTL_NMI_CAUSE_FFF
    csrs CSR_MMISC_CTL, t0

    /*
     * Intialize ECLIC vector interrupt
     * base address mtvt to vector_base
     */
    la t0, vector_base
    csrw CSR_MTVT, t0

    /*
     * Set ECLIC non-vector entry to be controlled
     * by mtvt2 CSR register.
     * Intialize ECLIC non-vector interrupt
     * base address mtvt2 to irq_entry.
     */
    la t0, irq_entry
    csrw CSR_MTVT2, t0
    csrs CSR_MTVT2, 0x1

    /*
     * Set Exception Entry MTVEC to early_exc_entry
     * Due to settings above, Exception and NMI
     * will share common entry.
     * This early_exc_entry is only used during early
     * boot stage before main
     */
    la t0, early_exc_entry
    csrw CSR_MTVEC, t0

    /* Set the interrupt processing mode to ECLIC mode */
    li t0, 0x3f
    csrc CSR_MTVEC, t0
    csrs CSR_MTVEC, 0x3

    /* ===== Startup Stage 2 ===== */

    /* Enable FPU and Vector Unit if f/d/v exist in march */
#if defined(__riscv_flen) && __riscv_flen > 0
    /* Enable FPU, and set state to initial */
    li t0, MSTATUS_FS
    csrc mstatus, t0
    li t0, MSTATUS_FS_INITIAL
    csrs mstatus, t0
#endif

#if defined(__riscv_vector)
    /* Enable Vector, and set state to initial */
    li t0, MSTATUS_VS
    csrc mstatus, t0
    li t0, MSTATUS_VS_INITIAL
    csrs mstatus, t0
#endif

    /* Enable mcycle and minstret counter */
    csrci CSR_MCOUNTINHIBIT, 0x5

    /* Clear bss section */
    la      a0, __bss_start__
    la      a1, __bss_end__
    bgeu    a0, a1, 2f
1:
    sw      zero, (a0)
    addi    a0, a0, 4
    bltu    a0, a1, 1b
2:

    jal SystemInit

    /*
     * When all initialization steps done
     * set exception entry to correct exception
     * entry and jump to main.
     * And set the interrupt processing mode to
     * ECLIC mode
     */
    la t0, exc_entry
    csrw CSR_MTVEC, t0
    li t0, 0x3f
    csrc CSR_MTVEC, t0
    csrs CSR_MTVEC, 0x3

    /* BPU cold bringup need time, so enable BPU before enter to main */
    li t0, MMISC_CTL_BPU
    csrs CSR_MMISC_CTL, t0

    jal entry

__exit:
    j      __exit

/* Early boot exception entry before main */
.align 6
.global early_exc_entry
.type early_exc_entry, @function
early_exc_entry:
    wfi
    j early_exc_entry
