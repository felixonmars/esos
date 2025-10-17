/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "ccu-common.h"
#include "ccu_plla.h"
#include "ccu_mix.h"
#include "ccu_ddn.h"
#include "ccu-spacemit-k3.h"
#include "../../platform/rt24/ccu-spacemit-k3.h"

struct spacemit_ccu {
	struct spacemit_k1x_clk clk_info;
	struct clk_hw_onecell_data *clk_cells;
};

static const struct ccu_plla_rate_tbl pll2_rate_tbl[] = {
	PLLA_RATE(3000000000UL, 0x0b3e2000, 0x00000000, 0xa0558c8c),
};

static const struct ccu_plla_rate_tbl pll6_rate_tbl[] = {
	PLLA_RATE(3200000000UL, 0x0b422aaa, 0x0000ab00, 0xa0558e8e),
};

static SPACEMIT_CCU_PLLA(pll2, "pll2", &pll2_rate_tbl, ARRAY_SIZE(pll2_rate_tbl),
	BASE_TYPE_APBS, APBS_PLL2_SWCR1, APBS_PLL2_SWCR2, APBS_PLL2_SWCR3,
	MPMU_POSR, POSR_PLL2_LOCK, 1,
	0);

static SPACEMIT_CCU_PLLA(pll6, "pll6", &pll6_rate_tbl, ARRAY_SIZE(pll6_rate_tbl),
	BASE_TYPE_APBS, APBS_PLL6_SWCR1, APBS_PLL6_SWCR2, APBS_PLL6_SWCR3,
	MPMU_POSR, POSR_PLL6_LOCK, 1,
	0);

//pll1
static SPACEMIT_CCU_GATE_FACTOR(pll1_d2, "pll1_d2", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(1), BIT(1), 0x0,
	2, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d3, "pll1_d3", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(2), BIT(2), 0x0,
	3, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d4, "pll1_d4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(3), BIT(3), 0x0,
	4, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d5, "pll1_d5", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(4), BIT(4), 0x0,
	5, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d6, "pll1_d6", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(5), BIT(5), 0x0,
	6, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d7, "pll1_d7", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(6), BIT(6), 0x0,
	7, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d8, "pll1_d8", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(7), BIT(7), 0x0,
	8, 1, 0);

static SPACEMIT_CCU_DIV_GATE(pll1_dx, "pll1_dx", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	23, 5, BIT(22), BIT(22), 0x0,
	0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d64_38p4, "pll1_d64_38p4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(31), BIT(31), 0x0,
	64, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_aud_245p7, "pll1_aud_245p7", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APBS_PLL1_SWCR2,
	BIT(21), BIT(21), 0x0,
	10, 1, 0);
static SPACEMIT_CCU_FACTOR(pll1_aud_24p5, "pll1_aud_24p5", "pll1_aud_245p7", 10, 1);

//pll2
static SPACEMIT_CCU_GATE_FACTOR(pll2_d3, "pll2_d3", "pll2",
	BASE_TYPE_APBS, APBS_PLL2_SWCR2,
	BIT(2), BIT(2), 0x0,
	3, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll2_d5, "pll2_d5", "pll2",
	BASE_TYPE_APBS, APBS_PLL2_SWCR2,
	BIT(4), BIT(4), 0x0,
	5, 1, 0);

//pll6
static SPACEMIT_CCU_GATE_FACTOR(pll6_d5, "pll6_d5", "pll6",
	BASE_TYPE_APBS, APBS_PLL6_SWCR2,
	BIT(4), BIT(4), 0x0,
	5, 1, 0);

//pll2_d5
static SPACEMIT_CCU_FACTOR(pll2_66, "pll2_66", "pll2_d5", 9, 1);
static SPACEMIT_CCU_FACTOR(pll2_33, "pll2_33", "pll2_66", 2, 1);
static SPACEMIT_CCU_FACTOR(pll2_50, "pll2_50", "pll2_d5", 12, 1);
static SPACEMIT_CCU_FACTOR(pll2_25, "pll2_25", "pll2_50", 2, 1);
static SPACEMIT_CCU_FACTOR(pll2_20, "pll2_20", "pll2_d5", 30, 1);

//pll2_d3
static SPACEMIT_CCU_FACTOR(pll2_d24_125, "pll2_d24_125", "pll2_d3", 8, 1);
static SPACEMIT_CCU_FACTOR(pll2_d120_25, "pll2_d120_25", "pll2_d3", 40, 1);

//pll6_d5
static SPACEMIT_CCU_FACTOR(pll6_80, "pll6_80", "pll6_d5", 8, 1);
static SPACEMIT_CCU_FACTOR(pll6_40, "pll6_40", "pll6_d5", 16, 1);
static SPACEMIT_CCU_FACTOR(pll6_20, "pll6_20", "pll6_d5", 32, 1);

//pll1_d8
static SPACEMIT_CCU_GATE(pll1_d8_307p2, "pll1_d8_307p2", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(13), BIT(13), 0x0,
	0);

static SPACEMIT_CCU_FACTOR(pll1_d32_76p8, "pll1_d32_76p8", "pll1_d8_307p2", 4, 1);
static SPACEMIT_CCU_FACTOR(pll1_d40_61p44, "pll1_d40_61p44", "pll1_d8_307p2", 5, 1);
static SPACEMIT_CCU_FACTOR(pll1_d16_153p6, "pll1_d16_153p6", "pll1_d8", 2, 1);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d24_102p4, "pll1_d24_102p4", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(12), BIT(12), 0x0,
	3, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d48_51p2, "pll1_d48_51p2", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(7), BIT(7), 0x0,
	6, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d48_51p2_ap, "pll1_d48_51p2_ap", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(11), BIT(11), 0x0,
	6, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_m3d128_57p6, "pll1_m3d128_57p6", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(8), BIT(8), 0x0,
	16, 3, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d96_25p6, "pll1_d96_25p6", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(4), BIT(4), 0x0,
	12, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d192_12p8, "pll1_d192_12p8", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(3), BIT(3), 0x0,
	24, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d192_12p8_wdt, "pll1_d192_12p8_wdt", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(19), BIT(19), 0x0,
	24, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d384_6p4, "pll1_d384_6p4", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(2), BIT(2), 0x0,
	48, 1, 0);

