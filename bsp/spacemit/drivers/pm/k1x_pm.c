/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <riscv_sleep.h>
#include <core_feature_base.h>
#include <spacemit_sdk_soc.h>
#include <register_defination.h>
#include <platform_info.h>

extern unsigned long __text_start;
extern unsigned long _end;

static int __suspend_asm_finish(rt_ubase_t arg, rt_ubase_t entry, rt_ubase_t context)
{
	/* for n308 when rcpu resumed from poweroff, it will start to run at 0 address */
	/* copy the rcpu runtime snapshots */
	memcpy((void *)RCPU_RUNTIME_MEM_SNAPSHOT_BASE,
			(void *)&__text_start,
			(unsigned long)&_end - (unsigned long)&__text_start);

	/* using a alive register to store the context ? */

	/* flush dcache all */
	MFlushInvalDCache();

	/* enter wfi */
	__ISB();
	__DSB();
	while (1) {
		__WFI();
	}
	__ISB();
	__DSB();

	/* should never be here */
	return -RT_EINVAL;
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
	if (__cpu_suspend_enter(&context)) {
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
	audio_pmu_vote_t *lpvote = (audio_pmu_vote_t *)AUDIO_PMU_VOTE_REG;
	audio_vote_for_main_mpu_t *vmp =
		(audio_vote_for_main_mpu_t *)AUDIO_VOTE_FOR_MAIN_PMU;

	switch (mode)
	{
	case PM_SLEEP_MODE_NONE:
		/* enter wfi */
		lpvote->bits.vote_for_lp = 1;
		*((unsigned int *)PWRCTL_LP_WAKEUP_MASK) = 1;
		__ISB();
		__DSB();
		__WFI();
		__ISB();
		__DSB();
		lpvote->bits.vote_for_lp = 0;
	break;

	case PM_SLEEP_MODE_IDLE:
		/* enter wfi */
		lpvote->bits.vote_for_lp = 1;
		*((unsigned int *)PWRCTL_LP_WAKEUP_MASK) = 1;
		__ISB();
		__DSB();
		__WFI();
		__ISB();
		__DSB();
		lpvote->bits.vote_for_lp = 0;
	break;

	case PM_SLEEP_MODE_LIGHT:
	break;

	case PM_SLEEP_MODE_DEEP:
		/* enter lower power mode */
		vmp->bits.audio_pmu_vote_vctcxosd = 1;
		vmp->bits.audio_pmu_vote_ddrsd = 1;
		vmp->bits.audio_pmu_vote_axisd = 1;
		lpvote->bits.vote_for_lp = 1;
		lpvote->bits.vote_for_plloff = 1;
		lpvote->bits.vote_for_pwroff = 1;
		*((unsigned int *)PWRCTL_LP_WAKEUP_MASK) = 0;

		rt_hw_eclic_save();

		cpu_suspend(0, __suspend_asm_finish);

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
		lpvote->bits.vote_for_lp = 0;
		lpvote->bits.vote_for_plloff = 0;
		lpvote->bits.vote_for_pwroff = 0;

		/* 1. request the DEFAULT_SLEEP_MODE */
		rt_pm_request(RT_PM_DEFAULT_SLEEP_MODE);
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

int rt_hw_k1x_pm_init(void)
{
	audio_vote_for_main_mpu_t *vmp =
		(audio_vote_for_main_mpu_t *)AUDIO_VOTE_FOR_MAIN_PMU;

	vmp->bits.audio_pmu_vote_vctcxosd = 0;
	vmp->bits.audio_pmu_vote_ddrsd = 0;
	vmp->bits.audio_pmu_vote_axisd = 0;

	audio_wakeup_en_t *wkup_en = (audio_wakeup_en_t *)AUDIO_WAKEUP_EN_REG;

	wkup_en->bits.timer_wkup_en = 1;
	wkup_en->bits.ipc_ap_wkup_en = 1;

	static const struct rt_pm_ops _ops = {
		sleep,
		run,
		pm_timer_start,
		pm_timer_stop,
		pm_timer_get_tick
	};

	rt_uint8_t timer_mask = 0;

	/* initialize timer mask */
	/* timer_mask = 1UL << PM_SLEEP_MODE_DEEP; */

	/* initialize system pm module */
	rt_system_pm_init(&_ops, timer_mask, RT_NULL);

	return 0;
}

INIT_APP_EXPORT(rt_hw_k1x_pm_init);
