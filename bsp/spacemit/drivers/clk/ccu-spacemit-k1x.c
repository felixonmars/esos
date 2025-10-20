/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "ccu_pll.h"
#include "ccu_mix.h"
#include "ccu-spacemit-k1x.h"
#include "../../platform/n308/ccu-spacemit-k1x.h"

struct spacemit_ccu {
	struct spacemit_k1x_clk clk_info;
	struct clk_hw_onecell_data *clk_cells;
};

static const struct ccu_pll_rate_tbl pll3_rate_tbl[] = {
	PLL_RATE(1600000000UL, 0x61, 0xcd, 0x50, 0x00, 0x43, 0xeaaaab),
	PLL_RATE(1800000000UL, 0x61, 0xcd, 0x50, 0x00, 0x4b, 0x000000),
	PLL_RATE(2000000000UL, 0x62, 0xdd, 0x50, 0x00, 0x2a, 0xeaaaab),
	PLL_RATE(3000000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3f, 0xe00000),
	PLL_RATE(3200000000UL, 0x67, 0xdd, 0x50, 0x00, 0x43, 0xeaaaab),
	PLL_RATE(2457600000UL, 0x64, 0xdd, 0x50, 0x00, 0x33, 0x0ccccd),
};

static SPACEMIT_CCU_PLL(pll3, "pll3", &pll3_rate_tbl, ARRAY_SIZE(pll3_rate_tbl),
		BASE_TYPE_APBS, APB_SPARE10_REG, APB_SPARE11_REG, APB_SPARE12_REG,
		MPMU_POSR, POSR_PLL3_LOCK, 1,
		CLK_IGNORE_UNUSED);

//pll1
static SPACEMIT_CCU_GATE_FACTOR(pll1_d2, "pll1_d2", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(1), BIT(1), 0x0,
		2, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d3, "pll1_d3", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(2), BIT(2), 0x0,
		3, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d4, "pll1_d4", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(3), BIT(3), 0x0,
		4, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d5, "pll1_d5", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(4), BIT(4), 0x0,
		5, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d6, "pll1_d6", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(5), BIT(5), 0x0,
		6, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d7, "pll1_d7", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(6), BIT(6), 0x0,
		7, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d8, "pll1_d8", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(7), BIT(7), 0x0,
		8, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d11_223p4, "pll1_d11_223p4", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(15), BIT(15), 0x0,
		11, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d13_189, "pll1_d13_189", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(16), BIT(16), 0x0,
		13, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d23_106p8, "pll1_d23_106p8", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(20), BIT(20), 0x0,
		23, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d64_38p4, "pll1_d64_38p4", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(0), BIT(0), 0x0,
		64, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_aud_245p7, "pll1_aud_245p7", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(10), BIT(10), 0x0,
		10, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_aud_24p5, "pll1_aud_24p5", "pll1_2457p6_vco",
		BASE_TYPE_APBS, APB_SPARE2_REG,
		BIT(11), BIT(11), 0x0,
		100, 1, CLK_IGNORE_UNUSED);

//pll3
static SPACEMIT_CCU_GATE_FACTOR(pll3_d1, "pll3_d1", "pll3",
		BASE_TYPE_APBS, APB_SPARE11_REG,
		BIT(0), BIT(0), 0x0,
		1, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll3_d2, "pll3_d2", "pll3",
		BASE_TYPE_APBS, APB_SPARE11_REG,
		BIT(1), BIT(1), 0x0,
		2, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll3_d3, "pll3_d3", "pll3",
		BASE_TYPE_APBS, APB_SPARE11_REG,
		BIT(2), BIT(2), 0x0,
		3, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll3_d4, "pll3_d4", "pll3",
		BASE_TYPE_APBS, APB_SPARE11_REG,
		BIT(3), BIT(3), 0x0,
		4, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll3_d5, "pll3_d5", "pll3",
		BASE_TYPE_APBS, APB_SPARE11_REG,
		BIT(4), BIT(4), 0x0,
		5, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll3_d6, "pll3_d6", "pll3",
		BASE_TYPE_APBS, APB_SPARE11_REG,
		BIT(5), BIT(5), 0x0,
		6, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll3_d7, "pll3_d7", "pll3",
		BASE_TYPE_APBS, APB_SPARE11_REG,
		BIT(6), BIT(6), 0x0,
		7, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll3_d8, "pll3_d8", "pll3",
		BASE_TYPE_APBS, APB_SPARE11_REG,
		BIT(7), BIT(7), 0x0,
		8, 1, CLK_IGNORE_UNUSED);
