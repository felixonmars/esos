// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023, spacemit Corporation.
 *
 */

#ifndef _CCU_SPACEMIT_K1X_H_
#define _CCU_SPACEMIT_K1X_H_

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#define BIT(x)		(1 << x)

/* MPMU register offset */
#define MPMU_POSR                       0x10 //no define
#define POSR_PLL1_LOCK                  BIT(27)
#define POSR_PLL2_LOCK                  BIT(28)
#define POSR_PLL3_LOCK                  BIT(29)

//pll1
#define APB_SPARE1_REG          0x100
#define APB_SPARE2_REG          0x104
#define APB_SPARE3_REG          0x108
//pll2
#define APB_SPARE7_REG          0x118
#define APB_SPARE8_REG          0x11c
#define APB_SPARE9_REG          0x120
//pll3
#define APB_SPARE10_REG         0x124
#define APB_SPARE11_REG         0x128
#define APB_SPARE12_REG         0x12c

#define MPMU_WDTPCR     0x200
#define MPMU_RIPCCR     0x210 //no define
#define MPMU_ACGR       0x1024
#define MPMU_SUCCR      0x14
#define MPMU_ISCCR      0x44
#define MPMU_SUCCR_1    0x10b0
#define MPMU_APBCSCR    0x1050

/* RCPU register offset */
#define RCPU_HDMI_CLK_RST	0x2044
#define RCPU_CAN_CLK_RST	0x4c
#define RCPU_I2C0_CLK_RST	0x30

#define RCPU_SSP0_CLK_RST	0x28
#define RCPU_IR_CLK_RST		0x48
#define RCPU_UART0_CLK_RST	0xd8
#define RCPU_UART1_CLK_RST	0x3c
/* end of RCPU register offset */

/* RCPU2 register offset */
#define RCPU2_PWM0_CLK_RST	0x00
#define RCPU2_PWM1_CLK_RST	0x04
#define RCPU2_PWM2_CLK_RST	0x08
#define RCPU2_PWM3_CLK_RST	0x0c
#define RCPU2_PWM4_CLK_RST	0x10
#define RCPU2_PWM5_CLK_RST	0x14
#define RCPU2_PWM6_CLK_RST	0x18
#define RCPU2_PWM7_CLK_RST	0x1c
#define RCPU2_PWM8_CLK_RST	0x20
#define RCPU2_PWM9_CLK_RST	0x24

#define CLK_PLL3     0
#define CLK_PLL1_D2  1
#define CLK_PLL1_D3  2
#define CLK_PLL1_D4  3
#define CLK_PLL1_D5  4
#define CLK_PLL1_D6  5
#define CLK_PLL1_D7  6
#define CLK_PLL1_D8  7
#define CLK_PLL1_D11 8
#define CLK_PLL1_D13 9
#define CLK_PLL1_D23 10
#define CLK_PLL1_D64 11
#define CLK_PLL1_D10_AUD  12
#define CLK_PLL1_D100_AUD 13

#define CLK_PLL3_D1  14
#define CLK_PLL3_D2  15
#define CLK_PLL3_D3  16
#define CLK_PLL3_D4  17
#define CLK_PLL3_D5  18
#define CLK_PLL3_D6  19
#define CLK_PLL3_D7  20
#define CLK_PLL3_D8  21

#define CLK_PLL3_80	22
#define CLK_PLL3_40	23
#define CLK_PLL3_20	24

