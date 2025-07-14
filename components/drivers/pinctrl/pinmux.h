#ifndef __RT_DRV_PINMUX_H__
#define __RT_DRV_PINMUX_H__

int pinmux_check_ops(struct pinctrl_dev *pctldev);
int pinmux_validate_map(struct pinctrl_map const *map, int i);
int pinmux_map_to_setting(struct pinctrl_map const *map,
                          struct pinctrl_setting *setting);
void pinmux_disable_setting(struct pinctrl_setting const *setting);
void pinmux_free_setting(struct pinctrl_setting const *setting);
int pinmux_enable_setting(struct pinctrl_setting const *setting);

int pinmux_request_gpio(struct pinctrl_dev *pctldev, struct pinctrl_gpio_range *range, rt_uint32_t pin, rt_uint32_t gpio);
void pinmux_free_gpio(struct pinctrl_dev *pctldev, rt_uint32_t pin, struct pinctrl_gpio_range *range);

#endif /* __RT_DRV_PINMUX_H__ */
