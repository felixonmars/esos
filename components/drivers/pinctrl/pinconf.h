#ifndef __RT_DRV_PINCONF_H__
#define __RT_DRV_PINCONF_H__

int pinconf_check_ops(struct pinctrl_dev *pctldev);
int pinconf_validate_map(struct pinctrl_map const *map, int i);
int pinconf_map_to_setting(struct pinctrl_map const *map,
		struct pinctrl_setting *setting);
void pinconf_free_setting(struct pinctrl_setting const *setting);
int pinconf_apply_setting(struct pinctrl_setting const *setting);

#endif /* __RT_DRV_PINCONF_H__ */