#define CLK_PLL1_307P2	25
#define CLK_PLL1_76P8	26
#define CLK_PLL1_61P44	27
#define CLK_PLL1_153P6	28
#define CLK_PLL1_102P4	29
#define CLK_PLL1_51P2	30
#define CLK_PLL1_51P2_AP	31
#define CLK_PLL1_57P6	32
#define CLK_PLL1_25P6	33
#define CLK_PLL1_12P8	34
#define CLK_PLL1_12P8_WDT	35
#define CLK_PLL1_6P4	36
#define CLK_PLL1_3P2	37
#define CLK_PLL1_1P6	38
#define CLK_PLL1_0P8	39
#define CLK_PLL1_351	40
#define CLK_PLL1_409P6	41
#define CLK_PLL1_204P8	42
#define CLK_PLL1_491	43
#define CLK_PLL1_245P76	44
#define CLK_PLL1_614	45
#define CLK_PLL1_47P26	46
#define CLK_PLL1_31P5	47
#define CLK_PLL1_819	48
#define CLK_PLL1_1228	49
#define CLK_RCPU_HDMIAUDIO	50
#define CLK_RCPU_CAN		51
#define CLK_RCPU_CAN_BUS	52
#define CLK_RCPU2_PWM0		53
#define CLK_RCPU2_PWM1		54
#define CLK_RCPU2_PWM2		55
#define CLK_RCPU2_PWM3		56
#define CLK_RCPU2_PWM4		57
#define CLK_RCPU2_PWM5		58
#define CLK_RCPU2_PWM6		59
#define CLK_RCPU2_PWM7		60
#define CLK_RCPU2_PWM8		61
#define CLK_RCPU2_PWM9		62
#define CLK_RCPU_I2C0		63
#define CLK_RCPU_IR		64
#define CLK_RCPU_UART0		65
#define CLK_RCPU_UART1		66
#define CLK_RCPU_SSP0		67
//resets
#define CLK_RST_RCPU_HDMIAUDIO	68
#define CLK_RST_RCPU_CAN	69
#define CLK_RST_RCPU2_PWM0	70
#define CLK_RST_RCPU2_PWM1	71
#define CLK_RST_RCPU2_PWM2	72
#define CLK_RST_RCPU2_PWM3	73
#define CLK_RST_RCPU2_PWM4	74
#define CLK_RST_RCPU2_PWM5	75
#define CLK_RST_RCPU2_PWM6	76
#define CLK_RST_RCPU2_PWM7	77
#define CLK_RST_RCPU2_PWM8	78
#define CLK_RST_RCPU2_PWM9	79

#define CLK_RST_RCPU_I2C0	80
#define CLK_RST_RCPU_IR		81
#define CLK_RST_RCPU_UART0	82
#define CLK_RST_RCPU_UART1	83
#define CLK_RST_RCPU_SSP0	84

#define CLK_MAX_NO		85

enum ccu_base_type{
	BASE_TYPE_MPMU       = 0,
	BASE_TYPE_APMU       = 1,
	BASE_TYPE_APBC       = 2,
	BASE_TYPE_APBS       = 3,
	BASE_TYPE_CIU        = 4,
	BASE_TYPE_DCIU       = 5,
	BASE_TYPE_DDRC       = 6,
	BASE_TYPE_APBC2      = 7,
	BASE_TYPE_RCPU       = 8,
	BASE_TYPE_RCPU2      = 9,
};

enum {
	CLK_DIV_TYPE_1REG_NOFC_V1 = 0,
	CLK_DIV_TYPE_1REG_FC_V2,
	CLK_DIV_TYPE_2REG_NOFC_V3,
	CLK_DIV_TYPE_2REG_FC_V4,
	CLK_DIV_TYPE_1REG_FC_DIV_V5,
	CLK_DIV_TYPE_1REG_FC_MUX_V6,
};

struct ccu_common {
	void *base;
	enum ccu_base_type base_type;
	unsigned int	reg_type;
	unsigned int	reg_ctrl;
	unsigned int	reg_sel;
	unsigned int	reg_xtc;
	unsigned int	fc;
	bool	is_pll;
	const char		*name;
	const struct clk_ops	*ops;
	const char		* const *parent_names;
	unsigned char num_parents;
	unsigned long	flags;
	struct rt_spinlock lock;
	struct clk_hw	hw;
};

struct spacemit_k1x_clk {
	void	*mpmu_base;
	void	*apmu_base;
	void	*apbc_base;
	void	*apbs_base;
	void	*ciu_base;
	void	*dciu_base;
	void	*ddrc_base;
	void	*apbc2_base;
	void	*rcpu_base;
	void	*rcpu2_base;
};

struct clk_hw_table {
	char	*name;
	unsigned int	clk_hw_id;
};

extern rt_ubase_t g_cru_lock;

static inline struct ccu_common *hw_to_ccu_common(struct clk_hw *hw)
{
	return rt_container_of(hw, struct ccu_common, hw);
}

#endif /* _CCU_SPACEMIT_K1X_H_ */
