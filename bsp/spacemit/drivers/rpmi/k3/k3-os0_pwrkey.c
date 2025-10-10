/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include <librpmi.h>
#include "../spacemit-rpmi.h"

static rt_int32_t  _k3_os0_pwrkey_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_pwrkey_config *config = priv;

	return 0;
}

static enum rpmi_error k3_os0_query_pending(void *priv, rpmi_uint32_t *status)
{
	return RPMI_SUCCESS;
}

static enum rpmi_error k3_os0_clear_pending(void *priv, rpmi_uint32_t clear)
{
	return RPMI_SUCCESS;
}

static struct rpmi_pwrkey_platform_ops k3_os0_pwrkey_pops = {
	.query_pending = k3_os0_query_pending,
	.clear_pending = k3_os0_clear_pending,
};

static struct spacemit_rpmi_pwrkey_ops k3_os0_pwrkey_ops = {
	.name = "k3-os0-rpmi-pwrkey",
	.init = _k3_os0_pwrkey_init,
	.pwrkey_ops = &k3_os0_pwrkey_pops,
};

static rt_int32_t k3_os0_pwrkey_init(void)
{
	rt_list_init(&k3_os0_pwrkey_ops.list);

	spacemit_rpmi_pwrkey_register(&k3_os0_pwrkey_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_pwrkey_init);
