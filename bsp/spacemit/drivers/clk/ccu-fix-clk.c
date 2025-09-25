/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>

static struct clk_hw *of_clk_hw_simple_get(struct fdt_phandle_args *clkspec, void *data)
{
	return data;
}

static int _of_fixed_clk_setup(void)
{
	struct clk_hw *hw;
	const char *clk_name;
	uint32_t rate;
	struct dtb_node *dn;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	int ret;

	for_each_of_allnodes_from(dtb_head_node, dn) {
		if (dtb_node_get_dtb_node_compatible_match(dn, "fixed-clock") &&
				dtb_node_get(dn)) {

			/* find the node */
			if (dtb_node_read_u32(dn, "clock-frequency", &rate)) {
				rt_kprintf("=>read clock-frequency failed\n");
				return -RT_EINVAL;
			}

			clk_name = dtb_node_read_string(dn, "clock-output-names");
			if (clk_name == RT_NULL) {
				rt_kprintf("=>read clock-output-names failed\n");
				return -RT_EINVAL;
			}

			hw = __clk_hw_register_fixed_rate(dn, clk_name, NULL,
					NULL, NULL, 0, rate, 0);
			if (IS_ERR(hw)) {	
				rt_kprintf("=> register fixed rate clock failed\n");
				return -RT_EINVAL;
			}

			clk_hw_register_clkdev(hw, clk_hw_get_name(hw), RT_NULL);

			ret = of_clk_add_hw_provider(dn, of_clk_hw_simple_get, hw);
			if (ret) {
				return ret;
			}

			dtb_node_put(dn);
		}
	}

	return 0;
}

/**
 * of_fixed_clk_setup() - Setup function for simple fixed rate clock
 * @node:       device node for the clock
 */
int of_fixed_clk_setup(void)
{
	return _of_fixed_clk_setup();
}
// INIT_PREV_EXPORT(of_fixed_clk_setup);
