/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
 #include <drivers/pm.h>
#include <rtthread.h>
#include <dtb_head.h>
#include <riscv-ops.h>
#include "k3_hsm.h"
#include <register_defination.h>
#include "../spacemit-rpmi.h"

#define CPU_TO_CLUSTER(cpu)    ((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

#undef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER

struct rpmi_hsm_hart {
	/** Lock to protect this structure and perform platform operations */
	void *lock;

	/** Current HSM hart state */
	enum rpmi_hsm_hart_state state;

	/** Current hart start parameter */
	rpmi_uint64_t start_addr;

	/** Current hart suspend parameter */
	const struct rpmi_hsm_suspend_type *suspend_type;
	rpmi_uint64_t resume_addr;
};

struct rpmi_hsm {
	/** Whether HSM instance is non-leaf (or hierarchical) instance */
	rpmi_bool_t is_non_leaf;

	union {
		/** Details required by leaf instance */
		struct {
			/** Number of harts */
			rpmi_uint32_t hart_count;

			/** Array of hart IDs */
			const rpmi_uint32_t *hart_ids;

			/** Array of harts */
			struct rpmi_hsm_hart *harts;

			/** Number of suspend types */
			rpmi_uint32_t suspend_type_count;

			/** Array of suspend types */
			const struct rpmi_hsm_suspend_type *suspend_types;

			/**
			 * Platform HSM operations
			 *
			 * Note: These operations are called with harts[i]->lock held
			 */
			const struct rpmi_hsm_platform_ops *ops;

			/** Private data of platform HSM operations */
			void *ops_priv;
		} leaf;

		/** Details required by non-leaf instance */
		struct {
			/** Number of child instances */
			rpmi_uint32_t child_count;

			/** Array of child instance pointers */
			struct rpmi_hsm **child_array;
		} nonleaf;
	};
};

static void __m2_enter(clusterx_m2_lp_ctrl *clx_m2_lp_ctl);
static void __m2_exit(clusterx_m2_lp_ctrl *clx_m2_lp_ctl);

static enum rpmi_hart_hw_state _k3_hsm_get_hw_state(void* priv,
	rpmi_uint32_t hart_index)
{
	rt_uint32_t val;
	rt_uint32_t index = hart_index;

	if (hart_index >= 8) {
		val = readl((unsigned int *)PMU_CORE_STATUS1);
		index -= 8;
	} else {
		val = readl((unsigned int *)PMU_CORE_STATUS0);	
	}

	switch (index) {
	case 0:
		val = val & (1 << 6);
		break;
	case 1:
		val = val & (1 << 9);
		break;
	case 2:
		val = val & (1 << 12);
		break;
	case 3:
		val = val & (1 << 15);
		break;
	case 4:
		val = val & (1 << 22);
		break;
	case 5:
		val = val & (1 << 25);
		break;
	case 6:
		val = val & (1 << 28);
		break;
	case 7:
		val = val & (1 << 31);
		break;
	}

	return (val) ? RPMI_HART_HW_STATE_STOPPED : RPMI_HART_HW_STATE_STARTED;
}

static int _k3_hsm_wait_enter_wfi(void* priv,
	rpmi_uint32_t hart_index)
{
	rt_uint32_t val;
	rt_uint32_t index = hart_index;

	if (hart_index >= 8) {
		val = readl((unsigned int *)PMU_CORE_STATUS1);
		index -= 8;
	} else {
		val = readl((unsigned int *)PMU_CORE_STATUS0);	
	}

	switch (index) {
	case 0:
		val = val & (1 << 4);
		break;
	case 1:
		val = val & (1 << 7);
		break;
	case 2:
		val = val & (1 << 10);
		break;
	case 3:
		val = val & (1 << 13);
		break;
	case 4:
		val = val & (1 << 20);
		break;
	case 5:
		val = val & (1 << 23);
		break;
	case 6:
		val = val & (1 << 26);
		break;
	case 7:
		val = val & (1 << 29);
		break;
	}

	return (val) ? 1 : 0;
}

