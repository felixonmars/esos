/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <drivers/hwtimer.h>
#include <riscv-ops.h>
#include <dtb_head.h>

#define TMR_CCR         (0x000c)
#define TMR_TN_MM(n, m) (0x0010 + ((n) << 4) + ((m) << 2))
#define TMR_CR(n)       (0x0090 + ((n) << 2))
#define TMR_SR(n)       (0x0080 + ((n) << 2))
#define TMR_IER(n)      (0x0060 + ((n) << 2))
#define TMR_PLVR(n)     (0x0040 + ((n) << 2))
#define TMR_PLCR(n)     (0x0050 + ((n) << 2))
#define TMR_WMER        (0x0068)
#define TMR_WMR         (0x006c)
#define TMR_WVR         (0x00cc)
#define TMR_WSR         (0x00c0)
#define TMR_ICR(n)      (0x0070 + ((n) << 2))
#define TMR_WICR        (0x00c4)
#define TMR_CER         (0x0000)
#define TMR_CMR         (0x0004)
#define TMR_WCR         (0x00c8)
#define TMR_WFAR        (0x00b0)
#define TMR_WSAR        (0x00b4)
#define TMR_CRSR        (0x0008)

#define TMR_CCR_CS_0(x) (((x) & 0x3) << 0)
#define TMR_CCR_CS_1(x) (((x) & 0x3) << 2)
#define TMR_CCR_CS_2(x) (((x) & 0x3) << 5)


#define SPACEMIT_MAX_COUNTER	3
#define SPACEMIT_ALL_COUNTERS	((1 << SPACEMIT_MAX_COUNTER) - 1)
#define TMR_CER_COUNTER(cid)    (1 << (cid))

#define	SPACEMIT_TIMER_CLOCK_32KHZ	32768

static struct dtb_compatible_array __compatible_sub0[] = {
	{ .compatible = "timer0-counter0", },
	{ .compatible = "timer0-counter1", },
	{ .compatible = "timer0-counter2", },
	{},
};

static struct dtb_compatible_array __compatible_sub1[] = {
	{ .compatible = "timer1-counter0", },
	{ .compatible = "timer1-counter1", },
	{ .compatible = "timer1-counter2", },
	{},
};

static struct dtb_compatible_array __compatible_sub2[] = {
	{ .compatible = "timer2-counter0", },
	{ .compatible = "timer2-counter1", },
	{ .compatible = "timer2-counter2", },
	{},
};

static struct dtb_compatible_array __compatible_sub3[] = {
	{ .compatible = "timer3-counter0", },
	{ .compatible = "timer3-counter1", },
	{ .compatible = "timer3-counter2", },
	{},
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,rtimer0", .data = __compatible_sub0 },
	{ .compatible = "spacemit,rtimer1", .data = __compatible_sub1 },
	{ .compatible = "spacemit,rtimer2", .data = __compatible_sub2 },
	{ .compatible = "spacemit,rtimer3", .data = __compatible_sub3 },
	{}
};

struct spacemit_hwtimer;

struct spacemit_counter {
	rt_uint32_t irq, index;
	struct dtb_node *node;
	rt_hwtimer_t hwtimer;
	struct spacemit_hwtimer *ht;
};

struct spacemit_hwtimer {
	rt_base_t reg;
	rt_uint32_t index, numcounter, loop_delay;
	struct dtb_node *node;
	rt_uint32_t fclk, apbclk, timerclk;
	struct clk *clk, *rst;
	struct spacemit_counter *ct;
};

static int timer_counter_switch_clock(struct spacemit_hwtimer *tm, rt_uint32_t freq);
static void __hwtimer_counter_stop(struct spacemit_counter *hc);

