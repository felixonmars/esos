/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-23     GuEe-GUI     first version
 */

#include "regulator_dm.h"

struct regulator_fixed
{
    struct rt_regulator_node parent;
    struct rt_regulator_param param;

    struct gpio_desc *enable_gpio;
    const char *input_supply;
};

#define raw_to_regulator_fixed(raw) rt_container_of(raw, struct regulator_fixed, parent)

static rt_err_t regulator_fixed_enable(struct rt_regulator_node *reg_np)
{
    struct regulator_fixed *rf = raw_to_regulator_fixed(reg_np);
    struct rt_regulator_param *param = &rf->param;

    if (rf->enable_gpio < 0 || param->always_on)
    {
        return RT_EOK;
    }

    gpio_direction_output(rf->enable_gpio);
    __gpio_set_value(rf->enable_gpio, param->enable_active_high ? 1 : 0);

    return RT_EOK;
}

static rt_err_t regulator_fixed_disable(struct rt_regulator_node *reg_np)
{
    struct regulator_fixed *rf = raw_to_regulator_fixed(reg_np);
    struct rt_regulator_param *param = &rf->param;

    if (rf->enable_gpio < 0 || param->always_on)
    {
        return RT_EOK;
    }

    gpio_direction_output(rf->enable_gpio);
    __gpio_set_value(rf->enable_gpio, param->enable_active_high ? 0 : 1);

    return RT_EOK;
}

static rt_bool_t regulator_fixed_is_enabled(struct rt_regulator_node *reg_np)
{
    rt_uint8_t active;
    struct regulator_fixed *rf = raw_to_regulator_fixed(reg_np);
    struct rt_regulator_param *param = &rf->param;

    if (rf->enable_gpio < 0 || param->always_on)
    {
        return RT_TRUE;
    }

    active = __gpio_get_value(rf->enable_gpio);

    if (param->enable_active_high)
    {
        return active == 1;
    }

    return active == 0;
}

static int regulator_fixed_get_voltage(struct rt_regulator_node *reg_np)
{
    struct regulator_fixed *rf = raw_to_regulator_fixed(reg_np);

    return rf->param.min_uvolt + (rf->param.max_uvolt - rf->param.min_uvolt) / 2;
}

static const struct dtb_compatible_array __compatible[] =
{
    { .compatible = "regulator-fixed" },
    { /* sentinel */ }
};

static const struct rt_regulator_ops regulator_fixed_ops =
{
    .enable = regulator_fixed_enable,
    .disable = regulator_fixed_disable,
    .is_enabled = regulator_fixed_is_enabled,
    .get_voltage = regulator_fixed_get_voltage,
};

static rt_err_t regulator_fixed_probe(void)
{
    rt_err_t err;
    rt_uint32_t val;
    struct rt_device *dev = &pdev->parent;
    struct regulator_fixed *rf = rt_calloc(1, sizeof(*rf));
    struct dtb_node *np;
    struct dtb_node *dtb_head_node = get_dtb_node_head();
    struct rt_regulator_node *rnp;

    if (!rf)
    {
        return -RT_ENOMEM;
    }

    for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
	np = dtb_node_find_compatible_node(dtb_head_node, __compatible[i].compatible);
	if (np != RT_NULL) {
	    if (!dtb_node_device_is_available(np))
		continue;
	    regulator_dtb_parse(np, &rf->param);

	    rnp = &rf->parent;
	    rnp->supply_name = rf->param.name;
	    rnp->ops = &regulator_fixed_ops;
	    rnp->param = &rf->param;
	    rnp->dev = &pdev->parent;

	    rf->enable_gpio = of_get_named_gpio_flags(node, "enable-gpios", 0, OF_GPIO_ACTIVE_LOW);
	    if (val < 0) {
		    rt_free(rf);
		    return val;
	    }

	    val = gpio_request(rf->enable_gpio, RT_NULL);
	    if (val < 0) {
	    	rt_free(rf);
		return val;
	    }

	    if (!dtb_node_read_u32(np, "startup-delay-us", &val))
		    rf->param.enable_delay = val;
	    if (!dtb_node_read_u32(np, "off-on-delay-us", &val))
		    rf->param.off_on_delay = val;

	    if ((err = rt_regulator_register(rnp))) {
		    rt_free(rf);
		    return err;
	    }
	}
    }

    return RT_EOK;
}
INIT_DEVICE_EXPORT(regulator_fixed_probe);
