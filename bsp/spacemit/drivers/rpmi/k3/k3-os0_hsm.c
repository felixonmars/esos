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

	/* this is a fake value */
	return RPMI_HART_HW_STATE_STARTED;
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
	return;
}

static enum rpmi_error k3_hsm_hart_stop_prepare(void* priv,
	rpmi_uint32_t hart_index)
{
	return 0;
}

static void k3_hsm_hart_stop_finalize(void* priv, rpmi_uint32_t hart_index)
{
	return;
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

#define CPU_TO_CLUSTER(cpu)    ((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

static void spacemit_cx_m2_int_enable(rt_uint32_t hartid)
{
	rt_uint32_t val;
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hartid);

	switch (cluster_id) {
	case 0:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
		val = readl((unsigned int *)AUDIO_WAKEUP_EN_REG);
		val |= (3 << 10);
		writel(val, (unsigned int *)AUDIO_WAKEUP_EN_REG);
		break;
	case 1:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
		val = readl((unsigned int *)AUDIO_WAKEUP_EN_REG);
		val |= (3 << 12);
		writel(val, (unsigned int *)AUDIO_WAKEUP_EN_REG);
		break;
	case 2:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C2_M2_INT_EN_REG;
		val = readl((unsigned int *)AUDIO_WAKEUP_EN_REG);
		val |= (3 << 14);
		writel(val, (unsigned int *)AUDIO_WAKEUP_EN_REG);
		break;
	case 3:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C3_M2_INT_EN_REG;
		val = readl((unsigned int *)AUDIO_WAKEUP_EN_REG);
		val |= (3 << 16);
		writel(val, (unsigned int *)AUDIO_WAKEUP_EN_REG);
		break;
	default:
		break;
	}

	val = readl((unsigned int *)clx_m2_lp_ctl);
	val |= (1 << 0);
	writel(val, (unsigned int *)clx_m2_lp_ctl);
}

static void spacemit_cx_m2_enter_wait(rt_uint32_t hartid)
{
	rt_uint32_t val;
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hartid);

	switch (cluster_id) {
	case 0:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
		break;
	case 1:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
		break;
	case 2:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C2_M2_INT_EN_REG;
		break;
	case 3:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C3_M2_INT_EN_REG;
		break;
	default:
		break;
	}

	val = readl((unsigned int *)clx_m2_lp_ctl);
	if (((val >> 6) & 0x3f) == 0x13) {
		rt_kprintf("Cluster:%d, enter M2 OK\n", cluster_id);
	} else {
		rt_kprintf("Cluster:%d, enter M2 Failed\n", cluster_id);
	}

	val = readl((unsigned int *)clx_m2_lp_ctl);
	val |= (1 << 2);
	writel(val, (unsigned int *)clx_m2_lp_ctl);

	val = readl((unsigned int *)clx_m2_lp_ctl);
	while ((val >> 4) & 0x1) {
		val = readl((unsigned int *)clx_m2_lp_ctl);	
	}

	val = readl((unsigned int *)clx_m2_lp_ctl);
	val &= ~(1 << 2);
	writel(val, (unsigned int *)clx_m2_lp_ctl);
}

