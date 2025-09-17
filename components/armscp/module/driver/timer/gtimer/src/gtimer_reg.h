/*
 * Arm SCP/MCP Software
 * Copyright (c) 2017-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GTIMER_REG_H
#define GTIMER_REG_H

#include <fwk_macros.h>

#include <stdint.h>

/*
 *  define the bits for TxControl
 */
#define DW_TIMER_TXCONTROL_ENABLE      (1UL << 0)
#define DW_TIMER_TXCONTROL_MODE        (1UL << 1)
#define DW_TIMER_TXCONTROL_INTMASK     (1UL << 2)

#define DW_TIMER_INIT_DEFAULT_VALUE     0x7ffffff

typedef struct {
    FWK_RW uint32_t TxLoadCount;              /* Offset: 0x000 (R/W)  Receive buffer register */
    FWK_R uint32_t TxCurrentValue;            /* Offset: 0x004 (R)  Transmission hold register */
    FWK_RW uint8_t TxControl: 4;              /* Offset: 0x008 (R/W)  Clock frequency division low section register */
    uint8_t  RESERVED0[3];
    FWK_R uint8_t TxEOI: 1;                   /* Offset: 0x00c (R)  Clock frequency division high section register */
    uint8_t  RESERVED1[3];
    FWK_R uint8_t TxIntStatus: 1;             /* Offset: 0x010 (R)  Interrupt enable register */
    uint8_t  RESERVED2[3];
} dw_timer_reg_t;

#endif /* GTIMER_REG_H */