static SPACEMIT_CCU_FACTOR(pll1_d768_3p2, "pll1_d768_3p2", "pll1_d384_6p4", 2, 1);
static SPACEMIT_CCU_FACTOR(pll1_d1536_1p6, "pll1_d1536_1p6", "pll1_d384_6p4", 4, 1);
static SPACEMIT_CCU_FACTOR(pll1_d3072_0p8, "pll1_d3072_0p8", "pll1_d384_6p4", 8, 1);

//pll1_d7
static SPACEMIT_CCU_FACTOR(pll1_d7_351p08, "pll1_d7_351p08", "pll1_d7", 1, 1);

//pll1_d6
static SPACEMIT_CCU_GATE(pll1_d6_409p6, "pll1_d6_409p6", "pll1_d6",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(0), BIT(0), 0x0,
	0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d12_204p8, "pll1_d12_204p8", "pll1_d6",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(5), BIT(5), 0x0,
	2, 1, 0);

//pll1_d5
static SPACEMIT_CCU_GATE(pll1_d5_491p52, "pll1_d5_491p52", "pll1_d5",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(21), BIT(21), 0x0,
	0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d10_245p76, "pll1_d10_245p76", "pll1_d5",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(18), BIT(18), 0x0,
	2, 1, 0);

//pll1_d4
static SPACEMIT_CCU_GATE(pll1_d4_614p4, "pll1_d4_614p4", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(15), BIT(15), 0x0,
	0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d52_47p26, "pll1_d52_47p26", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(10), BIT(10), 0x0,
	13, 1, 0);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d78_31p5, "pll1_d78_31p5", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(6), BIT(6), 0x0,
	39, 2, 0);

//pll1_d3
static SPACEMIT_CCU_GATE(pll1_d3_819p2, "pll1_d3_819p2", "pll1_d3",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(14), BIT(14), 0x0,
	0);

//pll1_d2
static SPACEMIT_CCU_GATE(pll1_d2_1228p8, "pll1_d2_1228p8", "pll1_d2",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(16), BIT(16), 0x0,
	0);

static struct ccu_ddn_info uart_ddn_mask_info = {
	.factor = 2,
	.num_mask = 0x1fff,
	.den_mask = 0x1fff,
	.num_shift = 16,
	.den_shift = 0,
};
static struct ccu_ddn_tbl slow_uart1_tbl[] = {
	{.num = 125, .den = 24}, /* rate = parent_rate*24/125/2) */
};
static SPACEMIT_CCU_DDN_GATE(slow_uart1_14p74, "slow_uart1_14p74", "pll1_d16_153p6",
	&uart_ddn_mask_info, &slow_uart1_tbl, ARRAY_SIZE(slow_uart1_tbl),
	BASE_TYPE_MPMU, MPMU_SUCCR, MPMU_ACGR, BIT(1),
	0);

//apmu
static const char * const rcpu_clk_parents[] = {
	"pll1_aud_245p7", "pll1_d8_307p2", "pll1_d5_491p52", "pll1_d6_409p6"
};
static SPACEMIT_CCU_DIV_FC_MUX_GATE(rcpu_clk, "rcpu_clk", rcpu_clk_parents,
	BASE_TYPE_APMU, APMU_RCPU_CLK_RES_CTRL,
	4, 3, BIT(15),
	7, 3,
	BIT(12), BIT(12), 0x0,
	CLK_IS_CRITICAL);

//rcpu5
static SPACEMIT_CCU_DIV_FC(rcpu_apb_clk, "rcpu_apb_clk", "rcpu_clk",
	BASE_TYPE_RCPU5, RCPU5_RCPU_BUS_CLK_CTRL,
	3, 3, BIT(8), 0);
static SPACEMIT_CCU_DIV_FC(rcpu_axi_clk, "rcpu_axi_clk", "rcpu_clk",
	BASE_TYPE_RCPU5, RCPU5_RCPU_BUS_CLK_CTRL,
	0, 2, BIT(8), 0);

static SPACEMIT_CCU_GATE(ripc2msa_clk, "ripc2msa_clk", "rcpu_clk",
	BASE_TYPE_RCPU5, RCPU5_AON_PER_CLK_RST_CTRL,
	BIT(5), BIT(5), 0x0,
	0);
