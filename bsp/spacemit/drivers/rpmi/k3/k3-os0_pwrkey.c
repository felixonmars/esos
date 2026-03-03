/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include <librpmi.h>
#include "../spacemit-rpmi.h"

#define PWRKEY_STATUS_REG		0x97

struct spacemit_pwrkey {
	struct rt_i2c_bus_device *handle_driver;
	int slave_addr;
};

static rt_int32_t  _k3_os0_pwrkey_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_pwrkey_config *config = priv;
	struct dtb_node *node = config->node;
	struct spacemit_pwrkey *sp;
	char *string, *strend;
	rt_int32_t size, ret;
	rt_uint8_t val;

	if (node == RT_NULL) {
		rt_kprintf("%s, %d: pwrkey dtb node error\n");
		return -RT_EINVAL;
	}
	sp = (struct spacemit_pwrkey *)rt_calloc(1, sizeof(struct spacemit_pwrkey));
	if (sp == RT_NULL) {
		rt_kprintf("%s, %d: No memory\n", __func__, __LINE__);
		return -RT_ENOMEM;
	}

	for_each_property_string_extend(node, "bind_driver", string, strend, size) {
		sp->handle_driver = rt_i2c_bus_device_find(string);
		if (sp->handle_driver == RT_NULL) {
			rt_kprintf("%s, %d: expected binding driver not ready\n", __func__, __LINE__);
			return -RT_EINVAL;
		}
	}

	if (dtb_node_read_u32_array(node, "slave_addr", &sp->slave_addr, 1)) {
		rt_kprintf("%s, %d: failed to get pwrkey slave addr\n", __func__, __LINE__);
		return -RT_EINVAL;
	}

	config->priv = (void *)sp;
	return 0;
}

static enum rpmi_error k3_os0_query_pending(void *priv, rpmi_uint32_t *status)
{
	struct spacemit_rpmi_pwrkey_config *config = priv;
	struct spacemit_pwrkey *sp = (struct spacemit_pwrkey *)config->priv;
	struct rt_i2c_bus_device *i2c = sp->handle_driver;
	struct rt_i2c_msg msgs[2];
	rt_uint8_t val;
	rt_uint8_t reg = PWRKEY_STATUS_REG;

	msgs[0].addr = sp->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = (rt_uint8_t *)&reg;
	msgs[0].len = 1;

	msgs[1].addr = sp->slave_addr;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(i2c, msgs, 2) != 2) {
		rt_kprintf("%s, %d: pwrkey i2c transfer error, addr:%x\n", __func__, __LINE__, reg);
		return -RT_ERROR;
	}

	*status = val;

	return RPMI_SUCCESS;
}

static enum rpmi_error k3_os0_clear_pending(void *priv, rpmi_uint32_t clear)
{
	struct spacemit_rpmi_pwrkey_config *config = priv;
	struct spacemit_pwrkey *sp = (struct spacemit_pwrkey *)config->priv;
	struct rt_i2c_bus_device *i2c = sp->handle_driver;
	struct rt_i2c_msg msgs[1];
	rt_uint8_t val_temp[2];
	rt_uint8_t reg = PWRKEY_STATUS_REG;

	msgs[0].addr = sp->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	val_temp[0] = reg;
	val_temp[1] = clear;
	msgs[0].buf = val_temp;
	msgs[0].len = 2;

	if (rt_i2c_transfer(i2c, msgs, 1) != 1) {
		rt_kprintf("%s, %d: pwrkey i2c transfer error, addr:%x\n", __func__, __LINE__, reg);
		return -RT_ERROR;
	}
	return RPMI_SUCCESS;
}

static struct rpmi_pwrkey_platform_ops k3_os0_pwrkey_pops = {
	.query_pending = k3_os0_query_pending,
	.clear_pending = k3_os0_clear_pending,
};

static struct spacemit_rpmi_pwrkey_ops k3_os0_pwrkey_ops = {
	.name = "k3-os0-rpmi-pwrkey",
	.init = _k3_os0_pwrkey_init,
	.pwrkey_ops = &k3_os0_pwrkey_pops,
};

static rt_int32_t k3_os0_pwrkey_init(void)
{
	rt_list_init(&k3_os0_pwrkey_ops.list);

	spacemit_rpmi_pwrkey_register(&k3_os0_pwrkey_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_pwrkey_init);
