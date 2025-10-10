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

static rt_list_t rpmi_rtc_list = RT_LIST_OBJECT_INIT(rpmi_rtc_list);
extern struct rt_mutex rpmi_rtc_mtx;

static rt_int32_t spacemit_rpmi_get_rtc_config(struct dtb_node *node, void *con, char *match)
{
	rt_int32_t ret = 0;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_rtc_config *config = &c->rtc_config;
	struct spacemit_rpmi_rtc_ops *pos = RT_NULL;

	config->node = node;

	/* initialize the platform related resources */
	rt_mutex_take(&rpmi_rtc_mtx, RT_WAITING_FOREVER);
	rt_list_for_each_entry(pos, &rpmi_rtc_list, list) {
		if (!rt_strcmp(pos->name, match))
			break;
	}
	rt_mutex_release(&rpmi_rtc_mtx);

	if (pos) {
		config->ops = pos->rtc_ops;
		ret = pos->init((void *)config);
	}

	return ret;
}

/** Set the rtc time */
static enum rpmi_error spacemit_set_time(void *priv, rpmi_uint32_t year,
				    rpmi_uint32_t mon,
				    rpmi_uint32_t dat,
				    rpmi_uint32_t hour,
				    rpmi_uint32_t min,
				    rpmi_uint32_t second)
{
	struct spacemit_rpmi_rtc_config *config = priv;

	return config->ops->set_time(priv, year, mon, dat, hour, min, second);
}

/**
 * Get the rtc time
 **/
static enum rpmi_error spacemit_get_time(void *priv, rpmi_uint32_t *year,
				    rpmi_uint32_t *mon,
				    rpmi_uint32_t *dat,
				    rpmi_uint32_t *hour,
				    rpmi_uint32_t *min,
				    rpmi_uint32_t *second)
{
	struct spacemit_rpmi_rtc_config *config = priv;

	return config->ops->get_time(priv, year, mon, dat, hour, min, second);
}

/** Set the rtc alarm time */
static enum rpmi_error spacemit_set_alarm(void *priv, rpmi_uint32_t year,
				    rpmi_uint32_t mon,
				    rpmi_uint32_t dat,
				    rpmi_uint32_t hour,
				    rpmi_uint32_t min,
				    rpmi_uint32_t second)
{
	struct spacemit_rpmi_rtc_config *config = priv;

	return config->ops->set_alarm(priv, year, mon, dat, hour, min, second);
}

/**
 * Get the rtc alarm time
 **/
static enum rpmi_error spacemit_get_alarm(void *priv, rpmi_uint32_t *year,
				    rpmi_uint32_t *mon,
				    rpmi_uint32_t *dat,
				    rpmi_uint32_t *hour,
				    rpmi_uint32_t *min,
				    rpmi_uint32_t *second)
{
	struct spacemit_rpmi_rtc_config *config = priv;

	return config->ops->get_alarm(priv, year, mon, dat, hour, min, second);
}

enum rpmi_error spacemit_get_alarm_en(void *priv, rpmi_uint32_t *status)
{
	struct spacemit_rpmi_rtc_config *config = priv;

	return config->ops->get_alarm_en(priv, status);
}

static enum rpmi_error spacemit_set_alarm_en(void *priv, rpmi_uint32_t en)
{
	struct spacemit_rpmi_rtc_config *config = priv;

	return config->ops->set_alarm_en(priv, en);
}

enum rpmi_error spacemit_query_pending(void *priv, rpmi_uint32_t *status)
{
	struct spacemit_rpmi_rtc_config *config = priv;

	return config->ops->query_pending(priv, status);
}

static enum rpmi_error spacemit_clear_pending(void *priv)
{
	struct spacemit_rpmi_rtc_config *config = priv;

	return config->ops->clear_pending(priv);
}

static struct rpmi_rtc_platform_ops spacemit_rtc_ops = {
	.set_time = spacemit_set_time,
	.get_time = spacemit_get_time,
	.set_alarm = spacemit_set_alarm,
	.get_alarm = spacemit_get_alarm,
	.get_alarm_en = spacemit_get_alarm_en,
	.set_alarm_en = spacemit_set_alarm_en,
	.query_pending = spacemit_query_pending,
	.clear_pending = spacemit_clear_pending,
};

static rt_int32_t spacemit_rpmi_register_rtc_service(void *con, struct rpmi_context *cntx)
{
	rt_int32_t ret;
	struct rpmi_service_group *group = NULL;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_rtc_config *config = &c->rtc_config;

	group = rpmi_service_group_rtc_create(&spacemit_rtc_ops, config);
	if (!group) {
		rt_kprintf("Failed to create rtc service group\n");
		return -RT_EINVAL;
	}

	/* Add HSM service group to context */
	ret = rpmi_context_add_group(cntx, group);
	if (ret != RPMI_SUCCESS) {
		rt_kprintf("Failed to add rtc service group (ret=%d)\n", ret);
		return -RT_EINVAL;
	}

	return 0;
}

struct spacemit_rpmi_func rpmi_rtc_func = {
	.rmpi_get_configuration = spacemit_rpmi_get_rtc_config,
	.rpmi_register_service = spacemit_rpmi_register_rtc_service,
};

rt_int32_t spacemit_rpmi_rtc_register(rt_list_t *node)
{
	rt_mutex_take(&rpmi_rtc_mtx, RT_WAITING_FOREVER);
	rt_list_insert_after(&rpmi_rtc_list, node);
	rt_mutex_release(&rpmi_rtc_mtx);

	return 0;
}