static SPACEMIT_CCU_GATE(ripc2cp_clk, "ripc2cp_clk", "rcpu_clk",
	BASE_TYPE_RCPU5, RCPU5_AON_PER_CLK_RST_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static SPACEMIT_CCU_GATE(ripc2ap_clk, "ripc2ap_clk", "rcpu_clk",
	BASE_TYPE_RCPU5, RCPU5_AON_PER_CLK_RST_CTRL,
	BIT(1), BIT(1), 0x0,
	0);

static const char * const rtimer_parents[] = {
	"pll1_d96_25p6", "pll1_d192_12p8", "pll1_d768_3p2"
};
static SPACEMIT_CCU_DIV_MUX_GATE(rtimer1_clk, "rtimer1_clk", rtimer_parents,
	BASE_TYPE_RCPU5, RCPU5_TIMER1_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rtimer2_clk, "rtimer2_clk", rtimer_parents,
	BASE_TYPE_RCPU5, RCPU5_TIMER2_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rtimer3_clk, "rtimer3_clk", rtimer_parents,
	BASE_TYPE_RCPU5, RCPU5_TIMER3_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rtimer4_clk, "rtimer4_clk", rtimer_parents,
	BASE_TYPE_RCPU5, RCPU5_TIMER4_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);

static const char * const rt24_core_parents[] = {
	"rcpu_clk", "pll1_d4_614p4", "pll1_d5_491p52"
};
static SPACEMIT_CCU_DIV_FC_MUX(rt24_core0_clk, "rt24_core0_clk", rt24_core_parents,
	BASE_TYPE_RCPU5, RCPU5_RT24_CORE0_CLK_CTRL,
	0, 2, BIT(8),
	4, 2, 0);
static SPACEMIT_CCU_DIV_FC_MUX(rt24_core1_clk, "rt24_core1_clk", rt24_core_parents,
	BASE_TYPE_RCPU5, RCPU5_RT24_CORE1_CLK_CTRL,
	0, 2, BIT(8),
	4, 2, 0);

static SPACEMIT_CCU_GATE(rgpio_clk, "rgpio_clk", "rcpu_apb_clk",
	BASE_TYPE_RCPU5, RCPU5_GPIO_AND_EDGE_CLK_RST,
	BIT(1), BIT(15), 0x0,
	0);
static SPACEMIT_CCU_GATE(rgpio_edge_clk, "rgpio_edge_clk", "rcpu_apb_clk",
	BASE_TYPE_RCPU5, RCPU5_GPIO_AND_EDGE_CLK_RST,
	BIT(3), BIT(3), 0x0,
	0);
static SPACEMIT_CCU_GATE(rgpio_lp_clk, "rgpio_lp_clk", "rcpu_apb_clk",
	BASE_TYPE_RCPU5, RCPU5_GPIO_AND_EDGE_CLK_RST,
	BIT(4), BIT(4), 0x0,
	0);

//rcpu
static const char * const rcan_parents[] = {
	"pll6_20", "pll6_40", "pll6_80"
};
static SPACEMIT_CCU_MUX_GATE(rcan0_clk, "rcan0_clk", rcan_parents,
	BASE_TYPE_RCPU, RCPU_CAN_CLK_RST,
	4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(rcan1_clk, "rcan1_clk", rcan_parents,
	BASE_TYPE_RCPU, RCPU_CAN1_CLK_RST,
	4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(rcan2_clk, "rcan2_clk", rcan_parents,
	BASE_TYPE_RCPU, RCPU_CAN2_CLK_RST,
	4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(rcan3_clk, "rcan3_clk", rcan_parents,
	BASE_TYPE_RCPU, RCPU_CAN3_CLK_RST,
	4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);
static SPACEMIT_CCU_MUX_GATE(rcan4_clk, "rcan4_clk", rcan_parents,
	BASE_TYPE_RCPU, RCPU_CAN4_CLK_RST,
	4, 2,
	BIT(1) | BIT(2), BIT(1) | BIT(2), 0x0,
	0);

static SPACEMIT_CCU_GATE(rirc0_clk, "rirc0_clk", "rcpu_apb_clk",
	BASE_TYPE_RCPU, RCPU_IRC_CLK_RST,
	BIT(2), BIT(2), 0x0,
	0);
static SPACEMIT_CCU_GATE(rirc1_clk, "rirc1_clk", "rcpu_apb_clk",
	BASE_TYPE_RCPU, RCPU_IRC1_CLK_RST,
	BIT(2), BIT(2), 0x0,
	0);

static const char * const espi_sclk_src_parents[] = {
	"pll2_20", "pll2_25", "pll2_33", "pll2_50", "pll2_66"
};
static SPACEMIT_CCU_MUX(respi_sclk_src, "respi_sclk_src", espi_sclk_src_parents,
	BASE_TYPE_RCPU, RCPU_ESPI_CLK_RST,
	4, 3,
	0);
static const char * const espi_sclk_parents[] = {
	"dummy_clk", "espi_sclk_src"
};
static SPACEMIT_CCU_MUX_GATE(respi_sclk, "respi_sclk", espi_sclk_parents,
	BASE_TYPE_RCPU, RCPU_ESPI_CLK_RST,
	8, 1,
	BIT(1), BIT(1), 0x0,
	0);

static SPACEMIT_CCU_GATE(remac_bus_clk, "remac_bus_clk", "rcpu_axi_clk",
	BASE_TYPE_RCPU, RCPU_GMAC_CLK_RST,
	BIT(0), BIT(0), 0x0,
	0);
static SPACEMIT_CCU_GATE(remac_ref_clk, "remac_ref_clk", "pll2_d120_25",
	BASE_TYPE_RCPU, RCPU_GMAC_CLK_RST,
	BIT(14), 0x0, BIT(14),
	0);
static const char * const emac_1588_parents[] = {
	"vctcxo_24", "pll2_d24_125"
};
static SPACEMIT_CCU_MUX(remac_1588_clk, "remac_1588_clk", emac_1588_parents,
	BASE_TYPE_RCPU, RCPU_GMAC_CLK_RST,
	15, 1,
	0);
static SPACEMIT_CCU_GATE(remac_rgmii_tx_clk, "remac_rgmii_tx_clk", "pll2_d24_125",
	BASE_TYPE_RCPU, RCPU_GMAC_CLK_RST,
	BIT(8), BIT(8), 0x0,
	0);

static const char * const ri2s01_sysclk_parents[] = {
	"pll1_aud_24p5", "pll1_aud_245p7", "pll1_d96_25p6", "pll1_d768_3p2"
};
static SPACEMIT_CCU_DIV_MUX_GATE(ri2s0_sysclk, "ri2s0_sysclk", ri2s01_sysclk_parents,
	BASE_TYPE_RCPU, RCPU_AUDIO_I2S0_SYS_CLK_CTRL,
	8, 11, 4, 2,
	BIT(2) | BIT(1), BIT(2) | BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ri2s1_sysclk, "ri2s1_sysclk", ri2s01_sysclk_parents,
	BASE_TYPE_RCPU, RCPU_AUDIO_I2S1_SYS_CLK_CTRL,
	8, 11, 4, 2,
	BIT(2) | BIT(1), BIT(2) | BIT(1), 0x0,
	0);

//rcpu1
static const char * const ruart_clk_parents[] = {
	"slow_uart1_14p74", "pll1_aud_245p7", "pll1_d96_25p6", "pll1_m3d128_57p6"
};
static SPACEMIT_CCU_DIV_MUX_GATE(ruart0_clk, "ruart0_clk", ruart_clk_parents,
	BASE_TYPE_RCPU1, RCPU1_UART0_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ruart1_clk, "ruart1_clk", ruart_clk_parents,
	BASE_TYPE_RCPU1, RCPU1_UART1_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ruart2_clk, "ruart2_clk", ruart_clk_parents,
	BASE_TYPE_RCPU1, RCPU1_UART2_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ruart3_clk, "ruart3_clk", ruart_clk_parents,
	BASE_TYPE_RCPU1, RCPU1_UART3_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ruart4_clk, "ruart4_clk", ruart_clk_parents,
	BASE_TYPE_RCPU1, RCPU1_UART4_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ruart5_clk, "ruart5_clk", ruart_clk_parents,
	BASE_TYPE_RCPU1, RCPU1_UART5_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);

//rcpu2
static const char *ri2s_clk_parents[] = {
	"pll1_aud_24p5", "pll1_aud_245p7"
};
static SPACEMIT_CCU_DIV_MUX_GATE(ri2s0_clk, "ri2s0_clk", ri2s_clk_parents,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S0_TX_RX_CLK_CTRL,
	4, 11, 16, 2,
	BIT(2) | BIT(1), BIT(2) | BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ri2s1_clk, "ri2s1_clk", ri2s_clk_parents,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S1_TX_RX_CLK_CTRL,
	4, 11, 16, 2,
	BIT(2) | BIT(1), BIT(2) | BIT(1), 0x0,
	0);

static const char *ri2s23_sysclk_parents[] = {
	"pll1_aud_24p5", "pll1_aud_245p7"
};
static SPACEMIT_CCU_DIV_MUX_GATE(ri2s2_sysclk, "ri2s2_sysclk", ri2s23_sysclk_parents,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S2_SYS_CLK_CTRL,
	4, 11, 16, 2,
	BIT(2) | BIT(1), BIT(2) | BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ri2s3_sysclk, "ri2s3_sysclk", ri2s23_sysclk_parents,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S3_SYS_CLK_CTRL,
	4, 11, 16, 2,
	BIT(2) | BIT(1), BIT(2) | BIT(1), 0x0,
	0);

static SPACEMIT_CCU_DIV_GATE(ri2s2_clk, "ri2s2_clk", "ri2s2_sysclk",
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S2_TX_RX_CLK_CTRL,
	4, 11,
	BIT(2) | BIT(1), BIT(2) | BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_GATE(ri2s3_clk, "ri2s3_clk", "ri2s3_sysclk",
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S3_TX_RX_CLK_CTRL,
	4, 11,
	BIT(2) | BIT(1), BIT(2) | BIT(1), 0x0,
	0);

//rcpu3
static const char *rspi_parent_names[] = {
	"pll1_aud_24p5", "pll1_aud_245p7", "pll1_d96_25p6"
};
static SPACEMIT_CCU_DIV_MUX_GATE(rspi0_clk, "rspi0_clk", rspi_parent_names,
	BASE_TYPE_RCPU3, RCPU3_SSP0_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rspi1_clk, "rspi1_clk", rspi_parent_names,
	BASE_TYPE_RCPU3, RCPU3_SSP1_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rspi2_clk, "rspi2_clk", rspi_parent_names,
	BASE_TYPE_RCPU3, RCPU3_PWR_SSP_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);

//rcpu4
static const char *ri2c_parent_names[] = {
	"pll1_aud_24p5", "pll1_aud_245p7", "pll1_d96_25p6"
};
static SPACEMIT_CCU_DIV_MUX_GATE(ri2c0_clk, "ri2c0_clk", ri2c_parent_names,
	BASE_TYPE_RCPU4, RCPU4_I2C0_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ri2c1_clk, "ri2c1_clk", ri2c_parent_names,
	BASE_TYPE_RCPU4, RCPU4_I2C1_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(ri2c2_clk, "ri2c2_clk", ri2c_parent_names,
	BASE_TYPE_RCPU4, RCPU4_PWR_I2C_CLK_RST,
	8, 11, 4, 2,
	BIT(1) | BIT(0), BIT(1) | BIT(0), 0x0,
	0);

//rcpu6
static const char *rpwm_parent_names[] = {
	"pll1_aud_245p7", "pll1_aud_24p5"
};
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm0_clk, "rpwm0_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM0_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm1_clk, "rpwm1_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM1_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm2_clk, "rpwm2_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM2_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm3_clk, "rpwm3_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM3_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm4_clk, "rpwm4_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM4_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm5_clk, "rpwm5_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM5_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm6_clk, "rpwm6_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM6_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm7_clk, "rpwm7_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM7_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm8_clk, "rpwm8_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM8_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm9_clk, "rpwm9_clk", rpwm_parent_names,
	BASE_TYPE_RCPU6, RCPU6_PWM9_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);

//resets
//rcpu5
static SPACEMIT_CCU_GATE_NO_PARENT(rtimer1_rst, "rtimer1_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_TIMER1_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rtimer2_rst, "rtimer2_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_TIMER2_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rtimer3_rst, "rtimer3_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_TIMER3_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rtimer4_rst, "rtimer4_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_TIMER4_CLK_RST,
	BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(ripc2ap_rst, "ripc2ap_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_AON_PER_CLK_RST_CTRL,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ripc2cp_rst, "ripc2cp_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_AON_PER_CLK_RST_CTRL,
	BIT(2), BIT(2), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ripc2msa_rst, "ripc2msa_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_AON_PER_CLK_RST_CTRL,
	BIT(4), BIT(4), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(rt24_core0_rst, "rt24_core0_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_RT24_CORE0_SW_RESET,
	BIT(0), 0, BIT(0), 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rt24_core1_rst, "rt24_core1_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_RT24_CORE1_SW_RESET,
	BIT(0), 0, BIT(0), 0);

static SPACEMIT_CCU_GATE_NO_PARENT(rgpio_rst, "rgpio_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_GPIO_AND_EDGE_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rgpio_edge_rst, "rgpio_edge_rst", NULL,
	BASE_TYPE_RCPU5, RCPU5_GPIO_AND_EDGE_CLK_RST,
	BIT(2), BIT(2), 0, 0);

//rcpu
static SPACEMIT_CCU_GATE_NO_PARENT(rcan0_rst, "rcan0_rst", NULL,
	BASE_TYPE_RCPU, RCPU_CAN_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rcan1_rst, "rcan1_rst", NULL,
	BASE_TYPE_RCPU, RCPU_CAN1_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rcan2_rst, "rcan2_rst", NULL,
	BASE_TYPE_RCPU, RCPU_CAN2_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rcan3_rst, "rcan3_rst", NULL,
	BASE_TYPE_RCPU, RCPU_CAN3_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rcan4_rst, "rcan4_rst", NULL,
	BASE_TYPE_RCPU, RCPU_CAN4_CLK_RST,
	BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(rirc0_rst, "rirc0_rst", NULL,
	BASE_TYPE_RCPU, RCPU_IRC_CLK_RST,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(rirc1_rst, "rirc1_rst", NULL,
	BASE_TYPE_RCPU, RCPU_IRC1_CLK_RST,
	BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(remac_rst, "remac_rst", NULL,
	BASE_TYPE_RCPU, RCPU_GMAC_CLK_RST,
	BIT(1), BIT(1), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(respi_rst, "respi_rst", NULL,
	BASE_TYPE_RCPU, RCPU_ESPI_CLK_RST,
	BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(ri2s0_sys_rst, "ri2s0_sys_rst", NULL,
	BASE_TYPE_RCPU, RCPU_AUDIO_I2S0_SYS_CLK_CTRL,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ri2s1_sys_rst, "ri2s1_sys_rst", NULL,
	BASE_TYPE_RCPU, RCPU_AUDIO_I2S1_SYS_CLK_CTRL,
	BIT(0), BIT(0), 0, 0);

//rcpu1
static SPACEMIT_CCU_GATE_NO_PARENT(ruart0_rst, "ruart0_rst", NULL,
	BASE_TYPE_RCPU1, RCPU1_UART0_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(ruart1_rst, "ruart1_rst", NULL,
	BASE_TYPE_RCPU1, RCPU1_UART1_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(ruart2_rst, "ruart2_rst", NULL,
	BASE_TYPE_RCPU1, RCPU1_UART2_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(ruart3_rst, "ruart3_rst", NULL,
	BASE_TYPE_RCPU1, RCPU1_UART3_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(ruart4_rst, "ruart4_rst", NULL,
	BASE_TYPE_RCPU1, RCPU1_UART4_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(ruart5_rst, "ruart5_rst", NULL,
	BASE_TYPE_RCPU1, RCPU1_UART5_CLK_RST,
	BIT(2), 0, BIT(2), 0 );

//rcpu2
static SPACEMIT_CCU_GATE_NO_PARENT(ri2s0_rst, "ri2s0_rst", NULL,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S0_TX_RX_CLK_CTRL,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ri2s1_rst, "ri2s1_rst", NULL,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S1_TX_RX_CLK_CTRL,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ri2s2_rst, "ri2s2_rst", NULL,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S2_TX_RX_CLK_CTRL,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ri2s3_rst, "ri2s3_rst", NULL,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S3_TX_RX_CLK_CTRL,
	BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(ri2s2_sys_rst, "ri2s2_sys_rst", NULL,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S2_SYS_CLK_CTRL,
	BIT(0), BIT(0), 0, 0);
static SPACEMIT_CCU_GATE_NO_PARENT(ri2s3_sys_rst, "ri2s3_sys_rst", NULL,
	BASE_TYPE_RCPU2, RCPU2_AUDIO_I2S3_SYS_CLK_CTRL,
	BIT(0), BIT(0), 0, 0);

//rcpu3
static SPACEMIT_CCU_GATE_NO_PARENT(rspi0_rst, "rspi0_rst", NULL,
	BASE_TYPE_RCPU3, RCPU3_SSP0_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(rspi1_rst, "rspi1_rst", NULL,
	BASE_TYPE_RCPU3, RCPU3_SSP1_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(rspi2_rst, "rspi2_rst", NULL,
	BASE_TYPE_RCPU3, RCPU3_PWR_SSP_CLK_RST,
	BIT(2), 0, BIT(2), 0 );

//rcpu4
static SPACEMIT_CCU_GATE_NO_PARENT(ri2c0_rst, "ri2c0_rst", NULL,
	BASE_TYPE_RCPU4, RCPU4_I2C0_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(ri2c1_rst, "ri2c1_rst", NULL,
	BASE_TYPE_RCPU4, RCPU4_I2C1_CLK_RST,
	BIT(2), 0, BIT(2), 0 );
static SPACEMIT_CCU_GATE_NO_PARENT(ri2c2_rst, "ri2c2_rst", NULL,
	BASE_TYPE_RCPU4, RCPU4_PWR_I2C_CLK_RST,
	BIT(2), 0, BIT(2), 0 );

//rcpu6
static SPACEMIT_CCU_GATE_NO_PARENT(rpwm0_rst, "rpwm0_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM0_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm1_rst, "rpwm1_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM1_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm2_rst, "rpwm2_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM2_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm3_rst, "rpwm3_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM3_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm4_rst, "rpwm4_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM4_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm5_rst, "rpwm5_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM5_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm6_rst, "rpwm6_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM6_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm7_rst, "rpwm7_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM7_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm8_rst, "rpwm8_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM8_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm9_rst, "rpwm9_rst", NULL,
	BASE_TYPE_RCPU6, RCPU6_PWM9_CLK_RST,
	BIT(2) | BIT(0), BIT(0), BIT(2), 0 );

static struct clk_hw_onecell_data spacemit_k3_hw_clks = {
	.num = CLK_MAX_NO,
	.hws = {
		[CLK_PLL2]		= &pll2.common.hw,
		[CLK_PLL6]		= &pll6.common.hw,
		[CLK_PLL1_D2]		= &pll1_d2.common.hw,
		[CLK_PLL1_D3]		= &pll1_d3.common.hw,
		[CLK_PLL1_D4]		= &pll1_d4.common.hw,
		[CLK_PLL1_D5]		= &pll1_d5.common.hw,
		[CLK_PLL1_D6]		= &pll1_d6.common.hw,
		[CLK_PLL1_D7]		= &pll1_d7.common.hw,
		[CLK_PLL1_D8]		= &pll1_d8.common.hw,
		[CLK_PLL1_DX]		= &pll1_dx.common.hw,
		[CLK_PLL1_D64]		= &pll1_d64_38p4.common.hw,
		[CLK_PLL1_D10_AUD]	= &pll1_aud_245p7.common.hw,
		[CLK_PLL1_D100_AUD]	= &pll1_aud_24p5.common.hw,
		[CLK_PLL2_D3]		= &pll2_d3.common.hw,
		[CLK_PLL2_D5]		= &pll2_d5.common.hw,
		[CLK_PLL2_66]		= &pll2_66.common.hw,
		[CLK_PLL2_33]		= &pll2_33.common.hw,
		[CLK_PLL2_50]		= &pll2_50.common.hw,
		[CLK_PLL2_25]		= &pll2_25.common.hw,
		[CLK_PLL2_20]		= &pll2_20.common.hw,
		[CLK_PLL2_D24_125]	= &pll2_d24_125.common.hw,
		[CLK_PLL2_D120_25]	= &pll2_d120_25.common.hw,
		[CLK_PLL6_D5]		= &pll6_d5.common.hw,
		[CLK_PLL6_80]		= &pll6_80.common.hw,
		[CLK_PLL6_40]		= &pll6_40.common.hw,
		[CLK_PLL6_20]		= &pll6_20.common.hw,
		//mpmu
		[CLK_PLL1_307P2]	= &pll1_d8_307p2.common.hw,
		[CLK_PLL1_76P8]		= &pll1_d32_76p8.common.hw,
		[CLK_PLL1_61P44]	= &pll1_d40_61p44.common.hw,
		[CLK_PLL1_153P6]	= &pll1_d16_153p6.common.hw,
		[CLK_PLL1_102P4]	= &pll1_d24_102p4.common.hw,
		[CLK_PLL1_51P2]		= &pll1_d48_51p2.common.hw,
		[CLK_PLL1_51P2_AP]	= &pll1_d48_51p2_ap.common.hw,
		[CLK_PLL1_57P6]		= &pll1_m3d128_57p6.common.hw,
		[CLK_PLL1_25P6]		= &pll1_d96_25p6.common.hw,
		[CLK_PLL1_12P8]		= &pll1_d192_12p8.common.hw,
		[CLK_PLL1_12P8_WDT]	= &pll1_d192_12p8_wdt.common.hw,
		[CLK_PLL1_6P4]		= &pll1_d384_6p4.common.hw,
		[CLK_PLL1_3P2]		= &pll1_d768_3p2.common.hw,
		[CLK_PLL1_1P6]		= &pll1_d1536_1p6.common.hw,
		[CLK_PLL1_0P8]		= &pll1_d3072_0p8.common.hw,
		[CLK_PLL1_351]		= &pll1_d7_351p08.common.hw,
		[CLK_PLL1_409P6]	= &pll1_d6_409p6.common.hw,
		[CLK_PLL1_204P8]	= &pll1_d12_204p8.common.hw,
		[CLK_PLL1_491]		= &pll1_d5_491p52.common.hw,
		[CLK_PLL1_245P76]	= &pll1_d10_245p76.common.hw,
		[CLK_PLL1_614]		= &pll1_d4_614p4.common.hw,
		[CLK_PLL1_47P26]	= &pll1_d52_47p26.common.hw,
		[CLK_PLL1_31P5]		= &pll1_d78_31p5.common.hw,
		[CLK_PLL1_819]		= &pll1_d3_819p2.common.hw,
		[CLK_PLL1_1228]		= &pll1_d2_1228p8.common.hw,
		[CLK_SLOW_UART1]	= &slow_uart1_14p74.common.hw,
		//apmu
		[CLK_RCPU]		= &rcpu_clk.common.hw,
		//rcpu5
		[CLK_RCPU_APB]		= &rcpu_apb_clk.common.hw,
		[CLK_RCPU_AXI]		= &rcpu_axi_clk.common.hw,
		[CLK_RCPU_IPC2MSA]	= &ripc2msa_clk.common.hw,
		[CLK_RCPU_IPC2CP]	= &ripc2cp_clk.common.hw,
		[CLK_RCPU_IPC2AP]	= &ripc2ap_clk.common.hw,
		[CLK_RCPU_TIMER1]	= &rtimer1_clk.common.hw,
		[CLK_RCPU_TIMER2]	= &rtimer2_clk.common.hw,
		[CLK_RCPU_TIMER3]	= &rtimer3_clk.common.hw,
		[CLK_RCPU_TIMER4]	= &rtimer4_clk.common.hw,
		[CLK_RCPU_RT24_CORE0]	= &rt24_core0_clk.common.hw,
		[CLK_RCPU_RT24_CORE1]	= &rt24_core1_clk.common.hw,
		[CLK_RCPU_GPIO]		= &rgpio_clk.common.hw,
		[CLK_RCPU_GPIO_EDGE]	= &rgpio_edge_clk.common.hw,
		[CLK_RCPU_GPIO_LP]	= &rgpio_lp_clk.common.hw,
		//rcpu
		[CLK_RCPU_CAN0]		= &rcan0_clk.common.hw,
		[CLK_RCPU_CAN1]		= &rcan1_clk.common.hw,
		[CLK_RCPU_CAN2]		= &rcan2_clk.common.hw,
		[CLK_RCPU_CAN3]		= &rcan3_clk.common.hw,
		[CLK_RCPU_CAN4]		= &rcan4_clk.common.hw,
		[CLK_RCPU_IRC0]		= &rirc0_clk.common.hw,
		[CLK_RCPU_IRC1]		= &rirc1_clk.common.hw,
		[CLK_RCPU_ESPI_SCLK_SRC]	= &respi_sclk_src.common.hw,
		[CLK_RCPU_ESPI_SCLK]		= &respi_sclk.common.hw,
		[CLK_RCPU_EMAC_BUS]		= &remac_bus_clk.common.hw,
		[CLK_RCPU_EMAC_REF]		= &remac_ref_clk.common.hw,
		[CLK_RCPU_EMAC_1588]		= &remac_1588_clk.common.hw,
		[CLK_RCPU_EMAC_RGMII_TX]	= &remac_rgmii_tx_clk.common.hw,
		[CLK_RCPU_I2S0_SYS]		= &ri2s0_sysclk.common.hw,
		[CLK_RCPU_I2S1_SYS]		= &ri2s1_sysclk.common.hw,
		//rcpu1
		[CLK_RCPU_UART0]	= &ruart0_clk.common.hw,
		[CLK_RCPU_UART1]	= &ruart1_clk.common.hw,
		[CLK_RCPU_UART2]	= &ruart2_clk.common.hw,
		[CLK_RCPU_UART3]	= &ruart3_clk.common.hw,
		[CLK_RCPU_UART4]	= &ruart4_clk.common.hw,
		[CLK_RCPU_UART5]	= &ruart5_clk.common.hw,
		//rcpu2
		[CLK_RCPU_I2S0]		= &ri2s0_clk.common.hw,
		[CLK_RCPU_I2S1]		= &ri2s1_clk.common.hw,
		[CLK_RCPU_I2S2]		= &ri2s2_clk.common.hw,
		[CLK_RCPU_I2S3]		= &ri2s3_clk.common.hw,
		[CLK_RCPU_I2S2_SYS]	= &ri2s2_sysclk.common.hw,
		[CLK_RCPU_I2S3_SYS]	= &ri2s3_sysclk.common.hw,
		//rcpu3
		[CLK_RCPU_SPI0] 	= &rspi0_clk.common.hw,
		[CLK_RCPU_SPI1] 	= &rspi1_clk.common.hw,
		[CLK_RCPU_SPI2] 	= &rspi2_clk.common.hw,
		//rcpu4
		[CLK_RCPU_I2C0]		= &ri2c0_clk.common.hw,
		[CLK_RCPU_I2C1]		= &ri2c1_clk.common.hw,
		[CLK_RCPU_I2C2]		= &ri2c2_clk.common.hw,
		//rcpu6
		[CLK_RCPU_PWM0]		= &rpwm0_clk.common.hw,
		[CLK_RCPU_PWM1]		= &rpwm1_clk.common.hw,
		[CLK_RCPU_PWM2]		= &rpwm2_clk.common.hw,
		[CLK_RCPU_PWM3]		= &rpwm3_clk.common.hw,
		[CLK_RCPU_PWM4]		= &rpwm4_clk.common.hw,
		[CLK_RCPU_PWM5]		= &rpwm5_clk.common.hw,
		[CLK_RCPU_PWM6]		= &rpwm6_clk.common.hw,
		[CLK_RCPU_PWM7]		= &rpwm7_clk.common.hw,
		[CLK_RCPU_PWM8]		= &rpwm8_clk.common.hw,
		[CLK_RCPU_PWM9]		= &rpwm9_clk.common.hw,
		//resets
		//rcpu5
		[CLK_RST_RCPU_IPC2MSA]		= &ripc2msa_rst.common.hw,
		[CLK_RST_RCPU_IPC2CP]		= &ripc2cp_rst.common.hw,
		[CLK_RST_RCPU_IPC2AP]		= &ripc2ap_rst.common.hw,
		[CLK_RST_RCPU_TIMER1]		= &rtimer1_rst.common.hw,
		[CLK_RST_RCPU_TIMER2]		= &rtimer2_rst.common.hw,
		[CLK_RST_RCPU_TIMER3]		= &rtimer3_rst.common.hw,
		[CLK_RST_RCPU_TIMER4]		= &rtimer4_rst.common.hw,
		[CLK_RST_RCPU_RT24_CORE0]	= &rt24_core0_rst.common.hw,
		[CLK_RST_RCPU_RT24_CORE1]	= &rt24_core1_rst.common.hw,
		[CLK_RST_RCPU_GPIO]		= &rgpio_rst.common.hw,
		[CLK_RST_RCPU_GPIO_EDGE]	= &rgpio_edge_rst.common.hw,
		//rcpu
		[CLK_RST_RCPU_CAN0]		= &rcan0_rst.common.hw,
		[CLK_RST_RCPU_CAN1]		= &rcan1_rst.common.hw,
		[CLK_RST_RCPU_CAN2]		= &rcan2_rst.common.hw,
		[CLK_RST_RCPU_CAN3]		= &rcan3_rst.common.hw,
		[CLK_RST_RCPU_CAN4]		= &rcan4_rst.common.hw,
		[CLK_RST_RCPU_IRC0]		= &rirc0_rst.common.hw,
		[CLK_RST_RCPU_IRC1]		= &rirc1_rst.common.hw,
		[CLK_RST_RCPU_ESPI]		= &respi_rst.common.hw,
		[CLK_RST_RCPU_EMAC]		= &remac_rst.common.hw,
		[CLK_RST_RCPU_I2S0_SYS]		= &ri2s0_sys_rst.common.hw,
		[CLK_RST_RCPU_I2S1_SYS]		= &ri2s1_sys_rst.common.hw,
		//rcpu1
		[CLK_RST_RCPU_UART0]		= &ruart0_rst.common.hw,
		[CLK_RST_RCPU_UART1]		= &ruart1_rst.common.hw,
		[CLK_RST_RCPU_UART2]		= &ruart2_rst.common.hw,
		[CLK_RST_RCPU_UART3]		= &ruart3_rst.common.hw,
		[CLK_RST_RCPU_UART4]		= &ruart4_rst.common.hw,
		[CLK_RST_RCPU_UART5]		= &ruart5_rst.common.hw,
		//rcpu2
		[CLK_RST_RCPU_I2S0]		= &ri2s0_rst.common.hw,
		[CLK_RST_RCPU_I2S1]		= &ri2s1_rst.common.hw,
		[CLK_RST_RCPU_I2S2]		= &ri2s2_rst.common.hw,
		[CLK_RST_RCPU_I2S3]		= &ri2s3_rst.common.hw,
		[CLK_RST_RCPU_I2S2_SYS]		= &ri2s2_sys_rst.common.hw,
		[CLK_RST_RCPU_I2S3_SYS]		= &ri2s3_sys_rst.common.hw,
		//rcpu3
		[CLK_RST_RCPU_SPI0]		= &rspi0_rst.common.hw,
		[CLK_RST_RCPU_SPI1]		= &rspi1_rst.common.hw,
		[CLK_RST_RCPU_SPI2]		= &rspi2_rst.common.hw,
		//rcpu4
		[CLK_RST_RCPU_I2C0]		= &ri2c0_rst.common.hw,
		[CLK_RST_RCPU_I2C1]		= &ri2c1_rst.common.hw,
		[CLK_RST_RCPU_I2C2]		= &ri2c2_rst.common.hw,
		//rcpu6
		[CLK_RST_RCPU_PWM0]		= &rpwm0_rst.common.hw,
		[CLK_RST_RCPU_PWM1]		= &rpwm1_rst.common.hw,
		[CLK_RST_RCPU_PWM2]		= &rpwm2_rst.common.hw,
		[CLK_RST_RCPU_PWM3]		= &rpwm3_rst.common.hw,
		[CLK_RST_RCPU_PWM4]		= &rpwm4_rst.common.hw,
		[CLK_RST_RCPU_PWM5]		= &rpwm5_rst.common.hw,
		[CLK_RST_RCPU_PWM6]		= &rpwm6_rst.common.hw,
		[CLK_RST_RCPU_PWM7]		= &rpwm7_rst.common.hw,
		[CLK_RST_RCPU_PWM8]		= &rpwm8_rst.common.hw,
		[CLK_RST_RCPU_PWM9]		= &rpwm9_rst.common.hw,

		[CLK_MAX_NO]			= RT_NULL,
	},
};


static struct spacemit_ccu spacemit_ccu_k3 = {
	.clk_cells = &spacemit_k3_hw_clks,
};

struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,rcpu-ccu-k3", .data = (void *)&spacemit_ccu_k3 },
	{},
};

int ccu_common_init(struct clk_hw * hw, struct spacemit_k1x_clk *clk_info)
{
	struct ccu_common *common = hw_to_ccu_common(hw);
	struct ccu_plla *pll = hw_to_ccu_plla(hw);

	if (!common)
		return -1;

	rt_spin_lock_init(&common->lock);

	switch(common->base_type) {
	case BASE_TYPE_MPMU:
		common->base = clk_info->mpmu_base;
		break;
	case BASE_TYPE_APBS:
		common->base = clk_info->apbs_base;
		break;
	case BASE_TYPE_APMU:
		common->base = clk_info->apmu_base;
		break;
	case BASE_TYPE_RCPU:
		common->base = clk_info->rcpu_base;
		break;
	case BASE_TYPE_RCPU1:
		common->base = clk_info->rcpu1_base;
		break;
	case BASE_TYPE_RCPU2:
		common->base = clk_info->rcpu2_base;
		break;
	case BASE_TYPE_RCPU3:
		common->base = clk_info->rcpu3_base;
		break;
	case BASE_TYPE_RCPU4:
		common->base = clk_info->rcpu4_base;
		break;
	case BASE_TYPE_RCPU5:
		common->base = clk_info->rcpu5_base;
		break;
	case BASE_TYPE_RCPU6:
		common->base = clk_info->rcpu6_base;
		break;
	default:
		common->base = clk_info->rcpu_base;
		break;
	}

	if(common->is_pll)
		pll->pll.lock_base = clk_info->mpmu_base;

	return 0;
}

static int spacemit_ccu_probe(struct dtb_node *np, struct spacemit_ccu *ccu)
{
	int i, ret;
	struct clk *clk;
	struct clk_hw *hw;

	for (i = 0; i < ccu->clk_cells->num; ++i) {
		hw = ccu->clk_cells->hws[i];
		if (!hw)
			continue;
		if (!hw->init)
			continue;

		ccu_common_init(hw, &ccu->clk_info);

		clk = of_clk_hw_register(np, hw);
		if (IS_ERR(clk)) {
			rt_kprintf("register hw clk error");
			return -RT_EINVAL;
		}

		clk_hw_register_clkdev(hw, clk_hw_get_name(hw), RT_NULL);
	}

	ret = of_clk_add_hw_provider(np, of_clk_hw_onecell_get, ccu->clk_cells);
	if (ret)
		return ret;

	return 0;
}

int spacemit_ccu_init(void)
{
	int i;
	struct spacemit_ccu *ccu;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			ccu = (struct spacemit_ccu *)__compatible[i].data;

			ccu->clk_info.mpmu_base = (void *)dtb_node_get_addr_index(compatible_node, 0);
			if (ccu->clk_info.mpmu_base < 0) {
				rt_kprintf("get mpmu error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.apmu_base = (void *)dtb_node_get_addr_index(compatible_node, 1);
			if (ccu->clk_info.apmu_base < 0) {
				rt_kprintf("get apmu error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.apbs_base = (void *)dtb_node_get_addr_index(compatible_node, 2);
			if (ccu->clk_info.apbs_base < 0) {
				rt_kprintf("get apbs error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu_base = (void *)dtb_node_get_addr_index(compatible_node, 3);
			if (ccu->clk_info.rcpu_base < 0) {
				rt_kprintf("get rcpu error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu1_base = (void *)dtb_node_get_addr_index(compatible_node, 4);
			if (ccu->clk_info.rcpu1_base < 0) {
				rt_kprintf("get rcpu1 error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu2_base = (void *)dtb_node_get_addr_index(compatible_node, 5);
			if (ccu->clk_info.rcpu2_base < 0) {
				rt_kprintf("get rcpu2 error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu3_base = (void *)dtb_node_get_addr_index(compatible_node, 6);
			if (ccu->clk_info.rcpu3_base < 0) {
				rt_kprintf("get rcpu3 error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu4_base = (void *)dtb_node_get_addr_index(compatible_node, 7);
			if (ccu->clk_info.rcpu4_base < 0) {
				rt_kprintf("get rcpu4 error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu5_base = (void *)dtb_node_get_addr_index(compatible_node, 8);
			if (ccu->clk_info.rcpu5_base < 0) {
				rt_kprintf("get rcpu5 error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu6_base = (void *)dtb_node_get_addr_index(compatible_node, 9);
			if (ccu->clk_info.rcpu6_base < 0) {
				rt_kprintf("get rcpu6 error\n");
				return -RT_ERROR;
			}

			spacemit_ccu_probe(compatible_node, ccu);
		}
	}

	return 0;
}
