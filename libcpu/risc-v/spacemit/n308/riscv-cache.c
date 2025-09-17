/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtdef.h>
#include <spacemit_sdk_soc.h>

void rt_hw_cpu_icache_enable(void)
{
#ifdef RT_USING_CACHE
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
    if (ICachePresent()) { // Check whether icache real present or not
        EnableICache();
    }
#endif
#endif
}

void rt_hw_cpu_icache_disable(void)
{
#ifdef RT_USING_CACHE
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
    if (ICachePresent()) { // Check whether icache real present or not
        DisableICache();
    }
#endif
#endif
}

rt_base_t rt_hw_cpu_icache_status(void)
{
#ifdef RT_USING_CACHE
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
	return 1;
#endif
#endif
}

void rt_hw_cpu_icache_ops(int ops, void* addr, int size)
{
#ifdef RT_USING_CACHE
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
#if defined(__CCM_PRESENT) && (__CCM_PRESENT == 1)
	if (ops == RT_HW_CACHE_FLUSH)
		/* Do nothing */;
	else if (ops == RT_HW_CACHE_INVALIDATE)
		MInvalICacheLines((unsigned long)addr, ((size % 32) > 0) ? ((size / 32) + 1) : (size / 32));
	else if (ops == (RT_HW_CACHE_FLUSH | RT_HW_CACHE_INVALIDATE))
		/* Do nothing */;
#endif
#endif
#endif
}

void rt_hw_cpu_dcache_enable(void)
{
#ifdef RT_USING_CACHE
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    if (DCachePresent()) { // Check whether dcache real present or not
        EnableDCache();
    }
#endif
#endif
}

void rt_hw_cpu_dcache_disable(void)
{
#ifdef RT_USING_CACHE
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    if (DCachePresent()) { // Check whether dcache real present or not
        DisableDCache();
    }
#endif
#endif
}

rt_base_t rt_hw_cpu_dcache_status(void)
{
#ifdef RT_USING_CACHE
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
	return 1;
#endif
#endif
	return 0;
}

void rt_hw_cpu_dcache_ops(int ops, void* addr, int size)
{
	/* the cache-line size = 32 byte */
#ifdef RT_USING_CACHE
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
#if defined(__CCM_PRESENT) && (__CCM_PRESENT == 1)
	if (ops == RT_HW_CACHE_FLUSH)
		MFlushDCacheLines((unsigned long)addr, ((size % 32) > 0) ? ((size / 32) + 1) : (size / 32));
	else if (ops == RT_HW_CACHE_INVALIDATE)
		MInvalDCacheLines((unsigned long)addr, ((size % 32) > 0) ? ((size / 32) + 1) : (size / 32));
	else if (ops == (RT_HW_CACHE_FLUSH | RT_HW_CACHE_INVALIDATE))
		MFlushInvalDCacheLines((unsigned long)addr, ((size % 32) > 0) ? ((size / 32) + 1) : (size / 32));
#endif
#endif
#endif
}

