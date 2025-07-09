#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include "../spacemit-rpmi.h"

struct rpmi_clock_data k2_os0_clk_data[1] = {
	[0] = {
		. parent_id = -1,
	},
};

static int  _k2_os0_clock_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_clk_config *config = priv;

	config->num_clk = 1;

	config->clk_data = k2_os0_clk_data;

	return 0;
}

static enum rpmi_error spacemit_set_state(void *priv, rpmi_uint32_t clock_id, enum rpmi_clock_state state)
{
	return 0;
}

static enum rpmi_error spacemit_get_state_and_rate(void *priv, rpmi_uint32_t clock_id, enum rpmi_clock_state *state, rpmi_uint64_t *rate)
{
	*state = RPMI_CLK_STATE_ENABLED;

	return 0;
}

static rpmi_bool_t spacemit_rate_change_match(void *priv, rpmi_uint32_t clock_id, rpmi_uint64_t rate)
{
	return 0;
}

static enum rpmi_error spacemit_set_rate(void *priv, rpmi_uint32_t clock_id, enum rpmi_clock_rate_match match, rpmi_uint64_t rate, rpmi_uint64_t *new_rate)
{
	return 0;
}

static enum rpmi_error spacemit_set_rate_recalc(void *priv, rpmi_uint32_t clock_id, rpmi_uint64_t parent_rate, rpmi_uint64_t *new_rate)
{
	return 0;
}

static struct rpmi_clock_platform_ops k2_os0_clock_pops = {
	.set_state = spacemit_set_state,
	.get_state_and_rate = spacemit_get_state_and_rate,
	.rate_change_match = spacemit_rate_change_match,
	.set_rate = spacemit_set_rate,
	.set_rate_recalc = spacemit_set_rate_recalc,
};

static struct spacemit_rpmi_clk_ops k2_os0_clk_ops = {
	.name = "k2-os0-rpmi-clock",
	.init = _k2_os0_clock_init,
	.clk_ops = &k2_os0_clock_pops,
};

static int k2_os0_clk_init(void)
{

	rt_list_init(&k2_os0_clk_ops.list);

	spacemit_rpmi_clk_register(&k2_os0_clk_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k2_os0_clk_init);
