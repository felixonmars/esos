/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rtconfig.h>
#include <drivers/i2c.h>
#include <drivers/regulator.h>
#include <drivers/regulator_dm.h>
#include "regulator.h"

static struct regulator_linear_range p1_buck_ranges[] = {
	REGULATOR_LINEAR_RANGE(500000, 0x0, 0xaa, 5000),
	REGULATOR_LINEAR_RANGE(1375000, 0xab, 0xfe, 25000),
};

static struct regulator_linear_range p1_ldo_ranges[] = {
	REGULATOR_LINEAR_RANGE(500000, 0xb, 0x7f, 25000),
};

static const struct regulator_desc p1_regulator_descs[]  = {
	REGULATOR_DESC_COMMON(P1_ID_DCDC1,
			255, P1_BUCK1_VSEL_REG, P1_BUCK_VSEL_MASK,
			P1_BUCK1_CTRL_REG, P1_BUCK_EN_MASK,
			P1_BUCK1_SVSEL_REG, P1_BUCK_SVSEL_MASK,
			p1_buck_ranges),
	REGULATOR_DESC_COMMON(P1_ID_DCDC2,
			255, P1_BUCK2_VSEL_REG, P1_BUCK_VSEL_MASK,
			P1_BUCK2_CTRL_REG, P1_BUCK_EN_MASK,
			P1_BUCK2_SVSEL_REG, P1_BUCK_SVSEL_MASK,
			p1_buck_ranges),
	REGULATOR_DESC_COMMON(P1_ID_DCDC3,
			255, P1_BUCK3_VSEL_REG, P1_BUCK_VSEL_MASK,
			P1_BUCK3_CTRL_REG, P1_BUCK_EN_MASK,
			P1_BUCK3_SVSEL_REG, P1_BUCK_SVSEL_MASK,
			p1_buck_ranges),
	REGULATOR_DESC_COMMON(P1_ID_DCDC4,
			255, P1_BUCK4_VSEL_REG, P1_BUCK_VSEL_MASK,
			P1_BUCK4_CTRL_REG, P1_BUCK_EN_MASK,
			P1_BUCK4_SVSEL_REG, P1_BUCK_SVSEL_MASK,
			p1_buck_ranges),
	REGULATOR_DESC_COMMON(P1_ID_DCDC5,
			255, P1_BUCK5_VSEL_REG, P1_BUCK_VSEL_MASK,
			P1_BUCK5_CTRL_REG, P1_BUCK_EN_MASK,
			P1_BUCK5_SVSEL_REG, P1_BUCK_SVSEL_MASK,
			p1_buck_ranges),
	REGULATOR_DESC_COMMON(P1_ID_DCDC6,
			255, P1_BUCK6_VSEL_REG, P1_BUCK_VSEL_MASK,
			P1_BUCK6_CTRL_REG, P1_BUCK_EN_MASK,
			P1_BUCK6_SVSEL_REG, P1_BUCK_SVSEL_MASK,
			p1_buck_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO1,
			128, P1_ALDO1_VOLT_REG, P1_ALDO_VSEL_MASK,
			P1_ALDO1_CTRL_REG, P1_ALDO_EN_MASK,
			P1_ALDO1_SVOLT_REG, P1_ALDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO2,
			128, P1_ALDO2_VOLT_REG, P1_ALDO_VSEL_MASK,
			P1_ALDO2_CTRL_REG, P1_ALDO_EN_MASK,
			P1_ALDO2_SVOLT_REG, P1_ALDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO3,
			128, P1_ALDO3_VOLT_REG, P1_ALDO_VSEL_MASK,
			P1_ALDO3_CTRL_REG, P1_ALDO_EN_MASK,
			P1_ALDO3_SVOLT_REG, P1_ALDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO4,
			128, P1_ALDO4_VOLT_REG, P1_ALDO_VSEL_MASK,
			P1_ALDO4_CTRL_REG, P1_ALDO_EN_MASK,
			P1_ALDO4_SVOLT_REG, P1_ALDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO5,
			128, P1_DLDO1_VOLT_REG, P1_DLDO_VSEL_MASK,
			P1_DLDO1_CTRL_REG, P1_DLDO_EN_MASK,
			P1_DLDO1_SVOLT_REG, P1_DLDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO6,
			128, P1_DLDO2_VOLT_REG, P1_DLDO_VSEL_MASK,
			P1_DLDO2_CTRL_REG, P1_DLDO_EN_MASK,
			P1_DLDO2_SVOLT_REG, P1_DLDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO7,
			128, P1_DLDO3_VOLT_REG, P1_DLDO_VSEL_MASK,
			P1_DLDO3_CTRL_REG, P1_DLDO_EN_MASK,
			P1_DLDO3_SVOLT_REG, P1_DLDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO8,
			128, P1_DLDO4_VOLT_REG, P1_DLDO_VSEL_MASK,
			P1_DLDO4_CTRL_REG, P1_DLDO_EN_MASK,
			P1_DLDO4_SVOLT_REG, P1_DLDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO9,
			128, P1_DLDO5_VOLT_REG, P1_DLDO_VSEL_MASK,
			P1_DLDO5_CTRL_REG, P1_DLDO_EN_MASK,
			P1_DLDO5_SVOLT_REG, P1_DLDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO10,
			128, P1_DLDO6_VOLT_REG, P1_DLDO_VSEL_MASK,
			P1_DLDO6_CTRL_REG, P1_DLDO_EN_MASK,
			P1_DLDO6_SVOLT_REG, P1_DLDO_SVSEL_MASK,
			p1_ldo_ranges),
	REGULATOR_DESC_COMMON(P1_ID_LDO11,
			128, P1_DLDO7_VOLT_REG, P1_DLDO_VSEL_MASK,
			P1_DLDO7_CTRL_REG, P1_DLDO_EN_MASK,
			P1_DLDO7_SVOLT_REG, P1_DLDO_SVSEL_MASK,
			p1_ldo_ranges),
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "regulator-mpq8655_0", .data = (void *)&p1_regulator_descs },
	{ .compatible = "regulator-mpq8655_1", .data = (void *)&p1_regulator_descs },
	{ .compatible = "p1-regulator", .data = (void *)&p1_regulator_descs },
	{}
};

