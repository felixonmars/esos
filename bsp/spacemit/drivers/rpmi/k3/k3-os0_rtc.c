/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <drivers/i2c.h>
#include <dtb_head.h>
#include <librpmi.h>
#include "../spacemit-rpmi.h"

struct spacemit_rtc_reg {
	/* seconds */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_s;

	/* mini */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_mi;

	/* hour */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_h;

	/* day */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_d;

	/* mounth */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_mo;

	/* year */
	struct {
		unsigned char reg;
		unsigned char msk;
	} cnt_y;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_s;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_mi;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_h;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_d;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_mo;

	struct {
		unsigned char reg;
		unsigned char msk;
	} alarm_y;

	struct {
		unsigned char reg;
		unsigned char msk;
	} rtc_ctl;

	struct {
		unsigned char reg;
		unsigned char msk;
	} sys_evt;
};

static const struct spacemit_rtc_reg spm8821_regdesc = {	\
	.cnt_s = {						\
		.reg = 0xd,					\
		.msk = 0x3f,					\
	},							\
								\
	.cnt_mi = {						\
		.reg = 0xe,					\
		.msk = 0x3f,					\
	},							\
								\
	.cnt_h = {						\
		.reg = 0xf,					\
		.msk = 0x1f,					\
	},							\
								\
	.cnt_d = {						\
		.reg = 0x10,					\
		.msk = 0x1f,					\
	},							\
								\
	.cnt_mo = {						\
		.reg = 0x11,					\
		.msk = 0xf,					\
	},							\
								\
	.cnt_y = {						\
		.reg = 0x12,					\
		.msk = 0x3f,					\
	},							\
								\
	.alarm_s = {						\
		.reg = 0x13,					\
		.msk = 0x3f,					\
	},							\
								\
	.alarm_mi = {						\
		.reg = 0x14,					\
		.msk = 0x3f,					\
	},							\
								\
	.alarm_h = {						\
		.reg = 0x15,					\
		.msk = 0x1f,					\
	},							\
								\
	.alarm_d = {						\
		.reg = 0x16,					\
		.msk = 0x1f,					\
	},							\
								\
	.alarm_mo = {						\
		.reg = 0x17,					\
		.msk = 0xf,					\
	},							\
								\
	.alarm_y = {						\
		.reg = 0x18,					\
		.msk = 0x3f,					\
	},							\
								\
	.rtc_ctl = {						\
		.reg = 0x1d,					\
		.msk = 0xff,					\
	},							\
								\
	.sys_evt = {						\
		.reg = 0x92,					\
		.msk = 0x30,					\
	},							\

};

#define CRYSTAL_CTRL_EN		0x1
#define OUT_32K_CTRL_EN		0x2
#define RTC_CTRL_EN		0x4
#define RTC_CLK_CTRL_EN		0x8
#define TICK_TYPE_CTRL		0x10
#define ALARM_CTRL_EN		0x20
#define TICK_CTRL_EN		0x40

struct rtc_reg {
	unsigned char reg;
	unsigned char msk;
};

struct spacemit_rtc {
	struct rt_i2c_bus_device *handle_driver;
	int slave_addr;
	int alarm_en;
};

static int k3_os0_rtc_read(struct spacemit_rtc *sr, struct rtc_reg *reg, rt_uint8_t *val)
{
	struct rt_i2c_bus_device *i2c = sr->handle_driver;
	struct rt_i2c_msg msgs[2];
	rt_uint8_t value;

	msgs[0].addr = sr->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = (rt_uint8_t *)&reg->reg;
	msgs[0].len = 1;

	msgs[1].addr = sr->slave_addr;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = &value;
	msgs[1].len = 1;

	if (rt_i2c_transfer(i2c, msgs, 2) != 2) {
		rt_kprintf("%s, %d: i2c transfer error, addr:%x\n", __func__, __LINE__, reg->reg);
		return -RT_ERROR;
	}

	value &= reg->msk;
	*val = value;

	return 0;
}