static int _k3_hsm_wait_enter_m2(void* priv,
	rpmi_uint32_t hart_index)
{
	rt_uint32_t val;
	rt_uint32_t index = hart_index;

	if (hart_index >= 8) {
		val = readl((unsigned int *)PMU_CORE_STATUS1);
		index -= 8;
	} else {
		val = readl((unsigned int *)PMU_CORE_STATUS0);	
	}

	switch (index) {
	case 0:
		val = val & (1 << 3);
		break;
	case 1:
		val = val & (1 << 3);
		break;
	case 2:
		val = val & (1 << 3);
		break;
	case 3:
		val = val & (1 << 3);
		break;
	case 4:
		val = val & (1 << 19);
		break;
	case 5:
		val = val & (1 << 19);
		break;
	case 6:
		val = val & (1 << 19);
		break;
	case 7:
		val = val & (1 << 19);
		break;
	}

	return (val) ? 1 : 0;
}
static enum rpmi_hart_hw_state k3_hsm_get_hw_state(void* priv,
	rpmi_uint32_t hart_index)
{
	struct spacemit_rpmi_hsm_config *config = priv;

	if (config->hsm->leaf.harts[hart_index].state == 0xffffffff) {
		if (hart_index == 0) {
			return RPMI_HART_HW_STATE_STARTED;
		} else {
			return RPMI_HART_HW_STATE_STOPPED;
		}
	}

	if (config->hsm->leaf.harts[hart_index].state == RPMI_HSM_HART_STATE_STOP_PENDING) {
		++config->stop_flag[hart_index];
		if (config->stop_flag[hart_index] == 1)
			return _k3_hsm_get_hw_state(priv, hart_index);
		else if (config->stop_flag[hart_index] == 2) {
			config->stop_flag[hart_index] = 0;
			return RPMI_HART_HW_STATE_SUSPENDED;
		}
	}

	if (config->hsm->leaf.harts[hart_index].state == RPMI_HSM_HART_STATE_START_PENDING) {
		return RPMI_HART_HW_STATE_STARTED;
	}


	if (config->hsm->leaf.harts[hart_index].state == RPMI_HSM_HART_STATE_SUSPEND_PENDING) {
		++config->suspend_flag[hart_index];
		if (config->suspend_flag[hart_index] == 1)
			return _k3_hsm_get_hw_state(priv, hart_index);
		else if (config->suspend_flag[hart_index] == 2) {
			config->suspend_flag[hart_index] = 0;
			return RPMI_HART_HW_STATE_SUSPENDED;
		}
	}

	/* this is a fake value */
	return RPMI_HART_HW_STATE_STARTED;
}

static int spacemit_wakeup_core(uint32_t hartid)
{
	switch (hartid) {
	case 0:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE0_WAKEUP);
		break;
	case 1:
	        writel((1 << hartid), (unsigned int *)PMU_CAP_CORE1_WAKEUP);
		break;
	case 2:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE2_WAKEUP);
		break;
	case 3:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE3_WAKEUP);
		break;
	case 4:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE4_WAKEUP);
		break;
	case 5:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE5_WAKEUP);
		break;
	case 6:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE6_WAKEUP);
		break;
	case 7:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE7_WAKEUP);
		break;
	case 8:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE8_WAKEUP);
		break;
	case 9:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE9_WAKEUP);
		break;
	case 10:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE10_WAKEUP);
		break;
	case 11:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE11_WAKEUP);
		break;
	case 12:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE12_WAKEUP);
		break;
	case 13:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE13_WAKEUP);
		break;
	case 14:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE14_WAKEUP);
		break;
	case 15:
		writel((1 << hartid), (unsigned int *)PMU_CAP_CORE15_WAKEUP);
		break;
	default:
		break;
	}

	return 0;
}
static void spacemit_cx_m2_int_enable(rt_uint32_t hartid)
{
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hartid);
	audio_wakeup_en_t *dwkup = (audio_wakeup_en_t *)AUDIO_WAKEUP_EN_REG;

	switch (cluster_id) {
	case 0:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;

		clx_m2_lp_ctl->bits.rcpu_ctrl_clx_m2_lp_en = 1;

#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
		dwkup->bits.ap_c0_m2_wkup_en = 1;
		dwkup->bits.ap_c0_m2_enter_wkup_en = 1;
#endif
		break;
	case 1:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;

		clx_m2_lp_ctl->bits.rcpu_ctrl_clx_m2_lp_en = 1;
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
		dwkup->bits.ap_c1_m2_wkup_en = 1;
		dwkup->bits.ap_c1_m2_enter_wkup_en = 1;
#endif
		break;
	case 2:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C2_M2_INT_EN_REG;

		clx_m2_lp_ctl->bits.rcpu_ctrl_clx_m2_lp_en = 1;

#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
		dwkup->bits.ap_c2_m2_wkup_en = 1;
		dwkup->bits.ap_c2_m2_enter_wkup_en = 1;
#endif
	case 3:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C3_M2_INT_EN_REG;

		clx_m2_lp_ctl->bits.rcpu_ctrl_clx_m2_lp_en = 1;
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
		dwkup->bits.ap_c3_m2_wkup_en = 1;
		dwkup->bits.ap_c3_m2_enter_wkup_en = 1;
#endif
		break;
	default:
		break;
	}
}

