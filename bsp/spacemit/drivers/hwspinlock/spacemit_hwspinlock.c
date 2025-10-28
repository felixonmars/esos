/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/hwspinlock.h>

#define SPINLOCK_LOCK(x)	(0x0 + 0x4 * x)
#define SPINLOCK_NOTTAKEN	0

static rt_err_t spacemit_hwspinlock_trylock(struct rt_hwspinlock_device *hwlock)
{
	void *base = hwlock->priv;

	if (readl(base) == SPINLOCK_NOTTAKEN)
		return RT_EOK;
	else
		return -RT_ETIMEOUT;
}

static void spacemit_hwspinlock_unlock(struct rt_hwspinlock_device *hwlock)
{
	void *base = hwlock->priv;

	writel(SPINLOCK_NOTTAKEN, base);
}

static rt_err_t spacemit_hwspinlock_control(struct rt_hwspinlock_device *hwlock, int cmd)
{
	rt_err_t ret;

	switch (cmd) {
		case HWSPINLOCK_UNLOCK:
			spacemit_hwspinlock_unlock(hwlock);
			ret = RT_EOK;
			break;
		case HWSPINLOCK_TRYLOCK:
			ret = spacemit_hwspinlock_trylock(hwlock);
			break;
		default:
			ret = RT_EINVAL;
	}

	return ret;
}

static const struct rt_hwspinlock_ops spacemit_hwspinlock_ops = {
	.control = spacemit_hwspinlock_control,
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k3-hwlock", .data = "k3-hwlock" },
	{},
};

int spacemit_hwspinlock_probe(void)
{
	int i, j;
	struct rt_hwspinlock_bank *bank;
	struct rt_hwspinlock_device *hwlock;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	void *base;
	rt_uint32_t val;

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); i++) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			bank = rt_calloc(1, sizeof(struct rt_hwspinlock_bank));
			if (!bank) {
				rt_kprintf("%s:%d, calloc bank failed\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}
			if (!dtb_node_read_u32(compatible_node, "num_locks", &val)) {
				bank->num_locks = val;
			} else
				return -RT_EINVAL;

			bank->hwlock = rt_calloc(val, sizeof(struct rt_hwspinlock_device));
			if (!bank->hwlock) {
				rt_kprintf("%s:%d, calloc lock failed\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			base = (void *)dtb_node_get_addr_index(compatible_node, 0);
			if (base < 0) {
				rt_kprintf("get hwspinlock base addr failed\n");
				return -RT_ERROR;
			}

			for (j = 0; j < val; j++) {
				hwlock = &bank->hwlock[j];
				hwlock->bank = bank;
				hwlock->priv = (void *)(base + SPINLOCK_LOCK(j));
			}

			bank->dev.node = compatible_node;
			rt_device_hwspinlock_register(bank, (char *)__compatible[i].data, &spacemit_hwspinlock_ops, RT_NULL);
		}
	}
	return 0;
}
INIT_DEVICE_EXPORT(spacemit_hwspinlock_probe);