struct spacemit_regulator;

struct regulator_dynamic {
	struct rt_regulator_node parent;
	struct rt_regulator_param param;
	struct rt_device dev;
	struct spacemit_regulator *sr;
};

struct spacemit_regulator {
	/* using i2c */
	int slave_addr;
	struct rt_i2c_bus_device *handle_driver;
	struct regulator_dynamic *rd;
	void *priv_data;
};

static rt_err_t regulator_dynamic_enable(struct rt_regulator_node *reg)
{
	int index;
	rt_uint8_t val, cmd;
	struct rt_i2c_msg msgs[2];
	struct regulator_desc *desc;
	struct regulator_dynamic *rd = (struct regulator_dynamic *)reg;
	struct spacemit_regulator *sr = rd->sr;

	desc = (struct regulator_desc *)sr->priv_data;

	/* regulator index */
	index = rd - sr->rd;

	msgs[0].addr  = sr->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = (rt_uint8_t *)&desc[index].enable_reg;
	msgs[0].len = 1;

	/* read the value */
	msgs[1].addr  = sr->slave_addr;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(sr->handle_driver, msgs, 2) != 2) {
		rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	val &= ~(desc[index].enable_msk);
	val |= (1 << (ffs(desc[index].enable_msk) - 1));

	/* write value */
	msgs[0].addr  = sr->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = (rt_uint8_t *)&desc[index].enable_reg;
	msgs[0].len = 1;

	/* read the value */
	msgs[1].addr  = sr->slave_addr;
	msgs[1].flags = RT_I2C_WR;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(sr->handle_driver, msgs, 2) != 2) {
		rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	return 0;
}