static void spacemit_cx_m2_int_disabled(rt_uint32_t hartid)
{
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hartid);
	audio_wakeup_en_t *dwkup = (audio_wakeup_en_t *)AUDIO_WAKEUP_EN_REG;

	switch (cluster_id) {
	case 0:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;

		clx_m2_lp_ctl->bits.rcpu_ctrl_clx_m2_lp_en = 0;

#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
		dwkup->bits.ap_c0_m2_wkup_en = 0;
		dwkup->bits.ap_c0_m2_enter_wkup_en = 0;
#endif
		break;
	case 1:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;

		clx_m2_lp_ctl->bits.rcpu_ctrl_clx_m2_lp_en = 0;
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
		dwkup->bits.ap_c1_m2_wkup_en = 0;
		dwkup->bits.ap_c1_m2_enter_wkup_en = 0;
#endif
		break;
	case 2:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C2_M2_INT_EN_REG;

		clx_m2_lp_ctl->bits.rcpu_ctrl_clx_m2_lp_en = 0;

#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
		dwkup->bits.ap_c2_m2_wkup_en = 0;
		dwkup->bits.ap_c2_m2_enter_wkup_en = 0;
#endif
	case 3:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C3_M2_INT_EN_REG;

		clx_m2_lp_ctl->bits.rcpu_ctrl_clx_m2_lp_en = 0;
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
		dwkup->bits.ap_c3_m2_wkup_en = 0;
		dwkup->bits.ap_c3_m2_enter_wkup_en = 0;
#endif
		break;
	default:
		break;
	}
}

static void spacemit_set_entry_point(rpmi_uint32_t hart_index, rpmi_uint64_t start_addr, void *priv)
{
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hart_index);

	switch (cluster_id) {
	case 0:
		writel(start_addr & 0xffffffff, (unsigned int *)(C0_RVBADDR_LO_ADDR));
		writel((start_addr >> 32) & 0xffffffff, (unsigned int*)(C0_RVBADDR_HI_ADDR));
		break;
	case 1:
		writel(start_addr & 0xffffffff, (unsigned int *)(C1_RVBADDR_LO_ADDR));
		writel((start_addr >> 32) & 0xffffffff, (unsigned int*)(C1_RVBADDR_HI_ADDR));
		break;
	case 2:
		writel(start_addr & 0xffffffff, (unsigned int *)(C2_RVBADDR_LO_ADDR));
		writel((start_addr >> 32) & 0xffffffff, (unsigned int*)(C2_RVBADDR_HI_ADDR));
		break;
	case 3:
		writel(start_addr & 0xffffffff, (unsigned int *)(C3_RVBADDR_LO_ADDR));
		writel((start_addr >> 32) & 0xffffffff, (unsigned int*)(C3_RVBADDR_HI_ADDR));
		break;
	default:
		break;
	}
}

static enum rpmi_error k3_hsm_hart_start_prepare(void* priv,
	rpmi_uint32_t hart_index,
	rpmi_uint64_t start_addr)
{
	/* set the entry point */
	spacemit_set_entry_point(hart_index, start_addr, priv);

	return 0;
}

