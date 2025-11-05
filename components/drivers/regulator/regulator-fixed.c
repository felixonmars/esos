/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-23     GuEe-GUI     first version
 */

#include <drivers/regulator_dm.h>

struct regulator_fixed
{
    struct rt_regulator_node parent;
    struct rt_regulator_param param;

    struct rt_device dev;
    int enable_gpio;
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

    gpio_direction_output(rf->enable_gpio, param->enable_active_high ? 1 : 0);

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

    gpio_direction_output(rf->enable_gpio, param->enable_active_high ? 0 : 1);

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

    return rf->param.min_uvolt/* + (rf->param.max_uvolt - rf->param.min_uvolt) / 2 */;
}

static const struct rt_regulator_ops regulator_fixed_ops =
{
    .enable = regulator_fixed_enable,
    .disable = regulator_fixed_disable,
    .is_enabled = regulator_fixed_is_enabled,
    .get_voltage = regulator_fixed_get_voltage,
};

#define for_each_property_of_node(dn, pp) \
    for (pp = dn->properties; pp != NULL; pp = pp->next)

static int regulator_fixed_probe(void)
{
    rt_err_t err;
    rt_uint32_t val;
    phandle supply_phandle;
    struct dtb_property *pp;
    struct dtb_node *np, *fdt_aliases;
    struct dtb_node *dtb_head_node = get_dtb_node_head();
    struct rt_regulator_node *rnp;

    fdt_aliases = dtb_node_get_dtb_node_by_path(dtb_head_node, "/aliases");
    if (fdt_aliases < 0) {
        rt_kprintf("Get alias error\n");
        return -RT_EINVAL;
    }

    for_each_property_of_node(fdt_aliases, pp)
    {
        if (strncmp(pp->name, "regulator-fixed", 15) != 0)
            continue;

         dtb_node_read_u32(fdt_aliases, pp->name, &supply_phandle);
         np = dtb_node_find_node_by_phandle(supply_phandle);
         if (!np)
         {
             rt_kprintf("%s:%d: %s, get node error\n", __func__, __LINE__, pp->value);
             return -RT_EINVAL;
         }

         struct regulator_fixed *rf = rt_calloc(1, sizeof(*rf));
         regulator_dtb_parse(np, &rf->param);

         rnp = &rf->parent;
         rnp->supply_name = rf->param.name;
         rnp->ops = &regulator_fixed_ops;
         rnp->param = &rf->param;
         rnp->dev = &rf->dev;
         rnp->dev->node = np;

         rf->enable_gpio = of_get_named_gpio_flags(np, "enable-gpios", 0, RT_NULL);
         if (rf->enable_gpio < 0)
             goto no_gpios;

         err = gpio_request(rf->enable_gpio, RT_NULL);
         if (err < 0)
             goto no_gpios;
no_gpios:
         if (!dtb_node_read_u32(np, "startup-delay-us", &val))
             rf->param.enable_delay = val;
         if (!dtb_node_read_u32(np, "off-on-delay-us", &val))
             rf->param.off_on_delay = val;

         if ((err = rt_regulator_register(rnp)))
         {
              rt_free(rf);
              return err;
         }
    }

    return RT_EOK;
}
INIT_PREV_EXPORT(regulator_fixed_probe);
