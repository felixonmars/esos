#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include "../spacemit-rpmi.h"

struct rpmi_voltage_data k3_os0_voltage_data[1] = {
	[0] = {
		. parent_id = -1,
	},
};

static int  _k3_os0_voltage_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_voltage_config *config = priv;

	config->domain_count = 1;

	config->voltage_data = k3_os0_voltage_data;

	return 0;
}

/** Set the voltage state enable/disable/others */
static enum rpmi_error spacemit_set_config(void *priv, rpmi_uint32_t domain_id, enum rpmi_voltage_state state)
{
	return 0;
}

static enum rpmi_error spacemit_get_config(void *priv, rpmi_uint32_t domain_id, enum rpmi_voltage_state *state)
{
	return 0;
}

static enum rpmi_error spacemit_set_voltage_level(void *priv, rpmi_uint32_t domain_id, rpmi_uint32_t level)
{
	return 0;
}

static enum rpmi_error spacemit_get_voltage_level(void *priv, rpmi_uint32_t domain_id, rpmi_uint32_t *level)
{
	return 0;
}

static struct rpmi_voltage_platform_ops k3_os0_voltage_pops = {
	.set_config = spacemit_set_config,
	.get_config = spacemit_get_config,
	.set_voltage_level = spacemit_set_voltage_level,
	.get_voltage_level = spacemit_get_voltage_level,
};

static struct spacemit_rpmi_voltage_ops k3_os0_voltage_ops = {
	.name = "k3-os0-rpmi-voltage",
	.init = _k3_os0_voltage_init,
	.voltage_ops = &k3_os0_voltage_pops,
};

static int k3_os0_voltage_init(void)
{
	rt_list_init(&k3_os0_voltage_ops.list);

	spacemit_rpmi_voltage_register(&k3_os0_voltage_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_voltage_init);
