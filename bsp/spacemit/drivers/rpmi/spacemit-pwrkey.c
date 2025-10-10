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

static rt_list_t rpmi_pwrkey_list = RT_LIST_OBJECT_INIT(rpmi_pwrkey_list);
extern struct rt_mutex rpmi_pwrkey_mtx;

/**
 * Query the pwrkey interrupt pending
 **/
static enum rpmi_error spacemit_query_pending(void *priv, rpmi_uint32_t *status)
{
	struct spacemit_rpmi_pwrkey_config *config = priv;

	return config->ops->query_pending(priv, status);
}

/**
 * Clear the pwrkey interrupt pending
 **/
static enum rpmi_error spacemit_clear_pending(void *priv, rpmi_uint32_t clear)
{
	struct spacemit_rpmi_pwrkey_config *config = priv;

	return config->ops->clear_pending(priv, clear);
}

static struct rpmi_pwrkey_platform_ops spacemit_pwrkey_ops = {
	.query_pending = spacemit_query_pending,
	.clear_pending = spacemit_clear_pending,
};

static rt_int32_t spacemit_rpmi_get_pwrkey_config(struct dtb_node *node, void *con, char *match)
{
	rt_int32_t ret = 0;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_pwrkey_config *config = &c->pwrkey_config;
	struct spacemit_rpmi_pwrkey_ops *pos = RT_NULL;

	config->node = node;

	/* initialize the platform related resources */
	rt_mutex_take(&rpmi_pwrkey_mtx, RT_WAITING_FOREVER);
	rt_list_for_each_entry(pos, &rpmi_pwrkey_list, list) {
		if (!rt_strcmp(pos->name, match))
			break;
	}
	rt_mutex_release(&rpmi_pwrkey_mtx);

	if (pos) {
		config->ops = pos->pwrkey_ops;
		ret = pos->init((void *)config);
	}

	return ret;
}

static rt_int32_t spacemit_rpmi_register_pwrkey_service(void *con, struct rpmi_context *cntx)
{
	rt_int32_t ret;
	struct rpmi_service_group *group = NULL;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_pwrkey_config *config = &c->pwrkey_config;

	group = rpmi_service_group_pwrkey_create(&spacemit_pwrkey_ops, config);
	if (!group) {
		rt_kprintf("Failed to create pwrkey service group\n");
		return -RT_EINVAL;
	}

	/* Add HSM service group to context */
	ret = rpmi_context_add_group(cntx, group);
	if (ret != RPMI_SUCCESS) {
		rt_kprintf("Failed to add pwrkey service group (ret=%d)\n", ret);
		return -RT_EINVAL;
	}

	return 0;
}

struct spacemit_rpmi_func rpmi_pwrkey_func = {
	.rmpi_get_configuration = spacemit_rpmi_get_pwrkey_config,
	.rpmi_register_service = spacemit_rpmi_register_pwrkey_service,
};

rt_int32_t spacemit_rpmi_pwrkey_register(rt_list_t *node)
{
	rt_mutex_take(&rpmi_pwrkey_mtx, RT_WAITING_FOREVER);
	rt_list_insert_after(&rpmi_pwrkey_list, node);
	rt_mutex_release(&rpmi_pwrkey_mtx);

	return 0;
}