static rt_err_t regulator_dynamic_disable(struct rt_regulator_node *reg)
{
	int index;
	rt_uint8_t val, cmd;
	struct rt_i2c_msg msgs[2];
	struct regulator_desc *desc;
	struct regulator_dynamic *rd = (struct regulator_dynamic *)reg;
	struct spacemit_regulator *sr = rd->sr;

	desc = (struct regulator_desc *)sr->priv_data;

	/* regulator index */
	index = rd - sr->rd;

	msgs[0].addr  = sr->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = (rt_uint8_t *)&desc[index].enable_reg;
	msgs[0].len = 1;

	/* read the value */
	msgs[1].addr  = sr->slave_addr;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(sr->handle_driver, msgs, 2) != 2) {
		rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	val &= ~(desc[index].enable_msk);
	val |= (0 << (ffs(desc[index].enable_msk) - 1));

	/* write value */
	msgs[0].addr  = sr->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = (rt_uint8_t *)&desc[index].enable_reg;
	msgs[0].len = 1;

	/* read the value */
	msgs[1].addr  = sr->slave_addr;
	msgs[1].flags = RT_I2C_WR;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(sr->handle_driver, msgs, 2) != 2) {
		rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	return 0;
}

static rt_bool_t regulator_dynamic_is_enabled(struct rt_regulator_node *reg)
{
	int index;
	rt_uint8_t val, cmd;
	struct rt_i2c_msg msgs[2];
	struct regulator_desc *desc;
	struct regulator_dynamic *rd = (struct regulator_dynamic *)reg;
	struct spacemit_regulator *sr = rd->sr;

	desc = (struct regulator_desc *)sr->priv_data;

	/* regulator index */
	index = rd - sr->rd;

	msgs[0].addr  = sr->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = (rt_uint8_t *)&desc[index].enable_reg;
	msgs[0].len = 1;

	/* read the value */
	msgs[1].addr  = sr->slave_addr;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(sr->handle_driver, msgs, 2) != 2) {
		rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	val &= desc[index].enable_msk;

	return (val ? true : false);
}

static int linear_range_get_value(const struct regulator_linear_range *r, unsigned int selector,
				  unsigned int *val)
{
	if (r->min_sel > selector || r->max_sel < selector)
		return -RT_EINVAL;

	*val = r->min + (selector - r->min_sel) * r->step;

	return 0;
}

static int linear_range_get_value_array(const struct regulator_linear_range *r, int ranges,
					unsigned int selector, unsigned int *val)
{
	int i;

	for (i = 0; i < ranges; i++)
		if (r[i].min_sel <= selector && r[i].max_sel >= selector)
			return linear_range_get_value(&r[i], selector, val);

	return -RT_EINVAL;
}

static int regulator_desc_list_voltage_linear_range(const struct regulator_desc *desc,
						    unsigned int selector)
{
	unsigned int val;
	int ret;

	RT_ASSERT(!desc->n_linear_ranges);

	ret = linear_range_get_value_array(desc->linear_ranges,
					   desc->n_linear_ranges, selector,
					   &val);
	if (ret)
		return ret;

	return val;
}

static unsigned int linear_range_get_max_value(const struct regulator_linear_range *r)
{
	return r->min + (r->max_sel - r->min_sel) * r->step;
}

static int linear_range_get_selector_high(const struct regulator_linear_range *r,
					  unsigned int val, unsigned int *selector,
					  bool *found)
{
	*found = false;

	if (linear_range_get_max_value(r) < val)
		return -RT_EINVAL;

	if (r->min > val) {
		*selector = r->min_sel;
		return 0;
	}

	*found = true;

	if (r->step == 0)
		*selector = r->max_sel;
	else
		*selector = DIV_ROUND_UP(val - r->min, r->step) + r->min_sel;

	return 0;
}

