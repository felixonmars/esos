#ifndef __RT_PINCTRL_CONSUMER_H__
#define __RT_PINCTRL_CONSUMER_H__

#include <rtthread.h>
#include <rtdef.h>
#include "../../../pinctrl/pinctrl-core.h"

struct pinctrl_state *pinctrl_lookup_state(struct pinctrl *p, const char *name);
int pinctrl_select_state(struct pinctrl *p, struct pinctrl_state *state);

#endif /* __RT_PINCTRL_CONSUMER_H__ */
