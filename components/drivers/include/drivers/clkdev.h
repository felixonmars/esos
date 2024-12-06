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


#endif /* */
