#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <dtb_node.h>
#include "spacemit-rpmi.h"

static rt_list_t rpmi_hsm_list = RT_LIST_OBJECT_INIT(rpmi_hsm_list);
extern struct rt_mutex rpmi_hsm_mtx;

/* hsm releated */
static enum rpmi_hart_hw_state hsm_get_hw_state(void* priv,
	rpmi_uint32_t hart_index)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->hsm_ops->hart_get_hw_state(priv, hart_index);
}

static enum rpmi_error hsm_hart_start_prepare(void* priv,
	rpmi_uint32_t hart_index,
	rpmi_uint64_t start_addr)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->hsm_ops->hart_start_prepare(priv, hart_index, start_addr);
}

static void hsm_hart_start_finalize(void* priv,
	rpmi_uint32_t hart_index,
	rpmi_uint64_t start_addr)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	config->hsm_ops->hart_start_finalize(priv, hart_index, start_addr);
}

static enum rpmi_error hsm_hart_stop_prepare(void* priv,
	rpmi_uint32_t hart_index)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->hsm_ops->hart_stop_prepare(priv, hart_index);
}

static void hsm_hart_stop_finalize(void* priv, rpmi_uint32_t hart_index)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	config->hsm_ops->hart_stop_finalize(priv, hart_index);
}

static enum rpmi_error hsm_hart_suspend_prepare(
	void* priv,
	rpmi_uint32_t hart_index,
	const struct rpmi_hsm_suspend_type* suspend_type,
	rpmi_uint64_t resume_addr)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->hsm_ops->hart_suspend_prepare(priv, hart_index, suspend_type, resume_addr);
}

static void hsm_hart_suspend_finalize(
	void* priv,
	rpmi_uint32_t hart_index,
	const struct rpmi_hsm_suspend_type* suspend_type,
	rpmi_uint64_t resume_addr)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->hsm_ops->hart_suspend_finalize(priv, hart_index, suspend_type, resume_addr);
}


/* HSM platform operations */
static struct rpmi_hsm_platform_ops hsm_ops = {
	.hart_get_hw_state = hsm_get_hw_state,
	.hart_start_prepare = hsm_hart_start_prepare,
	.hart_start_finalize = hsm_hart_start_finalize,
	.hart_stop_prepare = hsm_hart_stop_prepare,
	.hart_stop_finalize = hsm_hart_stop_finalize,
	.hart_suspend_prepare = hsm_hart_suspend_prepare,
	.hart_suspend_finalize = hsm_hart_suspend_finalize
};

static int spacemit_rpmi_get_hsm_config(struct dtb_node *node, void *con, char *match)
{
	int i = 0;
	struct dtb_node *node_ptr = node;
	int property_size;
	rt_uint32_t u32_value;
	rt_uint32_t *u32_ptr;
	const void* prop_data;
	int prop_len;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_hsm_config *config = &c->hsm_config;
	struct spacemit_rpmi_hsm_ops *pos = RT_NULL;

	config->node = node;

	/* get the start hardid */
	for_each_property_cell(node, "hartids", u32_value, u32_ptr, property_size) {
		config->hartids[i++] = u32_value;
	}

	config->hartcnt = i;

	/* get the suspend type */
	i = 0;
	for_each_node_child(node_ptr) {
		if (!dtb_node_get_dtb_node_compatible_match(node_ptr, "riscv,idle-state"))
			continue;

		/* get the property */
		prop_data = dtb_node_get_property(node_ptr, "riscv,sbi-suspend-param", &prop_len);
		if (prop_data && prop_len >= sizeof(uint32_t)) {
			config->stype[i].type = fdt32_to_cpu(*(uint32_t*)prop_data);
		}

		prop_data = dtb_node_get_property(node_ptr, "entry-latency-us", &prop_len);
		if (prop_data && prop_len >= sizeof(uint32_t)) {
			config->stype[i].info.entry_latency_us = fdt32_to_cpu(*(uint32_t*)prop_data);
		}

		prop_data = dtb_node_get_property(node_ptr, "exit-latency-us", &prop_len);
		if (prop_data && prop_len >= sizeof(uint32_t)) {
			config->stype[i].info.exit_latency_us = fdt32_to_cpu(*(uint32_t*)prop_data);
		}

		prop_data = dtb_node_get_property(node_ptr, "min-residency-us", &prop_len);
		if (prop_data && prop_len >= sizeof(uint32_t)) {
			config->stype[i].info.min_residency_us = fdt32_to_cpu(*(uint32_t*)prop_data);
		}

		prop_data = dtb_node_get_property(node_ptr, "wakeup-latency-us", &prop_len);
		if (prop_data && prop_len >= sizeof(uint32_t)) {
			config->stype[i].info.wakeup_latency_us = fdt32_to_cpu(*(uint32_t*)prop_data);
		}

		if (dtb_node_get_dtb_node_property(node_ptr, "local-timer-stop", RT_NULL))
			config->stype[i].info.flags = RPMI_HSM_SUSPEND_INFO_FLAGS_TIMER_STOP;
		++i;
	}

	config->type_cnt = i;

	/* support system suspend ? */
	if (dtb_node_get_dtb_node_property(node, "risv,support-syssup", RT_NULL)) {
		config->support_syssup = 1;
	}

	/* initialize the platform related resources */
	rt_mutex_take(&rpmi_hsm_mtx, RT_WAITING_FOREVER);
	rt_list_for_each_entry(pos, &rpmi_hsm_list, list) {
		if (!rt_strcmp(pos->name, match))
			break;
	}
	rt_mutex_release(&rpmi_hsm_mtx);

	if (pos) {
		config->hsm_ops = pos->hsm_ops;
		config->syssup_ops = pos->syssup_ops;
		i = pos->init((void *)config);
	}

	return i;
}

