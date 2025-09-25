/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include "ccu_pll.h"

#define PLL_MIN_FREQ 600000000
#define PLL_MAX_FREQ 3400000000
#define PLL_DELAYTIME 590 //(590*5)us

#define pll_readl(reg)		readl(reg)
#define pll_readl_pll_swcr1(p)	pll_readl(p.base + p.reg_ctrl)
#define pll_readl_pll_swcr2(p)	pll_readl(p.base + p.reg_sel)
#define pll_readl_pll_swcr3(p)	pll_readl(p.base + p.reg_xtc)

#define pll_writel(val, reg)		writel(val, reg)
#define pll_writel_pll_swcr1(val, p)	pll_writel(val, p.base + p.reg_ctrl)
#define pll_writel_pll_swcr2(val, p)	pll_writel(val, p.base + p.reg_sel)
#define pll_writel_pll_swcr3(val, p)	pll_writel(val, p.base + p.reg_xtc)

/* unified pllx_swcr1 for pll1~3 */
union pllx_swcr1 {
	struct {
		rt_uint32_t reg5:8;
		rt_uint32_t reg6:8;
		rt_uint32_t reg7:8;
		rt_uint32_t reg8:8;
	} b;
	rt_uint32_t v;
};

/* unified pllx_swcr2 for pll1~3 */
union pllx_swcr2 {
	struct {
		rt_uint32_t div1_en:1;
		rt_uint32_t div2_en:1;
		rt_uint32_t div3_en:1;
		rt_uint32_t div4_en:1;
		rt_uint32_t div5_en:1;
		rt_uint32_t div6_en:1;
		rt_uint32_t div7_en:1;
		rt_uint32_t div8_en:1;
		rt_uint32_t reserved1:4;
		rt_uint32_t atest_en:1;
		rt_uint32_t cktest_en:1;
		rt_uint32_t dtest_en:1;
		rt_uint32_t rdo:2;
		rt_uint32_t mon_cfg:4;
		rt_uint32_t reserved2:11;
	} b;
	rt_uint32_t v;
};

/* unified pllx_swcr3 for pll1~3 */
union pllx_swcr3{
	struct {
		rt_uint32_t div_frc:24;
		rt_uint32_t div_int:7;
		rt_uint32_t pll_en:1;
	} b;

	rt_uint32_t v;
};

static int ccu_pll_is_enabled(struct clk_hw *hw)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	union pllx_swcr3 swcr3;
	rt_uint32_t enabled;

	swcr3.v = pll_readl_pll_swcr3(p->common);
	enabled = swcr3.b.pll_en;

	return enabled;
}

/* frequency unit Mhz, return pll vco freq */
static rt_uint64_t __get_vco_freq(struct clk_hw *hw)
{
	rt_uint32_t reg5, reg6, reg7, reg8, size, i;
	rt_uint32_t div_int, div_frc;
	struct ccu_pll_rate_tbl *freq_pll_regs_table;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	union pllx_swcr1 swcr1;
	union pllx_swcr3 swcr3;

	swcr1.v = pll_readl_pll_swcr1(p->common);
	swcr3.v = pll_readl_pll_swcr3(p->common);

	reg5 = swcr1.b.reg5;
	reg6 = swcr1.b.reg6;
	reg7 = swcr1.b.reg7;
	reg8 = swcr1.b.reg8;

	div_int = swcr3.b.div_int;
	div_frc = swcr3.b.div_frc;

	freq_pll_regs_table = p->pll.rate_tbl;
	size = p->pll.tbl_size;

	for (i = 0; i < size; i++) {
		if ((freq_pll_regs_table[i].reg5 == reg5)
				&& (freq_pll_regs_table[i].reg6 == reg6)
				&& (freq_pll_regs_table[i].reg7 == reg7)
				&& (freq_pll_regs_table[i].reg8 == reg8)
				&& (freq_pll_regs_table[i].div_int == div_int)
				&& (freq_pll_regs_table[i].div_frac == div_frc))
			return freq_pll_regs_table[i].rate;
	}

	rt_kprintf("Unknown rate for clock %s\n", __clk_get_name(hw->clk));

	return 0;
}

