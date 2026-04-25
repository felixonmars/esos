/*
 * Copyright (c) 2022-2026, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <dtb_head.h>
#include <librpmi.h>
#include "../spacemit-rpmi.h"

#define P1_REG_NONVOLATILE_ADDR		0xab
#define P1_REG_NONVOLATILE_FASTBOOT	0xf1
#define P1_REG_NONVOLATILE_UPDATER	0xf3
#define P1_REG_PWR_CTRL2_ADDR		0x7e
#define P1_REG_PWR_CTRL2_SD		0x4
#define P1_REG_PWR_CTRL2_RST		0x2
#define LISTEN_ADDR			0xc087c000
#define FLAG_FASTBOOT			0x1
#define FLAG_UPDATER			0x3

static struct rt_i2c_bus_device *i2c_bus_device;
static rt_uint32_t i2c_addr = 0x0;

static struct rt_i2c_bus_device* k3_os0_sysreset_get_i2c(void)
{
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	struct dtb_node *compatible_node;
	char *string, *strend;
	rt_int32_t size;

	compatible_node = dtb_node_find_compatible_node(dtb_head_node, "spacemit,k3-pwrgpio");
	if (compatible_node == RT_NULL) {
		rt_kprintf("reboot daemon: get dtb node failed\n");
		return RT_NULL;
	}
	dtb_node_read_u32_array(compatible_node, "slave_addr", &i2c_addr, 1);

	for_each_property_string_extend(compatible_node, "bind_driver", string, strend, size) {
		i2c_bus_device = rt_i2c_bus_device_find(string);
		if (i2c_bus_device == RT_NULL) {
			rt_kprintf("k3_sysreset: the bind driver has not registered\n");
			return RT_NULL;
		}
	}
	return i2c_bus_device;
}

static rt_int32_t _k3_os0_sysreset_init(void *priv)
{
	struct spacemit_rpmi_sysreset_config *config = priv;
	rt_uint32_t gpio_num;
	int cnt;

	config->reset = -1;

	if (dtb_node_read_u32_array(config->node, "shutdown-gpio", &gpio_num, 1) == 0) {
		config->reset = (rt_int32_t)gpio_num;
		rt_kprintf("k3 sysreset: shutdown-gpio = %d\n", config->reset);
	}

	/* try get i2c device for approximately 5s, then fail */
	cnt = 50;
	while (cnt-- > 0) {
		i2c_bus_device = k3_os0_sysreset_get_i2c();
		if (i2c_bus_device != RT_NULL)
			break;
		rt_thread_mdelay(100);
	}
	if (cnt <= 0) {
		if (config->reset >= 0) {
			/* shutdown via GPIO still works, reboot via I2C won't */
			rt_kprintf("k3 sysreset: no pwr i2c, reboot may not work\n");
		} else {
			rt_kprintf("k3 sysreset: get pwr i2c failed\n");
			return RT_ERROR;
		}
	}

	return 0;
}

static void k3_os0_sysreset_soft_rst_sd(rpmi_uint32_t sysreset_type)
{
	struct rt_i2c_msg msgs[2];
	rt_uint8_t send_buf[2];

	send_buf[0] = P1_REG_PWR_CTRL2_ADDR;
	send_buf[1] = 0;
	msgs[0].addr = (rt_uint8_t)i2c_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = send_buf;
	msgs[0].len = 1;

	/* read the value */
	msgs[1].addr = (rt_uint8_t)i2c_addr;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = &send_buf[1];
	msgs[1].len = 1;

	if (!i2c_bus_device) {
		rt_kprintf("k3 sysreset: no valid pwr i2c\n");
		return;
	}

	if (rt_i2c_transfer(i2c_bus_device, msgs, 2) != 2) {
		rt_kprintf("k3 sysreset: i2c transfer error, %s:%d\n", __func__, __LINE__);
		return;
	}

	send_buf[0] = P1_REG_PWR_CTRL2_ADDR;
	send_buf[1] |= (sysreset_type == RPMI_SYSRST_TYPE_SHUTDOWN) ?
		       P1_REG_PWR_CTRL2_SD : P1_REG_PWR_CTRL2_RST;
	msgs[0].addr = (rt_uint8_t)i2c_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = send_buf;
	msgs[0].len = 2;

	if (rt_i2c_transfer(i2c_bus_device, msgs, 1) != 1) {
		rt_kprintf("k3 sysreset: i2c transfer error, %s:%d\n", __func__, __LINE__);
		return;
	}
}

