/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include "ccu_plla.h"

#define PLL_MIN_FREQ		600000000
#define PLL_MAX_FREQ		3400000000
#define PLL_DELAYTIME		590 //(590*5)us

#define PLLA_SWCR2_MASK		GENMASK(15, 8)

#define plla_readl(reg)			readl(reg)
#define plla_readl_pll_swcr1(p)		plla_readl(p.base + p.reg_ctrl)
#define plla_readl_pll_swcr2(p)		plla_readl(p.base + p.reg_sel)
#define plla_readl_pll_swcr3(p)		plla_readl(p.base + p.reg_xtc)

#define plla_writel(val, reg)		writel(val, reg)
#define plla_writel_pll_swcr1(val, p)	plla_writel(val, p.base + p.reg_ctrl)
#define plla_writel_pll_swcr2(val, p)	plla_writel(val, p.base + p.reg_sel)
#define plla_writel_pll_swcr3(val, p)	plla_writel(val, p.base + p.reg_xtc)

/* unified pllax_swcr1 for plla */
union pllax_swcr1 {
	struct {
		rt_uint32_t reg1:8;
		rt_uint32_t reg2:8;
		rt_uint32_t reg3:8;
		rt_uint32_t reg4:8;
	} b;
	rt_uint32_t v;
};

/* unified pllax_swcr2 for plla */
union pllax_swcr2 {
	struct {
		rt_uint32_t div1_en:1;
		rt_uint32_t div2_en:1;
		rt_uint32_t div3_en:1;
		rt_uint32_t div4_en:1;
		rt_uint32_t div5_en:1;
		rt_uint32_t div6_en:1;
		rt_uint32_t div7_en:1;
		rt_uint32_t div8_en:1;
		rt_uint32_t reg0:8;
		rt_uint32_t pll_en:1;
		rt_uint32_t mon_cfg:4;
		rt_uint32_t div10_en:1;
		rt_uint32_t mmd_en:1;
		rt_uint32_t mmd:5;
		rt_uint32_t reserved2:3;
		rt_uint32_t div64_en:1;
	} b;
	rt_uint32_t v;
};

/* unified pllax_swcr3 for plla */
union pllax_swcr3 {
	struct {
		rt_uint32_t reg5:8;
		rt_uint32_t reg6:8;
		rt_uint32_t reg7:8;
		rt_uint32_t reg8:8;
	} b;

	rt_uint32_t v;
};

static int ccu_plla_is_enabled(struct clk_hw *hw)
{
	struct ccu_plla *p = hw_to_ccu_plla(hw);
	union pllax_swcr2 swcr2;
	rt_uint32_t enabled;

	swcr2.v = plla_readl_pll_swcr2(p->common);
	enabled = swcr2.b.pll_en;

	return enabled;
}

/* frequency unit Mhz, return plla vco freq */
static rt_uint64_t __get_vco_freq(struct clk_hw *hw)
{
	rt_uint32_t size, i;
	struct ccu_plla_rate_tbl *freq_pll_regs_table;
	struct ccu_plla *p = hw_to_ccu_plla(hw);
	union pllax_swcr1 swcr1;
	union pllax_swcr2 swcr2;
	union pllax_swcr3 swcr3;

	swcr1.v = plla_readl_pll_swcr1(p->common);
	swcr2.v = plla_readl_pll_swcr2(p->common);
	swcr2.v &= PLLA_SWCR2_MASK;
	swcr3.v = plla_readl_pll_swcr3(p->common);

	freq_pll_regs_table = p->pll.rate_tbl;
	size = p->pll.tbl_size;

	for (i = 0; i < size; i++) {
		if ((freq_pll_regs_table[i].swcr1 == swcr1.v)
				&& (freq_pll_regs_table[i].swcr2 == swcr2.v)
				&& (freq_pll_regs_table[i].swcr3 == swcr3.v))
			return freq_pll_regs_table[i].rate;
	}

	rt_kprintf("Unknown rate for clock %s\n", __clk_get_name(hw->clk));

	return 0;
}

static int ccu_plla_enable(struct clk_hw *hw)
{
	rt_uint32_t delaytime = PLL_DELAYTIME;
	struct ccu_plla *p = hw_to_ccu_plla(hw);
	union pllax_swcr2 swcr2;
	rt_tick_t tick_delay, now;
	rt_uint64_t flags;

	if (ccu_plla_is_enabled(hw))
		return 0;

	flags = rt_spin_lock_irqsave(&p->common.lock);

	swcr2.v = plla_readl_pll_swcr2(p->common);
	swcr2.b.pll_en = 1;
	plla_writel_pll_swcr2(swcr2.v, p->common);

	rt_spin_unlock_irqrestore(&p->common.lock, flags);
#if 0
	/* check lock status */
	/* udelay(50); */
	tick_delay = rt_tick_from_millisecond(10);
	now = rt_tick_get_millisecond();
	tick_delay += now;

	while (now < tick_delay)
		now = rt_tick_get_millisecond();

	while ((!(readl(p->pll.lock_base + p->pll.reg_lock) & p->pll.lock_enable_bit))
		&& delaytime) {
		/* udelay(5); */
		tick_delay = rt_tick_from_millisecond(10);
		now = rt_tick_get_millisecond();
		tick_delay += now;

		while (now < tick_delay)
			now = rt_tick_get_millisecond();

		delaytime--;
	}

	if (!delaytime)
		rt_kprintf("%s enabling didn't get stable within 3000us!!!\n", __clk_get_name(hw->clk));

#endif
	return 0;
}

