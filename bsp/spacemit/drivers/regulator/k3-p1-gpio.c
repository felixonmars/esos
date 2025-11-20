/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/i2c.h>
#include <drivers/regulator.h>
#include <drivers/regulator_dm.h>
#include "regulator.h"

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k3-pwrgpio", },
	{ },
};

struct spacemit_pmic_chip {
	struct gpio_chip chip;
	rt_uint32_t slave_addr;
	rt_uint32_t gbase;
	rt_uint32_t ngpio;
	struct rt_i2c_bus_device *handle_driver;
};

static void pmic_gpio_set(struct gpio_chip *chip, unsigned offset, int value);

static int pmic_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	/* the function is pre-burned for p1 k3 */
	/* TODO */

	return 0;
}

static void pmic_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	/* the function is pre-burned for p1 k3 */
	/* TODO */
}

static int pmic_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	/* pre burned for p1 k3 */
	return 0;
}

static int pmic_gpio_direction_output(struct gpio_chip *chip,
					  unsigned offset, int value)
{
	pmic_gpio_set(chip, offset, value);

	return 0;
}

static int pmic_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	/* TODO */
}

static void pmic_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	rt_uint8_t base = P1_GPIO_ODR_REG, val;
	struct rt_i2c_msg msgs[2];
	struct spacemit_pmic_chip *pmic_chip =
		rt_container_of(chip, struct spacemit_pmic_chip, chip);

	msgs[0].addr = pmic_chip->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = &base;
	msgs[0].len = 1;

	/* read the value */
	msgs[1].addr  = pmic_chip->slave_addr;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(pmic_chip->handle_driver, msgs, 2) != 2) {
		rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
		return;
	}

	if (value) {
		/* output high */
		val &= P1_GPIO_ODR_MSK;
		val |= (1 << (pmic_chip->gbase + offset));
	} else {
		/* output low */
		val &= P1_GPIO_ODR_MSK;
		val &= ~(1 << (pmic_chip->gbase + offset));
	}

	/* write the value */
	msgs[0].addr = pmic_chip->slave_addr;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = &base;
	msgs[0].len = 1;

	msgs[1].addr  = pmic_chip->slave_addr;
	msgs[1].flags = RT_I2C_WR;
	msgs[1].buf = &val;
	msgs[1].len = 1;

	if (rt_i2c_transfer(pmic_chip->handle_driver, msgs, 2) != 2) {
		rt_kprintf("%s:%d, transfer error\n", __func__, __LINE__);
		return;
	}
}

static int pmic_gpio_of_xlate(struct gpio_chip *chip,
				  const struct fdt_phandle_args *gpiospec,
				  unsigned int *flags)
{
	struct spacemit_pmic_chip *pmic_chip =
		rt_container_of(chip, struct spacemit_pmic_chip, chip);

	if (gpiospec->args[0] >= pmic_chip->gbase + pmic_chip->ngpio)
		return -RT_EINVAL;

	if (flags)
		*flags = gpiospec->args[1];

	return gpiospec->args[0];
}

static int spacemit_pmic_gpio_init(void)
{
	int i, ret;
	char *string, *strend;
	rt_int32_t size;
	struct spacemit_pmic_chip *chip;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	struct dtb_node *compatible_node;

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			chip = (struct spacemit_pmic_chip *)rt_calloc(1, sizeof(*chip));
			if (!chip) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			dtb_node_read_u32_array(compatible_node, "slave_addr", &chip->slave_addr, 1);
			dtb_node_read_u32_array(compatible_node, "gpio_base", &chip->gbase, 1);
			dtb_node_read_u32_array(compatible_node, "ngpios", &chip->ngpio, 1);

			for_each_property_string_extend(compatible_node, "bind_driver", string, strend, size) {
				chip->handle_driver = rt_i2c_bus_device_find(string);
				if (chip->handle_driver == RT_NULL) {
					rt_kprintf("%s:%d, the bind driver has not registered\n", __func__, __LINE__);
					return -RT_EINVAL;
				}
			}

			chip->chip.label               = "pmic-gpio";
			chip->chip.request             = pmic_gpio_request;
			chip->chip.free                = pmic_gpio_free;
			chip->chip.direction_input     = pmic_gpio_direction_input;
			chip->chip.direction_output    = pmic_gpio_direction_output;
			chip->chip.get                 = pmic_gpio_get;
			chip->chip.set                 = pmic_gpio_set;
			chip->chip.to_irq              = RT_NULL;
			chip->chip.of_node             = compatible_node;
			chip->chip.of_xlate            = pmic_gpio_of_xlate;
			chip->chip.of_gpio_n_cells     = 2;
			chip->chip.base                = chip->gbase;
			chip->chip.ngpio               = chip->ngpio;

			ret = gpiochip_add(&chip->chip);
			if (ret) {
				rt_kprintf("%s:%d, add gpio chip error\n", __func__, __LINE__);
				return -RT_EINVAL;
			}
		}
	}

	return 0;
}
INIT_PREV_EXPORT(spacemit_pmic_gpio_init);