static int ccu_pll_enable(struct clk_hw *hw)
{
	rt_uint32_t delaytime = PLL_DELAYTIME;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	union pllx_swcr3 swcr3;
	rt_tick_t tick_delay, now;
	rt_uint64_t flags;

	if (ccu_pll_is_enabled(hw))
		return 0;

	flags = rt_spin_lock_irqsave(&p->common.lock);

	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.pll_en = 1;
	pll_writel_pll_swcr3(swcr3.v, p->common);

	rt_spin_unlock_irqrestore(&p->common.lock, flags);

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

	if (!delaytime) {
		rt_kprintf("%s enabling didn't get stable within 3000us!!!\n", __clk_get_name(hw->clk));
	}

	return 0;
}

static void ccu_pll_disable(struct clk_hw *hw)
{
	rt_uint64_t flags;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	union pllx_swcr3 swcr3;

	flags = rt_spin_lock_irqsave(&p->common.lock);

	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.pll_en = 0;
	pll_writel_pll_swcr3(swcr3.v, p->common);

	rt_spin_unlock_irqrestore(&p->common.lock, flags);
}

/*
 * pll rate change requires sequence:
 * clock off -> change rate setting -> clock on
 * This function doesn't really change rate, but cache the config
 */
static int ccu_pll_set_rate(struct clk_hw *hw, rt_uint64_t rate,
			       rt_uint64_t parent_rate)
{
	rt_uint32_t i, reg5 = 0, reg6 = 0, reg7 = 0, reg8 = 0;
	rt_uint32_t div_int, div_frc;
	rt_uint64_t new_rate = rate, old_rate;
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	struct ccu_pll_config *params = &p->pll;
	union pllx_swcr1 swcr1;
	union pllx_swcr3 swcr3;
	bool found = false;
	bool pll_enabled = false;
	rt_uint64_t flags;

	if (ccu_pll_is_enabled(hw)) {
		pll_enabled = true;
		ccu_pll_disable(hw);
	}

	old_rate = __get_vco_freq(hw);
	/* setp 1: calculate fbd frcd kvco and band */
	if (params->rate_tbl) {
		for (i = 0; i < params->tbl_size; i++) {
			if (rate == params->rate_tbl[i].rate) {
				found = true;

				reg5 = params->rate_tbl[i].reg5;
				reg6 = params->rate_tbl[i].reg6;
				reg7 = params->rate_tbl[i].reg7;
				reg8 = params->rate_tbl[i].reg8;
				div_int = params->rate_tbl[i].div_int;
				div_frc = params->rate_tbl[i].div_frac;
				break;
			}
		}

		if (!found) {
			rt_kprintf("cant't find the target\n");
			while (1);
		}
	} else {
		rt_kprintf("don't find freq table for pll\n");
		if (pll_enabled)
			ccu_pll_enable(hw);
		return -RT_EINVAL;
	}

	flags = rt_spin_lock_irqsave(&p->common.lock);

	/* setp 2: set pll kvco/band and fbd/frcd setting */
	swcr1.v = pll_readl_pll_swcr1(p->common);
	swcr1.b.reg5 = reg5;
	swcr1.b.reg6 = reg6;
	swcr1.b.reg7 = reg7;
	swcr1.b.reg8 = reg8;
	pll_writel_pll_swcr1(swcr1.v, p->common);

	swcr3.v = pll_readl_pll_swcr3(p->common);
	swcr3.b.div_int = div_int;
	swcr3.b.div_frc = div_frc;
	pll_writel_pll_swcr3(swcr3.v, p->common);

	rt_spin_unlock_irqrestore(&p->common.lock, flags);

	if (pll_enabled)
		ccu_pll_enable(hw);

	rt_kprintf("%s %s rate %lu->%lu!\n", __func__,
		 __clk_get_name(hw->clk), old_rate, new_rate);
	return 0;
}

static rt_uint64_t ccu_pll_recalc_rate(struct clk_hw *hw,
					 rt_uint64_t parent_rate)
{
	return __get_vco_freq(hw);
}

static long ccu_pll_round_rate(struct clk_hw *hw, rt_uint64_t rate,
			       rt_uint64_t *prate)
{
	struct ccu_pll *p = hw_to_ccu_pll(hw);
	rt_uint64_t max_rate = 0;
	rt_uint32_t i;
	struct ccu_pll_config *params = &p->pll;

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

const struct clk_ops ccu_pll_ops = {
	.enable = ccu_pll_enable,
	.disable = ccu_pll_disable,
	.set_rate = ccu_pll_set_rate,
	.recalc_rate = ccu_pll_recalc_rate,
	.round_rate = ccu_pll_round_rate,
	.is_enabled = ccu_pll_is_enabled,
};

