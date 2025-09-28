/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <drivers/pm.h>
#include <rtconfig.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <riscv_sleep.h>
#include <core_feature_base.h>
#include <spacemit_sdk_soc.h>
#include <register_defination.h>
#include <openamp/rpmsg.h>

static rt_thread_t trigger_tid;
struct rpmsg_endpoint lpwept;
extern struct rpmsg_device *rpdev;
extern unsigned char __resource_table_start__[];

#define RPMSG_LOW_PWR_SERV_NAME         "rcpu-pwr-management-service"

static int __suspend_asm_finish(rt_ubase_t arg, rt_ubase_t entry, rt_ubase_t context)
{
	audio_pmu_vote_t *lpvote = (audio_pmu_vote_t *)AUDIO_PMU_VOTE_REG;

	/* flush dcache all */
	MFlushInvalDCache();

	/* tell the Big-cpu that we have complete: using the reserved address of resource table, which defined in file k1-rproc.c
	 *
	 *struct remote_resource_table __resource resources[1][1] = {
	 *	{
	 *		{
	 *			// Version
 	 *			1,
	 *
	 *			// NUmber of table entries
	 *			NUM_TABLE_ENTRIES,
	 *
	 *			// reserved fields
	 *			{ 0, 0, },
	 *
	 *			// Offsets of rsc entries
	 *			{
	 *				 offsetof(struct remote_resource_table, rpmsg_vdev),
	 *			},
	 *
	 *			// Virtio device entry
	 *			{
	 *				RSC_VDEV, VIRTIO_ID_RPMSG_, 0, RPMSG_IPU_C0_FEATURES, 0, 0, 0,
	 *				NUM_VRINGS, {0, 0},
	 *			},
	 *
	 *			// Vring rsc entry - part of vdev rsc entry
	 *			{ RING_TX, VRING_ALIGN, VRING_SIZE, 0, 0 },
	 *
	 *			{ RING_RX, VRING_ALIGN, VRING_SIZE, 1, 0 },
	 *		},
	 *	},
	* }; */

	*((rt_uint32_t *)((unsigned int)__resource_table_start__ + 8)) = 1;

	/* flush dcache all */
	MFlushInvalDCache();

	lpvote->bits.vote_for_lp = 1;
	lpvote->bits.vote_for_plloff = 1;
	lpvote->bits.vote_for_pwroff = 1;
	*((unsigned int *)PWRCTL_LP_WAKEUP_MASK) = 0;

	/* enter wfi */
	while (1) {
		__ISB();
		__DSB();
		__WFI();
		__ISB();
		__DSB();
	}

	/* should never be here */
	return RT_EOK;
}

static void suspend_save_csrs(struct suspend_context *context)
{
        context->mscratch = __RV_CSR_READ(CSR_MSCRATCH);
        context->mie = __RV_CSR_READ(CSR_MIE);

        context->mmisc_ctl = __RV_CSR_READ(CSR_MMISC_CTL);
        context->mtvt = __RV_CSR_READ(CSR_MTVT);
        context->mtvt2 = __RV_CSR_READ(CSR_MTVT2);
        context->mtvec = __RV_CSR_READ(CSR_MTVEC);
}

static void suspend_restore_csrs(struct suspend_context *context)
{
        __RV_CSR_WRITE(CSR_MSCRATCH, context->mscratch);
        __RV_CSR_WRITE(CSR_MTVEC, context->mtvec);
        __RV_CSR_WRITE(CSR_MTVT2, context->mtvt2);
        __RV_CSR_WRITE(CSR_MTVT, context->mtvt);
        __RV_CSR_WRITE(CSR_MMISC_CTL, context->mmisc_ctl);
        __RV_CSR_WRITE(CSR_MIE, context->mie);
}

extern int __cpu_suspend_enter(rt_ubase_t context);
extern int __cpu_resume_enter(rt_ubase_t context);

int cpu_suspend(rt_ubase_t arg,
                int (*finish)(rt_ubase_t arg,
                              rt_ubase_t entry,
                              rt_ubase_t context))
{
	int rc = 0;
	struct suspend_context context = { 0 };

	/* Finisher should be non-NULL */
	if (!finish)
		return -RT_EINVAL;

	/* Save additional CSRs*/
	suspend_save_csrs(&context);

	/* Save context on stack */
	if (__cpu_suspend_enter((unsigned long)&context)) {
		/* Call the finisher */
		rc = finish(arg, (rt_ubase_t)__cpu_resume_enter, (rt_ubase_t)&context);

		/*
		 * Should never reach here, unless the suspend finisher
		 * fails. Successful cpu_suspend() should return from
		 * __cpu_resume_entry()
		 */
		if (!rc)
			rc = -RT_EINVAL;
	}

	/* Restore additional CSRs */
	suspend_restore_csrs(&context);

	return rc;
}

extern void ECLIC_Init(void);
extern void vPortSetupTimerInterrupt(void);
extern void rt_hw_eclic_save(void);
extern void rt_hw_eclic_restore(void);

/**
 * This function will put n308 into sleep mode.
 *
 * @param pm pointer to power manage structure
 */
