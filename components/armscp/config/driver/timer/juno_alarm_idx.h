/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Timer sub-element (alarm) indices.
 */

#ifndef JUNO_ALARM_IDX_H
#define JUNO_ALARM_IDX_H

#include <rtconfig.h>

/* Alarm indices for system  */
enum juno_system_alarm_idx {
#ifdef BUILD_USING_MTIMER
    JUNO_SYSTEM_TICK_ALARM_IDX,
#endif
    JUNO_SYSTEM_ALARM_IDX_COUNT,
};

/* Alarm indices for DVFS */
enum juno_dvfs_alarm_idx {
    JUNO_DVFS_ALARM_VLITTLE_IDX = JUNO_SYSTEM_ALARM_IDX_COUNT,
    JUNO_DVFS_ALARM_BIG_IDX,
    JUNO_DVFS_ALARM_GPU_IDX,
#ifdef BUILD_HAS_SCMI_PERF_FAST_CHANNELS
    JUNO_SCMI_FAST_CHANNEL_IDX,
#endif
    JUNO_DVFS_ALARM_IDX_CNT,
};

enum juno_misc_alarm_idx {
    JUNO_PPU_ALARM_IDX = JUNO_DVFS_ALARM_IDX_CNT,
    JUNO_THERMAL_ALARM_IDX,
#ifdef BUILD_HAS_MOD_STATISTICS
    JUNO_STATISTICS_ALARM_IDX,
#endif
    JUNO_SYSTEM_POWER_ALARM_IDX,
    JUNO_ALARM_IDX_COUNT
};

#endif /* JUNO_ALARM_IDX_H */

