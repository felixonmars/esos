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
    FWK_RW uint64_t MTIMECMP;
    uint32_t  RESERVED[8187];
    FWK_R uint64_t MTIME;
} dw_timer_reg_t;

#endif /* GTIMER_REG_H */
