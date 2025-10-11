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

static rt_int32_t  _k3_os0_sysreset_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_sysreset_config *config = priv;

	return 0;
}

static void k3_os0_system_reset(void *priv, rpmi_uint32_t sysreset_type)
{
}

static struct rpmi_sysreset_platform_ops k3_os0_sysreset_pops = {
	.do_system_reset = k3_os0_system_reset,
};

static struct spacemit_rpmi_sysreset_ops k3_os0_sysreset_ops = {
	.name = "k3-os0-rpmi-sysreset",
	.init = _k3_os0_sysreset_init,
	.sysreset_ops = &k3_os0_sysreset_pops,
};

static rt_int32_t k3_os0_sysreset_init(void)
{
	rt_list_init(&k3_os0_sysreset_ops.list);

	spacemit_rpmi_sysreset_register(&k3_os0_sysreset_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_sysreset_init);
