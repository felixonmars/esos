/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rtconfig.h>
#include <drivers/regulator.h>
#include <drivers/regulator_dm.h>
#include "regulator.h"

#ifdef RT_USING_FINSH

static struct dtb_compatible_array __test_compatible[] = {
	{ .compatible = "regulator-test", },
	{}
};

int regulator_test(void)
{
	int i;
	rt_err_t err;
	struct rt_regulator *regulator_ptr;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__test_compatible) / sizeof(__test_compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__test_compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			regulator_ptr = rt_regulator_get(compatible_node, "vin");
			if (regulator_ptr == RT_NULL) {
				rt_kprintf("%s:%d, get regulator error\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			err = rt_regulator_enable(regulator_ptr);
			if (err != RT_EOK) {
				rt_kprintf("%s:%d, set regulator enable error\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			err = rt_regulator_set_voltage(regulator_ptr, 500000, 500000);
			if (err != RT_EOK) {
				rt_kprintf("%s:%d, set voltage error\n", __func__, __LINE__);
				return -RT_EINVAL;
			}
		}
	}

	return 0;
}

#include <finsh.h>
MSH_CMD_EXPORT(regulator_test, regulator driver test. e.g: regulator_test());
#endif
