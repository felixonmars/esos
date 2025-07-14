#include <rtthread.h>
#include "pinctrl-core.h"

int pinmux_check_ops(struct pinctrl_dev *pctldev)
{
	const struct pinmux_ops *ops = pctldev->desc->pmxops;
	rt_uint32_t nfuncs;
	rt_uint32_t selector = 0;

	/* Check that we implement required operations */
	if (!ops ||
			!ops->get_functions_count ||
			!ops->get_function_name ||
			!ops->get_function_groups ||
			!ops->enable) {
		rt_kprintf("pinmux ops lacks necessary functions\n");
		return -RT_EINVAL;
	}

	/* Check that all functions registered have names */
	nfuncs = ops->get_functions_count(pctldev);
	while (selector < nfuncs) {
		const char *fname = ops->get_function_name(pctldev, selector);
		if (!fname) {
			rt_kprintf("pinmux ops has no name for function%u\n", selector);
			return -RT_EINVAL;
		}
		selector++;
	}

	return 0;
}

int pinmux_validate_map(struct pinctrl_map const *map, int i)
{
	if (!map->data.mux.function) {
		rt_kprintf("failed to register map %s (%d): no function given\n",
                       map->name, i);
		return -RT_EINVAL;
	}

	return 0;
}

static int pinmux_func_name_to_selector(struct pinctrl_dev *pctldev,
                                        const char *function)
{
	const struct pinmux_ops *ops = pctldev->desc->pmxops;
	rt_uint32_t nfuncs = ops->get_functions_count(pctldev);
	rt_uint32_t selector = 0;

	/* See if this pctldev has this function */
	while (selector < nfuncs) {
		const char *fname = ops->get_function_name(pctldev, selector);
		if (!rt_strcmp(function, fname))
			return selector;

		selector++;
	}

        rt_kprintf("%s does not support function %s\n",
               pinctrl_dev_get_name(pctldev), function);

        return -RT_EINVAL;
}

int pinmux_map_to_setting(struct pinctrl_map const *map,
                          struct pinctrl_setting *setting)
{
	struct pinctrl_dev *pctldev = setting->pctldev;
	const struct pinmux_ops *pmxops = pctldev->desc->pmxops;
	char const * const *groups;
	rt_uint32_t num_groups;
	int ret;
	const char *group;
	int i;

	if (!pmxops) {
		rt_kprintf("does not support mux function\n");
		return -RT_EINVAL;
	}

	ret = pinmux_func_name_to_selector(pctldev, map->data.mux.function);
	if (ret < 0) {
		rt_kprintf("invalid function %s in map table\n", map->data.mux.function);
		return ret;
	}
	setting->data.mux.func = ret;

	ret = pmxops->get_function_groups(pctldev, setting->data.mux.func, &groups, &num_groups);
	if (ret < 0) {
		rt_kprintf("can't query groups for function %s\n", map->data.mux.function);
		return ret;
	}
	if (!num_groups) {
		rt_kprintf("function %s can't be selected on any group\n", map->data.mux.function);
		return -RT_EINVAL;
	}
	if (map->data.mux.group) {
		bool found = false;
		group = map->data.mux.group;
		for (i = 0; i < num_groups; i++) {
			if (!strcmp(group, groups[i])) {
				found = true;
				break;
			}
		}
		if (!found) {
			rt_kprintf("invalid group \"%s\" for function \"%s\"\n", group, map->data.mux.function);
			return -RT_EINVAL;
		}
	} else {
		group = groups[0];
	}

	ret = pinctrl_get_group_selector(pctldev, group);
	if (ret < 0) {
		rt_kprintf("invalid group %s in map table\n", map->data.mux.group);
		return ret;
	}
	setting->data.mux.group = ret;

	return 0;
}

/**
 * pin_free() - release a single muxed in pin so something else can be muxed
 * @pctldev: pin controller device handling this pin
 * @pin: the pin to free
 * @gpio_range: the range matching the GPIO pin if this is a request for a
 *      single GPIO pin
 *
 * This function returns a pointer to the previous owner. This is used
 * for callers that dynamically allocate an owner name so it can be freed
 * once the pin is free. This is done for GPIO request functions.
 */