//pll3_div
static SPACEMIT_CCU_FACTOR(pll3_80, "pll3_80", "pll3_d8", 5, 1);
static SPACEMIT_CCU_FACTOR(pll3_40, "pll3_40", "pll3_d8", 10, 1);
static SPACEMIT_CCU_FACTOR(pll3_20, "pll3_20", "pll3_d8", 20, 1);

//pll1_d8
static SPACEMIT_CCU_GATE(pll1_d8_307p2, "pll1_d8_307p2", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(13), BIT(13), 0x0,
		CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_FACTOR(pll1_d32_76p8, "pll1_d32_76p8", "pll1_d8_307p2", 4, 1);
static SPACEMIT_CCU_FACTOR(pll1_d40_61p44, "pll1_d40_61p44", "pll1_d8_307p2", 5, 1);
static SPACEMIT_CCU_FACTOR(pll1_d16_153p6, "pll1_d16_153p6", "pll1_d8", 2, 1);
static SPACEMIT_CCU_GATE_FACTOR(pll1_d24_102p4, "pll1_d24_102p4", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(12), BIT(12), 0x0,
		3, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d48_51p2, "pll1_d48_51p2", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(7), BIT(7), 0x0,
		6, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d48_51p2_ap, "pll1_d48_51p2_ap", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(11), BIT(11), 0x0,
		6, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_m3d128_57p6, "pll1_m3d128_57p6", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(8), BIT(8), 0x0,
		16, 3, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d96_25p6, "pll1_d96_25p6", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(4), BIT(4), 0x0,
		12, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d192_12p8, "pll1_d192_12p8", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(3), BIT(3), 0x0,
		24, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d192_12p8_wdt, "pll1_d192_12p8_wdt", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(19), BIT(19), 0x0,
		24, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d384_6p4, "pll1_d384_6p4", "pll1_d8",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(2), BIT(2), 0x0,
		48, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_FACTOR(pll1_d768_3p2, "pll1_d768_3p2", "pll1_d384_6p4", 2, 1);
static SPACEMIT_CCU_FACTOR(pll1_d1536_1p6, "pll1_d1536_1p6", "pll1_d384_6p4", 4, 1);
static SPACEMIT_CCU_FACTOR(pll1_d3072_0p8, "pll1_d3072_0p8", "pll1_d384_6p4", 8, 1);

//pll1_d7
static SPACEMIT_CCU_FACTOR(pll1_d7_351p08, "pll1_d7_351p08", "pll1_d7", 1, 1);

//pll1_d6
static SPACEMIT_CCU_GATE(pll1_d6_409p6, "pll1_d6_409p6", "pll1_d6",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(0), BIT(0), 0x0,
		CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d12_204p8, "pll1_d12_204p8", "pll1_d6",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(5), BIT(5), 0x0,
		2, 1, CLK_IGNORE_UNUSED);

//pll1_d5
static SPACEMIT_CCU_GATE(pll1_d5_491p52, "pll1_d5_491p52", "pll1_d5",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(21), BIT(21), 0x0,
		CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d10_245p76, "pll1_d10_245p76", "pll1_d5",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(18), BIT(18), 0x0,
		2, 1, CLK_IGNORE_UNUSED);

//pll1_d4
static SPACEMIT_CCU_GATE(pll1_d4_614p4, "pll1_d4_614p4", "pll1_d4",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(15), BIT(15), 0x0,
		CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d52_47p26, "pll1_d52_47p26", "pll1_d4",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(10), BIT(10), 0x0,
		13, 1, CLK_IGNORE_UNUSED);

static SPACEMIT_CCU_GATE_FACTOR(pll1_d78_31p5, "pll1_d78_31p5", "pll1_d4",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(6), BIT(6), 0x0,
		39, 2, CLK_IGNORE_UNUSED);

//pll1_d3
static SPACEMIT_CCU_GATE(pll1_d3_819p2, "pll1_d3_819p2", "pll1_d3",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(14), BIT(14), 0x0,
		CLK_IGNORE_UNUSED);

//pll1_d2
static SPACEMIT_CCU_GATE(pll1_d2_1228p8, "pll1_d2_1228p8", "pll1_d2",
		BASE_TYPE_MPMU, MPMU_ACGR,
		BIT(16), BIT(16), 0x0,
		CLK_IGNORE_UNUSED);

//rcpu
static const char *rhdmi_audio_parent_names[] = {
	"pll1_aud_24p5", "pll1_aud_245p7"
};

static SPACEMIT_CCU_DIV_MUX_GATE(rhdmi_audio_clk, "rhdmi_audio_clk", rhdmi_audio_parent_names,
		BASE_TYPE_RCPU, RCPU_HDMI_CLK_RST,
		4, 11, 16, 2,
		0x6, 0x6, 0x0,
		0);

//rcan
static const char *rcan_parent_names[] = {
	"pll3_20", "pll3_40", "pll3_80"
};

static SPACEMIT_CCU_DIV_MUX_GATE(rcan_clk, "rcan_clk", rcan_parent_names,
		BASE_TYPE_RCPU, RCPU_CAN_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_GATE_NO_PARENT(rcan_bus_clk, "rcan_bus_clk", NULL,
		BASE_TYPE_RCPU, RCPU_CAN_CLK_RST,
		BIT(2), BIT(2), 0x0, 0);


//rcpu2
static const char *rpwm_parent_names[] = {
	"pll1_aud_245p7", "pll1_aud_24p5"
};
static SPACEMIT_CCU_DIV_MUX_GATE(rpwm0_clk, "rpwm0_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM0_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm1_clk, "rpwm1_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM1_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm2_clk, "rpwm2_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM2_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm3_clk, "rpwm3_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM3_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm4_clk, "rpwm4_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM4_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm5_clk, "rpwm5_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM5_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm6_clk, "rpwm6_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM6_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm7_clk, "rpwm7_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM7_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm8_clk, "rpwm8_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM8_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static SPACEMIT_CCU_DIV_MUX_GATE(rpwm9_clk, "rpwm9_clk", rpwm_parent_names,
		BASE_TYPE_RCPU2, RCPU2_PWM9_CLK_RST,
		8, 11, 4, 2,
		BIT(1), BIT(1), 0x0,
		0);

static const char *ri2c_parent_names[] = {
	"pll1_d40_61p44", "pll1_d96_25p6", "pll1_d192_12p8", "vctcxo_3"
};
static SPACEMIT_CCU_DIV_MUX_GATE(ri2c0_clk, "ri2c0_clk", ri2c_parent_names,
		BASE_TYPE_RCPU, RCPU_I2C0_CLK_RST,
		8, 11, 4, 2,
		0x6, 0x6, 0x0,
		0);

static SPACEMIT_CCU_GATE_NO_PARENT(rir_clk, "rir_clk", NULL,
		BASE_TYPE_RCPU, RCPU_IR_CLK_RST,
		BIT(2), BIT(2), 0x0,
		0);

static const char *ruart0_parent_names[] = {
	"pll1_aud_24p5", "pll1_aud_245p7", "vctcxo_24", "vctcxo_3"
};

static SPACEMIT_CCU_DIV_MUX_GATE(ruart0_clk, "ruart0_clk", ruart0_parent_names,
		BASE_TYPE_RCPU, RCPU_UART0_CLK_RST,
		8, 11, 4, 2,
		0x6, 0x6, 0x0,
		0);

static const char *ruart1_parent_names[] = {
	"pll1_aud_24p5", "pll1_aud_245p7", "vctcxo_24", "vctcxo_3"
};

static SPACEMIT_CCU_DIV_MUX_GATE(ruart1_clk, "ruart1_clk", ruart1_parent_names,
		BASE_TYPE_RCPU, RCPU_UART1_CLK_RST,
		8, 11, 4, 2,
		0x6, 0x6, 0x0,
		0);

static const char *rssp0_parent_names[] = {
	"pll1_d40_61p44", "pll1_d96_25p6", "pll1_d192_12p8", "vctcxo_3"
};

static SPACEMIT_CCU_DIV_MUX_GATE(rssp0_clk, "rssp0_clk", rssp0_parent_names,
		BASE_TYPE_RCPU, RCPU_SSP0_CLK_RST,
		8, 11, 4, 2,
		0x6, 0x6, 0x0,
		0);

//resets
static SPACEMIT_CCU_GATE_NO_PARENT(rhdmi_audio_rst, "rhdmi_audio_rst", NULL,
		BASE_TYPE_RCPU, RCPU_HDMI_CLK_RST,
		BIT(0), BIT(0), 0x0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(rcan_rst, "rcan_rst", NULL,
		BASE_TYPE_RCPU, RCPU_CAN_CLK_RST,
		BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm0_rst, "rpwm0_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM0_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm1_rst, "rpwm1_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM1_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm2_rst, "rpwm2_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM2_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm3_rst, "rpwm3_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM3_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm4_rst, "rpwm4_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM4_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm5_rst, "rpwm5_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM5_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm6_rst, "rpwm6_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM6_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm7_rst, "rpwm7_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM7_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm8_rst, "rpwm8_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM8_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(rpwm9_rst, "rpwm9_rst", NULL,
		BASE_TYPE_RCPU2, RCPU2_PWM9_CLK_RST,
		BIT(2)|BIT(0), BIT(0), BIT(2), 0 );

static SPACEMIT_CCU_GATE_NO_PARENT(ri2c0_rst, "ri2c0_rst", NULL,
		BASE_TYPE_RCPU, RCPU_I2C0_CLK_RST,
		BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(rir_rst, "rir_rst", NULL,
		BASE_TYPE_RCPU, RCPU_IR_CLK_RST,
		BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(ruart0_rst, "ruart0_rst", NULL,
		BASE_TYPE_RCPU, RCPU_UART0_CLK_RST,
		BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(ruart1_rst, "ruart1_rst", NULL,
		BASE_TYPE_RCPU, RCPU_UART1_CLK_RST,
		BIT(0), BIT(0), 0, 0);

static SPACEMIT_CCU_GATE_NO_PARENT(rssp0_rst, "rssp0_rst", NULL,
		BASE_TYPE_RCPU, RCPU_SSP0_CLK_RST,
		BIT(0), BIT(0), 0, 0);

static struct clk_hw_onecell_data spacemit_k1x_hw_clks = {
	.num = CLK_MAX_NO, 
	.hws = {
		[CLK_PLL3] = &pll3.common.hw,
		[CLK_PLL1_D2] = &pll1_d2.common.hw,
		[CLK_PLL1_D3] = &pll1_d3.common.hw,
		[CLK_PLL1_D4] = &pll1_d4.common.hw,
		[CLK_PLL1_D5] = &pll1_d5.common.hw,
		[CLK_PLL1_D6] = &pll1_d6.common.hw,
		[CLK_PLL1_D7] = &pll1_d7.common.hw,
		[CLK_PLL1_D8] = &pll1_d8.common.hw,
		[CLK_PLL1_D11] = &pll1_d11_223p4.common.hw,
		[CLK_PLL1_D13] = &pll1_d13_189.common.hw,
		[CLK_PLL1_D23] = &pll1_d23_106p8.common.hw,
		[CLK_PLL1_D64] = &pll1_d64_38p4.common.hw,
		[CLK_PLL1_D10_AUD] = &pll1_aud_245p7.common.hw,
		[CLK_PLL1_D100_AUD] = &pll1_aud_24p5.common.hw,

		[CLK_PLL3_D1] = &pll3_d1.common.hw,
		[CLK_PLL3_D2] = &pll3_d2.common.hw,
		[CLK_PLL3_D3] = &pll3_d3.common.hw,
		[CLK_PLL3_D4] = &pll3_d4.common.hw,
		[CLK_PLL3_D5] = &pll3_d5.common.hw,
		[CLK_PLL3_D6] = &pll3_d6.common.hw,
		[CLK_PLL3_D7] = &pll3_d7.common.hw,
		[CLK_PLL3_D8] = &pll3_d8.common.hw,
		[CLK_PLL3_80] = &pll3_80.common.hw,
		[CLK_PLL3_40] = &pll3_40.common.hw,
		[CLK_PLL3_20] = &pll3_20.common.hw,

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

		[CLK_RCPU_HDMIAUDIO]	= &rhdmi_audio_clk.common.hw,
		[CLK_RCPU_CAN]		= &rcan_clk.common.hw,
		[CLK_RCPU_CAN_BUS]	= &rcan_bus_clk.common.hw,
		[CLK_RCPU2_PWM0]        = &rpwm0_clk.common.hw,
		[CLK_RCPU2_PWM1]        = &rpwm1_clk.common.hw,
		[CLK_RCPU2_PWM2]        = &rpwm2_clk.common.hw,
		[CLK_RCPU2_PWM3]        = &rpwm3_clk.common.hw,
		[CLK_RCPU2_PWM4]        = &rpwm4_clk.common.hw,
		[CLK_RCPU2_PWM5]        = &rpwm5_clk.common.hw,
		[CLK_RCPU2_PWM6]        = &rpwm6_clk.common.hw,
		[CLK_RCPU2_PWM7]        = &rpwm7_clk.common.hw,
		[CLK_RCPU2_PWM8]        = &rpwm8_clk.common.hw,
		[CLK_RCPU2_PWM9]        = &rpwm9_clk.common.hw,

		[CLK_RCPU_I2C0]		= &ri2c0_clk.common.hw,
		[CLK_RCPU_IR]		= &rir_clk.common.hw,
		[CLK_RCPU_UART0]	= &ruart0_clk.common.hw,
		[CLK_RCPU_UART1]	= &ruart1_clk.common.hw,
		[CLK_RCPU_SSP0] 	= &rssp0_clk.common.hw,

		[CLK_RST_RCPU_HDMIAUDIO]    = &rhdmi_audio_rst.common.hw,
		[CLK_RST_RCPU_CAN]          = &rcan_rst.common.hw,
		[CLK_RST_RCPU2_PWM0]        = &rpwm0_rst.common.hw,
		[CLK_RST_RCPU2_PWM1]        = &rpwm1_rst.common.hw,
		[CLK_RST_RCPU2_PWM2]        = &rpwm2_rst.common.hw,
		[CLK_RST_RCPU2_PWM3]        = &rpwm3_rst.common.hw,
		[CLK_RST_RCPU2_PWM4]        = &rpwm4_rst.common.hw,
		[CLK_RST_RCPU2_PWM5]        = &rpwm5_rst.common.hw,
		[CLK_RST_RCPU2_PWM6]        = &rpwm6_rst.common.hw,
		[CLK_RST_RCPU2_PWM7]        = &rpwm7_rst.common.hw,
		[CLK_RST_RCPU2_PWM8]        = &rpwm8_rst.common.hw,
		[CLK_RST_RCPU2_PWM9]        = &rpwm9_rst.common.hw,

		[CLK_RST_RCPU_I2C0]         = &ri2c0_rst.common.hw,
		[CLK_RST_RCPU_IR]           = &rir_rst.common.hw,
		[CLK_RST_RCPU_UART0]        = &ruart0_rst.common.hw,
		[CLK_RST_RCPU_UART1]        = &ruart1_rst.common.hw,
		[CLK_RST_RCPU_SSP0]         = &rssp0_rst.common.hw,

		[CLK_MAX_NO]		= RT_NULL,
	},
};


static struct spacemit_ccu spacemit_ccu_k1x = {
	.clk_cells = &spacemit_k1x_hw_clks,
};

struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,rcpu-ccu-k1x", .data = (void *)&spacemit_ccu_k1x },
	{ .compatible = "spacemit,rcpu-ccu-k3", .data = (void *)&spacemit_ccu_k1x },
	{},
};

int ccu_common_init(struct clk_hw * hw, struct spacemit_k1x_clk *clk_info)
{
	struct ccu_common *common = hw_to_ccu_common(hw);
	struct ccu_pll *pll = hw_to_ccu_pll(hw);

	if (!common)
		return -1;

	rt_spin_lock_init(&common->lock);

	switch(common->base_type){
	case BASE_TYPE_MPMU:
		common->base = clk_info->mpmu_base;
		break;
	case BASE_TYPE_APMU:
		common->base = clk_info->apmu_base;
		break;
 	case BASE_TYPE_APBC:
		common->base = clk_info->apbc_base;
		break;
	case BASE_TYPE_APBS:
		common->base = clk_info->apbs_base;
		break;
	case BASE_TYPE_CIU:
		common->base = clk_info->ciu_base;
		break;
	case BASE_TYPE_DCIU:
		common->base = clk_info->dciu_base;
		break;
#ifndef SOC_SPACEMIT_K3
	case BASE_TYPE_DDRC:
		common->base = clk_info->ddrc_base;
		break;
	case BASE_TYPE_APBC2:
		common->base = clk_info->apbc2_base;
		break;
#endif
	case BASE_TYPE_RCPU:
		common->base = clk_info->rcpu_base;
		break;
	case BASE_TYPE_RCPU2:
		common->base = clk_info->rcpu2_base;
		break;
	default:
		common->base = clk_info->apbc_base;
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

			ccu->clk_info.apbc_base = (void *)dtb_node_get_addr_index(compatible_node, 2);
			if (ccu->clk_info.apbc_base < 0) {
				rt_kprintf("get apbc error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.apbs_base = (void *)dtb_node_get_addr_index(compatible_node, 3);
			if (ccu->clk_info.apbs_base < 0) {
				rt_kprintf("get apbs error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.ciu_base = (void *)dtb_node_get_addr_index(compatible_node, 4);
			if (ccu->clk_info.ciu_base < 0) {
				rt_kprintf("get ciu error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.dciu_base = (void *)dtb_node_get_addr_index(compatible_node, 5);
			if (ccu->clk_info.dciu_base < 0) {
				rt_kprintf("get dciu error\n");
				return -RT_ERROR;
			}

#ifndef SOC_SPACEMIT_K3
			ccu->clk_info.ddrc_base = (void *)dtb_node_get_addr_index(compatible_node, 6);
			if (ccu->clk_info.ddrc_base < 0) {
				rt_kprintf("get ddrc error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.apbc2_base = (void *)dtb_node_get_addr_index(compatible_node, 7);
			if (ccu->clk_info.apbc2_base < 0) {
				rt_kprintf("get apbc2 error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu_base = (void *)dtb_node_get_addr_index(compatible_node, 8);
			if (ccu->clk_info.rcpu_base < 0) {
				rt_kprintf("get rcpu error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu2_base = (void *)dtb_node_get_addr_index(compatible_node, 9);
			if (ccu->clk_info.rcpu2_base < 0) {
				rt_kprintf("get rcpu2 error\n");
				return -RT_ERROR;
			}
#else
			ccu->clk_info.rcpu_base = (void *)dtb_node_get_addr_index(compatible_node, 6);
			if (ccu->clk_info.rcpu_base < 0) {
				rt_kprintf("get rcpu error\n");
				return -RT_ERROR;
			}

			ccu->clk_info.rcpu2_base = (void *)dtb_node_get_addr_index(compatible_node, 7);
			if (ccu->clk_info.rcpu2_base < 0) {
				rt_kprintf("get rcpu2 error\n");
				return -RT_ERROR;
			}
#endif
			spacemit_ccu_probe(compatible_node, ccu);
		}
	}

	return 0;
}
// INIT_PREV_EXPORT(spacemit_ccu_init);

#if 0
int rt_clk_test_init(void)
{
	int ret;
	struct clk *clk;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			"spacemit,pxa-uart");
	if (compatible_node != RT_NULL) {
		clk = of_clk_get(compatible_node, 0);
		if (IS_ERR(clk)) {
			rt_kprintf("get clk failed\n");
			return -1;
		}

		ret = clk_prepare_enable(clk);
		if (ret) {
			rt_kprintf("clk prepare enable failed\n");
			return ret;
		}

		clk_set_rate(clk, 80000000);
	}

	return 0;
}
INIT_DEVICE_EXPORT(rt_clk_test_init);
#endif
