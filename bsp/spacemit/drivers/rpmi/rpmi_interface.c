#include <rtthread.h>
#include <rthw.h>
#include <stdint.h>
#include <string.h>
#include <rtconfig.h>

/* Include only the necessary types without the problematic inline functions */
typedef unsigned long rpmi_size_t;
typedef unsigned int rpmi_uint32_t;
typedef unsigned long long rpmi_uint64_t;

void rt_hw_us_delay(rt_uint32_t us);

#ifdef RT_LIBRPMI_DEBUG
#define RPMI_DEBUG(fmt, ...) rt_kprintf("[RPMI] " fmt, ##__VA_ARGS__)
#else
#define RPMI_DEBUG(fmt, ...)
#endif

/*
 * The following functions are our implementations of the RPMI environment
 * functions that override the static inline versions in librpmi_env.h
 */

/* Memory management functions */
void* rpmi_env_malloc(rpmi_size_t size)
{
	return rt_malloc(size);
}

void* rpmi_env_zalloc(rpmi_size_t size)
{
	return rt_calloc(1, size);
}

void rpmi_env_free(void* ptr)
{
	if (ptr) {
		rt_free(ptr);
	}
}

/* String operations */
void* rpmi_env_memcpy(void* dst, const void* src, rpmi_size_t len)
{
	return rt_memcpy(dst, src, len);
}

void* rpmi_env_memset(void* dst, int val, rpmi_size_t len)
{
	return rt_memset(dst, val, len);
}

char* rpmi_env_strncpy(char* dst, const char* src, rpmi_size_t len)
{
	return rt_strncpy(dst, src, len);
}

rpmi_size_t rpmi_env_strlen(const char* str)
{
	return rt_strlen(str);
}

/* Lock operations */
void* rpmi_env_alloc_lock(void)
{
	char tmp[32];
	static int count;

	rt_snprintf(tmp, 32, "rpmi_lock:%d", count++);

	return rt_mutex_create(tmp, RT_IPC_FLAG_FIFO);
}

void rpmi_env_free_lock(void* lock)
{
	rt_mutex_delete(lock);
}

void rpmi_env_lock(void* lock)
{
	rt_mutex_take(lock, RT_WAITING_FOREVER);
}

void rpmi_env_unlock(void* lock)
{
	rt_mutex_release(lock);
}

/* Time related functions */
rpmi_uint64_t rpmi_env_get_timestamp(void)
{
	/* Get current timestamp in microseconds */
	return (rpmi_uint64_t)rt_tick_get() * 1000 / RT_TICK_PER_SECOND * 1000;
}

void rpmi_env_udelay(rpmi_uint32_t usecs)
{
	rt_hw_us_delay(usecs);
}

void rpmi_env_mdelay(rpmi_uint64_t msecs)
{
	/* Delay for specified milliseconds */
	rt_thread_mdelay(msecs);
}

/* Print function */
void rpmi_env_print(const char* str)
{
	rt_kprintf("%s", str);
}

/* Printf wrapper function */
int rpmi_env_printf(const char* fmt, ...)
{
    /* TODO */	
    return 0;
}

/* Write a 32-bit value to memory */
void rpmi_env_writel(rpmi_uint64_t addr, rpmi_uint32_t val)
{
	*(volatile rpmi_uint32_t*)(uintptr_t)(addr) = val;
}
