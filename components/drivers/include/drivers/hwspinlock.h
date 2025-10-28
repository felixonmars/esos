/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HWSPINLOCK_H__
#define __HWSPINLOCK_H__

#include <rtthread.h>

#define HWSPINLOCK_UNLOCK 0
#define HWSPINLOCK_TRYLOCK 1

struct rt_hwspinlock_bank;

struct rt_hwspinlock_device {
	struct rt_hwspinlock_bank *bank;
	void *priv;
};

struct rt_hwspinlock_ops {
	rt_err_t (*control)(struct rt_hwspinlock_device *dev, int cmd);
};

struct rt_hwspinlock_bank {
	struct rt_device dev;
	rt_uint32_t num_locks;
	rt_int32_t base_id;
	const struct rt_hwspinlock_ops *ops;
	struct rt_hwspinlock_device *hwlock;
};

rt_err_t rt_device_hwspinlock_register(struct rt_hwspinlock_bank *bank,
				 const char *name,
				 const struct rt_hwspinlock_ops *ops,
				 const void *user_data);

struct rt_hwspinlock_device *of_hwspinlock_get(struct dtb_node *np);

#endif
