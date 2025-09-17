/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	rtthread/mutex.h
 * @brief	rtthread mutex primitives for libmetal.
 */

#ifndef __METAL_MUTEX__H__
#error "Include metal/mutex.h instead of metal/rtthread/mutex.h"
#endif

#ifndef __METAL_RTTHREAD_MUTEX__H__
#define __METAL_RTTHREAD_MUTEX__H__

#include <metal/assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *name;
	rt_mutex_t m;
} metal_mutex_t;

static inline void __metal_mutex_init(metal_mutex_t *mutex)
{
	metal_assert(mutex);
	mutex->m = rt_mutex_create(mutex->name, RT_IPC_FLAG_PRIO);
	metal_assert(mutex->m);
}

static inline void __metal_mutex_deinit(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m);
	rt_mutex_delete(mutex->m);
	mutex->m = RT_NULL;
}

static inline int __metal_mutex_try_acquire(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m);
	return rt_mutex_trytake(mutex->m);
}

static inline void __metal_mutex_acquire(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m);
	rt_mutex_take(mutex->m, RT_WAITING_FOREVER);
}

static inline void __metal_mutex_release(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m);
	rt_mutex_release(mutex->m);
}

static inline int __metal_mutex_is_acquired(metal_mutex_t *mutex)
{
	metal_assert(mutex && mutex->m);
	/* rtthread has not complite this function, so return 0 by default */
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_RTTHREAD_MUTEX__H__ */
