/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#ifdef RT_USING_ARMSCP_MODULE
#include <fwk_arch.h>
#endif

extern unsigned char __bss_end__[];

#if defined(SOC_SPACEMIT_K1_X) && defined(RT_USING_OPENAMP)
#include <platform_info.h>
#endif

#if defined(RT_USING_CLK) && defined(RT_USING_MUTEX)
struct rt_mutex clk_prepare_mutex;
struct rt_mutex of_clk_mutex;
struct rt_mutex clocks_mutex;
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

/**
 * This function will initial smart-evb board.
 */
void rt_hw_board_init(void)
{
#ifdef RT_USING_HEAP
#if defined(SOC_SPACEMIT_K1_X)
#if defined(RT_USING_OPENAMP)
    rt_system_heap_init((void *)RT_HEAP_START, (void *)RT_HEAP_END);
#else
    rt_system_heap_init((void *)__bss_end__, (void *)(0x40000));
#endif
#endif
#endif

    device_tree_setup((void *)RT_FDT_BASE);

#ifdef RT_USING_RADIX_TREE
    radix_tree_init_maxindex();
#endif

#if defined(RT_USING_CLK) && defined(RT_USING_MUTEX)
    rt_mutex_init(&clk_prepare_mutex, "clk_prepare_mutex", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&of_clk_mutex, "of_clk_mutex", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&clocks_mutex, "clk_mutex", RT_IPC_FLAG_PRIO);
    of_fixed_clk_setup();
    spacemit_ccu_init();
#endif

#if defined(RT_USING_PIN) && defined(RT_USING_MUTEX)
    rt_mutex_init(&pinctrldev_list_mutex, "gpindev_mut", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&pinctrl_list_mutex, "gpin_mut", RT_IPC_FLAG_PRIO);
    rt_mutex_init(&pinctrl_maps_mutex, "gpin_maps", RT_IPC_FLAG_PRIO);
    spacemit_pcs_init();
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