static void spacemit_cx_m2_int_disabled(rt_uint32_t hartid)
{
	rt_uint32_t val;
	clusterx_m2_lp_ctrl *clx_m2_lp_ctl;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(hartid);

	switch (cluster_id) {
	case 0:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C0_M2_INT_EN_REG;
		val = readl((unsigned int *)AUDIO_WAKEUP_EN_REG);
		val &= ~(3 << 10);
		writel(val, (unsigned int *)AUDIO_WAKEUP_EN_REG);
		break;
	case 1:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C1_M2_INT_EN_REG;
		val = readl((unsigned int *)AUDIO_WAKEUP_EN_REG);
		val &= ~(3 << 12);
		writel(val, (unsigned int *)AUDIO_WAKEUP_EN_REG);
		break;
	case 2:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C2_M2_INT_EN_REG;
		val = readl((unsigned int *)AUDIO_WAKEUP_EN_REG);
		val &= ~(3 << 14);
		writel(val, (unsigned int *)AUDIO_WAKEUP_EN_REG);
		break;
	case 3:
		clx_m2_lp_ctl = (clusterx_m2_lp_ctrl *)AP_C3_M2_INT_EN_REG;
		val = readl((unsigned int *)AUDIO_WAKEUP_EN_REG);
		val &= ~(3 << 16);
		writel(val, (unsigned int *)AUDIO_WAKEUP_EN_REG);
		break;
	default:
		break;
	}

	val = readl((unsigned int *)clx_m2_lp_ctl);
	val |= (0x1 << 1);
	writel(val, (unsigned int *)clx_m2_lp_ctl);

	val = readl((unsigned int *)clx_m2_lp_ctl);
	while (((val >> 6) & 0x3f) != 1) {
		val = readl((unsigned int *)clx_m2_lp_ctl);
	}

	val = readl((unsigned int *)clx_m2_lp_ctl);
	val &= ~(0x1 << 1);
	writel(val, (unsigned int *)clx_m2_lp_ctl);

	val = readl((unsigned int *)clx_m2_lp_ctl);
	val &= ~(0x1 << 0);
	writel(val, (unsigned int *)clx_m2_lp_ctl);
}

