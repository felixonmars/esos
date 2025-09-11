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

#define D2_ENTER_EN_IRQ_NUM	91
#define D2_WAKEUP_EN_IRQ_NUM	92
// #define CSR_MSCRATCH		(0x340)
// #define CSR_MIE		(0x304)
// #define CSR_MTVEC		(0x305)

static int __suspend_asm_finish(rt_ubase_t arg, rt_ubase_t entry, rt_ubase_t context)
{
	audio_pmu_vote_t *lpvote = (audio_pmu_vote_t *)AUDIO_PMU_VOTE_REG;
	audio_vote_for_main_mpu_t *vmp = (audio_vote_for_main_mpu_t *)AUDIO_VOTE_FOR_MAIN_PMU;
	soc_top_d2_lp_ctrl *sdc = (soc_top_d2_lp_ctrl *)SOC_TOP_D2_LP_CTRL;
	audio_wakeup_en_t *dwkup = (audio_wakeup_en_t *)AUDIO_WAKEUP_EN_REG;
	rt24_core0_idle_cfg *idle_cfg = (rt24_core0_idle_cfg *)(read_csr(mhartid) ?
					(void *)RT24_CORE1_IDLE_CFG_REG : (void *)RT24_CORE0_IDLE_CFG_REG);

	lpvote->bits.vote_for_lp = 1;
	lpvote->bits.vote_for_plloff = 1;
	*((unsigned int *)PWRCTL_LP_WAKEUP_MASK) = 0;

	if (read_csr(mhartid) == 0) {
		vmp->bits.audio_pmu_vote_vctcxosd = 1;
		vmp->bits.audio_pmu_vote_ddrsd = 1;
		vmp->bits.audio_pmu_vote_axisd = 1;
		vmp->bits.audio_pmu_vote_stben = 1;
		vmp->bits.audio_pmu_vote_slpen = 1;
		/* enable d2 wakeup enable */
		dwkup->bits.d2_exit_en = 1;
	}

	if (read_csr(mhartid) == 0) {
		/* set the rcpu entery point */
		writel(entry & 0xffffffff, (void *)RCPU_CORE0_BOOT_ENTRY_LO);
		writel((entry >> 32) & 0xffffffff, (void *)RCPU_CORE0_BOOT_ENTRY_HI);
	} else {
		writel(entry & 0xffffffff, (void *)RCPU_CORE1_BOOT_ENTRY_LO);
		writel((entry >> 32) & 0xffffffff, (void *)RCPU_CORE1_BOOT_ENTRY_HI);
	}

	/* let core powrdown */
	idle_cfg->bits.core_idle = 1;
	idle_cfg->bits.core_pwrdwn = 1;

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
	context->mscratch = read_csr(0x340);
	context->mie = read_csr(0x304);
	context->mtvec = read_csr(0x305);
}