static int k3_os0_rtc_write(struct spacemit_rtc *sr, struct rtc_reg *reg, rt_uint8_t val)
{
	struct rt_i2c_bus_device *i2c = sr->handle_driver;
	struct rt_i2c_msg msgs[1];
	rt_uint8_t val_temp[2];

	msgs[0].addr = sr->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	val_temp[0] = reg->reg;
	val_temp[1] = val & reg->msk;
	msgs[0].buf = val_temp;
	msgs[0].len = 2;

	if (rt_i2c_transfer(i2c, msgs, 1) != 1) {
		rt_kprintf("%s, %d: i2c transfer error, addr:%x\n", __func__, __LINE__, reg->reg);
		return -RT_ERROR;
	}

	return 0;
}

static rt_int32_t  _k3_os0_rtc_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_rtc_config *config = priv;
	struct dtb_node *node = config->node;
	struct spacemit_rtc *sr;
	struct rtc_reg *reg;
	char *string, *strend;
	rt_int32_t size, ret;
	rt_uint8_t val;

	if (node == RT_NULL) {
		rt_kprintf("%s, %d: rtc dtb node error\n");
		return -RT_EINVAL;
	}
	sr = (struct spacemit_rtc *)rt_calloc(1, sizeof(struct spacemit_rtc));
	if (sr == RT_NULL) {
		rt_kprintf("%s, %d: No memory\n", __func__, __LINE__);
		return -RT_ENOMEM;
	}

	for_each_property_string_extend(node, "bind_driver", string, strend, size) {
		sr->handle_driver = rt_i2c_bus_device_find(string);
		if (sr->handle_driver == RT_NULL) {
			rt_kprintf("%s, %d: expected binding driver not ready\n", __func__, __LINE__);
			return -RT_EINVAL;
		}
	}

	if (dtb_node_read_u32_array(node, "slave_addr", &sr->slave_addr, 1)) {
		rt_kprintf("%s, %d: failed to get rtc slave addr\n", __func__, __LINE__);
		return -RT_EINVAL;
	}

	config->priv = (void *)sr;

	return 0;
}