static void k3_hsm_hart_start_finalize(void* priv,
	rpmi_uint32_t hart_index,
	rpmi_uint64_t start_addr)
{
	int i;
	int flags = 0;
	struct spacemit_rpmi_hsm_config *config = priv;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hart_index);
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;

	/* wakeup the core */
	spacemit_wakeup_core(hart_index);

	for (i = (hart_index & ~(PLATFORM_MAX_CPUS_PER_CLUSTER - 1)); i <
				((hart_index & ~(PLATFORM_MAX_CPUS_PER_CLUSTER - 1)) + PLATFORM_MAX_CPUS_PER_CLUSTER); ++i) {
		if (i == hart_index)
			continue;

		if (config->hsm->leaf.harts[i].state == RPMI_HSM_HART_STATE_STOPPED)
			++flags;
	}

	if (flags == (PLATFORM_MAX_CPUS_PER_CLUSTER - 1)) {
		/* the first core of this cluster */
		/* then wait */
		if (cluster_id == 0) {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_exit0, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
			__m2_exit(clx_m2_lp_ctl);
#endif
		} else if (cluster_id == 1) {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_exit1, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
			__m2_exit(clx_m2_lp_ctl);
#endif
		} else if (cluster_id == 2) {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_exit2, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C2_M2_INT_EN_REG;
			/* __m2_exit(clx_m2_lp_ctl); */
#endif
		} else {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_exit3, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C3_M2_INT_EN_REG;
			__m2_exit(clx_m2_lp_ctl);
#endif
		}

		spacemit_cx_m2_int_disabled(hart_index);
	}
}

static enum rpmi_error k3_hsm_hart_stop_prepare(void* priv,
	rpmi_uint32_t hart_index)
{
	return 0;
}

static void k3_hsm_hart_stop_finalize(void* priv, rpmi_uint32_t hart_index)
{
	int i;
	int flags = 0;
	struct spacemit_rpmi_hsm_config *config = priv;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hart_index);
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;

	while (_k3_hsm_get_hw_state(priv, hart_index) == RPMI_HART_HW_STATE_STARTED);

	for (i = (hart_index & ~(PLATFORM_MAX_CPUS_PER_CLUSTER - 1)); i <
				((hart_index & ~(PLATFORM_MAX_CPUS_PER_CLUSTER - 1)) + PLATFORM_MAX_CPUS_PER_CLUSTER); ++i) {
		if (i == hart_index)
			continue;

		if (config->hsm->leaf.harts[i].state == RPMI_HSM_HART_STATE_STOPPED)
			++flags;
	}

	if (flags == (PLATFORM_MAX_CPUS_PER_CLUSTER - 1)) {
		/* the first core of this cluster */
		/* then wait */
		if (cluster_id == 0) {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_enter0, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
			__m2_enter(clx_m2_lp_ctl);
#endif
		} else if (cluster_id == 1) {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_enter1, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
			__m2_enter(clx_m2_lp_ctl);
#endif
		} else if (cluster_id == 2) {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_enter2, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
			/* __m2_enter(clx_m2_lp_ctl); */
#endif
		} else {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_enter3, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
			__m2_enter(clx_m2_lp_ctl);
#endif
		}

		spacemit_cx_m2_int_enable(hart_index);
	}
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
	/* do nothing */
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

static void __m2_enter(clusterx_m2_lp_ctrl *clx_m2_lp_ctl)
{
	rt_uint32_t val;

	/* clear the pending */
	val = clx_m2_lp_ctl->bits.clx_mp_state;
	if ((val & 0x3f) == 0x13) {
		clx_m2_lp_ctl->bits.clx_m2_enter_int_clk = 1;

		while (clx_m2_lp_ctl->bits.clx_m2_enter_int);

		clx_m2_lp_ctl->bits.clx_m2_enter_int_clk = 0;
	}
}