static int regulator_map_voltage_linear_range(const struct regulator_desc *desc,
					      int min_uV, int max_uV)
{
	const struct regulator_linear_range *range;
	int ret = -RT_EINVAL;
	unsigned int sel;
	bool found;
	int voltage, i;

	if (!desc->n_linear_ranges) {
		RT_ASSERT(!desc->n_linear_ranges);
		return -RT_EINVAL;
	}

	for (i = 0; i < desc->n_linear_ranges; i++) {
		range = &desc->linear_ranges[i];

		ret = linear_range_get_selector_high(range, min_uV, &sel,
						     &found);
		if (ret)
			continue;

		ret = sel;

		/*
		 * Map back into a voltage to verify we're still in bounds.
		 * If we are not, then continue checking rest of the ranges.
		 */
		voltage = regulator_desc_list_voltage_linear_range(desc, sel);
		if (voltage >= min_uV && voltage <= max_uV)
			break;
	}

	if (i == desc->n_linear_ranges)
		return -RT_EINVAL;

	return ret;
}

static rt_err_t regulator_dynamic_set_voltage(struct rt_regulator_node *reg, int min_uvolt, int max_uvolt)
{
	int index, sel;
	rt_uint8_t val, cmd;
	struct rt_i2c_msg msgs[2];
	struct regulator_desc *desc;
	struct regulator_dynamic *rd = (struct regulator_dynamic *)reg;
	struct spacemit_regulator *sr = rd->sr;

	desc = (struct regulator_desc *)sr->priv_data;

	/* regulator index */
	index = rd - sr->rd;

	sel = regulator_map_voltage_linear_range(&desc[index], min_uvolt, max_uvolt);
	if (sel >= 0) {
		sel <<= ffs(desc[index].vsel_msk) - 1;

		msgs[0].addr  = sr->slave_addr;
		msgs[0].flags = RT_I2C_WR;
		msgs[0].buf = (rt_uint8_t *)&desc[index].vsel_reg;
		msgs[0].len = 1;

		/* read the value */
		msgs[1].addr  = sr->slave_addr;
		msgs[1].flags = RT_I2C_RD;
		msgs[1].buf = &val;
		msgs[1].len = 1;

		if (rt_i2c_transfer(sr->handle_driver, msgs, 2) != 2) {
			rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
			return -RT_ERROR;
		}

		val &= ~(desc[index].vsel_msk);
		val |= sel;

		/* set the value */
		msgs[0].addr  = sr->slave_addr;
		msgs[0].flags = RT_I2C_WR;
		msgs[0].buf = (rt_uint8_t *)&desc[index].vsel_reg;
		msgs[0].len = 1;

		/* read the value */
		msgs[1].addr  = sr->slave_addr;
		msgs[1].flags = RT_I2C_WR;
		msgs[1].buf = &val;
		msgs[1].len = 1;

		if (rt_i2c_transfer(sr->handle_driver, msgs, 2) != 2) {
			rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
			return -RT_ERROR;
		}
	} else {
		rt_kprintf("%s:%d, set the wrong voltage\n", __func__, __LINE__);
		return -RT_EINVAL;
	}

	return 0;
}

static int regulator_dynamic_get_voltage(struct rt_regulator_node *reg)
{
	int index, sel;
	rt_uint8_t val, cmd;
	struct rt_i2c_msg msgs[2];
	struct regulator_desc *desc;
	struct regulator_dynamic *rd = (struct regulator_dynamic *)reg;
	struct spacemit_regulator *sr = rd->sr;

	desc = (struct regulator_desc *)sr->priv_data;

	/* regulator index */
	index = rd - sr->rd;

	msgs[0].addr  = sr->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = (rt_uint8_t *)&desc[index].vsel_reg;
	msgs[0].len = 1;

	/* read the value */
	msgs[1].addr  = sr->slave_addr;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(sr->handle_driver, msgs, 2) != 2) {
		rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	val &= desc[index].vsel_msk;
	val >>= (ffs(desc[index].vsel_msk) - 1);

	return regulator_desc_list_voltage_linear_range(&desc[index], val);
}

