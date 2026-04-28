/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>

#ifdef RT_USING_FINSH

struct clk *dfs_clk;

static rt_uint64_t cpu_dfs_table[] = {
	245760000,
	307200000,
	491520000,
	614400000,
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,rt24", },
	{},
};

static int rt_cpu_dfs(int argc, char **argv)
{
	int i, loops, j, ret;
	rt_uint64_t origin_clk, temp_clk;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	if (argc != 2) {
		rt_kprintf("Usage: rt_cpu_dfs <loops>\n");
		return -RT_ERROR;
	}

	loops = atoi(argv[1]);

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			dfs_clk = of_clk_get(compatible_node, 0);
			if (IS_ERR(dfs_clk)) {
				rt_kprintf("Get cpu clk failed\n");
				return -RT_ERROR;
			}

			clk_prepare_enable(dfs_clk);

			/* get the origin clk of cpu */
			origin_clk = clk_get_rate(dfs_clk);

			while (loops-- >= 0) {
				for (j = 0; j < sizeof(cpu_dfs_table) / sizeof(cpu_dfs_table[0]); ++j) {
					rt_kprintf("Start setting CLK:%lld\n", cpu_dfs_table[j]);
					ret = clk_set_rate(dfs_clk, cpu_dfs_table[j]);
					if (ret < 0) {
						rt_kprintf("%s:%d, set Clk rate:%lld falied\n", __func__, __LINE__, cpu_dfs_table[j]);
						clk_set_rate(dfs_clk, origin_clk);
						return -RT_ERROR;
					}

					temp_clk = clk_get_rate(dfs_clk);
					if (temp_clk != cpu_dfs_table[j]) {
						rt_kprintf("%s:%d, set Clk rate:%lld falied\n", __func__, __LINE__, cpu_dfs_table[j]);
						rt_kprintf("Expected: %lld, Actual: %lld\n", cpu_dfs_table[j], temp_clk);
						clk_set_rate(dfs_clk, origin_clk);
						return -RT_ERROR;
					}
					rt_kprintf("setting CLK:%lld, OK\n", cpu_dfs_table[j]);

					rt_thread_delay(100);
				}
				rt_kprintf("loops: %d\n\n\n",loops);
			}

			/* set the origin clk */
			ret = clk_set_rate(dfs_clk, origin_clk);
			if (ret < 0) {
				rt_kprintf("%s:%d, set Clk rate:%lld falied\n", __func__, __LINE__, cpu_dfs_table[j]);
				return -RT_ERROR;
			}

			rt_kprintf("All loops OK\n");
		}
	}
}

#include <finsh.h>
MSH_CMD_EXPORT_ALIAS(rt_cpu_dfs, rt_cpu_dfs, rt_cpu_dfs <loops>);
#endif
