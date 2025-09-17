/*
 * Copyright (c) 2018, Linaro Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/template/sys.c
 * @brief	machine specific system primitives implementation.
 */

#include <metal/io.h>
#include <metal/sys.h>
#include <metal/utilities.h>
#include <stdint.h>

void sys_irq_restore_enable(unsigned int flags)
{
	metal_unused(flags);
	/* Add implementation here */
	rt_hw_interrupt_enable(flags);
}

unsigned int sys_irq_save_disable(void)
{
	/* Add implementation here */
	return rt_hw_interrupt_disable();
}

void sys_irq_enable(unsigned int vector)
{
	metal_unused(vector);

	/* Add implementation here */
	rt_hw_interrupt_umask(vector);
}

void sys_irq_disable(unsigned int vector)
{
	metal_unused(vector);

	/* Add implementation here */
	rt_hw_interrupt_mask(vector);
}

void metal_machine_cache_flush(void *addr, unsigned int len)
{
	metal_unused(addr);
	metal_unused(len);

	/* Add implementation here */
	rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, addr, len);	
}

void metal_machine_cache_invalidate(void *addr, unsigned int len)
{
	metal_unused(addr);
	metal_unused(len);

	/* Add implementation here */
	rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, addr, len);
}

void metal_generic_default_poll(void)
{
	/* Add implementation here */
}

void *metal_machine_io_mem_map(void *va, metal_phys_addr_t pa,
			       size_t size, unsigned int flags)
{
	metal_unused(pa);
	metal_unused(size);
	metal_unused(flags);

	/* Add implementation here */

	return va;
}