/* system suspend releated */
struct rpmi_system_suspend_type system_suspend_types[1] = {
	{ .type = RPMI_SYSSUSP_TYPE_SUSPEND_TO_RAM,
		.attr = RPMI_SYSSUSP_ATTRS_FLAGS_RESUMEADDR | RPMI_SYSSUSP_ATTRS_FLAGS_SUSPENDTYPE }
};

/* System suspend platform operations (stubs for hardware-related functions) */
static enum rpmi_error syssusp_prepare(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->syssup_ops->system_suspend_prepare(priv, hart_index, syssusp_type, resume_addr);
}

static rpmi_bool_t syssusp_ready(void* priv, rpmi_uint32_t hart_index)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->syssup_ops->system_suspend_ready(priv, hart_index);
}

static void syssusp_finalize(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
        /* Hardware-related stub */
	struct spacemit_rpmi_hsm_config *config = priv;

	config->syssup_ops->system_suspend_finalize(priv, hart_index, syssusp_type, resume_addr);
}

static rpmi_bool_t syssusp_can_resume(void* priv, rpmi_uint32_t hart_index)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->syssup_ops->system_suspend_can_resume(priv, hart_index);
}

static enum rpmi_error syssusp_resume(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	return config->syssup_ops->system_suspend_resume(priv, hart_index, syssusp_type, resume_addr);
}

/* System suspend platform operations */
static struct rpmi_syssusp_platform_ops syssusp_ops = {
	.system_suspend_prepare = syssusp_prepare,
	.system_suspend_ready = syssusp_ready,
	.system_suspend_finalize = syssusp_finalize,
	.system_suspend_can_resume = syssusp_can_resume,
	.system_suspend_resume = syssusp_resume
};

static int spacemit_rpmi_register_service(void *con, struct rpmi_context *cntx)
{
	int ret;
	struct rpmi_hsm* hsm = NULL;
	struct rpmi_service_group *group = NULL;
	struct spacemit_rpmi_config *c = (struct spacemit_rpmi_config *)con;
	struct spacemit_rpmi_hsm_config *config = &c->hsm_config;

	hsm = rpmi_hsm_create(config->hartcnt, config->hartids, config->type_cnt, config->stype, &hsm_ops, config);
	if (!hsm) {
		rt_kprintf("create the hsm failed\n");
		return -RT_EINVAL;
	}

	/* Create HSM service group */
	group = rpmi_service_group_hsm_create(hsm);
	if (!group) {
		rt_kprintf("Failed to create HSM service group\n");
		return -RT_EINVAL;
	}

	/* Add HSM service group to context */
	ret = rpmi_context_add_group(cntx, group);
	if (ret != RPMI_SUCCESS) {
		rt_kprintf("Failed to add HSM service group (ret=%d)\n", ret);
		return -RT_EINVAL;
	}

	if (config->support_syssup) {
		/* create the system suspend services */
		group = rpmi_service_group_syssusp_create(hsm, 1, system_suspend_types,
				&syssusp_ops, config);
		if (!group) {
			rt_kprintf("ERROR: Failed to create System Suspend service group\n");
			return -RT_EINVAL;
		}

		if (rpmi_context_add_group(cntx, group) != RPMI_SUCCESS) {
                	rt_kprintf("ERROR: Failed to add System Suspend service group\n");
			return -RT_EINVAL;
		}
	}

	return 0;
}

struct spacemit_rpmi_func rpmi_hsm_func = {
	.rmpi_get_configuration = spacemit_rpmi_get_hsm_config,
	.rpmi_register_service = spacemit_rpmi_register_service,
};

int spacemit_rpmi_hsm_register(rt_list_t *node)
{
	rt_mutex_take(&rpmi_hsm_mtx, RT_WAITING_FOREVER);
	rt_list_insert_after(&rpmi_hsm_list, node);
	rt_mutex_release(&rpmi_hsm_mtx);

	return 0;
}