static void timer_write_check(struct spacemit_hwtimer *tm, rt_uint32_t reg, rt_uint32_t val,
			      rt_uint32_t mask, bool clr, bool clk_switch)
{
	int loop = 3, retry = 100;
	rt_uint32_t t_read, t_check = clr ? !val : val;

reg_re_write:
	writel(val, (void *)(tm->reg + reg));

	if (clk_switch)
		timer_counter_switch_clock(tm, tm->fclk);

	t_read = readl((void *)(tm->reg + reg));

	while (((t_read & mask) != (t_check & mask)) && loop) {
		/* avoid trying frequently to worsen bus contention */
		rt_hw_us_delay(30);
		t_read = readl((void *)(tm->reg + reg));
		loop--;

		if (!loop) {
			loop = 3;
			if (--retry)
				goto reg_re_write;
			else {
				rt_kprintf("%s:%d, timeout\n", __func__, __LINE__);
				return;
			}
		}
	}
}

static int timer_counter_switch_clock(struct spacemit_hwtimer *tm, rt_uint32_t freq)
{
	rt_uint32_t ccr, val, mask;

	ccr = readl((void *)(tm->reg + TMR_CCR));

	switch (tm->index) {
	case 0:
		mask = TMR_CCR_CS_0(3);
		break;
	case 1:
		mask = TMR_CCR_CS_1(3);
		break;
	case 2:
		mask = TMR_CCR_CS_2(3);
		break;
	default:
		rt_kprintf("wrong timer: %d\n", tm->index);
		return -RT_EINVAL;
	}

	ccr &= ~mask;

	if (freq == tm->fclk)
		val = 0;
	else if (freq == SPACEMIT_TIMER_CLOCK_32KHZ)
		val = 1;
	else {
		rt_kprintf("Timer:%d: invalid clock rate %d\n", tm->index, freq);
		return -RT_EINVAL;
	}

	switch (tm->index) {
	case 0:
		ccr |= TMR_CCR_CS_0(val);
		break;
	case 1:
		ccr |= TMR_CCR_CS_1(val);
		break;
	case 2:
		ccr |= TMR_CCR_CS_2(val);
		break;
	}

	timer_write_check(tm, TMR_CCR, ccr, mask, false, false);

	return 0;
}
static int __hw_pre_init(struct spacemit_hwtimer *ht)
{
	int ret;
	rt_uint32_t delay, tmp;

	/* clk enable */
	ret = clk_prepare_enable(ht->clk);
	if (ret) {
		rt_kprintf("%s:%d, Enable clk error\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	/* clk set rate */
	clk_set_rate(ht->clk, ht->fclk);

	/* assert timer module */
	ret = clk_prepare_enable(ht->rst);
	if (ret) {
		rt_kprintf("%s:%d, Enable rst error\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	delay = ((ht->apbclk * 2) / ht->fclk / 8) + 1;

	timer_counter_switch_clock(ht, ht->fclk);

	/* disalbe all counters */
	tmp = readl((void *)(ht->reg + TMR_CER)) & ~SPACEMIT_ALL_COUNTERS;
	writel(tmp, (void *)(ht->reg + TMR_CER));

	/* disable matching interrupt */
	writel(0x00, (void *)(ht->reg + TMR_IER(0)));
	writel(0x00, (void *)(ht->reg + TMR_IER(1)));
	writel(0x00, (void *)(ht->reg + TMR_IER(2)));

	while (delay--) {
		/* Clear pending interrupt status */
		writel(0x1, (void *)(ht->reg + TMR_ICR(0)));
		writel(0x1, (void *)(ht->reg + TMR_ICR(1)));
		writel(0x1, (void *)(ht->reg + TMR_ICR(2)));
		writel(tmp, (void *)(ht->reg + TMR_CER));
	}

	ht->loop_delay = delay;

	return 0;
}

static void __hwtimer_counter_frequency_set(struct spacemit_counter *hc)
{
	rt_uint32_t tmp;
	struct spacemit_hwtimer *ht = hc->ht;

	timer_counter_switch_clock(ht, ht->fclk);

	tmp = readl((void *)(ht->reg + TMR_CMR)) | TMR_CER_COUNTER(hc->index);
	writel(tmp, (void *)(ht->reg + TMR_CMR));

	/* free-running */
	writel(0x0, (void *)(ht->reg + TMR_PLCR(hc->index)));
	/* clear status */
	writel(0x7, (void *)(ht->reg + TMR_ICR(hc->index)));

	return;
}

static void __hwtimer_counter_stop(struct spacemit_counter *hc)
{
	rt_uint32_t cer;
	struct spacemit_hwtimer *ht = hc->ht;

	cer = readl((void *)(ht->reg + TMR_CER));
	timer_write_check(ht, TMR_CER, (cer & ~(1 << hc->index)),
			  (1 << hc->index), false, false);

	return;
}

static void spacemit_hwtimer_init(struct rt_hwtimer_device *timer, rt_uint32_t state)
{
	/* TODO */
}

static rt_err_t spacemit_hwtimer_start(struct rt_hwtimer_device *timer, rt_uint32_t cnt, rt_hwtimer_mode_t mode)
{
	rt_uint32_t cer;
	struct spacemit_counter *hc = (struct spacemit_counter *)timer->parent.user_data;
	struct spacemit_hwtimer *ht = hc->ht;

	cer = readl((void *)(ht->reg + TMR_CER));

	/* disable counter fist */
	cer = readl((void *)(ht->reg + TMR_CER));
	if (cer & (1 << hc->index))
		__hwtimer_counter_stop(hc);

	/* Setup new counter value */
	timer_write_check(ht, TMR_TN_MM(hc->index, 0), (cnt - 1),
			  (rt_uint32_t)(-1), false, false);

	/* enable the matching interrupt */
	timer_write_check(ht, TMR_IER(hc->index), 0x1, 0x1, false, false);

	/* enable counter */
	cer = readl((void *)(ht->reg + TMR_CER));
	timer_write_check(ht, TMR_CER, (cer | (1 << hc->index)),
			 (1 << hc->index), false, false);

	return 0;
}

static void spacemit_hwtimer_stop(struct rt_hwtimer_device *timer)
{
	struct spacemit_counter *hc = (struct spacemit_counter *)timer->parent.user_data;
	struct spacemit_hwtimer *ht = hc->ht;

	__hwtimer_counter_stop(hc);
}

static rt_uint32_t spacemit_hwtimer_count_get(struct rt_hwtimer_device *timer)
{
	struct spacemit_counter *hc = (struct spacemit_counter *)timer->parent.user_data;
	struct spacemit_hwtimer *ht = hc->ht;

	return readl((void *)(ht->reg + TMR_CR(hc->index)));
}

static rt_err_t spacemit_hwtimer_control(struct rt_hwtimer_device *timer, rt_uint32_t cmd, void *args)
{

	struct spacemit_counter *hc = (struct spacemit_counter *)timer->parent.user_data;
	struct spacemit_hwtimer *ht = hc->ht;

	switch (cmd) {
	case HWTIMER_CTRL_FREQ_SET:
		__hwtimer_counter_frequency_set(hc);
	break;
	case HWTIMER_CTRL_STOP:
		__hwtimer_counter_stop(hc);
	break;
	case HWTIMER_CTRL_INFO_GET:
	break;
	case HWTIMER_CTRL_MODE_SET:
	break;
	default:
	break;
	}

	return 0;
}

static struct rt_hwtimer_ops hw_ops = {
	.init = spacemit_hwtimer_init,
	.start = spacemit_hwtimer_start,
	.stop = spacemit_hwtimer_stop,
	.count_get = spacemit_hwtimer_count_get,
	.control = spacemit_hwtimer_control,
};

static struct rt_hwtimer_info hw_info = {
	.maxfreq = 1000000,
	.minfreq = 1000000,
	.maxcnt = 0xffffffff,
	.cntmode = HWTIMER_CNTMODE_UP,
};

static void spacemit_hwtimer_irq(rt_int32_t irq, void *dev_id)
{
	struct spacemit_counter *hc = (struct spacemit_counter *)dev_id;
	struct spacemit_hwtimer *ht = hc->ht;

	/* clear the pending */
	if (readl((void *)(ht->reg + TMR_SR(hc->index))) & 0x1) {
		/* disable counter */
		__hwtimer_counter_stop(hc);

		/* disable interrupt */
		timer_write_check(ht, TMR_IER(hc->index), 0, 0x7, false, false);

		/* Clear interrupt status */
		timer_write_check(ht, TMR_ICR(hc->index), 0x1, 0x7, true, false);

		/* call handler */
		rt_device_hwtimer_isr(&hc->hwtimer);
	}
}

rt_int32_t rt_hwtimer_init(void)
{
	int ret, i, j;
	char str[32];
	struct spacemit_hwtimer *ht;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);

		if (compatible_node != RT_NULL) {
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			ht = (struct spacemit_hwtimer *)rt_calloc(1, sizeof(struct spacemit_hwtimer));
			if (ht == RT_NULL) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			ht->reg = dtb_node_get_addr_index(compatible_node, 0);
			if (ht->reg < 0) {
				rt_kprintf("%s:%d, Get register failed\n", __func__, __LINE__);
				return -RT_ERROR;
			}

			ht->clk = of_clk_get(compatible_node, 0);
			if (IS_ERR(ht->clk)) {
				rt_kprintf("%s:%d, Get clk error\n", __func__, __LINE__);
				return -RT_ERROR;
			}

			ht->rst= of_clk_get(compatible_node, 1);
			if (IS_ERR(ht->rst)) {
				rt_kprintf("%s:%d, Get rst error\n", __func__, __LINE__);
				return -RT_ERROR;
			}

			ht->node = compatible_node;

			dtb_node_read_u32_array(ht->node, "spacemit,timer-fastclk-frequency", &ht->fclk, 1);
			dtb_node_read_u32_array(ht->node, "spacemit,timer-apb-frequency", &ht->apbclk, 1);
			dtb_node_read_u32_array(ht->node, "spacemit,timer-frequency", &ht->timerclk, 1);
			dtb_node_read_u32_array(ht->node, "num_counter", &ht->numcounter, 1);

			ht->index = i;

			ret = __hw_pre_init(ht);
			if (ret < 0) {
				rt_kprintf("%s:%d, hw pre initialize error\n", __func__, __LINE__, ht->index);
				return -RT_ERROR;
			}

			/* initialize counter */
			j = 0;
			struct dtb_compatible_array *sub_node = (struct dtb_compatible_array *)__compatible[i].data;

			while (sub_node[j].compatible) {
				compatible_node = dtb_node_find_compatible_node(dtb_head_node, sub_node[j].compatible);

				/* check the status */
				if (!dtb_node_device_is_available(compatible_node))
					break;
				++j;
			}

			if (ht->numcounter != j) {
				rt_kprintf("%s:%d, wrong timer counter\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			ht->ct = rt_calloc(ht->numcounter, sizeof(struct spacemit_counter));
			if (ht->ct == RT_NULL) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			for (j = 0; j < ht->numcounter; ++j) {
				compatible_node = dtb_node_find_compatible_node(dtb_head_node, sub_node[j].compatible);

				ht->ct[j].node = compatible_node;
				ht->ct[j].ht = ht;
				ht->ct[j].index = j;

				ht->ct[j].irq = dtb_node_irq_get(compatible_node, 0);

				rt_snprintf(str, 32, "timer:%d:%d", i, j);
				ht->ct[j].hwtimer.freq = ht->timerclk;
				ht->ct[j].hwtimer.info = &hw_info;
				ht->ct[j].hwtimer.ops = &hw_ops;
				rt_device_hwtimer_register(&ht->ct[j].hwtimer, str, &ht->ct[j]);

				rt_hw_interrupt_install(ht->ct[j].irq, spacemit_hwtimer_irq, (void *)&ht->ct[j], str);
				rt_hw_interrupt_umask(ht->ct[j].irq);
			}
		}
	}
}
INIT_DEVICE_EXPORT(rt_hwtimer_init);
