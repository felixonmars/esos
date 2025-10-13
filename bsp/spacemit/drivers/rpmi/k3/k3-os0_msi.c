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

static rt_int32_t  _k3_os0_msi_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_msi_config *config = priv;

	/* 0: sysmsi
	 * 1 ~ 8: tsensor
	 */

	config->num_msi = 9;
	config->p2a_index = 0;

	return 0;
}

static rpmi_bool_t k3_os0_validate_msi_addr(void *priv, rpmi_uint64_t msi_addr)
{
	return 1;
}

static rpmi_bool_t k3_os0_mmode_preferred(void *priv, rpmi_uint32_t msi_index)
{
	if (msi_index == 0)
		return 1;
	else
		return 0;
}

static void k3_os0_get_name(void *priv, rpmi_uint32_t msi_index,
			   char *out_name, rpmi_uint32_t out_name_sz)
{
	char str[12], size;

	if (msi_index == 0)
		rt_strncpy(out_name, "sysmsi", rt_strlen("sysmsi"));
	else {
		size = rt_sprintf(str, "btg:%d", msi_index);
		rt_strncpy(out_name, str, size);
	}
}

static struct rpmi_sysmsi_platform_ops k3_os0_msi_pops = {
	.validate_msi_addr = k3_os0_validate_msi_addr,
	.mmode_preferred = k3_os0_mmode_preferred,
	.get_name = k3_os0_get_name,
};

static struct spacemit_rpmi_msi_ops k3_os0_msi_ops = {
	.name = "k3-os0-rpmi-sysmsi",
	.init = _k3_os0_msi_init,
	.msi_ops = &k3_os0_msi_pops,
};

static rt_int32_t k3_os0_msi_init(void)
{
	rt_list_init(&k3_os0_msi_ops.list);

	spacemit_rpmi_msi_register(&k3_os0_msi_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_msi_init);
