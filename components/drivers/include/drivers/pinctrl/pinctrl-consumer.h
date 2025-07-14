#ifndef __RT_PINCTRL_CONSUMER_H__
#define __RT_PINCTRL_CONSUMER_H__

#include <rtthread.h>
#include <rtdef.h>
#include "../../../pinctrl/pinctrl-core.h"

struct pinctrl_state *pinctrl_lookup_state(struct pinctrl *p, const char *name);
int pinctrl_select_state(struct pinctrl *p, struct pinctrl_state *state);
extern int pinctrl_request_gpio(rt_uint32_t gpio);
extern void pinctrl_free_gpio(rt_uint32_t gpio);

#endif /* __RT_PINCTRL_CONSUMER_H__ */
