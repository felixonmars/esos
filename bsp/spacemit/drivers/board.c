/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include <dtb_node.h>
#include <rtdevice.h>
#include <spacemit_sdk_soc.h>
#include <register_defination.h>
#ifdef RT_USING_ARMSCP_MODULE
#include <fwk_arch.h>
#endif

extern unsigned char __bss_end__[];

#if defined(RT_USING_CLK) && defined(RT_USING_MUTEX)
struct rt_mutex clk_prepare_mutex;
struct rt_mutex of_clk_mutex;
struct rt_mutex clocks_mutex;

struct rt_spinlock enable_lock;

extern int of_fixed_clk_setup(void);
extern int spacemit_ccu_init(void);
#endif

#ifdef RT_USING_RADIX_TREE
extern void radix_tree_init_maxindex(void);
#endif

#if defined(RT_USING_PIN) && defined(RT_USING_MUTEX)
struct rt_mutex pinctrldev_list_mutex;
struct rt_mutex pinctrl_list_mutex;
struct rt_mutex pinctrl_maps_mutex;
extern int spacemit_pcs_init(void);
#endif

#if defined(RT_USING_GPIO)
struct rt_spinlock gpio_lock;
extern int spacemit_gpio_init(void);
#endif

#if defined(RT_USING_LIBRPMI)
struct rt_mutex rpmi_hsm_mtx;
struct rt_mutex rpmi_clk_mtx;
struct rt_mutex rpmi_voltage_mtx;
struct rt_mutex rpmi_device_power_mtx;
struct rt_mutex rpmi_rtc_mtx;
struct rt_mutex rpmi_pwrkey_mtx;
struct rt_mutex rpmi_sysreset_mtx;
struct rt_mutex rpmi_msi_mtx;
#endif

extern unsigned long __irf_start[];

void rt_hw_us_delay(rt_uint32_t us)
{
	rt_uint64_t _start;
	rt_uint64_t _delte;

	_start = SysTimer_GetLoadValue();

	_delte = (rt_uint64_t)us * (SOC_TIMER_FREQ) / 1000000;

	while ((SysTimer_GetLoadValue() - _start) < _delte)
		asm volatile ("nop");
}

static void rt_hw_core_frequency_set(void)
{
	struct clk *cpu_clk, *apb_clk, *axi_clk, *rcpu_clk;
	unsigned int cpu_frequency;
	unsigned int apb_frequency;
	unsigned int axi_frequency;
	unsigned int rcpu_frequency;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	struct dtb_node *cpus = dtb_node_get_dtb_node_by_path(dtb_head_node, "/cpus/cpu@0");

	dtb_node_read_u32(cpus, "cpu_frequency", &cpu_frequency);

	cpu_clk = of_clk_get(cpus, 0);
	if (IS_ERR(cpu_clk))
		return;

	/* set the cpu clk */
	clk_prepare_enable(cpu_clk);
	clk_set_rate(cpu_clk, cpu_frequency);

	/* set the bus & axi clk */
	apb_clk = of_clk_get(cpus, 1);
	if (IS_ERR(apb_clk))
		return;

	axi_clk = of_clk_get(cpus, 2);
	if (IS_ERR(axi_clk))
		return;

	rcpu_clk = of_clk_get(cpus, 3);
	if (IS_ERR(rcpu_clk))
		return;

	/* get the cpu_frequency */
	dtb_node_read_u32(cpus, "apb_frequency", &apb_frequency);
	dtb_node_read_u32(cpus, "axi_frequency", &axi_frequency);
	dtb_node_read_u32(cpus, "rcpu_frequency", &rcpu_frequency);

	clk_prepare_enable(cpu_clk);
	clk_prepare_enable(apb_clk);
	clk_prepare_enable(axi_clk);
	clk_prepare_enable(rcpu_clk);


	clk_set_rate(rcpu_clk, rcpu_frequency);
	clk_set_rate(apb_clk, apb_frequency);
	clk_set_rate(axi_clk, axi_frequency);
}

/**
 * This function will initial smart-evb board.
 */
void rt_hw_board_init(void)
{
#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)RT_HEAP_START, (void *)RT_HEAP_END);
#endif

    device_tree_setup((void *)__irf_start);

#ifdef RT_USING_RADIX_TREE
    radix_tree_init_maxindex();
#endif

#if defined(RT_USING_CLK) && defined(RT_USING_MUTEX)
    rt_mutex_init(&clk_prepare_mutex, "clk_prepare_mutex", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&of_clk_mutex, "of_clk_mutex", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&clocks_mutex, "clk_mutex", RT_IPC_FLAG_PRIO);

    rt_spin_lock_init(&enable_lock);

    of_fixed_clk_setup();
    spacemit_ccu_init();

    rt_hw_core_frequency_set();
#endif


#if defined(RT_USING_PIN) && defined(RT_USING_MUTEX)
    rt_mutex_init(&pinctrldev_list_mutex, "gpindev_mut", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&pinctrl_list_mutex, "gpin_mut", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&pinctrl_maps_mutex, "gpin_maps", RT_IPC_FLAG_PRIO);
    spacemit_pcs_init();
#endif

#if defined(RT_USING_GPIO)
    rt_spin_lock_init(&gpio_lock);
    spacemit_gpio_init();
#endif

#if defined(RT_USING_LIBRPMI)
    rt_mutex_init(&rpmi_hsm_mtx, "rpmi_hsm_mtx", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&rpmi_clk_mtx, "rpmi_clk_mtx", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&rpmi_voltage_mtx, "rpmi_vol_mtx", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&rpmi_device_power_mtx, "rpmi_pm_mtx", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&rpmi_rtc_mtx, "rpmi_rtc_mtx", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&rpmi_pwrkey_mtx, "rpmi_pwrkey_mtx", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&rpmi_sysreset_mtx, "rpmi_sysreset_mtx", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&rpmi_msi_mtx, "rpmi_msi_mtx", RT_IPC_FLAG_PRIO);
#endif
    /* uart must be initialize here */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#ifdef RT_USING_ARMSCP_MODULE
    /* this framework will enable the tick timer */
    fwk_arch_init();
#endif
}

/*@}*/
