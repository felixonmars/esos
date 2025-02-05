/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtdef.h>
#include <spacemit_sdk_soc.h>
#include <core_feature_base.h>
#include <register_defination.h>

#define SOC_TIMER_FREQ	(32768 * 4)

#define SYSTICK_TICK_CONST	(SOC_TIMER_FREQ / RT_TICK_PER_SECOND)

#ifndef configTIMER_INTERRUPT_PRIORITY
#define configTIMER_INTERRUPT_PRIORITY         0
#endif

#ifndef configSWI_INTERRUPT_PRIORITY
#define configSWI_INTERRUPT_PRIORITY         0
#endif

#define SysTick_Handler     eclic_mtip_handler

/* This is the timer interrupt service routine. */
/* void SysTick_Handler(void) */
void SysTick_Handler(int vector, void *param)
{
    // Reload timer
    SysTick_Reload(SYSTICK_TICK_CONST);

    /* enter interrupt */
    /* rt_interrupt_enter(); */

    /* tick increase */
    rt_tick_increase();

    /* leave interrupt */
    /* rt_interrupt_leave(); */
}


void vPortSetupTimerInterrupt(void)
{
    uint64_t ticks = SYSTICK_TICK_CONST;

    /* Make SWI and SysTick the lowest priority interrupts. */
    /* Stop and clear the SysTimer. SysTimer as Non-Vector Interrupt */
    SysTick_Config(ticks);
    ECLIC_DisableIRQ(SysTimer_IRQn);
    ECLIC_SetLevelIRQ(SysTimer_IRQn, configTIMER_INTERRUPT_PRIORITY);
    ECLIC_SetShvIRQ(SysTimer_IRQn, ECLIC_NON_VECTOR_INTERRUPT);
    /* ECLIC_EnableIRQ(SysTimer_IRQn); */

    /* Set SWI interrupt level to lowest level/priority, SysTimerSW as Vector Interrupt */
    ECLIC_SetShvIRQ(SysTimerSW_IRQn, ECLIC_VECTOR_INTERRUPT);
    ECLIC_SetLevelIRQ(SysTimerSW_IRQn, configSWI_INTERRUPT_PRIORITY);
    ECLIC_EnableIRQ(SysTimerSW_IRQn);
}

/**
 * \brief initialize eclic config
 * \details
 * ECLIC needs be initialized after boot up,
 * Vendor could also change the initialization
 * configuration.
 */
void ECLIC_Init(void)
{
    /* Global Configuration about MTH and NLBits.
     * TODO: Please adapt it according to your system requirement.
     * This function is called in _init function */
    ECLIC_SetMth(0);
    ECLIC_SetCfgNlbits(__ECLIC_INTCTLBITS);
}

volatile IRegion_Info_Type SystemIRegionInfo;

#define FALLBACK_DEFAULT_ECLIC_BASE             0x0C000000UL
#define FALLBACK_DEFAULT_SYSTIMER_BASE          0x02000000UL

static void _get_iregion_info(IRegion_Info_Type *iregion)
{
    unsigned long mcfg_info;

    if (iregion == NULL) {
        return;
    }

    mcfg_info = __RV_CSR_READ(CSR_MCFG_INFO);

    if (mcfg_info & MCFG_INFO_IREGION_EXIST) { // IRegion Info present
        iregion->iregion_base = (__RV_CSR_READ(CSR_MIRGB_INFO) >> 10) << 10;
        iregion->eclic_base = iregion->iregion_base + IREGION_ECLIC_OFS;
        iregion->systimer_base = iregion->iregion_base + IREGION_TIMER_OFS;
        iregion->smp_base = iregion->iregion_base + IREGION_SMP_OFS;
        iregion->idu_base = iregion->iregion_base + IREGION_IDU_OFS;
    } else {
        iregion->eclic_base = FALLBACK_DEFAULT_ECLIC_BASE;
        iregion->systimer_base = FALLBACK_DEFAULT_SYSTIMER_BASE;
    }
}

extern void Exception_Init(void);

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
    _get_iregion_info((IRegion_Info_Type *)&SystemIRegionInfo);

#ifdef RT_USING_CACHE
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1)
    if (ICachePresent()) { // Check whether icache real present or not
        EnableICache();
    }
#endif

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1)
    if (DCachePresent()) { // Check whether dcache real present or not
        EnableDCache();
    }

#if defined(RT_USING_OPENAMP)
    /* set the vring & share memory to non-cacheable */
    /* base: SHARED_MEM_PA; size SHARED_MEM_SIZE */
    __RV_CSR_SET(CSR_MNOCM, 0xfff00000);
    __RV_CSR_SET(CSR_MNOCB, SHARED_MEM_PA | 0x1);
#endif
#endif
#endif

    /* Do fence and fence.i to make sure previous ilm/dlm/icache/dcache control done */
    __RWMB();
    __FENCE_I();

    Exception_Init();

    ECLIC_Init();

    rt_hw_interrupt_init();

    vPortSetupTimerInterrupt();

    /* set the ticktimer's irq handler */
    rt_hw_interrupt_install(SysTimer_IRQn, SysTick_Handler, NULL, NULL);

    /* enable systimer */
    ECLIC_EnableIRQ(SysTimer_IRQn);
}
