/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/hwspinlock.h>

static rt_err_t __hwspinlock_control(rt_device_t dev, int cmd, void *args)
{
	rt_err_t result = RT_EOK;
	struct rt_hwspinlock_bank *bank = (struct rt_hwspinlock_bank *)dev;
	struct rt_hwspinlock_device *hwlock = (struct rt_hwspinlock_device *)args;
	rt_int32_t id;

	if (bank->ops->control)
		result = bank->ops->control(hwlock, cmd);

	return result;
}

struct rt_hwspinlock_device *of_hwspinlock_get(struct dtb_node *np)
{
	struct rt_hwspinlock_bank *bank;
	struct rt_hwspinlock_device *hwlock;
	struct rt_device *dev;
	phandle parent_phandle = 0;
	struct dtb_node *pnp;
	const char *hwlock_name;
	rt_err_t err;
	int id;

	dtb_node_read_u32(np, "hwlock", &parent_phandle);
	pnp = dtb_node_find_node_by_phandle(parent_phandle);
	if (pnp == RT_NULL) {
		rt_kprintf("can't find hwlock parent\n");
		return RT_NULL;
	}

	bank = rt_dtb_data(pnp);

	if (dtb_node_read_u32(np, "hwlock-id", &id))
		return RT_NULL;

	return &bank->hwlock[id];
}

rt_err_t rt_device_hwspinlock_register(struct rt_hwspinlock_bank *bank,
				       const char *name,
				       const struct rt_hwspinlock_ops *ops,
				       const void *user_data)
{
	rt_err_t result = RT_EOK;
#ifdef RT_USING_DEVICE_OPS
	bank->dev->ops = &hwspinlock_ops;
#else
	bank->dev.init = RT_NULL;
	bank->dev.open = RT_NULL;
	bank->dev.close = RT_NULL;
	bank->dev.read = RT_NULL;
	bank->dev.write = RT_NULL;
	bank->dev.control = __hwspinlock_control;
#endif

	bank->dev.type = RT_Device_Class_Miscellaneous;
	bank->ops = ops;
	bank->dev.user_data = (void *)user_data;

	if (bank->dev.node)
		rt_dtb_data(bank->dev.node) = bank;
	result = rt_device_register(&bank->dev, name, RT_DEVICE_FLAG_RDWR);

	return result;
}
