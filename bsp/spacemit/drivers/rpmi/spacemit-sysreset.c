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

static rt_list_t rpmi_sysreset_list = RT_LIST_OBJECT_INIT(rpmi_sysreset_list);
extern struct rt_mutex rpmi_sysreset_mtx;

static rt_int32_t spacemit_rpmi_get_sysreset_config(struct dtb_node *node, void *con, char *match)
{
	rt_int32_t ret = 0;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_sysreset_config *config = &c->sysreset_config;
	struct spacemit_rpmi_sysreset_ops *pos = RT_NULL;

	config->node = node;

	/* initialize the platform related resources */
	rt_mutex_take(&rpmi_sysreset_mtx, RT_WAITING_FOREVER);
	rt_list_for_each_entry(pos, &rpmi_sysreset_list, list) {
		if (!rt_strcmp(pos->name, match))
			break;
	}
	rt_mutex_release(&rpmi_sysreset_mtx);

	if (pos) {
		config->ops = pos->sysreset_ops;
		ret = pos->init((void *)config);
	}

	return ret;
}

static rpmi_uint32_t sysreset_types[] = {
	RPMI_SYSRST_TYPE_SHUTDOWN,
	RPMI_SYSRST_TYPE_COLD_REBOOT,
};

static void spacemit_do_system_reset(void *priv, rpmi_uint32_t sysreset_type)
{
	struct spacemit_rpmi_sysreset_config *config = priv;

	config->ops->do_system_reset(priv, sysreset_type);
}

static struct rpmi_sysreset_platform_ops spacemit_sysreset_ops = {
		.do_system_reset = spacemit_do_system_reset,
};

static rt_int32_t spacemit_rpmi_register_sysreset_service(void *con, struct rpmi_context *cntx)
{
	rt_int32_t ret;
	struct rpmi_service_group *group = NULL;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_sysreset_config *config = &c->sysreset_config;

	group = rpmi_service_group_sysreset_create(sizeof(sysreset_types) / sizeof(sysreset_types[0]),
						   sysreset_types, &spacemit_sysreset_ops, config);
	if (!group) {
		rt_kprintf("Failed to create sysreset service group\n");
		return -RT_EINVAL;
	}

	/* Add HSM service group to context */
	ret = rpmi_context_add_group(cntx, group);
	if (ret != RPMI_SUCCESS) {
		rt_kprintf("Failed to add sysreset service group (ret=%d)\n", ret);
		return -RT_EINVAL;
	}

	return 0;
}

struct spacemit_rpmi_func rpmi_sysreset_func = {
	.rmpi_get_configuration = spacemit_rpmi_get_sysreset_config,
	.rpmi_register_service = spacemit_rpmi_register_sysreset_service,
};

rt_int32_t spacemit_rpmi_sysreset_register(rt_list_t *node)
{
	rt_mutex_take(&rpmi_sysreset_mtx, RT_WAITING_FOREVER);
	rt_list_insert_after(&rpmi_sysreset_list, node);
	rt_mutex_release(&rpmi_sysreset_mtx);

	return 0;
}
