/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	rtthread/alloc.c
 * @brief	rtthread libmetal memory allocattion definitions.
 */

#ifndef __METAL_ALLOC__H__
#error "Include metal/alloc.h instead of metal/rtthread/alloc.h"
#endif

#ifndef __METAL_RTTHREAD_ALLOC__H__
#define __METAL_RTTHREAD_ALLOC__H__

#ifdef __cplusplus
extern "C" {
#endif

static inline void *__metal_allocate_memory(unsigned int size)
{
	/* cacheline aligned */
	return rt_malloc_align(size, 32);
}

static inline void __metal_free_memory(void *ptr)
{
	rt_free_align(ptr);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_RTTHREAD_ALLOC__H__ */
