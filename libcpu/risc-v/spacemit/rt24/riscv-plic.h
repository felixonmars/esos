/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-03     Bernard      The first version
 */

#ifndef RISCV_PLIC_H__
#define RISCV_PLIC_H__

#include <riscv-ops.h>

#ifndef PLIC_BASE_ADDR
#define PLIC_BASE_ADDR 0xe0000000
#endif

/* Priority Register - 32 bits per source */
#define PLIC_PRIORITY_OFFSET (0x00000000UL)
#define PLIC_PRIORITY_SHIFT_PER_SOURCE 2

/* Pending Register - 1 bit per soirce */
#define PLIC_PENDING_OFFSET (0x00001000UL)
#define PLIC_PENDING_SHIFT_PER_SOURCE 0

/* Enable Register - 0x80 per target */
#define PLIC_ENABLE_OFFSET (0x00002000UL)
#define PLIC_ENABLE_SHIFT_PER_TARGET 27

/* Priority Threshold Register - 0x1000 per target */
#define PLIC_THRESHOLD_OFFSET (0x00200000UL)

/* Claim Register - 0x1000 per target */
#define PLIC_CLAIM_OFFSET (0x00200004UL)

#if defined(__GNUC__) && !defined(__ASSEMBLER__)
__attribute__((always_inline)) static inline void __plic_set_feature(unsigned int feature)
{
    volatile unsigned int *feature_ptr = (volatile unsigned int *)PLIC_BASE_ADDR;
    *feature_ptr = feature;
}

__attribute__((always_inline)) static inline void __plic_set_threshold(unsigned int threshold)
{
    unsigned int hart_id = read_csr(mhartid);
    volatile unsigned int *threshold_ptr = (volatile unsigned int *)(PLIC_BASE_ADDR +
                                                                     PLIC_THRESHOLD_OFFSET +
                                                                     (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET));
    *threshold_ptr = threshold;
}

__attribute__((always_inline)) static inline void __plic_set_priority(unsigned int source, unsigned int priority)
{
    unsigned int hartid = read_csr(mhartid);
    volatile unsigned int *priority_ptr = (volatile unsigned int *)(PLIC_BASE_ADDR +
                                                                    PLIC_PRIORITY_OFFSET +
								    (hartid << PLIC_ENABLE_SHIFT_PER_TARGET) +
                                                                    (source << PLIC_PRIORITY_SHIFT_PER_SOURCE));
    *priority_ptr = priority;
}

__attribute__((always_inline)) static inline void __plic_set_pending(unsigned int source)
{
    unsigned int hartid = read_csr(mhartid);
    volatile unsigned int *current_ptr = (volatile unsigned int *)(PLIC_BASE_ADDR +
								   (hartid << PLIC_ENABLE_SHIFT_PER_TARGET) +
                                                                   PLIC_PENDING_OFFSET +
                                                                   ((source >> 5) << 2));
    *current_ptr = (1 << (source & 0x1F));
}

__attribute__((always_inline)) static inline void __plic_clr_pending(unsigned int source)
{
    volatile unsigned int *current_ptr = (volatile unsigned int *)(PLIC_BASE_ADDR +
                                                                   PLIC_PENDING_OFFSET +
                                                                   ((source >> 5) << 2));
    *current_ptr &= ~(1 << (source & 0x1F));
}

__attribute__((always_inline)) static inline void __plic_irq_enable(unsigned int source)
{
    unsigned int hart_id = read_csr(mhartid);
    volatile unsigned int *current_ptr = (volatile unsigned int *)(PLIC_BASE_ADDR +
                                                                   PLIC_ENABLE_OFFSET +
                                                                   (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET) +
                                                                   ((source >> 5) << 2));
    unsigned int current = *current_ptr;
    current = current | (1 << (source & 0x1F));
    *current_ptr = current;
}

__attribute__((always_inline)) static inline unsigned int  __plic_irq_is_enabled(unsigned int source)
{
    unsigned int hart_id = read_csr(mhartid);
    volatile unsigned int *current_ptr = (volatile unsigned int *)(PLIC_BASE_ADDR +
                                                                   PLIC_ENABLE_OFFSET +
                                                                   (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET) +
                                                                   ((source >> 5) << 2));
    unsigned int current = *current_ptr;
    return (current & (1 << (source & 0x1F)));
}

__attribute__((always_inline)) static inline void __plic_irq_disable(unsigned int source)
{
    unsigned int hart_id = read_csr(mhartid);
    volatile unsigned int *current_ptr = (volatile unsigned int *)(PLIC_BASE_ADDR +
                                                                   PLIC_ENABLE_OFFSET +
                                                                   (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET) +
                                                                   ((source >> 5) << 2));
    unsigned int current = *current_ptr;
    current = current & ~((1 << (source & 0x1F)));
    *current_ptr = current;
}

__attribute__((always_inline)) static inline unsigned int __plic_irq_claim(void)
{
    unsigned int hart_id = read_csr(mhartid);
    volatile unsigned int *claim_addr = (volatile unsigned int *)(PLIC_BASE_ADDR +
                                                                  PLIC_CLAIM_OFFSET +
                                                                  (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET));
    return *claim_addr;
}

__attribute__((always_inline)) static inline void __plic_irq_complete(unsigned int source)
{
    unsigned int hart_id = read_csr(mhartid);
    volatile unsigned int *claim_addr = (volatile unsigned int *)(PLIC_BASE_ADDR +
                                                                  PLIC_CLAIM_OFFSET +
                                                                  (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET));
    *claim_addr = source;
}
#endif /* end of __GNUC__ */

#endif
