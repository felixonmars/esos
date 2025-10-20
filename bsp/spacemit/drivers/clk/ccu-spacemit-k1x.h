/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CCU_SPACEMIT_K1X_H_
#define _CCU_SPACEMIT_K1X_H_

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

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

#endif /* _CCU_SPACEMIT_K1X_H_ */
