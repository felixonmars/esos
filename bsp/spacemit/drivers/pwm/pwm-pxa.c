/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/rt_drv_pwm.h>

/* PWM registers and bits definitions */
#define PWMCR		(0x00)
#define PWMDCR		(0x04)
#define PWMPCR		(0x08)

#define PWMCR_SD	(1 << 6)
#define PWMDCR_FD	(1 << 10)

struct pxa_pwm_configuration
{
	rt_uint32_t period;
	rt_uint32_t pulse;
	bool enabled;
};

struct pxa_pwm_chip {
	struct rt_device_pwm dev;
	char * name;
	struct clk	*clk;
	struct clk	*rst;
	void *mmio_base;
	struct pxa_pwm_configuration config;
#ifdef SOC_SPACEMIT_K1_X
	int dcr_fd; /* Controller PWM_DCR FD feature */
#endif
};

static inline struct pxa_pwm_chip *to_pxa_pwm_chip(struct rt_device_pwm *dev)
{
	return rt_container_of(dev, struct pxa_pwm_chip, dev);
}

/*
 * period_ns = 10^9 * (PRESCALE + 1) * (PV + 1) / PWM_CLK_RATE
 * duty_ns   = 10^9 * (PRESCALE + 1) * DC / PWM_CLK_RATE
 */
int pxa_pwm_config(struct rt_device_pwm *dev,
				rt_uint64_t duty_ns, rt_uint64_t period_ns)
{
	struct pxa_pwm_chip *pc = to_pxa_pwm_chip(dev);
	rt_uint64_t c = 0;
	rt_uint32_t period_cycles, prescale, pv, dc;
	rt_uint32_t offset = 0;

	c = clk_get_rate(pc->clk);
	c = c * period_ns;
	do_div(c, 1000000000);
	period_cycles = c;

	if (period_cycles < 1)
		period_cycles = 1;
	prescale = (period_cycles - 1) / 1024;
	pv = period_cycles / (prescale + 1) - 1;

	if (prescale > 63)
		return -RT_EINVAL;

	if (duty_ns == period_ns)
#ifdef SOC_SPACEMIT_K1_X
	{
		if (pc->dcr_fd)
			dc = PWMDCR_FD;
		else {
			dc = (pv + 1) * duty_ns / period_ns;
			if (dc >= PWMDCR_FD) {
				dc = PWMDCR_FD - 1;
				pv = dc - 1;
			}
		}
	}
#else
		dc = PWMDCR_FD;
#endif
	else
		dc = ((pv + 1) * duty_ns / period_ns);

	writel(prescale | PWMCR_SD, pc->mmio_base + offset + PWMCR);
	writel(dc, pc->mmio_base + offset + PWMDCR);
	writel(pv, pc->mmio_base + offset + PWMPCR);

	return 0;
}

rt_err_t pxa_pwm_control(struct rt_device_pwm *dev, int cmd, void *arg)
{
	struct rt_pwm_configuration * config = (struct rt_pwm_configuration *)arg;
	struct pxa_pwm_chip *pc = to_pxa_pwm_chip(dev);
	int err = 0;

	switch (cmd)
	{
		case PWM_CMD_ENABLE:
			if (pc->config.enabled)
				break;
			err = clk_prepare_enable(pc->clk);
			if (err)
				return err;
			pc->config.enabled = true;
			break;
		case PWM_CMD_DISABLE:
			clk_disable_unprepare(pc->clk);
			pc->config.enabled = false;
			break;
		case PWM_CMD_SET:
			err = pxa_pwm_config(dev, config->pulse, config->period);
			if (err)
				return err;
			pc->config.period = config->period;
			pc->config.pulse = config->pulse;
			break;
		case PWM_CMD_GET:
		case PWMN_CMD_ENABLE:
		case PWMN_CMD_DISABLE:
		default:
			break;
	}
	return 0;
}

static const struct rt_pwm_ops pxa_pwm_ops = {
	.control = pxa_pwm_control,
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k1x-rpwm0", .data = "rpwm0" },
	{ .compatible = "spacemit,k1x-rpwm1", .data = "rpwm1" },
	{ .compatible = "spacemit,k1x-rpwm2", .data = "rpwm2" },
	{ .compatible = "spacemit,k1x-rpwm3", .data = "rpwm3" },
	{ .compatible = "spacemit,k1x-rpwm4", .data = "rpwm4" },
	{ .compatible = "spacemit,k1x-rpwm5", .data = "rpwm5" },
	{ .compatible = "spacemit,k1x-rpwm6", .data = "rpwm6" },
	{ .compatible = "spacemit,k1x-rpwm7", .data = "rpwm7" },
	{ .compatible = "spacemit,k1x-rpwm8", .data = "rpwm8" },
	{ .compatible = "spacemit,k1x-rpwm9", .data = "rpwm9" },
	{},
};

int pwm_probe(void)
{
	int i;
	struct pxa_pwm_chip* pc;
	struct rt_device_pwm * dev;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
#ifdef SOC_SPACEMIT_K1_X
	void *property_status = RT_NULL;
	int property_size;
	rt_uint32_t rate;
	rt_uint32_t *list;
#endif

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;
			pc = rt_calloc(1, sizeof(struct pxa_pwm_chip));
			if (!pc) {
				rt_kprintf("%s:%d, calloc failed\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}
			dev = &pc->dev;
			pc->name = (char *)__compatible[i].data;
			pc->mmio_base = (void *)dtb_node_get_addr_index(compatible_node, 0);
			if (pc->mmio_base < 0) {
				rt_kprintf("get pwm mmio_base failed\n");
				return -RT_ERROR;
			}
			pc->clk = of_clk_get(compatible_node, 0);
			if (IS_ERR(pc->clk)) {
				rt_kprintf("get pwm clk failed\n");
				return -RT_ERROR;
			}
			pc->rst = of_clk_get(compatible_node, 1);
			if (IS_ERR(pc->rst)) {
				rt_kprintf("get pwm rst failed\n");
				return -RT_ERROR;
			}
			clk_prepare_enable(pc->rst);
#ifdef SOC_SPACEMIT_K1_X
			property_status = dtb_node_get_dtb_node_property(compatible_node, "k1x,pwm-disable-fd", RT_NULL);
			if (property_status)
				pc->dcr_fd = 1;
			else
				pc->dcr_fd = 0;

			rate = 0;
			property_size = 0;
			list = NULL;
			for_each_property_cell(compatible_node, "clock-rate", rate, list, property_size)
			{
				clk_set_rate(pc->clk, rate);
			}
#endif
			rt_device_pwm_register(dev, pc->name, &pxa_pwm_ops, NULL);
		}
	}
	return 0;
}
INIT_DEVICE_EXPORT(pwm_probe);
