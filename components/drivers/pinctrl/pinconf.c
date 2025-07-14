#include <rtthread.h>
#include "pinctrl-core.h"

int pinconf_check_ops(struct pinctrl_dev *pctldev)
{
	const struct pinconf_ops *ops = pctldev->desc->confops;

	/* We must be able to read out pin status */
	if (!ops->pin_config_get && !ops->pin_config_group_get) {
		rt_kprintf("pinconf must be able to read out pin status\n");
		return -RT_EINVAL;
	}

	/* We have to be able to config the pins in SOME way */
	if (!ops->pin_config_set && !ops->pin_config_group_set) {
		rt_kprintf("pinconf has to be able to set a pins config\n");
	       	return -RT_EINVAL;
	}

	return 0;
}

int pinconf_validate_map(struct pinctrl_map const *map, rt_uint32_t i)
{
	if (!map->data.configs.group_or_pin) {
		rt_kprintf("failed to register map %s (%d): no group/pin given\n",
                       map->name, i);
		return -RT_EINVAL;
	}

	if (!map->data.configs.num_configs ||
			!map->data.configs.configs) {
		rt_kprintf("failed to register map %s (%d): no configs given\n",
                       map->name, i);
		return -RT_EINVAL;
	}

	return 0;
}

int pinconf_map_to_setting(struct pinctrl_map const *map,
                          struct pinctrl_setting *setting)
{
	struct pinctrl_dev *pctldev = setting->pctldev;
	rt_uint32_t pin;

	switch (setting->type) {
		case PIN_MAP_TYPE_CONFIGS_PIN:
		pin = pin_get_from_name(pctldev, map->data.configs.group_or_pin);
		if (pin < 0) {
			rt_kprintf("could not map pin config for \"%s\"", map->data.configs.group_or_pin);
			return pin;
		}
		setting->data.configs.group_or_pin = pin;
		break;
		case PIN_MAP_TYPE_CONFIGS_GROUP:
		pin = pinctrl_get_group_selector(pctldev, map->data.configs.group_or_pin);
		if (pin < 0) {
			rt_kprintf("could not map group config for \"%s\"", map->data.configs.group_or_pin);
			return pin;
		}
		setting->data.configs.group_or_pin = pin;
		break;
		default:
		return -RT_EINVAL;
	}

	setting->data.configs.num_configs = map->data.configs.num_configs;
	setting->data.configs.configs = map->data.configs.configs;

	return 0;
}

void pinconf_free_setting(struct pinctrl_setting const *setting)
{
}

int pinconf_apply_setting(struct pinctrl_setting const *setting)
{
	struct pinctrl_dev *pctldev = setting->pctldev;
	const struct pinconf_ops *ops = pctldev->desc->confops;
	int i, ret;

	if (!ops) {
		rt_kprintf("missing confops\n");
		return -RT_EINVAL;
	}

	switch (setting->type) {
	case PIN_MAP_TYPE_CONFIGS_PIN:
		if (!ops->pin_config_set) {
			rt_kprintf("missing pin_config_set op\n");
			return -RT_EINVAL;
		}
		for (i = 0; i < setting->data.configs.num_configs; i++) {
			ret = ops->pin_config_set(pctldev,
					setting->data.configs.group_or_pin,
					setting->data.configs.configs[i]);
			if (ret < 0) {
				rt_kprintf("pin_config_set op failed for pin %d config %08lx\n",
					setting->data.configs.group_or_pin,
					setting->data.configs.configs[i]);
				return ret;
			}
		}
		break;
	case PIN_MAP_TYPE_CONFIGS_GROUP:
		if (!ops->pin_config_group_set) {
			rt_kprintf("missing pin_config_group_set op\n");
			return -RT_EINVAL;
		}
		for (i = 0; i < setting->data.configs.num_configs; i++) {
			ret = ops->pin_config_group_set(pctldev,
					setting->data.configs.group_or_pin,
					setting->data.configs.configs[i]);
			if (ret < 0) {
				rt_kprintf("pin_config_group_set op failed for group %d config %08lx\n",
					setting->data.configs.group_or_pin,
					setting->data.configs.configs[i]);
				return ret;
			}
		}
		break;
	default:
		return -RT_EINVAL;
	}

	return 0;
}