static inline bool k3_sysreset_is_reboot(rpmi_uint32_t sysreset_type)
{
	return (sysreset_type == RPMI_SYSRST_TYPE_COLD_REBOOT ||
		sysreset_type == RPMI_SYSRST_TYPE_WARM_REBOOT);
}

static inline bool k3_sys_reset_is_shutdown(rpmi_uint32_t sysreset_type)
{
	return (sysreset_type == RPMI_SYSRST_TYPE_SHUTDOWN);
}

static rt_size_t k3_sysreset_write_flag_to_p1(rpmi_uint8_t flag)
{
	struct rt_i2c_msg msgs[1];
	rt_uint8_t send_buf[2];

	if (!i2c_bus_device) {
		rt_kprintf("k3 sysreset: no valid pwr i2c!\n");
		return 0;
	}

	send_buf[0] = P1_REG_NONVOLATILE_ADDR;
	send_buf[1] = flag;
	msgs[0].addr = (rt_uint8_t)i2c_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = send_buf;
	msgs[0].len = 2;

	return rt_i2c_transfer(i2c_bus_device, msgs, 1);
}

static void k3_os0_system_reset(void *priv, rpmi_uint32_t sysreset_type)
{
	struct spacemit_rpmi_sysreset_config *config = priv;
	volatile rt_uint8_t val = 0;

	if (k3_sys_reset_is_shutdown(sysreset_type) && config->reset >= 0) {
		rt_kprintf("k3 sysreset: shutdown via GPIO%d\n", config->reset);
		rt_pin_mode(config->reset, PIN_MODE_OUTPUT);
		rt_pin_write(config->reset, PIN_LOW);
		while (1)
			rt_thread_mdelay(100);
	}

	if (k3_sysreset_is_reboot(sysreset_type)) {
		/* read from share memory */
		/* to decide perform fastboot reboot or normal reboot */
		asm volatile("fence rw, rw");
		val = *((volatile rt_uint8_t*)LISTEN_ADDR);
		if (val == FLAG_FASTBOOT) {
			if (k3_sysreset_write_flag_to_p1(P1_REG_NONVOLATILE_FASTBOOT) != 1) {
				rt_kprintf("k3 sysreset: i2c transfer error, %s:%d\n",
					   __func__, __LINE__);
				return;
			}
			rt_kprintf("k3 sysreset: fastboot reboot\n");
		} else if (val == FLAG_UPDATER) {
			if (k3_sysreset_write_flag_to_p1(P1_REG_NONVOLATILE_UPDATER) != 1) {
				rt_kprintf("k3 sysreset: i2c transfer error, %s:%d\n",
					   __func__, __LINE__);
				return;
			}
			rt_kprintf("k3 sysreset: updater reboot\n");
		} else {
			rt_kprintf("k3 sysreset: normal reboot\n");
		}
		k3_os0_sysreset_soft_rst_sd(sysreset_type);
	} else if (k3_sys_reset_is_shutdown(sysreset_type)) {
		rt_kprintf("k3 sysreset: shutdown\n");
		k3_os0_sysreset_soft_rst_sd(sysreset_type);
	} else {
		rt_kprintf("k3 sysreset: unknown sysreset type\n");
	}
}

static struct rpmi_sysreset_platform_ops k3_os0_sysreset_pops = {
	.do_system_reset = k3_os0_system_reset,
};

static struct spacemit_rpmi_sysreset_ops k3_os0_sysreset_ops = {
	.name = "k3-os0-rpmi-sysreset",
	.init = _k3_os0_sysreset_init,
	.sysreset_ops = &k3_os0_sysreset_pops,
};

static rt_int32_t k3_os0_sysreset_init(void)
{
	rt_list_init(&k3_os0_sysreset_ops.list);

	spacemit_rpmi_sysreset_register(&k3_os0_sysreset_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_sysreset_init);
