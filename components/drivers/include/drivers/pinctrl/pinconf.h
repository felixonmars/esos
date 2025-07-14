#ifndef __RT_PINCONF_H__
#define __RT_PINCONF_H__

struct pinctrl_dev;

/**
 * struct pinconf_ops - pin config operations, to be implemented by
 * pin configuration capable drivers.
 * @is_generic: for pin controllers that want to use the generic interface,
 *      this flag tells the framework that it's generic.
 * @pin_config_get: get the config of a certain pin, if the requested config
 *      is not available on this controller this should return -ENOTSUPP
 *      and if it is available but disabled it should return -EINVAL
 * @pin_config_set: configure an individual pin
 * @pin_config_group_get: get configurations for an entire pin group
 * @pin_config_group_set: configure all pins in a group
 * @pin_config_dbg_parse_modify: optional debugfs to modify a pin configuration
 * @pin_config_dbg_show: optional debugfs display hook that will provide
 *      per-device info for a certain pin in debugfs
 * @pin_config_group_dbg_show: optional debugfs display hook that will provide
 *      per-device info for a certain group in debugfs
 * @pin_config_config_dbg_show: optional debugfs display hook that will decode
 *      and display a driver's pin configuration parameter
 */
struct pinconf_ops {
	bool is_generic;
	int (*pin_config_get) (struct pinctrl_dev *pctldev, rt_uint32_t pin, rt_uint64_t *config);
	int (*pin_config_set) (struct pinctrl_dev *pctldev, rt_uint32_t pin, rt_uint64_t config);
	int (*pin_config_group_get) (struct pinctrl_dev *pctldev, rt_uint32_t selector, rt_uint64_t *config);
	int (*pin_config_group_set) (struct pinctrl_dev *pctldev, rt_uint32_t selector, rt_uint64_t config);
};

#endif /* __RT_PINCONF_H__ */
