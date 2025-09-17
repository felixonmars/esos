/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef JUNO_CLOCK_H
#define JUNO_CLOCK_H

#include <fwk_attributes.h>
#include <fwk_macros.h>

#include <stdint.h>

/* Juno clock indices */
enum juno_clock_idx {
    JUNO_CLOCK_IDX_LITTLECLK,
    JUNO_CLOCK_IDX_GPUCLK,
    JUNO_CLOCK_IDX_COUNT
};

#endif /* JUNO_CLOCK_H */
