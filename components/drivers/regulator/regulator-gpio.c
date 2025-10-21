/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-23     GuEe-GUI     first version
 */

#include <dt-bindings/pin/state.h>
#include <errno.h>
#include <drivers/regulator_dm.h>

struct regulator_gpio_state
{
    uint32_t value;
    uint32_t gpios;
};

struct regulator_gpio_desc
{
    rt_base_t pin;
    rt_uint32_t flags;
};

struct regulator_gpio
{
    struct rt_regulator_node parent;

    rt_base_t enable_pin;

    rt_size_t pins_nr;
    struct regulator_gpio_desc *pins_desc;
    struct dtb_node *node;

    int state;
    rt_size_t states_nr;
    struct regulator_gpio_state *states;

    const char *input_supply;
    uint32_t startup_delay;
    uint32_t off_on_delay;
    rt_bool_t enabled_at_boot;
    struct rt_regulator_param param;
};

#define raw_to_regulator_gpio(raw) rt_container_of(raw, struct regulator_gpio, parent)

static rt_err_t regulator_gpio_enable(struct rt_regulator_node *reg_np)
{
    struct regulator_gpio *rg = raw_to_regulator_gpio(reg_np);
    struct rt_regulator_param *param = &rg->param;

    if (param->always_on)
    {
        return RT_EOK;
    }

    if (rg->enable_pin >= 0)
    {
        gpio_direction_output(rg->enable_pin, 1);
        __gpio_set_value(rg->enable_pin, param->enable_active_high ? PIN_HIGH : PIN_LOW);
    }

    return RT_EOK;
}

static rt_err_t regulator_gpio_disable(struct rt_regulator_node *reg_np)
{
    struct regulator_gpio *rg = raw_to_regulator_gpio(reg_np);
    struct rt_regulator_param *param = &rg->param;

    if (param->always_on)
    {
        return RT_EOK;
    }

    if (rg->enable_pin >= 0)
    {
        gpio_direction_output(rg->enable_pin, 1);
        __gpio_set_value(rg->enable_pin, param->enable_active_high ? PIN_LOW : PIN_HIGH);
    }

    return RT_EOK;
}

static rt_bool_t regulator_gpio_is_enabled(struct rt_regulator_node *reg_np)
{
    struct regulator_gpio *rg = raw_to_regulator_gpio(reg_np);
    struct rt_regulator_param *param = &rg->param;

    if (param->always_on)
    {
        return RT_TRUE;
    }

    if (rg->enable_pin >= 0)
    {
        rt_uint8_t active_val = param->enable_active_high ? PIN_LOW : PIN_HIGH;

        return __gpio_get_value(rg->enable_pin) == active_val;
    }

    return RT_TRUE;
}

static rt_err_t regulator_gpio_set_voltage(struct rt_regulator_node *reg_np,
        int min_uvolt, int max_uvolt)
{
    int target = 0, best_val = RT_REGULATOR_UVOLT_INVALID;
    struct regulator_gpio *rg = raw_to_regulator_gpio(reg_np);

    for (int i = 0; i < rg->states_nr; ++i)
    {
        struct regulator_gpio_state *state = &rg->states[i];

        if (state->value < best_val &&
            state->value >= min_uvolt &&
            state->value <= max_uvolt)
        {
            target = state->gpios;
            best_val = state->value;
        }
    }

    if (best_val == RT_REGULATOR_UVOLT_INVALID)
    {
        return -RT_EINVAL;
    }

    for (int i = 0; i < rg->pins_nr; ++i)
    {
        int state = (target >> i) & 1;
        struct regulator_gpio_desc *gpiod = &rg->pins_desc[i];

        gpio_direction_output(gpiod->pin, 1);
        __gpio_set_value(gpiod->pin, gpiod->flags == PIND_OUT_HIGH ? state : !state);
    }

    rg->state = target;

    return RT_EOK;
}

static int regulator_gpio_get_voltage(struct rt_regulator_node *reg_np)
{
    struct regulator_gpio *rg = raw_to_regulator_gpio(reg_np);

    for (int i = 0; i < rg->states_nr; ++i)
    {
        if (rg->states[i].gpios == rg->state)
        {
            return rg->states[i].value;
        }
    }

    return -RT_EINVAL;
}

