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

static rt_list_t rpmi_device_power_list = RT_LIST_OBJECT_INIT(rpmi_device_power_list);
extern struct rt_mutex rpmi_device_power_mtx;

static rt_int32_t spacemit_rpmi_get_domain_config(struct dtb_node *node, void *con, char *match)
{
	rt_int32_t ret = 0;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_domain_config *config = &c->domain_config;
	struct spacemit_rpmi_domain_ops *pos = RT_NULL;

	/* initialize the platform related resources */
	rt_mutex_take(&rpmi_device_power_mtx, RT_WAITING_FOREVER);
	rt_list_for_each_entry(pos, &rpmi_device_power_list, list) {
		if (!rt_strcmp(pos->name, match))
			break;
	}
	rt_mutex_release(&rpmi_device_power_mtx);

	if (pos) {
		config->ops = pos->domain_ops;
		ret = pos->init((void *)config);
	}

	return ret;
}

/** Set the domain state ON/OFF */
static enum rpmi_error spacemit_domain_set_state(void *priv, rpmi_uint32_t domain_id, enum rpmi_device_power_state state)
{
	struct spacemit_rpmi_domain_config *config = priv;

	return config->ops->set_state(priv, domain_id, state);
}

static enum rpmi_error spacemit_domain_get_state(void *priv, rpmi_uint32_t domain_id, enum rpmi_device_power_state *state)
{
	struct spacemit_rpmi_domain_config *config = priv;

	return config->ops->get_state(priv, domain_id, state);
}

static struct rpmi_domain_platform_ops spacemit_device_power_ops = {
	.set_state = spacemit_domain_set_state,
	.get_state = spacemit_domain_get_state,
};

static rt_int32_t spacemit_rpmi_register_domain_service(void *con, struct rpmi_context *cntx)
{
	rt_int32_t ret;
	struct rpmi_service_group *group = NULL;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_domain_config *config = &c->domain_config;

	group = rpmi_service_group_domain_create(config->domain_count, config->domain_data, &spacemit_device_power_ops, config);
	if (!group) {
		rt_kprintf("Failed to create device power service group\n");
		return -RT_EINVAL;
	}

	/* Add HSM service group to context */
	ret = rpmi_context_add_group(cntx, group);
	if (ret != RPMI_SUCCESS) {
		rt_kprintf("Failed to add device_power service group (ret=%d)\n", ret);
		return -RT_EINVAL;
	}

	return 0;
}

struct spacemit_rpmi_func rpmi_domain_func = {
	.rmpi_get_configuration = spacemit_rpmi_get_domain_config,
	.rpmi_register_service = spacemit_rpmi_register_domain_service,
};

rt_int32_t spacemit_rpmi_domain_register(rt_list_t *node)
{
	rt_mutex_take(&rpmi_device_power_mtx, RT_WAITING_FOREVER);
	rt_list_insert_after(&rpmi_device_power_list, node);
	rt_mutex_release(&rpmi_device_power_mtx);

	return 0;
}
