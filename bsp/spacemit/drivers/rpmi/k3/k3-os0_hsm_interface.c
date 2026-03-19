#include <rthw.h>
#include <rtthread.h>
#include <riscv-ops.h>
#include "k3_hsm.h"
#include <register_defination.h>
#include "../spacemit-rpmi.h"

void spacemit_cx_m2_int_enable(rt_uint32_t hartid)
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

void spacemit_cx_m2_enter_wait(rt_uint32_t hartid)
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

void spacemit_cx_m2_int_disabled(rt_uint32_t hartid)
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

void spacemit_assert_corex(unsigned int hartid)
{
	unsigned int val;

	switch (hartid) {
	case 0:
		val = readl((unsigned int *)PMU_CC2_AP);
		val |= (1 << 0);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;	
	case 1:
		val = readl((unsigned int *)PMU_CC2_AP);
		val |= (1 << 3);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 2:
		val = readl((unsigned int *)PMU_CC2_AP);
		val |= (1 << 6);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 3:
		val = readl((unsigned int *)PMU_CC2_AP);
		val |= (1 << 9);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 4:
		val = readl((unsigned int *)PMU_CC2_AP);
		val |= (1 << 16);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 5:
		val = readl((unsigned int *)PMU_CC2_AP);
		val |= (1 << 19);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 6:
		val = readl((unsigned int *)PMU_CC2_AP);
		val |= (1 << 22);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 7:
		val = readl((unsigned int *)PMU_CC2_AP);
		val |= (1 << 25);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 8:
		val = readl((unsigned int *)PMU_CC3_AP);
		val |= (1 << 6);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 9:
		val = readl((unsigned int *)PMU_CC3_AP);
		val |= (1 << 9);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 10:
		val = readl((unsigned int *)PMU_CC3_AP);
		val |= (1 << 12);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 11:
		val = readl((unsigned int *)PMU_CC3_AP);
		val |= (1 << 15);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 12:
		val = readl((unsigned int *)PMU_CC3_AP);
		val |= (1 << 16);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 13:
		val = readl((unsigned int *)PMU_CC3_AP);
		val |= (1 << 19);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 14:
		val = readl((unsigned int *)PMU_CC3_AP);
		val |= (1 << 22);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 15:
		val = readl((unsigned int *)PMU_CC3_AP);
		val |= (1 << 25);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	}
}

void spacemit_deassert_corex(unsigned int hartid)
{
	unsigned int val;

	switch (hartid) {
	case 0:
		val = readl((unsigned int *)PMU_CC2_AP);
		val &= ~(1 << 0);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;	
	case 1:
		val = readl((unsigned int *)PMU_CC2_AP);
		val &= ~(1 << 3);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 2:
		val = readl((unsigned int *)PMU_CC2_AP);
		val &= ~(1 << 6);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 3:
		val = readl((unsigned int *)PMU_CC2_AP);
		val &= ~(1 << 9);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 4:
		val = readl((unsigned int *)PMU_CC2_AP);
		val &= ~(1 << 16);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 5:
		val = readl((unsigned int *)PMU_CC2_AP);
		val &= ~(1 << 19);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 6:
		val = readl((unsigned int *)PMU_CC2_AP);
		val &= ~(1 << 22);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 7:
		val = readl((unsigned int *)PMU_CC2_AP);
		val &= ~(1 << 25);
		writel(val, (unsigned int *)PMU_CC2_AP);
		break;
	case 8:
		val = readl((unsigned int *)PMU_CC3_AP);
		val &= ~(1 << 6);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 9:
		val = readl((unsigned int *)PMU_CC3_AP);
		val &= ~(1 << 9);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 10:
		val = readl((unsigned int *)PMU_CC3_AP);
		val &= ~(1 << 12);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 11:
		val = readl((unsigned int *)PMU_CC3_AP);
		val &= ~(1 << 15);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 12:
		val = readl((unsigned int *)PMU_CC3_AP);
		val &= ~(1 << 16);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 13:
		val = readl((unsigned int *)PMU_CC3_AP);
		val &= ~(1 << 19);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 14:
		val = readl((unsigned int *)PMU_CC3_AP);
		val &= ~(1 << 22);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	case 15:
		val = readl((unsigned int *)PMU_CC3_AP);
		val &= ~(1 << 25);
		writel(val, (unsigned int *)PMU_CC3_AP);
		break;
	}
}
