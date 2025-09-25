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

static rt_list_t rpmi_clk_list = RT_LIST_OBJECT_INIT(rpmi_clk_list);
extern struct rt_mutex rpmi_clk_mtx;

static rt_int32_t spacemit_rpmi_get_clk_config(struct dtb_node *node, void *con, char *match)
{
	rt_int32_t ret = 0;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_clk_config *config = &c->clk_config;
	struct spacemit_rpmi_clk_ops *pos = RT_NULL;

	config->node = node;

	/* initialize the platform related resources */
	rt_mutex_take(&rpmi_clk_mtx, RT_WAITING_FOREVER);
	rt_list_for_each_entry(pos, &rpmi_clk_list, list) {
		if (!rt_strcmp(pos->name, match))
			break;
	}
	rt_mutex_release(&rpmi_clk_mtx);

	if (pos) {
		config->ops = pos->clk_ops;
		ret = pos->init((void *)config);
	}

	return ret;
}

static enum rpmi_error spacemit_set_state(void *priv, rpmi_uint32_t clock_id, enum rpmi_clock_state state)
{
	struct spacemit_rpmi_clk_config *config = priv;

	return config->ops->set_state(priv, clock_id, state);
}

static enum rpmi_error spacemit_get_state_and_rate(void *priv, rpmi_uint32_t clock_id, enum rpmi_clock_state *state, rpmi_uint64_t *rate)
{
	struct spacemit_rpmi_clk_config *config = priv;

	return config->ops->get_state_and_rate(priv, clock_id, state, rate);
}

static rpmi_bool_t spacemit_rate_change_match(void *priv, rpmi_uint32_t clock_id, rpmi_uint64_t rate)
{
	struct spacemit_rpmi_clk_config *config = priv;

	return config->ops->rate_change_match(priv, clock_id, rate);
}

static enum rpmi_error spacemit_set_rate(void *priv, rpmi_uint32_t clock_id, enum rpmi_clock_rate_match match, rpmi_uint64_t rate, rpmi_uint64_t *new_rate)
{
	struct spacemit_rpmi_clk_config *config = priv;

	return config->ops->set_rate(priv, clock_id, match, rate, new_rate);
}

static enum rpmi_error spacemit_set_rate_recalc(void *priv, rpmi_uint32_t clock_id, rpmi_uint64_t parent_rate, rpmi_uint64_t *new_rate)
{
	struct spacemit_rpmi_clk_config *config = priv;

	return config->ops->set_rate_recalc(priv, clock_id, parent_rate, new_rate);
}

static struct rpmi_clock_platform_ops spacemit_clk_ops = {
	.set_state = spacemit_set_state,
	.get_state_and_rate = spacemit_get_state_and_rate,
	.rate_change_match = spacemit_rate_change_match,
	.set_rate = spacemit_set_rate,
	.set_rate_recalc = spacemit_set_rate_recalc,
};

static rt_int32_t spacemit_rpmi_register_clk_service(void *con, struct rpmi_context *cntx)
{
	rt_int32_t ret;
	struct rpmi_service_group *group = NULL;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_clk_config *config = &c->clk_config;

	group = rpmi_service_group_clock_create(config->num_clk, config->clk_data, &spacemit_clk_ops, config);
	if (!group) {
		rt_kprintf("Failed to create clk service group\n");
		return -RT_EINVAL;
	}

	/* Add HSM service group to context */
	ret = rpmi_context_add_group(cntx, group);
	if (ret != RPMI_SUCCESS) {
		rt_kprintf("Failed to add clk service group (ret=%d)\n", ret);
		return -RT_EINVAL;
	}

	return 0;
}

struct spacemit_rpmi_func rpmi_clk_func = {
	.rmpi_get_configuration = spacemit_rpmi_get_clk_config,
	.rpmi_register_service = spacemit_rpmi_register_clk_service,
};

rt_int32_t spacemit_rpmi_clk_register(rt_list_t *node)
{
	rt_mutex_take(&rpmi_clk_mtx, RT_WAITING_FOREVER);
	rt_list_insert_after(&rpmi_clk_list, node);
	rt_mutex_release(&rpmi_clk_mtx);

	return 0;
}