static void sleep(struct rt_pm *pm, uint8_t mode)
{
	audio_pmu_vote_t *lpvote;
	audio_vote_for_main_mpu_t *vmp;

	switch (mode)
	{
	case PM_SLEEP_MODE_NONE:
	break;

	case PM_SLEEP_MODE_IDLE:
	break;

	case PM_SLEEP_MODE_LIGHT:
	break;

	case PM_SLEEP_MODE_DEEP:

		lpvote = (audio_pmu_vote_t *)AUDIO_PMU_VOTE_REG;
		vmp = (audio_vote_for_main_mpu_t *)AUDIO_VOTE_FOR_MAIN_PMU;

		/* enter lower power mode */
		rt_hw_eclic_save();

		cpu_suspend(0, __suspend_asm_finish);

		/* enable ICACHE & DCACHE */
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

		/* initialize the eclic again */
		ECLIC_Init();

		rt_hw_eclic_restore();

		/* initialize the sw interrupt again */
		vPortSetupTimerInterrupt();

		/* enable systimer */
		ECLIC_EnableIRQ(SysTimer_IRQn);

		vmp->bits.audio_pmu_vote_vctcxosd = 0;
		vmp->bits.audio_pmu_vote_ddrsd = 0;
		vmp->bits.audio_pmu_vote_axisd = 0;
		vmp->bits.audio_pmu_vote_stben = 0;
		vmp->bits.audio_pmu_vote_slpen = 0;
		lpvote->bits.vote_for_lp = 0;
		lpvote->bits.vote_for_plloff = 0;
		lpvote->bits.vote_for_pwroff = 0;

		*((unsigned int *)PWRCTL_LP_WAKEUP_MASK) = 1;

		rt_pm_request(RT_PM_DEFAULT_SLEEP_MODE);

		/* using the reserved address of resource table to tell the big-cpu that we have complete resume */
		*((rt_uint32_t *)((unsigned int)__resource_table_start__ + 8)) = 2;

		/* flush dcache all */
		MFlushInvalDCache();
	break;

	case PM_SLEEP_MODE_STANDBY:
	break;

	case PM_SLEEP_MODE_SHUTDOWN:
	break;

	default:
		RT_ASSERT(0);
	break;
	}
}

static void run(struct rt_pm *pm, uint8_t mode)
{
}

/**
 * This function start the timer of pm
 *
 * @param pm Pointer to power manage structure
 * @param timeout How many OS Ticks that MCU can sleep
 */
static void pm_timer_start(struct rt_pm *pm, rt_uint32_t timeout)
{
	RT_ASSERT(pm != RT_NULL);
	RT_ASSERT(timeout > 0);
}

/**
 * This function stop the timer of pm
 *
 * @param pm Pointer to power manage structure
 */
static void pm_timer_stop(struct rt_pm *pm)
{
	RT_ASSERT(pm != RT_NULL);
}

/**
 * This function calculate how many OS Ticks that MCU have suspended
 *
 * @param pm Pointer to power manage structure
 *
 * @return OS Ticks
 */
static rt_tick_t pm_timer_get_tick(struct rt_pm *pm)
{
	return 0;
}

static int rpmsg_lpw_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	if (strcmp(data, "pwr_management") == 0) {
		rpmsg_send(ept, "pwr_management_ok", 17);
		return 0;
	}

	/* Enter sleep mode */
	/* 1. release the DEFAULT_SLEEP_MODE */
	rt_pm_release(RT_PM_DEFAULT_SLEEP_MODE);
	
	return 0;
}

static void rpmsg_lpw_service_unbind(struct rpmsg_endpoint *ept)
{
	/* do nothing */
}

static void pm_creat_endpoint(void *parameter)
{
	int ret;

	/* wait for rproc dev ready */
	while (1) {
		if (!rpdev)
			rt_thread_delay(2);
		else
			break;
	}

	/* create lowpower mode endpoint */
	ret = rpmsg_create_ept(&lpwept, rpdev, RPMSG_LOW_PWR_SERV_NAME,
			RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			rpmsg_lpw_endpoint_cb, rpmsg_lpw_service_unbind);
	if (ret) {
		rt_kprintf("Failed to create endpoint\n");
		return;
	}
}

int rt_hw_k1x_pm_init(void)
{
	rt_uint8_t timer_mask = 0;
	audio_vote_for_main_mpu_t *vmp =
		(audio_vote_for_main_mpu_t *)AUDIO_VOTE_FOR_MAIN_PMU;
	audio_pmu_vote_t *lpvote = (audio_pmu_vote_t *)AUDIO_PMU_VOTE_REG;

	static const struct rt_pm_ops _ops = {
		sleep,
		run,
		pm_timer_start,
		pm_timer_stop,
		pm_timer_get_tick
	};


	vmp->bits.audio_pmu_vote_vctcxosd = 0;
	vmp->bits.audio_pmu_vote_ddrsd = 0;
	vmp->bits.audio_pmu_vote_axisd = 0;
	vmp->bits.audio_pmu_vote_stben = 0;
	vmp->bits.audio_pmu_vote_slpen = 0;
	lpvote->bits.vote_for_lp = 0;
	lpvote->bits.vote_for_plloff = 0;
	lpvote->bits.vote_for_pwroff = 0;

	/* initialize timer mask */
	/* timer_mask = 1UL << PM_SLEEP_MODE_DEEP; */

	/* initialize system pm module */
	rt_system_pm_init(&_ops, timer_mask, RT_NULL);

	/* create the rpmsg trigger irq thread */
	trigger_tid = rt_thread_create("pm_create_endpoint",
			pm_creat_endpoint,
			NULL,
			2048,
			RT_THREAD_PRIORITY_MAX / 5,
			20);
	if (!trigger_tid) {
		rt_kprintf("Failed to create pm thread\n");
		return -1;
	}

	rt_thread_startup(trigger_tid);

	return 0;
}

INIT_APP_EXPORT(rt_hw_k1x_pm_init);
