#include <rthw.h>
#include <drivers/pm.h>
#include <rtconfig.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <riscv_sleep.h>
#include <riscv-ops.h>
#include <riscv_encoding.h>
#include <clint.h>
#include <stdlib.h>
#include <spacemit_sdk_soc.h>
#include <register_defination.h>

extern unsigned long __esos_lite_start[], __esos_lite_end[];

static int __suspend_asm_finish(rt_ubase_t arg, rt_ubase_t entry, rt_ubase_t context)
{
	unsigned int val;
	typedef void (*__entry)(void *);
	__entry ptr;
	rt24_core0_idle_cfg *idle_cfg = (rt24_core0_idle_cfg *)RT24_CORE1_IDLE_CFG_REG;

	if (read_csr(mhartid) == 0) {
		rt_memcpy((void *)0x0, (void *)__esos_lite_start,
				(unsigned long)__esos_lite_end - (unsigned long)__esos_lite_start);
		/* vote per */
		asm volatile ("fence.i");

		/* jump to sram */
		ptr = (__entry)0x0;
		ptr((void *)entry);
	} else {
		writel(entry & 0xffffffff, (void *)RCPU_CORE1_BOOT_ENTRY_LO);
		writel((entry >> 32) & 0xffffffff, (void *)RCPU_CORE1_BOOT_ENTRY_HI);

		idle_cfg->bits.core_idle = 1;
		idle_cfg->bits.core_pwrdwn = 1;
	}


	/* enter wfi */
	while (1) {
		asm volatile ("fence");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile ("wfi");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
		asm volatile("nop");
	}

	/* should never be here */
	return RT_EOK;
}

static void suspend_save_csrs(struct suspend_context *context)
{
	context->mscratch = read_csr(mscratch);
	context->mie = read_csr(mie);
	context->mtvec = read_csr(mtvec);
}

static void suspend_restore_csrs(struct suspend_context *context)
{
	write_csr(mscratch, context->mscratch);
	write_csr(mtvec, context->mtvec);
	write_csr(mie, context->mie);
}

extern int __cpu_suspend_enter(rt_ubase_t context);
extern int __cpu_resume_enter(rt_ubase_t context);

struct suspend_context context = { 0 };

int cpu_suspend(rt_ubase_t arg,
                int (*finish)(rt_ubase_t arg,
                              rt_ubase_t entry,
                              rt_ubase_t context))
{
	int rc = 0;

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

extern void rt_hw_eclic_save(void);
extern void rt_hw_eclic_restore(void);

/**
 * This function will put n308 into sleep mode.
 *
 * @param pm pointer to power manage structure
 */
static void sleep(struct rt_pm *pm, uint8_t mode)
{
	rt24_core0_idle_cfg *idle_cfg = (rt24_core0_idle_cfg *)(read_csr(mhartid) ?
			(void *)RT24_CORE1_IDLE_CFG_REG : (void *)RT24_CORE0_IDLE_CFG_REG);
	rt_uint64_t time;

	switch (mode)
	{
	case PM_SLEEP_MODE_NONE:
	break;

	case PM_SLEEP_MODE_IDLE:
	break;

	case PM_SLEEP_MODE_LIGHT:
	break;

	case PM_SLEEP_MODE_DEEP:
		/* save the plic configuration */
		rt_hw_eclic_save();

		/* disable the clint timer */
		time = SysTimer_GetLoadValue();
		SysTimer_SetCompareValue(0xffffffffffffffff);
		/* clear the timer pending */
		clear_csr(mip, MIP_MTIP);

		cpu_suspend(0, __suspend_asm_finish);

		/* enable the clint timer */
		SysTimer_SetCompareValue(time);

		/* restore the plic configuration */
		rt_hw_eclic_restore();

		/* devote core powrdown */
		idle_cfg->bits.core_idle = 0;
		idle_cfg->bits.core_pwrdwn = 0;

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

int rt_hw_k3_pm_init(void)
{
	int ret;
	unsigned int value;
	rt_uint8_t timer_mask = 0;
	audio_pmu_vote_t *lpvote = (audio_pmu_vote_t *)AUDIO_PMU_VOTE_REG;

	static const struct rt_pm_ops _ops = {
		sleep,
		run,
		pm_timer_start,
		pm_timer_stop,
		pm_timer_get_tick
	};

	if (read_csr(mhartid) == 0) {
		/* clear the vote registers before system low power mode */
		writel(0x0, (unsigned int *)AUDIO_VOTE_FOR_MAIN_PMU);

		lpvote->bits.vote_for_clk_off = 0;
		lpvote->bits.vote_for_plloff = 0;

		writel(0, (unsigned int *)APCR_PER_VETE_REG);
	}

	/* initialize timer mask */
	/* timer_mask = 1UL << PM_SLEEP_MODE_DEEP; */

	/* initialize system pm module */
	rt_system_pm_init(&_ops, timer_mask, RT_NULL);

	return 0;
}
INIT_APP_EXPORT(rt_hw_k3_pm_init);
