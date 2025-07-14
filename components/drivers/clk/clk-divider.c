#include <rtthread.h>
#include <rtdevice.h>

static rt_uint32_t _get_table_div(const struct clk_div_table *table,
                                                        rt_uint32_t val)
{
        const struct clk_div_table *clkt;

        for (clkt = table; clkt->div; clkt++)
                if (clkt->val == val)
                        return clkt->div;
        return 0;
}

static rt_uint32_t _get_div(const struct clk_div_table *table,
                             rt_uint32_t val, rt_uint64_t flags, unsigned char width)
{
	if (flags & CLK_DIVIDER_ONE_BASED)
		return val;
	if (flags & CLK_DIVIDER_POWER_OF_TWO)
		return 1 << val;
	if (flags & CLK_DIVIDER_MAX_AT_ZERO)
		return val ? val : clk_div_mask(width) + 1;
	if (table)
		return _get_table_div(table, val);

	return val + 1;
}

rt_uint64_t divider_recalc_rate(struct clk_hw *hw, rt_uint64_t parent_rate,
                                  rt_uint32_t val,
                                  const struct clk_div_table *table,
                                  rt_uint64_t flags, rt_uint64_t width)
{
	rt_uint32_t div;

	div = _get_div(table, val, flags, width);
	if (!div) {
		if (!(flags & CLK_DIVIDER_ALLOW_ZERO)) {
			rt_kprintf("%s: Zero divisor and CLK_DIVIDER_ALLOW_ZERO not set\n",
					clk_hw_get_name(hw));
		}

		return parent_rate;
	}

	return DIV_ROUND_UP_ULL((rt_uint64_t)parent_rate, div);
}

