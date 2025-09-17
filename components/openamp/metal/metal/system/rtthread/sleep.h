/*
 * Copyright (c) 2018, Linaro Limited. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	rtthread/sleep.h
 * @brief	rtthread sleep primitives for libmetal.
 */

#ifndef __METAL_SLEEP__H__
#error "Include metal/sleep.h instead of metal/rtthread/sleep.h"
#endif

#ifndef __METAL_RTTHREAD_SLEEP__H__
#define __METAL_RTTHREAD_SLEEP__H__

#ifdef __cplusplus
extern "C" {
#endif

static inline int __metal_sleep_usec(unsigned int usec)
{
	rt_tick_t xDelay = (usec / 1000) / (1000 / RT_TICK_PER_SECOND);

	rt_thread_delay(xDelay ? xDelay : 1);

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_RTTHREAD_SLEEP__H__ */
