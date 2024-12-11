#ifndef __RT_CLKDEV_H__
#define __RT_CLKDEV_H__

#include <rtdef.h>

struct clk;
struct clk_hw;

struct clk_lookup {
	rt_list_t        node;
	const char              *dev_id;
	const char              *con_id;
	struct clk              *clk;
	struct clk_hw           *clk_hw;
};

int clk_hw_register_clkdev(struct clk_hw *hw, const char *con_id,
        const char *dev_id);
struct clk_hw *clk_find_hw(const char *dev_id, const char *con_id);

#endif /* */
