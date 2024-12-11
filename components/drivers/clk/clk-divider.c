#include <rtthread.h>
#include <rtdevice.h>

static unsigned int _get_table_div(const struct clk_div_table *table,
                                                        unsigned int val)
{
        const struct clk_div_table *clkt;

        for (clkt = table; clkt->div; clkt++)
                if (clkt->val == val)
                        return clkt->div;
        return 0;
}

static unsigned int _get_div(const struct clk_div_table *table,
                             unsigned int val, unsigned long flags, unsigned char width)
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

unsigned long divider_recalc_rate(struct clk_hw *hw, unsigned long parent_rate,
                                  unsigned int val,
                                  const struct clk_div_table *table,
                                  unsigned long flags, unsigned long width)
{
	unsigned int div;

	div = _get_div(table, val, flags, width);
	if (!div) {
		if (!(flags & CLK_DIVIDER_ALLOW_ZERO)) {
			rt_kprintf("%s: Zero divisor and CLK_DIVIDER_ALLOW_ZERO not set\n",
					clk_hw_get_name(hw));
		}

		return parent_rate;
	}

	return DIV_ROUND_UP_ULL((unsigned long)parent_rate, div);
}

