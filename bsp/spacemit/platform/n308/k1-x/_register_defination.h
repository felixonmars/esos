/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SPACEMIT_REGISTER_DEF_H__
#define __SPACEMIT_REGISTER_DEF_H__

#include <rtdef.h>

typedef struct {
	union {
		rt_uint32_t RBR;	/* Offset: 0x000 Receive buffer register */
		rt_uint32_t THR;	/* Offset: 0x000 Transmission hold register */
		rt_uint32_t DLL;	/* Offset: 0x000 Clock frequency division low section register */
	};
	union {
		rt_uint32_t DLH;	/* Offset: 0x004 Interrupt enable register */
		rt_uint32_t IER;	/* Offset: 0x004 Clock frequency division high section register */
	};
	union {
		rt_uint32_t IIR;	/* Offset: 0x008 Interrupt indicia register */
		rt_uint32_t FCR;	/* Offset: 0x008 FIFO control register */
	};
	rt_uint32_t LCR;	/* Offset: 0x00C Transmission control register */
	rt_uint32_t MCR;
	rt_uint32_t LSR;	/* Offset: 0x014 Transmission state register */
	rt_uint32_t MSR;	/* Offset: 0x018 Modem state register */
	rt_uint32_t SCR;
	rt_uint32_t ISR;
	rt_uint32_t FOR;
	rt_uint32_t ABR;
	rt_uint32_t ACR;
} pxa_uart_reg_t;

/* AUDIO PMU */
#define AUDIO_PMU_VOTE_REG	0xc088c018
#define VOTE_FOR_AUDIO_ENTER_PWROFF_MODE	3
#define VOTE_FOR_AUDIO_ENTER_PLLOFF_MODE	2
#define VOTE_FOR_AUDIO_ENTER_LOWPWR_MODE	1

typedef union audio_pmu_vote_reg {
	rt_uint32_t val;
	struct {
		rt_uint32_t reserved0:1;
		rt_uint32_t vote_for_lp:1;
		rt_uint32_t vote_for_plloff:1;
		rt_uint32_t vote_for_pwroff:1;
		rt_uint32_t reserved2:28;
	} bits;
} audio_pmu_vote_t;

#define AUDIO_VOTE_FOR_MAIN_PMU	0xc088c020
#define VOTE_FOR_AP_AXI_CLK_OFF			6
#define VOTE_FOR_DDR_SHUTDOWN			5
#define VOTE_FOR_VCTCXO_OFF			3
#define VOTE_FOR_MAIN_PMU_ENTER_SLEEP_STATE	2
#define VOTE_FOR_AP_GOTO_STANDBY_STATE		1

typedef union audio_vote_for_main_mpu_reg {
	rt_uint32_t val;
	struct {
		rt_uint32_t reserved0:1;
		rt_uint32_t audio_pmu_vote_stben:1;
		rt_uint32_t audio_pmu_vote_slpen:1;
		rt_uint32_t audio_pmu_vote_vctcxosd:1;
		rt_uint32_t reserved1:1;
		rt_uint32_t audio_pmu_vote_ddrsd:1;
		rt_uint32_t audio_pmu_vote_axisd:1;
		rt_uint32_t reserved2:25;
	} bits;
} audio_vote_for_main_mpu_t;


#define AUDIO_WAKEUP_EN_REG	0xc088c028

typedef union audio_wakeup_en_reg {
	rt_uint32_t val;
	struct {
		rt_uint32_t shub_int_wkup_en:1;
		rt_uint32_t ipc_ap_wkup_en:1;
		rt_uint32_t reserved0:1;
		rt_uint32_t ap_wkup_en:1;
		rt_uint32_t timer_wkup_en:1;
		rt_uint32_t reserved1:27;
	} bits;
} audio_wakeup_en_t;

#define PWRCTL_LP_WAKEUP_MASK        (0xc088c060)

#define RT_HEAP_START		0x30000000
#define RT_HEAP_END		0x30200000
#define SHARED_MEM_PA	0x30200000

#endif