static const char *pin_free(struct pinctrl_dev *pctldev, int pin,
                            struct pinctrl_gpio_range *gpio_range)
{
	const struct pinmux_ops *ops = pctldev->desc->pmxops;
	struct pin_desc *desc;
	const char *owner;

	desc = pin_desc_get(pctldev, pin);
	if (desc == NULL) {
                rt_kprintf("pin is not registered so it cannot be freed\n");
		return NULL;
	}

	if (!gpio_range) {
		/*
		 * A pin should not be freed more times than allocated.
		 */
		if (!desc->mux_usecount)
			return NULL;
		desc->mux_usecount--;
		if (desc->mux_usecount)
			return NULL;
	}

	/*
	 * If there is no kind of request function for the pin we just assume
	 * we got it by default and proceed.
	 */
	if (gpio_range && ops->gpio_disable_free)
		ops->gpio_disable_free(pctldev, gpio_range, pin);
	else if (ops->free)
		ops->free(pctldev, pin);

	if (gpio_range) {
		owner = desc->gpio_owner;
		desc->gpio_owner = NULL;
	} else {
		owner = desc->mux_owner;
		desc->mux_owner = NULL;
		desc->mux_setting = NULL;
	}

	/* module_put(pctldev->owner); */

        return owner;
}

void pinmux_disable_setting(struct pinctrl_setting const *setting)
{
	struct pinctrl_dev *pctldev = setting->pctldev;
	const struct pinctrl_ops *pctlops = pctldev->desc->pctlops;
	const struct pinmux_ops *ops = pctldev->desc->pmxops;
	int ret;
	const rt_uint32_t *pins;
	rt_uint32_t num_pins;
	int i;
	struct pin_desc *desc;

	ret = pctlops->get_group_pins(pctldev, setting->data.mux.group, &pins, &num_pins);
	if (ret) {
		/* errors only affect debug data, so just warn */
		rt_kprintf("could not get pins for group selector %d\n", setting->data.mux.group);
		num_pins = 0;
	}

	/* Flag the descs that no setting is active */
	for (i = 0; i < num_pins; i++) {
		desc = pin_desc_get(pctldev, pins[i]);
		if (desc == NULL) {
			rt_kprintf("could not get pin desc for pin %d\n", pins[i]);
			continue;
		}
		desc->mux_setting = NULL;
	}

	/* And release the pins */
	for (i = 0; i < num_pins; i++)
		pin_free(pctldev, pins[i], NULL);

	if (ops->disable)
		ops->disable(pctldev, setting->data.mux.func, setting->data.mux.group);
}

void pinmux_free_setting(struct pinctrl_setting const *setting)
{
	/* This function is currently unused */
}

/**
 * pin_request() - request a single pin to be muxed in, typically for GPIO
 * @pin: the pin number in the global pin space
 * @owner: a representation of the owner of this pin; typically the device
 *      name that controls its mux function, or the requested GPIO name
 * @gpio_range: the range matching the GPIO pin if this is a request for a
 *      single GPIO pin
 */
static int pin_request(struct pinctrl_dev *pctldev,
                       int pin, const char *owner,
                       struct pinctrl_gpio_range *gpio_range)
{
	struct pin_desc *desc;
	const struct pinmux_ops *ops = pctldev->desc->pmxops;
	int status = -RT_EINVAL;

	desc = pin_desc_get(pctldev, pin);
	if (desc == NULL) {
                rt_kprintf("pin %d is not registered so it cannot be requested\n", pin);
		goto out;
	}

	rt_kprintf("request pin %d (%s) for %s\n", pin, desc->name, owner);

	if (gpio_range) {
		/* There's no need to support multiple GPIO requests */
		if (desc->gpio_owner) {
                        rt_kprintf("pin %s already requested by %s; cannot claim for %s\n", desc->name, desc->gpio_owner, owner);
			goto out;
		}

		desc->gpio_owner = owner;
	} else {
		if (desc->mux_usecount && rt_strcmp(desc->mux_owner, owner)) {
			rt_kprintf("pin %s already requested by %s; cannot claim for %s\n",
					desc->name, desc->mux_owner, owner);
			goto out;
		}

		desc->mux_usecount++;
		if (desc->mux_usecount > 1)
			return 0;

		desc->mux_owner = owner;
	}

#if 0
	/* Let each pin increase references to this module */
	if (!try_module_get(pctldev->owner)) {
		rt_kprintf("could not increase module refcount for pin %d\n", pin);
		status = -EINVAL;
		goto out_free_pin;
	}
#endif

