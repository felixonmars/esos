/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <dtb_node.h>
#include "spacemit-rpmi.h"

static rt_list_t rpmi_msi_list = RT_LIST_OBJECT_INIT(rpmi_msi_list);
extern struct rt_mutex rpmi_msi_mtx;

static rt_int32_t spacemit_rpmi_get_msi_config(struct dtb_node *node, void *con, char *match)
{
	rt_int32_t ret = 0;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_msi_config *config = &c->msi_config;
	struct spacemit_rpmi_msi_ops *pos = RT_NULL;

	config->node = node;

	/* initialize the platform related resources */
	rt_mutex_take(&rpmi_msi_mtx, RT_WAITING_FOREVER);
	rt_list_for_each_entry(pos, &rpmi_msi_list, list) {
		if (!rt_strcmp(pos->name, match))
			break;
	}
	rt_mutex_release(&rpmi_msi_mtx);

	if (pos) {
		config->ops = pos->msi_ops;
		ret = pos->init((void *)config);
	}

	return ret;
}

static rpmi_bool_t spacemit_validate_msi_addr(void *priv, rpmi_uint64_t msi_addr)
{
	struct spacemit_rpmi_msi_config *config = priv;

	return config->ops->validate_msi_addr(priv, msi_addr);
}

static rpmi_bool_t spacemit_mmode_preferred(void *priv, rpmi_uint32_t msi_index)
{
	struct spacemit_rpmi_msi_config *config = priv;

	return config->ops->mmode_preferred(priv, msi_index);
}

static void spacemit_get_name(void *priv, rpmi_uint32_t msi_index,
			 char *out_name, rpmi_uint32_t out_name_sz)
{
	struct spacemit_rpmi_msi_config *config = priv;

	config->ops->get_name(priv, msi_index, out_name, out_name_sz);
}

struct rpmi_sysmsi_platform_ops spacemit_msi_ops = {
	.validate_msi_addr = spacemit_validate_msi_addr,
	.mmode_preferred = spacemit_mmode_preferred,
	.get_name = spacemit_get_name,
};

static rt_int32_t spacemit_rpmi_register_msi_service(void *con, struct rpmi_context *cntx)
{
	rt_int32_t ret;
	struct rpmi_service_group *group = NULL;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_msi_config *config = &c->msi_config;

	group = rpmi_service_group_sysmsi_create(config->num_msi, config->p2a_index, &spacemit_msi_ops, config);
	if (!group) {
		rt_kprintf("Failed to create msi service group\n");
		return -RT_EINVAL;
	}

	/* Add HSM service group to context */
	ret = rpmi_context_add_group(cntx, group);
	if (ret != RPMI_SUCCESS) {
		rt_kprintf("Failed to add msi service group (ret=%d)\n", ret);
		return -RT_EINVAL;
	}

	return 0;
}

struct spacemit_rpmi_func rpmi_msi_func = {
	.rmpi_get_configuration = spacemit_rpmi_get_msi_config,
	.rpmi_register_service = spacemit_rpmi_register_msi_service,
};

rt_int32_t spacemit_rpmi_msi_register(rt_list_t *node)
{
	rt_mutex_take(&rpmi_msi_mtx, RT_WAITING_FOREVER);
	rt_list_insert_after(&rpmi_msi_list, node);
	rt_mutex_release(&rpmi_msi_mtx);

	return 0;
}