void spacemit_core_assert(uint32_t hartid)
{
	unsigned int value;

	/* vote core power-down & cluster power-down */
	switch (hartid) {
	case 0:
		value = readl((unsigned int *)PMU_CC2_AP);
		value |= (1 << CORE0_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 1:
		value = readl((unsigned int *)PMU_CC2_AP);
		value |= (1 << CORE1_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 2:
		value = readl((unsigned int *)PMU_CC2_AP);
		value |= (1 << CORE2_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 3:
		value = readl((unsigned int *)PMU_CC2_AP);
		value |= (1 << CORE3_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 4:
		value = readl((unsigned int *)PMU_CC2_AP);
		value |= (1 << CORE4_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 5:
		value = readl((unsigned int *)PMU_CC2_AP);
		value |= (1 << CORE5_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 6:
		value = readl((unsigned int *)PMU_CC2_AP);
		value |= (1 << CORE6_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 7:
		value = readl((unsigned int *)PMU_CC2_AP);
		value |= (1 << CORE7_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 8:
		value = readl((unsigned int *)PMU_CC3_AP);
		value |= (1 << CORE8_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 9:
		value = readl((unsigned int *)PMU_CC3_AP);
		value |= (1 << CORE9_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 10:
		value = readl((unsigned int *)PMU_CC3_AP);
		value |= (1 << CORE10_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 11:
		value = readl((unsigned int *)PMU_CC3_AP);
		value |= (1 << CORE11_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 12:
		value = readl((unsigned int *)PMU_CC3_AP);
		value |= (1 << CORE12_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 13:
		value = readl((unsigned int *)PMU_CC3_AP);
		value |= (1 << CORE13_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 14:
		value = readl((unsigned int *)PMU_CC3_AP);
		value |= (1 << CORE14_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 15:
		value = readl((unsigned int *)PMU_CC3_AP);
		value |= (1 << CORE15_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	default:
		break;
	}
}

void spacemit_core_de_assert(uint32_t hartid)
{
	unsigned int value;

	/* vote core power-down & cluster power-down */
	switch (hartid) {
	case 0:
		value = readl((unsigned int *)PMU_CC2_AP);
		value &= ~(1 << CORE0_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 1:
		value = readl((unsigned int *)PMU_CC2_AP);
		value &= ~(1 << CORE1_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 2:
		value = readl((unsigned int *)PMU_CC2_AP);
		value &= ~(1 << CORE2_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 3:
		value = readl((unsigned int *)PMU_CC2_AP);
		value &= ~(1 << CORE3_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 4:
		value = readl((unsigned int *)PMU_CC2_AP);
		value &= ~(1 << CORE4_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 5:
		value = readl((unsigned int *)PMU_CC2_AP);
		value &= ~(1 << CORE5_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 6:
		value = readl((unsigned int *)PMU_CC2_AP);
		value &= ~(1 << CORE6_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 7:
		value = readl((unsigned int *)PMU_CC2_AP);
		value &= ~(1 << CORE7_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC2_AP);
		break;
	case 8:
		value = readl((unsigned int *)PMU_CC3_AP);
		value &= ~(1 << CORE8_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 9:
		value = readl((unsigned int *)PMU_CC3_AP);
		value &= ~(1 << CORE9_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 10:
		value = readl((unsigned int *)PMU_CC3_AP);
		value &= ~(1 << CORE10_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 11:
		value = readl((unsigned int *)PMU_CC3_AP);
		value &= ~(1 << CORE11_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 12:
		value = readl((unsigned int *)PMU_CC3_AP);
		value &= ~(1 << CORE12_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 13:
		value = readl((unsigned int *)PMU_CC3_AP);
		value &= ~(1 << CORE13_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 14:
		value = readl((unsigned int *)PMU_CC3_AP);
		value &= ~(1 << CORE14_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	case 15:
		value = readl((unsigned int *)PMU_CC3_AP);
		value &= ~(1 << CORE15_POP_RST_BIT);
		writel(value, (unsigned int *)PMU_CC3_AP);
		break;
	default:
		break;
	}
}

static void spacemit_vote_core_apcr(void *priv)
{
	int i;
	unsigned int val;
	int hartid;
	struct spacemit_rpmi_hsm_config *config = priv;

	for (i = 0; i < config->hartcnt; ++i) {
		hartid = config->hartids[i];

		switch (hartid) {
		case 0:
			val = readl((unsigned int *)APCR_CORE0_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE0_VETE_REG);
			break;
		case 1:
			val = readl((unsigned int *)APCR_CORE1_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE1_VETE_REG);
			break;
		case 2:
			val = readl((unsigned int *)APCR_CORE2_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE2_VETE_REG);
			break;
		case 3:
			val = readl((unsigned int *)APCR_CORE3_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE3_VETE_REG);
			break;
		case 4:
			val = readl((unsigned int *)APCR_CORE4_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE4_VETE_REG);
			break;
		case 5:
			val = readl((unsigned int *)APCR_CORE5_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE5_VETE_REG);
			break;
		case 6:
			val = readl((unsigned int *)APCR_CORE6_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE6_VETE_REG);
			break;
		case 7:
			val = readl((unsigned int *)APCR_CORE7_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE7_VETE_REG);
			break;
		case 8:
			val = readl((unsigned int *)APCR_CORE8_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE8_VETE_REG);
			break;
		case 9:
			val = readl((unsigned int *)APCR_CORE9_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE9_VETE_REG);
			break;
		case 10:
			val = readl((unsigned int *)APCR_CORE10_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE10_VETE_REG);
			break;
		case 11:
			val = readl((unsigned int *)APCR_CORE11_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE11_VETE_REG);
			break;
		case 12:
			val = readl((unsigned int *)APCR_CORE12_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE12_VETE_REG);
			break;
		case 13:
			val = readl((unsigned int *)APCR_CORE13_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE13_VETE_REG);
			break;
		case 14:
			val = readl((unsigned int *)APCR_CORE14_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE14_VETE_REG);
			break;
		case 15:
			val = readl((unsigned int *)APCR_CORE15_VETE_REG);
			val |= APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE15_VETE_REG);
			break;
		default:
			break;
		}
	}
}

static void spacemit_devote_core_apcr(void *priv)
{
	int i;
	unsigned int val;
	int hartid;
	struct spacemit_rpmi_hsm_config *config = priv;

	for (i = 0; i < config->hartcnt; ++i) {
		hartid = config->hartids[i];

		switch (hartid) {
		case 0:
			val = readl((unsigned int *)APCR_CORE0_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE0_VETE_REG);
			break;
		case 1:
			val = readl((unsigned int *)APCR_CORE1_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE1_VETE_REG);
			break;
		case 2:
			val = readl((unsigned int *)APCR_CORE2_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE2_VETE_REG);
			break;
		case 3:
			val = readl((unsigned int *)APCR_CORE3_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE3_VETE_REG);
			break;
		case 4:
			val = readl((unsigned int *)APCR_CORE4_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE4_VETE_REG);
			break;
		case 5:
			val = readl((unsigned int *)APCR_CORE5_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE5_VETE_REG);
			break;
		case 6:
			val = readl((unsigned int *)APCR_CORE6_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE6_VETE_REG);
			break;
		case 7:
			val = readl((unsigned int *)APCR_CORE7_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE7_VETE_REG);
			break;
		case 8:
			val = readl((unsigned int *)APCR_CORE8_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE8_VETE_REG);
			break;
		case 9:
			val = readl((unsigned int *)APCR_CORE9_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE9_VETE_REG);
			break;
		case 10:
			val = readl((unsigned int *)APCR_CORE10_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE10_VETE_REG);
			break;
		case 11:
			val = readl((unsigned int *)APCR_CORE11_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE11_VETE_REG);
			break;
		case 12:
			val = readl((unsigned int *)APCR_CORE12_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE12_VETE_REG);
			break;
		case 13:
			val = readl((unsigned int *)APCR_CORE13_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE13_VETE_REG);
			break;
		case 14:
			val = readl((unsigned int *)APCR_CORE14_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE14_VETE_REG);
			break;
		case 15:
			val = readl((unsigned int *)APCR_CORE15_VETE_REG);
			val &= ~APCR_COREX_DEFAULT_VATE_VALUE;
			writel(val, (unsigned int *)APCR_CORE15_VETE_REG);
			break;
		default:
			break;
		}
	}
}

static void spacemit_m2_enter_exit(int vector, void *param)
{
	struct spacemit_rpmi_hsm_config *config = param;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(config->bootcore_index);

	if ((vector == AP_C0_M2_ENTER_INT_NUM) ||
			(vector == AP_C1_M2_ENTER_INT_NUM) ||
			(vector == AP_C2_M2_ENTER_INT_NUM) ||
			(vector == AP_C3_M2_ENTER_INT_NUM)) {

		/* clear the pending */
		spacemit_cx_m2_enter_wait(config->bootcore_index);

		rt_sem_release(config->cm2_etr_sem);

		return;
	}

	/* mask the AP wakeup */
	spacemit_core_assert(config->bootcore_index);

	/* clear the pending and wakeup the cluster */
	spacemit_cx_m2_int_disabled(config->bootcore_index);

	/* send the signal */
	rt_sem_release(config->cm2_ext_sem);
}

static rt_int32_t _k3_os0_hsm_init(void *priv)
{
	char *tmp;
	struct spacemit_rpmi_hsm_config *config = priv;

	tmp = rt_calloc(1, 64);
	rt_snprintf(tmp, 64, "Cr%d_sem", config->bootcore_index);

	config->cm2_etr_sem = rt_sem_create(tmp, 0, RT_IPC_FLAG_FIFO);

	tmp = rt_calloc(1, 64);
	rt_snprintf(tmp, 64, "Ce%d_sem", config->bootcore_index);

	config->cm2_ext_sem = rt_sem_create(tmp, 0, RT_IPC_FLAG_FIFO);

	tmp = rt_calloc(1, 64);
	rt_snprintf(tmp, 64, "CWK%d_sem", config->bootcore_index);

	config->cmwk_sem = rt_sem_create(tmp, 0, RT_IPC_FLAG_FIFO);

	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(config->bootcore_index);

	switch (cluster_id) {
	case 0:
		/* exit m2 */
		rt_hw_interrupt_install(AP_C0_M2_EXIT_INT_NUM, spacemit_m2_enter_exit, priv, "c0_m2_exit");
		rt_hw_interrupt_install(AP_C0_M2_ENTER_INT_NUM, spacemit_m2_enter_exit, priv, "c0_m2_enter");
		rt_hw_interrupt_umask(AP_C0_M2_EXIT_INT_NUM);
		rt_hw_interrupt_umask(AP_C0_M2_ENTER_INT_NUM);
	break;
	case 1:
		rt_hw_interrupt_install(AP_C1_M2_EXIT_INT_NUM, spacemit_m2_enter_exit, priv, "c1_m2_exit");
		rt_hw_interrupt_install(AP_C1_M2_ENTER_INT_NUM, spacemit_m2_enter_exit, priv, "c1_m2_enter");
		rt_hw_interrupt_umask(AP_C1_M2_EXIT_INT_NUM);
		rt_hw_interrupt_umask(AP_C1_M2_ENTER_INT_NUM);
	break;
	case 2:
		rt_hw_interrupt_install(AP_C2_M2_EXIT_INT_NUM, spacemit_m2_enter_exit, priv, "c2_m2_exit");
		rt_hw_interrupt_install(AP_C2_M2_ENTER_INT_NUM, spacemit_m2_enter_exit, priv, "c2_m2_enter");
		rt_hw_interrupt_umask(AP_C2_M2_EXIT_INT_NUM);
		rt_hw_interrupt_umask(AP_C2_M2_ENTER_INT_NUM);
	break;
	case 3:
		rt_hw_interrupt_install(AP_C3_M2_EXIT_INT_NUM, spacemit_m2_enter_exit, priv, "c3_m2_exit");
		rt_hw_interrupt_install(AP_C3_M2_ENTER_INT_NUM, spacemit_m2_enter_exit, priv, "c3_m2_enter");
		rt_hw_interrupt_umask(AP_C3_M2_EXIT_INT_NUM);
		rt_hw_interrupt_umask(AP_C3_M2_ENTER_INT_NUM);
        break;
	default:
        	break;
	}

	return 0;
}

/* System suspend platform operations (stubs for hardware-related functions) */
static enum rpmi_error syssusp_prepare(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	struct spacemit_rpmi_hsm_config *config = (struct spacemit_rpmi_hsm_config *)priv;
	rpmi_uint32_t cluster_id = CPU_TO_CLUSTER(config->bootcore_index);

	/* vote core enter D2 */
	spacemit_vote_core_apcr(priv);

	spacemit_cx_m2_int_enable(config->bootcore_index);

	return 0;
}

static rpmi_bool_t syssusp_ready(void* priv, rpmi_uint32_t hart_index)
{
	struct spacemit_rpmi_hsm_config *config = (struct spacemit_rpmi_hsm_config *)priv;

	rt_sem_take(config->cm2_etr_sem, RT_WAITING_FOREVER);

	return true;
}

static void syssusp_finalize(
		void* priv,
		rpmi_uint32_t hart_index,
		const struct rpmi_system_suspend_type* syssusp_type,
		rpmi_uint64_t resume_addr)
{
	struct spacemit_rpmi_hsm_config *config = (struct spacemit_rpmi_hsm_config *)priv;

	rt_event_send(config->event, (1 << config->bootcore_index));
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

	/* wait resume signale */
	rt_sem_take(config->cm2_ext_sem, RT_WAITING_FOREVER);

	/* we should first let the rcpu1 wakeup, so wait for the notify by spacmeit-hsm layer */
	rt_sem_take(config->cmwk_sem, RT_WAITING_FOREVER);

	/* devote core enter D2 */
	spacemit_devote_core_apcr(priv);

	/* release the core */
	spacemit_core_de_assert(config->bootcore_index);

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