static void suspend_restore_csrs(struct suspend_context *context)
{
	write_csr(0x340, context->mscratch);
	write_csr(0x304, context->mtvec);
	write_csr(0x305, context->mie);
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
	audio_pmu_vote_t *lpvote;
	audio_vote_for_main_mpu_t *vmp;
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

		lpvote = (audio_pmu_vote_t *)AUDIO_PMU_VOTE_REG;
		vmp = (audio_vote_for_main_mpu_t *)AUDIO_VOTE_FOR_MAIN_PMU;

		/* save the plic configuration */
		rt_hw_eclic_save();

		/* disable the clint timer */
		time = SysTimer_GetLoadValue();
		SysTimer_SetCompareValue(0xffffffffffffffff);
		/* clear the timer pending */
		clear_csr(mip, MIP_MTIP);

		cpu_suspend(0, __suspend_asm_finish);

		/* Do fence and fence.i to make sure previous ilm/dlm/icache/dcache control done */
		asm volatile ("fence iorw, iorw");
		asm volatile ("fence.i");

		/* enable the clint timer */
		SysTimer_SetCompareValue(time);

		/* restore the plic configuration */
		rt_hw_eclic_restore();

		if (read_csr(mhartid) == 0) {
			vmp->bits.audio_pmu_vote_vctcxosd = 0;
			vmp->bits.audio_pmu_vote_ddrsd = 0;
			vmp->bits.audio_pmu_vote_axisd = 0;
			vmp->bits.audio_pmu_vote_stben = 0;
			vmp->bits.audio_pmu_vote_slpen = 0;
		} else {
			*((unsigned int *)PWRCTL_LP_WAKEUP_MASK) = 1;
		}

		lpvote->bits.vote_for_lp = 0;
		lpvote->bits.vote_for_plloff = 0;
		lpvote->bits.vote_for_pwroff = 0;

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

static void spacemit_d2_wakeup(rt_int32_t irq, void *dev_id)
{
	rt_uint32_t val;
	soc_top_d2_lp_ctrl *sdc = (soc_top_d2_lp_ctrl *)SOC_TOP_D2_LP_CTRL;
	audio_wakeup_en_t *dwkup = (audio_wakeup_en_t *)AUDIO_WAKEUP_EN_REG;

	/* clear the wakeup en pending, then the ap will wakeup */
	sdc->bits.clr_soc_top_d2_wakeup_hw_mask = 1;
	for (int i = 0; i < 10; ++i) {};
	sdc->bits.clr_soc_top_d2_wakeup_hw_mask = 0;

	while (1) {
		val = readl((unsigned int *)RT24_PMU_STATUS);
		if (((val >> 8) & 0xf) == 0xd)
			break;
	}

	/* disable en wakeup interrupt */
	dwkup->bits.d2_exit_en = 0;

	return;
}

static void spacemit_d2_enter(rt_int32_t irq, void *dev_id)
{
	rt_uint32_t val;
	soc_top_d2_lp_ctrl *sdc = (soc_top_d2_lp_ctrl *)SOC_TOP_D2_LP_CTRL;
	audio_wakeup_en_t *dwkup = (audio_wakeup_en_t *)AUDIO_WAKEUP_EN_REG;

	val = readl((unsigned int *)RT24_PMU_STATUS);
	if (((val >> 8) & 0xf) != 0x6)
		rt_kprintf("%s:%d, soc enter D2 error\n", __func__, __LINE__);

	/* clear the pending */
	sdc->bits.soc_top_d2_enter_int_clr = 1;

	while (sdc->bits.soc_top_d2_enter_int_status);

	sdc->bits.soc_top_d2_enter_int_clr = 0;

	return;
}

int rt_hw_k3_pm_init(void)
{
	int ret;
	rt_uint8_t timer_mask = 0;
	audio_vote_for_main_mpu_t *vmp =
		(audio_vote_for_main_mpu_t *)AUDIO_VOTE_FOR_MAIN_PMU;
	audio_pmu_vote_t *lpvote = (audio_pmu_vote_t *)AUDIO_PMU_VOTE_REG;
	soc_top_d2_lp_ctrl *sdc = (soc_top_d2_lp_ctrl *)SOC_TOP_D2_LP_CTRL;
	audio_wakeup_en_t *dwkup = (audio_wakeup_en_t *)AUDIO_WAKEUP_EN_REG;

	static const struct rt_pm_ops _ops = {
		sleep,
		run,
		pm_timer_start,
		pm_timer_stop,
		pm_timer_get_tick
	};

	if (read_csr(mhartid) == 0) {
		vmp->bits.audio_pmu_vote_vctcxosd = 0;
		vmp->bits.audio_pmu_vote_ddrsd = 0;
		vmp->bits.audio_pmu_vote_axisd = 0;
		vmp->bits.audio_pmu_vote_stben = 0;
		vmp->bits.audio_pmu_vote_slpen = 0;

		/* enable rcpu control soc top d2 low power en by default */
		sdc->bits.rcpu_ctrl_soc_top_d2_lp_en = 1;
		/* enable d2 enter interrupt by default*/
		dwkup->bits.d2_enter_en = 1;

		/* register d2 wakeup en interrupt */
		rt_hw_interrupt_install(D2_WAKEUP_EN_IRQ_NUM, spacemit_d2_wakeup, RT_NULL, "d2_wkup");
		rt_hw_interrupt_umask(D2_WAKEUP_EN_IRQ_NUM);

		/* register d2 enter interrupt */
		rt_hw_interrupt_install(D2_ENTER_EN_IRQ_NUM, spacemit_d2_enter, RT_NULL, "d2_enter");
		rt_hw_interrupt_umask(D2_ENTER_EN_IRQ_NUM);
	}

	lpvote->bits.vote_for_lp = 0;
	lpvote->bits.vote_for_plloff = 0;
	lpvote->bits.vote_for_pwroff = 0;


	/* initialize timer mask */
	/* timer_mask = 1UL << PM_SLEEP_MODE_DEEP; */

	/* initialize system pm module */
	rt_system_pm_init(&_ops, timer_mask, RT_NULL);

	return 0;
}

INIT_APP_EXPORT(rt_hw_k3_pm_init);