static void __m2_exit(clusterx_m2_lp_ctrl *clx_m2_lp_ctl)
{
	rt_uint32_t val;

	clx_m2_lp_ctl->bits.clr_clx_m2_wkup_hw_msk = 1;

	while (1) {
		val = clx_m2_lp_ctl->bits.clx_mp_state;
		if ((val & 0x3f) == 0x1)
			break;
	}

	clx_m2_lp_ctl->bits.clr_clx_m2_wkup_hw_msk = 0;
}

#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
static void spacemit_m2_enter_exit(int irq, void *dev_id)
{
	struct spacemit_rpmi_hsm_config *config = (struct spacemit_rpmi_hsm_config *)dev_id;

	switch (irq) {
	case AP_C0_M2_ENTER_INT_NUM:
		rt_hw_interrupt_mask(AP_C0_M2_ENTER_INT_NUM);
		rt_event_send(config->event, (1 << 0));
		break;
	case AP_C1_M2_ENTER_INT_NUM:
		rt_hw_interrupt_mask(AP_C1_M2_ENTER_INT_NUM);
		rt_event_send(config->event, (1 << 2));
		break;
	case AP_C2_M2_ENTER_INT_NUM:
		rt_hw_interrupt_mask(AP_C2_M2_ENTER_INT_NUM);
		rt_event_send(config->event, (1 << 4));
		break;
	case AP_C3_M2_ENTER_INT_NUM:
		rt_hw_interrupt_mask(AP_C3_M2_ENTER_INT_NUM);
		rt_event_send(config->event, (1 << 6));
		break;
	};

	switch (irq) {
	case AP_C0_M2_EXIT_INT_NUM:
		rt_hw_interrupt_mask(AP_C0_M2_EXIT_INT_NUM);
		rt_event_send(config->event, (1 << 1));
		break;
	case AP_C1_M2_EXIT_INT_NUM:
		rt_hw_interrupt_mask(AP_C1_M2_EXIT_INT_NUM);
		rt_event_send(config->event, (1 << 3));
		break;
	case AP_C2_M2_EXIT_INT_NUM:
		rt_hw_interrupt_mask(AP_C2_M2_EXIT_INT_NUM);
		rt_event_send(config->event, (1 << 5));
		break;
	case AP_C3_M2_EXIT_INT_NUM:
		rt_hw_interrupt_mask(AP_C3_M2_EXIT_INT_NUM);
		rt_event_send(config->event, (1 << 7));
		break;
	}
}

static void spacemit_m2_poll(void *priv)
{
	int ret;
	rt_uint32_t e;
	struct spacemit_rpmi_hsm_config *config = (struct spacemit_rpmi_hsm_config *)priv;
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;

	while(1) {
		/* wait the tick or 20ms polling */
		ret = rt_event_recv(config->event,
				/**
				 * bit0: c0 enter
				 * bit1: c0 exit
				 * bit2: c1 enter
				 * bit3: c1 exit
				 * bit4: c2 enter
				 * bit5: c2 exit
				 * bit6: c3 enter
				 * bit7: c3 exit
				 */
				(1 << 0) | (1 << 1) |
				(1 << 2) | (1 << 3) |
				(1 << 4) | (1 << 5) |
				(1 << 6) | (1 << 7),
				RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
				RT_WAITING_FOREVER, &e);

		if (e & (1 << 0)) {
			/* c0 enter */
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
			__m2_enter(clx_m2_lp_ctl);

			/* do the regulator */

			rt_hw_interrupt_umask(AP_C0_M2_ENTER_INT_NUM);
			rt_sem_release(config->sem_enter0);
		}

		if (e & (1 << 2)) {
			/* c1 enter */
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
			__m2_enter(clx_m2_lp_ctl);

			/* do the regulator */

			rt_hw_interrupt_umask(AP_C1_M2_ENTER_INT_NUM);
			rt_sem_release(config->sem_enter1);
		}

		if (e & (1 << 4)) {
			/* c2 enter */
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C2_M2_INT_EN_REG;
			__m2_enter(clx_m2_lp_ctl);

			/* do the regulator */

			rt_hw_interrupt_umask(AP_C2_M2_ENTER_INT_NUM);
			rt_sem_release(config->sem_enter2);
		}

		if (e & (1 << 6)) {
			/* c3 enter */
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C3_M2_INT_EN_REG;
			__m2_enter(clx_m2_lp_ctl);

			/* do the regulator */

			rt_hw_interrupt_umask(AP_C3_M2_ENTER_INT_NUM);
			rt_sem_release(config->sem_enter3);
		}

		if (e & (1 << 1)) {

			/* do the regulator */
			/* c0 exit */
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
			__m2_exit(clx_m2_lp_ctl);
			rt_hw_interrupt_umask(AP_C0_M2_EXIT_INT_NUM);
			rt_sem_release(config->sem_exit0);
		}

		if (e & (1 << 3)) {

			/* do the regulator */

			/* c1 exit */
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
			__m2_exit(clx_m2_lp_ctl);
			rt_hw_interrupt_umask(AP_C1_M2_EXIT_INT_NUM);
			rt_sem_release(config->sem_exit1);
		}

		if (e & (1 << 5)) {

			/* do the regulator */

			/* c2 exit */
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C2_M2_INT_EN_REG;
			__m2_exit(clx_m2_lp_ctl);
			rt_hw_interrupt_umask(AP_C2_M2_EXIT_INT_NUM);
			rt_sem_release(config->sem_exit2);
		}

		if (e & (1 << 7)) {

			/* do the regulator */

			/* c3 exit */
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C3_M2_INT_EN_REG;
			__m2_exit(clx_m2_lp_ctl);
			rt_hw_interrupt_umask(AP_C3_M2_EXIT_INT_NUM);
			rt_sem_release(config->sem_exit3);
		}
	}
}
#endif

