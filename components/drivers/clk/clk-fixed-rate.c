#include <rtdevice.h>
#include <rtthread.h>
#include <dtb_node.h>

#define to_clk_fixed_rate(_hw) rt_container_of(_hw, struct clk_fixed_rate, hw)

static rt_uint64_t clk_fixed_rate_recalc_rate(struct clk_hw *hw,
               rt_uint64_t parent_rate)
{
	return to_clk_fixed_rate(hw)->fixed_rate;
}

#if 0
staticrt_uint64_t clk_fixed_rate_recalc_accuracy(struct clk_hw *hw,
               rt_uint64_t parent_accuracy)
{
	struct clk_fixed_rate *fixed = to_clk_fixed_rate(hw);

 	if (fixed->flags & CLK_FIXED_RATE_PARENT_ACCURACY)
		return parent_accuracy;

	return fixed->fixed_accuracy;
}
#endif

const struct clk_ops clk_fixed_rate_ops = {
	.recalc_rate = clk_fixed_rate_recalc_rate,
	/* .recalc_accuracy = clk_fixed_rate_recalc_accuracy, */
};

struct clk_hw *__clk_hw_register_fixed_rate(/*struct device *dev, */
                struct dtb_node *np, const char *name,
                const char *parent_name, const struct clk_hw *parent_hw,
                const struct clk_parent_data *parent_data,rt_uint64_t flags,
               rt_uint64_t fixed_rate/*,rt_uint64_t fixed_accuracy */,
               rt_uint64_t clk_fixed_flags/*, bool devm */)
{
	struct clk_fixed_rate *fixed;
	struct clk_hw *hw;
	struct clk_init_data init = {};
	struct clk *ret;

	/* allocate fixed-rate clock */
	fixed = rt_calloc(1, sizeof(*fixed));
	if (!fixed)
		return ERR_PTR(-RT_ENOMEM);

	init.name = name;
	init.ops = &clk_fixed_rate_ops;
	init.flags = flags;
	init.parent_names = parent_name ? &parent_name : RT_NULL;
	init.parent_hws = parent_hw ? &parent_hw : RT_NULL;
	init.parent_data = parent_data;
	if (parent_name || parent_hw || parent_data)
		init.num_parents = 1;
	else
		init.num_parents = 0;

	/* struct clk_fixed_rate assignments */
        fixed->flags = clk_fixed_flags;
	fixed->fixed_rate = fixed_rate;
	/* fixed->fixed_accuracy = fixed_accuracy; */
	fixed->hw.init = &init;

	/* register the clock */
	hw = &fixed->hw;
	ret = of_clk_hw_register(np, hw);
	if (IS_ERR(ret)) {
		rt_free(fixed);
		hw = ERR_PTR(ret);
	}

	return hw;
}

