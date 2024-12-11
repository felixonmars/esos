/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-10-03     Bernard      The first version
 */

#ifndef RISCV_OPS_H__
#define RISCV_OPS_H__

#if defined(__GNUC__) && !defined(__ASSEMBLER__)

#define read_csr(reg) ({ unsigned long __tmp;                               \
    asm volatile ("csrr %0, " #reg : "=r"(__tmp));                          \
        __tmp; })

#define write_csr(reg, val) ({                                              \
    if (__builtin_constant_p(val) && (unsigned long)(val) < 32)             \
        asm volatile ("csrw " #reg ", %0" :: "i"(val));                     \
    else                                                                    \
        asm volatile ("csrw " #reg ", %0" :: "r"(val)); })

#define set_csr(reg, bit) ({ unsigned long __tmp;                           \
    if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32)             \
        asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit));   \
    else                                                                    \
        asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit));   \
            __tmp; })

#define clear_csr(reg, bit) ({ unsigned long __tmp;                         \
    if (__builtin_constant_p(bit) && (unsigned long)(bit) < 32)             \
        asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "i"(bit));   \
    else                                                                    \
        asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "r"(bit));   \
            __tmp; })

static inline unsigned int __raw_readl(const volatile void *addr)
{
	unsigned int val;

	asm volatile("lw %0, 0(%1)" : "=r" (val) : "r" (addr));
	return val;
}

static inline void __raw_writel(unsigned int val, volatile void *addr)
{
	asm volatile("sw %0, 0(%1)" : : "r" (val), "r" (addr));
}

#define le32_to_cpu(x) x
#define cpu_to_le32(x) x

/*
 * Unordered I/O memory access primitives.  These are even more relaxed than
 * the relaxed versions, as they don't even order accesses between successive
 * operations to the I/O regions.
 */
#define readl_cpu(c)            ({ unsigned int __r = le32_to_cpu((unsigned int)__raw_readl(c)); __r; })
#define writel_cpu(v, c)	((void)__raw_writel((unsigned int)cpu_to_le32(v), (c)))

#define RISCV_FENCE_ASM(p, s)	"\tfence " #p "," #s "\n"
#define RISCV_FENCE(p, s) \
        ({ __asm__ __volatile__ (RISCV_FENCE_ASM(p, s) : : : "memory"); })

#define __io_br()       do {} while (0)
#define __io_ar(v)	RISCV_FENCE(i, ir)
#define __io_bw()	RISCV_FENCE(w, o)
#define __io_aw()	do { } while (0)

#define readl(c)	({ unsigned int __v; __io_br(); __v = readl_cpu(c); __io_ar(__v); __v; })
#define writel(v, c)	({ __io_bw(); writel_cpu((v), (c)); __io_aw(); })

#endif /* end of __GNUC__ */

#endif
