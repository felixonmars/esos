#include <rtthread.h>
#include <rtdevice.h>

/* Private data structure for of_gpiochip_find_and_xlate */
struct gg_data {
	enum of_gpio_flags *flags;
	struct fdt_phandle_args gpiospec;

	int out_gpio;
};

/**
 * of_gpio_simple_xlate - translate gpio_spec to the GPIO number and flags
 * @gc:         pointer to the gpio_chip structure
 * @np:         device node of the GPIO chip
 * @gpio_spec:  gpio specifier as found in the device tree
 * @flags:      a flags pointer to fill in
 *
 * This is simple translation function, suitable for the most 1:1 mapped
 * gpio chips. This function performs only one sanity check: whether gpio
 * is less than ngpios (that is specified in the gpio_chip).
 */
int of_gpio_simple_xlate(struct gpio_chip *gc,
                         const struct fdt_phandle_args *gpiospec, rt_uint32_t *flags)
{
        /*
         * We're discouraging gpio_cells < 2, since that way you'll have to
         * write your own xlate function (that will have to retrive the GPIO
         * number and the flags from a single gpio cell -- this is possible,
         * but not recommended).
         */
	if (gc->of_gpio_n_cells < 2) {
		rt_kprintf("%s:%d, error\n", __func__, __LINE__);
                return -RT_EINVAL;
        }

	if (gpiospec->args_count < gc->of_gpio_n_cells)
		return -RT_EINVAL;

	if (gpiospec->args[0] >= gc->ngpio)
		return -RT_EINVAL;

	if (flags)
		*flags = gpiospec->args[1];

	return gpiospec->args[0];
}

static void of_gpiochip_add_pin_range(struct gpio_chip *chip)
{
	struct dtb_node *np = chip->of_node;
	struct fdt_phandle_args pinspec;
	struct pinctrl_dev *pctldev;
	int index = 0, ret;

	if (!np)
		return;

	for (;; index++) {
		ret = dtb_node_parse_phandle_with_args(np, "gpio-ranges",
				"#gpio-range-cells", index, &pinspec);
		if (ret)
			break;

		pctldev = get_pinctrl_dev_from_of_node(pinspec.np);
		if (!pctldev)
			break;

		ret = gpiochip_add_pin_range(chip,
					pctldev->dev->name,
					pinspec.args[0],
					pinspec.args[1],
					pinspec.args[2]);
		
		if (ret)
			break;
	}
}

void of_gpiochip_add(struct gpio_chip *chip)
{
	if (!chip->of_node)
		return;

	if (!chip->of_xlate) {
		chip->of_gpio_n_cells = 2;
		chip->of_xlate = of_gpio_simple_xlate;
	}

	of_gpiochip_add_pin_range(chip);
}

/* Private function for resolving node pointer to gpio_chip */
static int of_gpiochip_find_and_xlate(struct gpio_chip *gc, void *data)
{
	struct gg_data *gg_data = data;
	int ret;

	if ((gc->of_node != gg_data->gpiospec.np) ||
			(gc->of_gpio_n_cells != gg_data->gpiospec.args_count) ||
			(!gc->of_xlate))
		return false;

	ret = gc->of_xlate(gc, &gg_data->gpiospec, gg_data->flags);
	if (ret < 0)
		return false;

	gg_data->out_gpio = ret + gc->base;
	
	return true;
}

#undef EPROBE_DEFER
#define EPROBE_DEFER    517     /* Driver requests probe retry */

/**
 * of_get_named_gpio_flags() - Get a GPIO number and flags to use with GPIO API
 * @np:         device node to get GPIO from
 * @propname:   property name containing gpio specifier(s)
 * @index:      index of the GPIO
 * @flags:      a flags pointer to fill in
 *
 * Returns GPIO number to use with Linux generic GPIO API, or one of the errno
 * value on the error condition. If @flags is not NULL the function also fills
 * in flags for the GPIO.
 */
int of_get_named_gpio_flags(struct dtb_node *np, const char *propname,
				int index, enum of_gpio_flags *flags)
{
	/* Return -EPROBE_DEFER to support probe() functions to be called
	 * later when the GPIO actually becomes available
	 */
	struct gg_data gg_data = { .flags = flags, .out_gpio = -EPROBE_DEFER };
	int ret;

	/* .of_xlate might decide to not fill in the flags, so clear it. */
	if (flags)
		*flags = 0;

	ret = dtb_node_parse_phandle_with_args(np, propname, "#gpio-cells", index,
                                         &gg_data.gpiospec);
	if (ret) {
		rt_kprintf("%s: can't parse gpios property\n", __func__);
		return ret;
	}

	gpiochip_find(&gg_data, of_gpiochip_find_and_xlate);

	dtb_node_put(gg_data.gpiospec.np);
	rt_kprintf("%s exited with status %d\n", __func__, gg_data.out_gpio);

	return gg_data.out_gpio;
}

