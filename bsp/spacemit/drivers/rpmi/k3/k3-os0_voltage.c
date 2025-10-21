/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include <drivers/regulator.h>
#include <drivers/dtb_node.h>
#include "../spacemit-rpmi.h"
#include "k3-os0_voltage.h"

struct rt_regulator_data {
	struct rt_regulator *reg;
};

struct rt_regulator_data *volt_data;

struct rpmi_voltage_data *k3_os0_voltage_data;

static rt_int32_t  _k3_os0_voltage_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_voltage_config *config = priv;
	struct rt_device *dev;
	rt_uint32_t *property_ptr;
	rt_uint32_t u32_value;
	rt_int32_t property_size;

	for_each_property_cell(config->node, "num_domains", u32_value, property_ptr, property_size)
	{
		config->domain_count = u32_value;
	}
	if (!config->domain_count)
		return -RT_EINVAL;
	volt_data = rt_calloc(config->domain_count, sizeof(struct rt_regulator_data));
	if (volt_data == RT_NULL)
		return -RT_ENOMEM;
	k3_os0_voltage_data = rt_calloc(config->domain_count, sizeof(struct rpmi_voltage_data));
	if (k3_os0_voltage_data == RT_NULL)
		return -RT_ENOMEM;

	config->voltage_data = k3_os0_voltage_data;
	config->voltage_data->voltage_level_array = (const unsigned int *)test_reg[0].linear_ranges;
	config->voltage_data->level_count = test_reg[0].n_linear_ranges;

	for (int i = 0; i < config->domain_count; i++) {
		dev = rt_device_find(k3_os0_voltage_data[i].name);
		if (dev == RT_NULL)
			return -RT_EINVAL;
		volt_data[i].reg = rt_dtb_data(dev->node);
	}

	return 0;
}

/** Set the voltage state enable/disable/others */
static enum rpmi_error spacemit_set_config(void *priv, rpmi_uint32_t domain_id, enum rpmi_voltage_state state)
{
	struct rt_regulator *reg = volt_data[domain_id].reg;
	int ret;
	switch (state) {
		case RPMI_VOLTAGE_STATE_DISABLED:
			ret = rt_regulator_disable(reg);
			break;
		case RPMI_VOLTAGE_STATE_ENABLED:
			ret = rt_regulator_enable(reg);
			break;
		default:
			ret = -RT_EINVAL;
			break;
	}
	if (!ret)
		ret = RPMI_SUCCESS;
	else
		ret = RPMI_ERR_INVALID_PARAM;

	return ret;
}

static enum rpmi_error spacemit_get_config(void *priv, rpmi_uint32_t domain_id, enum rpmi_voltage_state *state)
{
	struct rt_regulator *reg = volt_data[domain_id].reg;
	if (rt_regulator_is_enabled(reg))
		*state = RPMI_VOLTAGE_STATE_ENABLED;
	else
		*state = RPMI_VOLTAGE_STATE_DISABLED;
	return 0;
}

static enum rpmi_error spacemit_set_voltage_level(void *priv, rpmi_uint32_t domain_id, rpmi_uint32_t level)
{
	struct rt_regulator *reg = volt_data[domain_id].reg;
	int ret;

	ret = rt_regulator_set_voltage(reg, level, level);
	if (!ret)
		ret = RPMI_SUCCESS;
	else
		ret = RPMI_ERR_INVALID_PARAM;

	return ret;
}

static enum rpmi_error spacemit_get_voltage_level(void *priv, rpmi_uint32_t domain_id, rpmi_uint32_t *level)
{
	struct rt_regulator *reg = volt_data[domain_id].reg;

	*level = rt_regulator_get_voltage(reg);
	return 0;
}

static struct rpmi_voltage_platform_ops k3_os0_voltage_pops = {
	.set_config = spacemit_set_config,
	.get_config = spacemit_get_config,
	.set_voltage_level = spacemit_set_voltage_level,
	.get_voltage_level = spacemit_get_voltage_level,
};

static struct spacemit_rpmi_voltage_ops k3_os0_voltage_ops = {
	.name = "k3-os0-rpmi-voltage",
	.init = _k3_os0_voltage_init,
	.voltage_ops = &k3_os0_voltage_pops,
};

static rt_int32_t k3_os0_voltage_init(void)
{
	rt_list_init(&k3_os0_voltage_ops.list);

	spacemit_rpmi_voltage_register(&k3_os0_voltage_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_voltage_init);