	/*
	 * If there is no kind of request function for the pin we just assume
	 * we got it by default and proceed.
	 */
	if (gpio_range && ops->gpio_request_enable)
		/* This requests and enables a single GPIO pin */
		status = ops->gpio_request_enable(pctldev, gpio_range, pin);
	else if (ops->request)
		status = ops->request(pctldev, pin);
	else
		status = 0;

	if (status) {
		rt_kprintf("request() failed for pin %d\n", pin);
//		module_put(pctldev->owner);
	}

// out_free_pin:
	if (status) {
		if (gpio_range) {
			desc->gpio_owner = NULL;
		} else {
			desc->mux_usecount--;
			if (!desc->mux_usecount)
				desc->mux_owner = NULL;
		}
	}
out:
   if (status)
	   rt_kprintf("pin-%d (%s) status %d\n", pin, owner, status);

	return status;
}

int pinmux_enable_setting(struct pinctrl_setting const *setting)
{
	struct pinctrl_dev *pctldev = setting->pctldev;
	const struct pinctrl_ops *pctlops = pctldev->desc->pctlops;
	const struct pinmux_ops *ops = pctldev->desc->pmxops;
	int ret;
	const rt_uint32_t *pins;
	rt_uint32_t num_pins;
	int i;
	struct pin_desc *desc;

	ret = pctlops->get_group_pins(pctldev, setting->data.mux.group,	&pins, &num_pins);
	if (ret) {
		/* errors only affect debug data, so just warn */
		rt_kprintf("could not get pins for group selector %d\n", setting->data.mux.group);
		num_pins = 0;
	}

	/* Try to allocate all pins in this group, one by one */
	for (i = 0; i < num_pins; i++) {
		ret = pin_request(pctldev, pins[i], setting->dev_name, NULL);
		if (ret) {
                        rt_kprintf("could not request pin %d on device %s\n", pins[i], pinctrl_dev_get_name(pctldev));
			goto err_pin_request;
		}
	}

	/* Now that we have acquired the pins, encode the mux setting */
	for (i = 0; i < num_pins; i++) {
		desc = pin_desc_get(pctldev, pins[i]);
		if (desc == NULL) {
			rt_kprintf("could not get pin desc for pin %d\n", pins[i]);
			continue;
		}
		desc->mux_setting = &(setting->data.mux);
	}

	ret = ops->enable(pctldev, setting->data.mux.func,
			setting->data.mux.group);

	if (ret)
		goto err_enable;

	return 0;

err_enable:
	for (i = 0; i < num_pins; i++) {
		desc = pin_desc_get(pctldev, pins[i]);
		if (desc)
			desc->mux_setting = NULL;
	}
err_pin_request:
	/* On error release all taken pins */
	while (--i >= 0)
		pin_free(pctldev, pins[i], NULL);

        return ret;
}

extern char *rt_kasprintf(const char *fmt, ...);

/**
 * pinmux_request_gpio() - request pinmuxing for a GPIO pin
 * @pctldev: pin controller device affected
 * @pin: the pin to mux in for GPIO
 * @range: the applicable GPIO range
 */
int pinmux_request_gpio(struct pinctrl_dev *pctldev,
                        struct pinctrl_gpio_range *range,
                        rt_uint32_t pin, rt_uint32_t gpio)
{
	char *owner;
	int ret;

	/* Conjure some name stating what chip and pin this is taken by */
	owner = rt_kasprintf("%s:%d", range->name, gpio);
	if (!owner)
		return -RT_EINVAL;

	ret = pin_request(pctldev, pin, owner, range);
	if (ret < 0)
		rt_free((void *)owner);

	return ret;
}

/**
 * pinmux_free_gpio() - release a pin from GPIO muxing
 * @pctldev: the pin controller device for the pin
 * @pin: the affected currently GPIO-muxed in pin
 * @range: applicable GPIO range
 */
void pinmux_free_gpio(struct pinctrl_dev *pctldev, rt_uint32_t pin,
                      struct pinctrl_gpio_range *range)
{
	const char *owner;

	owner = pin_free(pctldev, pin, range);
	rt_free((void *)owner);
}