static const struct rt_regulator_ops regulator_gpio_ops =
{
    .enable = regulator_gpio_enable,
    .disable = regulator_gpio_disable,
    .is_enabled = regulator_gpio_is_enabled,
    .set_voltage = regulator_gpio_set_voltage,
    .get_voltage = regulator_gpio_get_voltage,
};

static const struct dtb_compatible_array __compatible[] =
{
    { .compatible = "regulator-gpio" },
    { /* sentinel */ }
};

static rt_err_t regulator_gpio_probe(void)
{
    rt_err_t err;
    struct regulator_gpio *rg = rt_calloc(1, sizeof(*rg));
    struct rt_regulator_node *rgp;
    struct dtb_node *compatible_node;
    struct dtb_node *dtb_head_node = get_dtb_node_head();
    struct regulator_gpio_desc *gpiod;
    struct fdt_phandle_args *args;
    int i, j;

    if (!rg)
    {
        return -RT_ENOMEM;
    }

    for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
        compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);
	if (compatible_node != RT_NULL) {
	    if (!dtb_node_device_is_available(compatible_node))
		    continue;
	    regulator_dtb_parse(compatible_node, &rg->param);

	    rgp = &rg->parent;
	    rgp->supply_name = rg->param.name;
	    rgp->ops = &regulator_gpio_ops;
	    rgp->param = &rg->param;
	    rg->node = compatible_node;

	    dtb_node_read_u32(compatible_node, "startup_delay_us", &rg->startup_delay);
	    dtb_node_read_u32(compatible_node, "off-on-delay-us", &rg->off_on_delay);

	    /* GPIO flags are ignored, we check by enable-active-high */
	    rg->enable_pin = of_get_named_gpio_flags(compatible_node, "enable-gpios", 0, NULL);
	    if ((rg->enable_pin < 0) && (rg->enable_pin != -ENOENT))
	    {
	        err = rg->enable_pin;
		goto _fail;
	    }

	    if (rg->enable_pin >= 0) {
		    err = gpio_request(rg->enable_pin, RT_NULL);
		    if (err < 0) {
		        goto _fail;
		    }
	    }
	    dtb_node_read_u32(compatible_node, "gpios-num", &rg->pins_nr);
	    if (rg->pins_nr > 0) {
		rg->pins_desc = rt_malloc(sizeof(*rg->pins_desc) * rg->pins_nr);
		if (!rg->pins_desc) {
		    err = -RT_ENOMEM;
		    goto _fail;
		}
		for (j = 0; j < rg->pins_nr; ++i) {
		    uint32_t val;
		    struct regulator_gpio_desc *gpiod = &rg->pins_desc[i];

		    dtb_node_parse_phandle_with_args(compatible_node, "gpios", "#gpio-cells", i, args);
		    gpiod->pin = args->args[0];

		    err = gpio_request(gpiod->pin, RT_NULL);

		    if (dtb_node_read_u32_index(compatible_node, "gpios-states", i, &val) < 0)
			    gpiod->flags = 1;
		    else
			    gpiod->flags = val ? 1 : 0;
		    if (gpiod->flags == 1)
			    rg->state |= 1 << i;

		}
	    }

	    rg->states_nr = dtb_node_property_read_count_u32(compatible_node, "states") / 2;
	    if (rg->states_nr < 0) {
	        err = -RT_EIO;
		return err;
	    }

	    rg->states = rt_malloc(sizeof(*rg->states) * rg->states_nr);
	    if (!rg->states)
	    {
	    	err = -RT_ENOMEM;
		return err;
	    }

	    for (int i = 0; i < rg->states_nr; ++i) {
	        dtb_node_read_u32_index(compatible_node, "states", i * 2, &rg->states[i].value);
		dtb_node_read_u32_index(compatible_node, "states", i * 2 + 1, &rg->states[i].gpios);
	    }

	    if (err = rt_regulator_register(rgp)) {
	    	return err;
	    }
	}
    }

    return RT_EOK;

_fail:
    if (rg->pins_desc)
    {
        rt_free(rg->pins_desc);
    }
    if (rg->states)
    {
        rt_free(rg->states);
    }
    rt_free(rg);

    return err;
}
INIT_DEVICE_EXPORT(regulator_gpio_probe);