/** Set the rtc time */
static enum rpmi_error k3_os0_set_time(void *priv, rpmi_uint32_t year,
				    rpmi_uint32_t mon,
				    rpmi_uint32_t day,
				    rpmi_uint32_t hour,
				    rpmi_uint32_t min,
				    rpmi_uint32_t second)
{
	struct spacemit_rpmi_rtc_config *config = priv;
	struct spacemit_rtc *sr = (struct spacemit_rtc *)config->priv;
	struct rtc_reg *reg;
	rt_uint8_t v[6], val;
	rt_uint32_t ret;

	reg = (struct rtc_reg *)&spm8821_regdesc.rtc_ctl;
	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("failed to get ctrl reg value\n");
		return ret;
	}

	val &= ~RTC_CTRL_EN;
	ret = k3_os0_rtc_write(sr, reg, val);
	if (ret) {
		rt_kprintf("%s, %d: failed to set ctrl reg value\n", __func__, __LINE__);
		return ret;
	}

	while (1) {
		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_s;
		ret = k3_os0_rtc_write(sr, reg, second);
		if (ret != 0) {
			rt_kprintf("failed to set second time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_mi;
		ret = k3_os0_rtc_write(sr, reg, min);
		if (ret != 0) {
			rt_kprintf("failed to set minute time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_h;
		ret = k3_os0_rtc_write(sr, reg, hour);
		if (ret != 0) {
			rt_kprintf("failed to set hour time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_d;
		ret = k3_os0_rtc_write(sr, reg, day - 1);
		if (ret != 0) {
			rt_kprintf("failed to set day time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_mo;
		ret = k3_os0_rtc_write(sr, reg, mon);
		if (ret != 0) {
			rt_kprintf("failed to set month time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_y;
		ret = k3_os0_rtc_write(sr, reg, year - 100);
		if (ret != 0) {
			rt_kprintf("failed to set year time\n");
			return ret;
		}

		/* check value is valid */
		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_s;
		ret = k3_os0_rtc_read(sr, reg, &v[0]);
		if (ret != 0) {
			rt_kprintf("failed to get second time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_mi;
		ret = k3_os0_rtc_read(sr, reg, &v[1]);
		if (ret != 0) {
			rt_kprintf("failed to get minute time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_h;
		ret = k3_os0_rtc_read(sr, reg, &v[2]);
		if (ret != 0) {
			rt_kprintf("failed to get hour time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_d;
		ret = k3_os0_rtc_read(sr, reg, &v[3]);
		if (ret != 0) {
			rt_kprintf("failed to get day time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_mo;
		ret = k3_os0_rtc_read(sr, reg, &v[4]);
		if (ret != 0) {
			rt_kprintf("failed to get month time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_y;
		ret = k3_os0_rtc_read(sr, reg, &v[5]);
		if (ret != 0) {
			rt_kprintf("failed to get year time\n");
			return ret;
		}

		if ((v[0] == (second & spm8821_regdesc.cnt_s.msk)) &&
		    (v[1] == (min & spm8821_regdesc.cnt_mi.msk)) &&
		    (v[2] == (hour & spm8821_regdesc.cnt_h.msk)) &&
		    (v[3] == ((day - 1) & spm8821_regdesc.cnt_d.msk)) &&
		    (v[4] == (mon & spm8821_regdesc.cnt_mo.msk)) &&
		    (v[5] == ((year - 100) & spm8821_regdesc.cnt_y.msk)))
			break;
	}

	val |= RTC_CTRL_EN;
	reg = (struct rtc_reg *)&spm8821_regdesc.rtc_ctl;
	ret = k3_os0_rtc_write(sr, reg, val);
	if (ret) {
		rt_kprintf("%s, %d: failed to set ctrl reg value\n", __func__, __LINE__);
		return ret;
	}

	return RPMI_SUCCESS;
}

/**
 * Get the rtc time
 **/
static enum rpmi_error k3_os0_get_time(void *priv, rpmi_uint32_t *year,
				    rpmi_uint32_t *mon,
				    rpmi_uint32_t *day,
				    rpmi_uint32_t *hour,
				    rpmi_uint32_t *min,
				    rpmi_uint32_t *second)
{
	struct spacemit_rpmi_rtc_config *config = priv;
	struct spacemit_rtc *sr = (struct spacemit_rtc *)config->priv;
	struct rtc_reg *reg;
	rt_uint8_t v[6], pre_v[6] = {0}, val;
	rt_int32_t ret;

	while (1) {
		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_s;
		ret = k3_os0_rtc_read(sr, reg, &v[0]);
		if (ret != 0) {
			rt_kprintf("failed to get second time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_mi;
		ret = k3_os0_rtc_read(sr, reg, &v[1]);
		if (ret != 0) {
			rt_kprintf("failed to get minute time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_h;
		ret = k3_os0_rtc_read(sr, reg, &v[2]);
		if (ret != 0) {
			rt_kprintf("failed to get hour time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_d;
		ret = k3_os0_rtc_read(sr, reg, &v[3]);
		if (ret != 0) {
			rt_kprintf("failed to get day time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_mo;
		ret = k3_os0_rtc_read(sr, reg, &v[4]);
		if (ret != 0) {
			rt_kprintf("failed to get month time\n");
			return ret;
		}

		reg = (struct rtc_reg *)&spm8821_regdesc.cnt_y;
		ret = k3_os0_rtc_read(sr, reg, &v[5]);
		if (ret != 0) {
			rt_kprintf("failed to get year time\n");
			return ret;
		}

		if ((pre_v[0] == v[0]) && (pre_v[1] == v[1]) &&
		    (pre_v[2] == v[2]) && (pre_v[3] == v[3]) &&
		    (pre_v[4] == v[4]) && (pre_v[5] == v[5]))
			break;
		else {
			pre_v[0] = v[0];
			pre_v[1] = v[1];
			pre_v[2] = v[2];
			pre_v[3] = v[3];
			pre_v[4] = v[4];
			pre_v[5] = v[5];
		}
	}

	*second = v[0];
	*min = v[1];
	*hour = v[2];
	*day = v[3] + 1;
	*mon = v[4];
	*year = v[5] + 100;

	reg = (struct rtc_reg *)&spm8821_regdesc.rtc_ctl;
	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("%s, %d: failed to get ctrl reg value\n", __func__, __LINE__);
		return ret;
	}

	return RPMI_SUCCESS;
}

/** Set the rtc alarm time */
static enum rpmi_error k3_os0_set_alarm(void *priv, rpmi_uint32_t year,
				    rpmi_uint32_t mon,
				    rpmi_uint32_t day,
				    rpmi_uint32_t hour,
				    rpmi_uint32_t min,
				    rpmi_uint32_t second)
{
	struct spacemit_rpmi_rtc_config *config = priv;
	struct spacemit_rtc *sr = (struct spacemit_rtc *)config->priv;
	struct rtc_reg *reg;
	rt_uint8_t val;
	rt_uint32_t ret;

	reg = (struct rtc_reg *)&spm8821_regdesc.rtc_ctl;
	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("%s, %d: failed to get ctrl reg value\n", __func__, __LINE__);
		return ret;
	}

	val &= ~ALARM_CTRL_EN;

	ret = k3_os0_rtc_write(sr, reg, val);
	if (ret) {
		rt_kprintf("%s, %d: failed to set ctrl reg value\n", __func__, __LINE__);
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_s;
	ret = k3_os0_rtc_write(sr, reg, second);
	if (ret != 0) {
		rt_kprintf("failed to set alarm second time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_mi;
	ret = k3_os0_rtc_write(sr, reg, min);
	if (ret != 0) {
		rt_kprintf("failed to set alarm minute time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_h;
	ret = k3_os0_rtc_write(sr, reg, hour);
	if (ret != 0) {
		rt_kprintf("failed to set alarm hour time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_d;
	ret = k3_os0_rtc_write(sr, reg, day - 1);
	if (ret != 0) {
		rt_kprintf("failed to set alarm day time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_mo;
	ret = k3_os0_rtc_write(sr, reg, mon);
	if (ret != 0) {
		rt_kprintf("failed to set alarm month time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_y;
	ret = k3_os0_rtc_write(sr, reg, year - 100);
	if (ret != 0) {
		rt_kprintf("failed to set alarm year time\n");
		return ret;
	}

	if (sr->alarm_en) {
		reg = (struct rtc_reg *)&spm8821_regdesc.rtc_ctl;
		val |= ALARM_CTRL_EN;
		ret = k3_os0_rtc_write(sr, reg, val);
		if (ret) {
			rt_kprintf("%s, %d: failed to set ctrl reg value\n", __func__, __LINE__);
			return ret;
		}
	}

	return RPMI_SUCCESS;
}

/**
 * Get the rtc alarm time
 **/
static enum rpmi_error k3_os0_get_alarm(void *priv, rpmi_uint32_t *year,
				    rpmi_uint32_t *mon,
				    rpmi_uint32_t *day,
				    rpmi_uint32_t *hour,
				    rpmi_uint32_t *min,
				    rpmi_uint32_t *second)
{
	struct spacemit_rpmi_rtc_config *config = priv;
	struct spacemit_rtc *sr = (struct spacemit_rtc *)config->priv;
	struct rtc_reg *reg;
	rt_uint8_t v[6], val;
	rt_int32_t ret;

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_s;
	ret = k3_os0_rtc_read(sr, reg, &v[0]);
	if (ret != 0) {
		rt_kprintf("failed to get alarm second time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_mi;
	ret = k3_os0_rtc_read(sr, reg, &v[1]);
	if (ret != 0) {
		rt_kprintf("failed to get alarm minute time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_h;
	ret = k3_os0_rtc_read(sr, reg, &v[2]);
	if (ret != 0) {
		rt_kprintf("failed to get alarm hour time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_d;
	ret = k3_os0_rtc_read(sr, reg, &v[3]);
	if (ret != 0) {
		rt_kprintf("failed to get alarm day time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_mo;
	ret = k3_os0_rtc_read(sr, reg, &v[4]);
	if (ret != 0) {
		rt_kprintf("failed to get alarm month time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.alarm_y;
	ret = k3_os0_rtc_read(sr, reg, &v[5]);
	if (ret != 0) {
		rt_kprintf("failed to get alarm year time\n");
		return ret;
	}

	reg = (struct rtc_reg *)&spm8821_regdesc.rtc_ctl;
	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("%s, %d: failed to get ctrl reg value\n", __func__, __LINE__);
		return ret;
	}

	sr->alarm_en = !!(val & ALARM_CTRL_EN);
	*second = v[0];
	*min = v[1];
	*hour = v[2];
	*day = v[3] + 1;
	*mon = v[4];
	*year = v[5] + 100;

	return RPMI_SUCCESS;
}

enum rpmi_error k3_os0_get_alarm_en(void *priv, rpmi_uint32_t *status)
{
	struct spacemit_rpmi_rtc_config *config = priv;
	struct spacemit_rtc *sr = (struct spacemit_rtc *)config->priv;
	struct rtc_reg *reg;
	rt_uint8_t val;
	rt_int32_t ret;

	reg = (struct rtc_reg *)&spm8821_regdesc.rtc_ctl;
	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("%s, %d: failed to get ctrl reg value\n", __func__, __LINE__);
		return ret;
	}

	sr->alarm_en = !!(val & ALARM_CTRL_EN);
	*status = sr->alarm_en;

	return RPMI_SUCCESS;
}

static enum rpmi_error k3_os0_set_alarm_en(void *priv, rpmi_uint32_t en)
{
	struct spacemit_rpmi_rtc_config *config = priv;
	struct spacemit_rtc *sr = (struct spacemit_rtc *)config->priv;
	struct rtc_reg *reg;
	rt_uint8_t val;
	rt_int32_t ret;

	reg = (struct rtc_reg *)&spm8821_regdesc.rtc_ctl;
	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("%s, %d: failed to get ctrl reg value\n", __func__, __LINE__);
		return ret;
	}

	if (en)
		val |= ALARM_CTRL_EN;
	else
		val &= ~ALARM_CTRL_EN;

	ret = k3_os0_rtc_write(sr, reg, val);
	if (ret) {
		rt_kprintf("%s, %d: failed to set ctrl reg value\n", __func__, __LINE__);
		return ret;
	}
	sr->alarm_en = en;

	return RPMI_SUCCESS;
}

enum rpmi_error k3_os0_query_pending(void *priv, rpmi_uint32_t *status)
{
	struct spacemit_rpmi_rtc_config *config = priv;
	struct spacemit_rtc *sr = (struct spacemit_rtc *)config->priv;
	struct rtc_reg *reg;
	rt_uint8_t val;
	rt_uint32_t ret;

	reg = (struct rtc_reg *)&spm8821_regdesc.sys_evt;
	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("%s, %d: failed to get rtc pending status\n", __func__, __LINE__);
		return ret;
	}

	*status = val == 0 ? 0 : 1;

	return RPMI_SUCCESS;
}

static enum rpmi_error k3_os0_clear_pending(void *priv)
{
	struct spacemit_rpmi_rtc_config *config = priv;
	struct spacemit_rtc *sr = (struct spacemit_rtc *)config->priv;
	struct rtc_reg *reg;
	rt_uint8_t val;
	rt_uint32_t ret;

	reg = (struct rtc_reg *)&spm8821_regdesc.sys_evt;
	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("%s, %d: failed to get rtc pending status\n", __func__, __LINE__);
		return ret;
	}

	ret = k3_os0_rtc_write(sr, reg, val);
	if (ret) {
		rt_kprintf("%s, %d: failed to clear rtc pending\n", __func__, __LINE__);
		return ret;
	}

	ret = k3_os0_rtc_read(sr, reg, &val);
	if (ret) {
		rt_kprintf("%s, %d: failed to get rtc pending status\n", __func__, __LINE__);
		return ret;
	}

	return RPMI_SUCCESS;
}

static struct rpmi_rtc_platform_ops k3_os0_rtc_pops = {
	.set_time = k3_os0_set_time,
	.get_time = k3_os0_get_time,
	.set_alarm = k3_os0_set_alarm,
	.get_alarm = k3_os0_get_alarm,
	.get_alarm_en = k3_os0_get_alarm_en,
	.set_alarm_en = k3_os0_set_alarm_en,
	.query_pending = k3_os0_query_pending,
	.clear_pending = k3_os0_clear_pending,
};

static struct spacemit_rpmi_rtc_ops k3_os0_rtc_ops = {
	.name = "k3-os0-rpmi-rtc",
	.init = _k3_os0_rtc_init,
	.rtc_ops = &k3_os0_rtc_pops,
};

static rt_int32_t k3_os0_rtc_init(void)
{
	rt_list_init(&k3_os0_rtc_ops.list);

	spacemit_rpmi_rtc_register(&k3_os0_rtc_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_rtc_init);