static rt_int32_t _k3_os0_hsm_init(void *priv)
{
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
	int i;
	struct spacemit_rpmi_hsm_config *config = priv;

	config->sem_enter0 = rt_sem_create("c0_e_sem", 0, RT_IPC_FLAG_FIFO);
	config->sem_enter1 = rt_sem_create("c1_e_sem", 0, RT_IPC_FLAG_FIFO);
	config->sem_enter2 = rt_sem_create("c2_e_sem", 0, RT_IPC_FLAG_FIFO);
	config->sem_enter3 = rt_sem_create("c3_e_sem", 0, RT_IPC_FLAG_FIFO);

	config->sem_exit0 = rt_sem_create("c0_ei_sem", 0, RT_IPC_FLAG_FIFO);
	config->sem_exit1 = rt_sem_create("c1_ei_sem", 0, RT_IPC_FLAG_FIFO);
	config->sem_exit2 = rt_sem_create("c2_ei_sem", 0, RT_IPC_FLAG_FIFO);
	config->sem_exit3 = rt_sem_create("c3_ei_sem", 0, RT_IPC_FLAG_FIFO);

	/* register the interrupt */
	for (i = 0; i < config->hartcnt; i += 4) {
		struct rt_thread *thread;
		rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(i);

		switch (cluster_id) {
		case 0:
			/* register the m2 enter & wakeup interruput handler */
			rt_hw_interrupt_install(AP_C0_M2_ENTER_INT_NUM, spacemit_m2_enter_exit, priv, "c0_m2_enter");
			rt_hw_interrupt_umask(AP_C0_M2_ENTER_INT_NUM);

			rt_hw_interrupt_install(AP_C0_M2_EXIT_INT_NUM, spacemit_m2_enter_exit, priv, "c0_m2_exit");
			rt_hw_interrupt_umask(AP_C0_M2_EXIT_INT_NUM);
			break;
		case 1:
			/* register the m2 enter & wakeup interruput handler */
			rt_hw_interrupt_install(AP_C1_M2_ENTER_INT_NUM, spacemit_m2_enter_exit, priv, "c1_m2_enter");
			rt_hw_interrupt_umask(AP_C1_M2_ENTER_INT_NUM);

			rt_hw_interrupt_install(AP_C1_M2_EXIT_INT_NUM, spacemit_m2_enter_exit, priv, "c1_m2_exit");
			rt_hw_interrupt_umask(AP_C1_M2_EXIT_INT_NUM);
			break;
		case 2:
			/* register the m2 enter & wakeup interruput handler */
			rt_hw_interrupt_install(AP_C2_M2_ENTER_INT_NUM, spacemit_m2_enter_exit, priv, "c2_m2_enter");
			rt_hw_interrupt_umask(AP_C2_M2_ENTER_INT_NUM);

			rt_hw_interrupt_install(AP_C2_M2_EXIT_INT_NUM, spacemit_m2_enter_exit, priv, "c2_m2_exit");
			rt_hw_interrupt_umask(AP_C2_M2_EXIT_INT_NUM);
			break;
		case 3:
			/* register the m2 enter & wakeup interruput handler */
			rt_hw_interrupt_install(AP_C3_M2_ENTER_INT_NUM, spacemit_m2_enter_exit, priv, "c3_m2_enter");
			rt_hw_interrupt_umask(AP_C3_M2_ENTER_INT_NUM);

			rt_hw_interrupt_install(AP_C3_M2_EXIT_INT_NUM, spacemit_m2_enter_exit, priv, "c3_m2_exit");
			rt_hw_interrupt_umask(AP_C3_M2_EXIT_INT_NUM);
			break;
		default:
			break;
		}

		spacemit_cx_m2_int_enable(i);
	}

	/* create a event */
	config->event = rt_event_create("m2_event", RT_IPC_FLAG_FIFO);

	/* create the rpmsg poll thread */
	config->tid = rt_thread_create("m2_thread",
			spacemit_m2_poll,
			(void *)priv,
			2048,
			RT_THREAD_PRIORITY_MAX / 3,
			20);
	if (!config->tid) {
		rt_kprintf("Failed to create hsm service\n");
		return -RT_EINVAL;
	}

	rt_thread_startup(config->tid);
#endif

	return 0;
}

