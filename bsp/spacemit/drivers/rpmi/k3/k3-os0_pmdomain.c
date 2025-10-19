/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <dtb_head.h>
#include <drivers/dtb_node.h>
#include "../spacemit-rpmi.h"

#define DEVICE_POWER_CTRL_BASE		0xd4282800
#define DEVICE_POWER_STATE_OFFSET	0xf0
#define PWR_NAME_MAX			15
struct rt_domain_data {
	char name[PWR_NAME_MAX];
	uint32_t offset;
	uint32_t bit_hw_mode;
	uint32_t bit_sleep2;
	uint32_t bit_sleep1;
	uint32_t bit_isolation;
	uint32_t bit_pwr_stat;
	uint32_t bit_hw_pwr_stat;
	uint32_t bit_auto_pwr_on;
	uint32_t use_hw;
	enum rpmi_device_power_state current_state;
};

struct rt_domain_data pm_data[1];

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "test-domain" },
	{},
};

struct rpmi_device_power_attrs k3_os0_domain_data[1] = {
	[0] = {
		.name = "domain-test",
	},
};

static rt_int32_t  _k3_os0_domain_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_domain_config *config = priv;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	struct rt_device *dev;
	int property_size;
	rt_uint32_t *property_ptr;
	rt_uint32_t u32_value;

	config->domain_count = 1;

	config->domain_data = k3_os0_domain_data;

	for (int i = 0; i < config->domain_count; i++) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			memcpy(pm_data[i].name, __compatible[i].compatible, PWR_NAME_MAX);
			pm_data[i].current_state = 0;
			for_each_property_cell(compatible_node, "bit_sleep2", u32_value, property_ptr, property_size)
			{
				pm_data[i].bit_sleep2 = u32_value;
			}
			for_each_property_cell(compatible_node, "bit_sleep1", u32_value, property_ptr, property_size)
			{
				pm_data[i].bit_sleep1 = u32_value;
			}
			for_each_property_cell(compatible_node, "bit_isolation", u32_value, property_ptr, property_size)
			{
				pm_data[i].bit_isolation = u32_value;
			}
			for_each_property_cell(compatible_node, "use_hw", u32_value, property_ptr, property_size)
			{
				pm_data[i].use_hw = u32_value;
			}
			if (pm_data[i].use_hw == 0)
				for_each_property_cell(compatible_node, "bit_pwr_stat", u32_value, property_ptr, property_size)
				{
					pm_data[i].bit_pwr_stat = u32_value;
				}
			else {
				for_each_property_cell(compatible_node, "bit_hw_pwr_stat", u32_value, property_ptr, property_size)
				{
					pm_data[i].bit_hw_pwr_stat = u32_value;
				}
				for_each_property_cell(compatible_node, "bit_hw_mode", u32_value, property_ptr, property_size)
				{
					pm_data[i].bit_hw_mode = u32_value;
				}
			}
			for_each_property_cell(compatible_node, "offset", u32_value, property_ptr, property_size)
			{
				pm_data[i].offset = u32_value;
			}
		}
	}

	return 0;
}

/** Set the power domain state ON_STATE */
static enum rpmi_error spacemit_set_state(void *priv, rpmi_uint32_t domain_id, enum rpmi_device_power_state state)
{
	void *base = (void *)DEVICE_POWER_CTRL_BASE;
	rt_uint32_t val;

	if (state == RPMI_DEVICE_POWER_STATE_ON) {
		if (pm_data[domain_id].use_hw == 0) {
			val = readl(base + pm_data[domain_id].offset);
			val |= (1 << pm_data[domain_id].bit_sleep1);
			writel(val, base + pm_data[domain_id].offset);
			//rt_hw_us_delay(20);
			rt_thread_delay(1);

			val = readl(base + pm_data[domain_id].offset);
			val |= (1 << pm_data[domain_id].bit_sleep2);
			writel(val, base + pm_data[domain_id].offset);
			//rt_hw_us_delay(20);
			rt_thread_delay(1);

			val = readl(base + pm_data[domain_id].offset);
			val |= (1 << pm_data[domain_id].bit_isolation);
			writel(val, base + pm_data[domain_id].offset);
			//rt_hw_us_delay(10);
			rt_thread_delay(1);
		} else {
			val = readl(base + pm_data[domain_id].offset);
			val |= (1 << pm_data[domain_id].bit_auto_pwr_on) |
				(1 << pm_data[domain_id].bit_hw_mode);
			writel(val, base + pm_data[domain_id].offset);
			//rt_hw_us_delay(290);
			rt_thread_delay(1);
		}

		for (int loop = 10000; loop >= 0; --loop){
			val = readl(base + DEVICE_POWER_STATE_OFFSET);
			if ((val & (1 << pm_data[domain_id].bit_pwr_stat)) == 1)
				break;
			//rt_hw_us_delay(4);
			rt_thread_delay(1);
		}

		pm_data[domain_id].current_state = 1;
	} else {
		if (pm_data[domain_id].use_hw == 0) {
			val = readl(base + pm_data[domain_id].offset);
			val &= ~(1 << pm_data[domain_id].bit_sleep1);
			writel(val, base + pm_data[domain_id].offset);
			//rt_hw_us_delay(20);
			rt_thread_delay(1);

			val = readl(base + pm_data[domain_id].offset);
			val &= ~(1 << pm_data[domain_id].bit_sleep2);
			writel(val, base + pm_data[domain_id].offset);
			//rt_hw_us_delay(20);
			rt_thread_delay(1);

			val = readl(base + pm_data[domain_id].offset);
			val &= ~(1 << pm_data[domain_id].bit_isolation);
			writel(val, base + pm_data[domain_id].offset);
			//rt_hw_us_delay(10);
			rt_thread_delay(1);
		} else {
			val = readl(base + pm_data[domain_id].offset);
			val &= ~(1 << pm_data[domain_id].bit_auto_pwr_on);
			val &= ~(1 << pm_data[domain_id].bit_hw_mode);
			writel(val, base + pm_data[domain_id].offset);
			//rt_hw_us_delay(290);
			rt_thread_delay(1);
		}

		for (int loop = 10000; loop >= 0; --loop){
			val = readl(base + DEVICE_POWER_STATE_OFFSET);
			if ((val & (1 << pm_data[domain_id].bit_pwr_stat)) == 0)
				break;
			//rt_hw_us_delay(4);
			rt_thread_delay(1);
		}

		pm_data[domain_id].current_state = 0;
	}
	return 0;
}

static enum rpmi_error spacemit_get_state(void *priv, rpmi_uint32_t domain_id, enum rpmi_device_power_state *state)
{
	*state = pm_data[domain_id].current_state;
	return 0;
}

static struct rpmi_domain_platform_ops k3_os0_domain_pops = {
	.set_state = spacemit_set_state,
	.get_state = spacemit_get_state,
};

static struct spacemit_rpmi_domain_ops k3_os0_device_power_ops = {
	.name = "k3-os0-rpmi-domain",
	.init = _k3_os0_domain_init,
	.domain_ops = &k3_os0_domain_pops,
};

static rt_int32_t k3_os0_device_power_init(void)
{
	rt_list_init(&k3_os0_device_power_ops.list);

	spacemit_rpmi_domain_register(&k3_os0_device_power_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_device_power_init);
