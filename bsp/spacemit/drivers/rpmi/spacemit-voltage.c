#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <dtb_node.h>
#include "spacemit-rpmi.h"

static rt_list_t rpmi_voltage_list = RT_LIST_OBJECT_INIT(rpmi_voltage_list);
extern struct rt_mutex rpmi_voltage_mtx;

static int spacemit_rpmi_get_voltage_config(struct dtb_node *node, void *con, char *match)
{
	int ret = 0;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_voltage_config *config = &c->voltage_config;
	struct spacemit_rpmi_voltage_ops *pos = RT_NULL;

	config->node = node;

	/* initialize the platform related resources */
	rt_mutex_take(&rpmi_voltage_mtx, RT_WAITING_FOREVER);
	rt_list_for_each_entry(pos, &rpmi_voltage_list, list) {
		if (!rt_strcmp(pos->name, match))
			break;
	}
	rt_mutex_release(&rpmi_voltage_mtx);

	if (pos) {
		config->ops = pos->voltage_ops;
		ret = pos->init((void *)config);
	}

	return ret;
}

/** Set the voltage state enable/disable/others */
static enum rpmi_error spacemit_set_config(void *priv, rpmi_uint32_t domain_id, enum rpmi_voltage_state state)
{
	struct spacemit_rpmi_voltage_config *config = priv;

	return config->ops->set_config(priv, domain_id, state);
}

static enum rpmi_error spacemit_get_config(void *priv, rpmi_uint32_t domain_id, enum rpmi_voltage_state *state)
{
	struct spacemit_rpmi_voltage_config *config = priv;

	return config->ops->get_config(priv, domain_id, state);
}

static enum rpmi_error spacemit_set_voltage_level(void *priv, rpmi_uint32_t domain_id, rpmi_uint32_t level)
{
	struct spacemit_rpmi_voltage_config *config = priv;

	return config->ops->set_voltage_level(priv, domain_id, level);
}

static enum rpmi_error spacemit_get_voltage_level(void *priv, rpmi_uint32_t domain_id, rpmi_uint32_t *level)
{
	struct spacemit_rpmi_voltage_config *config = priv;

	return config->ops->get_voltage_level(priv, domain_id, level);
}

static struct rpmi_voltage_platform_ops spacemit_voltage_ops = {
	.set_config = spacemit_set_config,
	.get_config = spacemit_get_config,
	.set_voltage_level = spacemit_set_voltage_level,
	.get_voltage_level = spacemit_get_voltage_level,
};

static int spacemit_rpmi_register_volatge_service(void *con, struct rpmi_context *cntx)
{
	int ret;
	struct rpmi_service_group *group = NULL;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_voltage_config *config = &c->voltage_config;

	group = rpmi_service_group_voltage_create(config->domain_count, config->voltage_data, &spacemit_voltage_ops, config);
	if (!group) {
		rt_kprintf("Failed to create voltage service group\n");
		return -RT_EINVAL;
	}

	/* Add HSM service group to context */
	ret = rpmi_context_add_group(cntx, group);
	if (ret != RPMI_SUCCESS) {
		rt_kprintf("Failed to add voltage service group (ret=%d)\n", ret);
		return -RT_EINVAL;
	}

	return 0;
}

struct spacemit_rpmi_func rpmi_voltage_func = {
	.rmpi_get_configuration = spacemit_rpmi_get_voltage_config,
	.rpmi_register_service = spacemit_rpmi_register_volatge_service,
};

int spacemit_rpmi_voltage_register(rt_list_t *node)
{
	rt_mutex_take(&rpmi_voltage_mtx, RT_WAITING_FOREVER);
	rt_list_insert_after(&rpmi_voltage_list, node);
	rt_mutex_release(&rpmi_voltage_mtx);

	return 0;
}