static void ccu_plla_disable(struct clk_hw *hw)
{
	rt_uint64_t flags;
	struct ccu_plla *p = hw_to_ccu_plla(hw);
	union pllax_swcr2 swcr2;

	flags = rt_spin_lock_irqsave(&p->common.lock);

	swcr2.v = plla_readl_pll_swcr2(p->common);
	swcr2.b.pll_en = 0;
	plla_writel_pll_swcr2(swcr2.v, p->common);

	rt_spin_unlock_irqrestore(&p->common.lock, flags);
}

/*
 * pll rate change requires sequence:
 * clock off -> change rate setting -> clock on
 * This function doesn't really change rate, but cache the config
 */
static int ccu_plla_set_rate(struct clk_hw *hw, rt_uint64_t rate,
			     rt_uint64_t parent_rate)
{
	rt_uint32_t i, reg0 = 0;
	rt_uint64_t new_rate = rate, old_rate;
	struct ccu_plla *p = hw_to_ccu_plla(hw);
	struct ccu_plla_config *params = &p->pll;
	union pllax_swcr1 swcr1;
	union pllax_swcr2 swcr2;
	union pllax_swcr3 swcr3;
	bool found = false;
	bool pll_enabled = false;
	rt_uint64_t flags;

	if (ccu_plla_is_enabled(hw)) {
		pll_enabled = true;
		ccu_plla_disable(hw);
	}

	old_rate = __get_vco_freq(hw);
	/* setp 1: calculate fbd frcd kvco and band */
	if (params->rate_tbl) {
		for (i = 0; i < params->tbl_size; i++) {
			if (rate == params->rate_tbl[i].rate) {
				found = true;

				swcr1.v = params->rate_tbl[i].swcr1;
				reg0 = params->rate_tbl[i].swcr2;
				swcr3.v = params->rate_tbl[i].swcr3;
				break;
			}
		}

		if (!found) {
			rt_kprintf("cant't find the target\n");
			while (1)
				;
		}
	} else {
		rt_kprintf("don't find freq table for pll\n");
		if (pll_enabled)
			ccu_plla_enable(hw);
		return -RT_EINVAL;
	}

	flags = rt_spin_lock_irqsave(&p->common.lock);

	/* setp 2: set pll kvco/band and fbd/frcd setting */
	plla_writel_pll_swcr1(swcr1.v, p->common);

	swcr2.v = plla_readl_pll_swcr2(p->common);
	swcr2.v &= ~PLLA_SWCR2_MASK;
	swcr2.v |= reg0;
	plla_writel_pll_swcr2(swcr2.v, p->common);

	plla_writel_pll_swcr3(swcr3.v, p->common);

	rt_spin_unlock_irqrestore(&p->common.lock, flags);

	if (pll_enabled)
		ccu_plla_enable(hw);

	rt_kprintf("%s %s rate %lu->%lu!\n", __func__,
		 __clk_get_name(hw->clk), old_rate, new_rate);
	return 0;
}

static rt_uint64_t ccu_plla_recalc_rate(struct clk_hw *hw,
				        rt_uint64_t parent_rate)
{
	return __get_vco_freq(hw);
}

static long ccu_plla_round_rate(struct clk_hw *hw, rt_uint64_t rate,
				rt_uint64_t *prate)
{
	struct ccu_plla *p = hw_to_ccu_plla(hw);
	rt_uint64_t max_rate = 0;
	rt_uint32_t i;
	struct ccu_plla_config *params = &p->pll;

	if (rate > PLL_MAX_FREQ || rate < PLL_MIN_FREQ) {
		rt_kprintf("%lu rate out of range!\n", rate);
		return -RT_EINVAL;
	}

	if (params->rate_tbl) {
		for (i = 0; i < params->tbl_size; i++) {
			if (params->rate_tbl[i].rate <= rate) {
				if (max_rate < params->rate_tbl[i].rate)
					max_rate = params->rate_tbl[i].rate;
			}
		}
	} else {
		rt_kprintf("don't find freq table for pll\n");
	}

	return max_rate;
}

const struct clk_ops ccu_plla_ops = {
	.enable		= ccu_plla_enable,
	.disable	= ccu_plla_disable,
	.set_rate	= ccu_plla_set_rate,
	.recalc_rate	= ccu_plla_recalc_rate,
	.round_rate	= ccu_plla_round_rate,
	.is_enabled	= ccu_plla_is_enabled,
};

