/*
 * Copyright (c) 2018, Linaro Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	rtthread/template/sys.h
 * @brief	rtthread template system primitives for libmetal.
 */

#ifndef __METAL_RTTHREAD_SYS__H__
#error "Include metal/sys.h instead of metal/rtthread/spacemit/sys.h"
#endif

#ifndef __METAL_RTTHREAD_TEMPLATE_SYS__H__
#define __METAL_RTTHREAD_TEMPLATE_SYS__H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	metal_spacemit_irq_int
 *
 * Spacemit interrupt controller initialization. It will initialize
 * the metal Spacemit IRQ controller data structure.
 *
 * @return 0 for success, or negative value for failure
 */
int metal_spacemit_irq_init(void);

#ifdef METAL_INTERNAL

void sys_irq_enable(unsigned int vector);

void sys_irq_disable(unsigned int vector);

#endif /* METAL_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_RTTHREAD_TEMPLATE_SYS__H__ */