static const struct rt_regulator_ops regulator_dynamic_ops =
{
	.enable = regulator_dynamic_enable,
	.disable = regulator_dynamic_disable,
	.is_enabled = regulator_dynamic_is_enabled,
	.get_voltage = regulator_dynamic_get_voltage,
	.set_voltage = regulator_dynamic_set_voltage
};

static rt_int32_t spacemit_regulator_probe(void)
{
	int ret;
	rt_int32_t i, val, j = 0;
	char *string;
	rt_int32_t size;
	struct spacemit_regulator *sr;
	struct rt_regulator_node *rnp;
	struct dtb_node *compatible_node, *child_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			sr = (struct spacemit_regulator *)rt_calloc(1, sizeof(struct spacemit_regulator));
			if (sr == RT_NULL) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			/* get the handle driver */
			for_each_property_string(compatible_node, "bind_driver", string, size) {
				sr->handle_driver = rt_i2c_bus_device_find(string);
				if (sr->handle_driver == RT_NULL) {
					rt_kprintf("%s:%d, the bind driver has not registered\n", __func__, __LINE__);
					return -RT_EINVAL;
				}
			}

			/* get the slave address */
			dtb_node_read_u32_array(compatible_node, "slave_addr", &sr->slave_addr, 1);

			sr->priv_data = (void *)__compatible[i].data;

			if (dtb_node_get_dtb_node_compatible_match(compatible_node, "regulator-dynamic")) {
				/* this is the parent node */
				dtb_node_read_u32_array(compatible_node, "num_regulators", &val, 1);

				sr->rd = (struct regulator_dynamic *)rt_calloc(val, sizeof(struct regulator_dynamic));
				if (sr->rd == RT_NULL) {
					rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
					return -RT_EINVAL;
				}

				child_node = compatible_node;

				j = 0;

                		for_each_node_child(child_node) {
					if(!dtb_node_get_dtb_node_compatible_match(child_node, "regulator-dynamic"))
						continue;

					regulator_dtb_parse(child_node, &sr->rd[j].param);

					rnp = &sr->rd[j].parent;
					rnp->supply_name = sr->rd[j].param.name;
					rnp->ops = &regulator_dynamic_ops;
					rnp->param = &sr->rd[j].param;
					rnp->dev = &sr->rd[j].dev;
					rnp->dev->node = child_node;

					sr->rd[j].sr = sr;

					/* register the regulator */
					ret = rt_regulator_register(rnp);
					if (ret) {
						rt_kprintf("%s:%d, register regulator error\n", __func__, __LINE__);
						return -RT_EINVAL;
					}

					++j;
				}
			} else {
				sr->rd = (struct regulator_dynamic *)rt_calloc(1, sizeof(struct regulator_dynamic));
				if (sr->rd == RT_NULL) {
					rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
					return -RT_EINVAL;
				}

				/* only has one node */
				regulator_dtb_parse(compatible_node, &sr->rd->param);

				rnp = &sr->rd->parent;
				rnp->supply_name = sr->rd->param.name;
				rnp->ops = &regulator_dynamic_ops;
				rnp->param = &sr->rd->param;
				rnp->dev = &sr->rd->dev;
				rnp->dev->node = compatible_node;

				sr->rd->sr = sr;

				/* register the regulator */
				ret = rt_regulator_register(rnp);
				if (ret) {
					rt_kprintf("%s:%d, register regulator error\n", __func__, __LINE__);
					return -RT_EINVAL;
				}
			}
		}
	}

	return 0;
}
INIT_DEVICE_EXPORT(spacemit_regulator_probe);