/* System suspend platform operations (stubs for hardware-related functions) */
static enum rpmi_error syssusp_prepare(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	/* Do nothing */

	return 0;
}

static rpmi_bool_t syssusp_ready(void* priv, rpmi_uint32_t hart_index)
{
	return true;
}

/* extern void mbox_test_start(void); */

static void syssusp_finalize(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	int i;
	int flags = 0;
	struct spacemit_rpmi_hsm_config *config = priv;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hart_index);
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;

	/* here we just wait the cluster0 enter M2 */
	for (i = (hart_index & ~(PLATFORM_MAX_CPUS_PER_CLUSTER - 1)); i <
				((hart_index & ~(PLATFORM_MAX_CPUS_PER_CLUSTER - 1)) + PLATFORM_MAX_CPUS_PER_CLUSTER); ++i) {
		if (i == hart_index)
			continue;

		if (config->hsm->leaf.harts[i].state == RPMI_HSM_HART_STATE_STOPPED)
			++flags;
	}

	if (flags == (PLATFORM_MAX_CPUS_PER_CLUSTER - 1)) {
		/* the first core of this cluster */
		/* then wait */
		if (cluster_id == 0) {
#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
			rt_sem_take(config->sem_enter0, RT_WAITING_FOREVER);
#else
			clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
			__m2_enter(clx_m2_lp_ctl);
#endif
		} else {
			rt_kprintf("%s:%d, the last cluster is not cluster0\n", __func__, __LINE__);
			while (1);
		}
	} else {
		rt_kprintf("%s:%d, cluster0 has the unstoped cores\n", __func__, __LINE__);
		while (1);
	}

	/* let rcpu1 enter lp mode */
	/* mbox_test_start(); */

	/* trigger the system suspend */
	/* rt_pm_release(RT_PM_DEFAULT_SLEEP_MODE); */
}

static rpmi_bool_t syssusp_can_resume(void* priv, rpmi_uint32_t hart_index)
{
	return true;
}

static enum rpmi_error syssusp_resume(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	struct spacemit_rpmi_hsm_config *config = priv;
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;

#ifdef USING_INTERRUPT_TO_TRIGGER_STATE_TRANSITION_OF_CLUSTER
	rt_sem_take(config->sem_exit0, RT_WAITING_FOREVER);
#else
	clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
	__m2_exit(clx_m2_lp_ctl);
#endif

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
