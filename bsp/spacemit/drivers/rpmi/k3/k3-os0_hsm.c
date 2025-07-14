#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include "../spacemit-rpmi.h"


static enum rpmi_hart_hw_state k3_hsm_get_hw_state(void* priv,
	rpmi_uint32_t hart_index)
{
	return 0;
}

static enum rpmi_error k3_hsm_hart_start_prepare(void* priv,
	rpmi_uint32_t hart_index,
	rpmi_uint64_t start_addr)
{
	return 0;
}

static void k3_hsm_hart_start_finalize(void* priv,
	rpmi_uint32_t hart_index,
	rpmi_uint64_t start_addr)
{
}

static enum rpmi_error k3_hsm_hart_stop_prepare(void* priv,
	rpmi_uint32_t hart_index)
{
	return 0;
}

static void k3_hsm_hart_stop_finalize(void* priv, rpmi_uint32_t hart_index)
{

}

static enum rpmi_error k3_hsm_hart_suspend_prepare(
	void* priv,
	rpmi_uint32_t hart_index,
	const struct rpmi_hsm_suspend_type* suspend_type,
	rpmi_uint64_t resume_addr)
{
	return 0;
}

static void k3_hsm_hart_suspend_finalize(
	void* priv,
	rpmi_uint32_t hart_index,
	const struct rpmi_hsm_suspend_type* suspend_type,
	rpmi_uint64_t resume_addr)
{

}

struct rpmi_hsm_platform_ops k3_os0_hsm_pops = {
	.hart_get_hw_state = k3_hsm_get_hw_state,
	.hart_start_prepare = k3_hsm_hart_start_prepare,
	.hart_start_finalize = k3_hsm_hart_start_finalize,
	.hart_stop_prepare = k3_hsm_hart_stop_prepare,
	.hart_stop_finalize = k3_hsm_hart_stop_finalize,
	.hart_suspend_prepare = k3_hsm_hart_suspend_prepare,
	.hart_suspend_finalize = k3_hsm_hart_suspend_finalize
};

static rt_int32_t _k3_os0_hsm_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	return 0;
}

/* System suspend platform operations (stubs for hardware-related functions) */
static enum rpmi_error syssusp_prepare(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	return 0;
}

static rpmi_bool_t syssusp_ready(void* priv, rpmi_uint32_t hart_index)
{
	return 0;
}

static void syssusp_finalize(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{

}

static rpmi_bool_t syssusp_can_resume(void* priv, rpmi_uint32_t hart_index)
{
	return 0;
}

static enum rpmi_error syssusp_resume(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	return 0;
}

/* System suspend platform operations */
static struct rpmi_syssusp_platform_ops k3_os0_syssup_ops = {
	.system_suspend_prepare = syssusp_prepare,
	.system_suspend_ready = syssusp_ready,
	.system_suspend_finalize = syssusp_finalize,
	.system_suspend_can_resume = syssusp_can_resume,
	.system_suspend_resume = syssusp_resume
};

static struct spacemit_rpmi_hsm_ops k3_os0_hsm_ops = {
	.name = "k3-os0-rpmi-hsm",
	.init = _k3_os0_hsm_init,
	.hsm_ops = &k3_os0_hsm_pops,
	.syssup_ops = &k3_os0_syssup_ops,
};

static rt_int32_t k3_os0_hsm_init(void)
{

	rt_list_init(&k3_os0_hsm_ops.list);

	spacemit_rpmi_hsm_register(&k3_os0_hsm_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_hsm_init);
