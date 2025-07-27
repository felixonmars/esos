#include <rthw.h>
#include <rtdef.h>
#include <spacemit_sdk_soc.h>

void rt_hw_cpu_icache_enable(void)
{
#ifdef RT_USING_CACHE
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
#endif
#endif
}

void rt_hw_cpu_icache_disable(void)
{
#ifdef RT_USING_CACHE
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
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
#endif
#endif
}

void rt_hw_cpu_dcache_enable(void)
{
#ifdef RT_USING_CACHE
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
#endif
#endif
}

void rt_hw_cpu_dcache_disable(void)
{
#ifdef RT_USING_CACHE
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
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
#endif
#endif
	asm volatile ("fence");
}

