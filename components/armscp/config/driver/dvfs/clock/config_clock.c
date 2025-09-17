/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_mock_clock.h"
#include "config_power_domain.h"
#include "juno_clock.h"

#include <mod_clock.h>
#include <mod_mock_clock.h>
#include <mod_power_domain.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static struct fwk_element clock_dev_desc_table[] = {
    [JUNO_CLOCK_IDX_LITTLECLK] = {
        .name = "LITTLECLK",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MOCK_CLOCK,
                MOD_MOCK_CLOCK_ELEMENT_IDX_LITTLECLK),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MOCK_CLOCK,
                                      MOD_MOCK_CLOCK_API_TYPE_DRIVER),
            .pd_source_id = FWK_ID_NONE_INIT,
        }),
    },
    [JUNO_CLOCK_IDX_GPUCLK] = {
        .name = "GPUCLK",
        .data = &((struct mod_clock_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MOCK_CLOCK,
                MOD_MOCK_CLOCK_ELEMENT_IDX_GPUCLK),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MOCK_CLOCK,
                                      MOD_MOCK_CLOCK_API_TYPE_DRIVER),
            .pd_source_id = FWK_ID_NONE_INIT,
        }),
    },
    [JUNO_CLOCK_IDX_COUNT] = { 0 }, /* Termination description. */
};

static const struct fwk_element *clock_get_dev_desc_table(fwk_id_t module_id)
{
    return clock_dev_desc_table;
}

struct fwk_module_config config_clock = {
    .data =
        &(struct mod_clock_config){
            .pd_transition_notification_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_POWER_DOMAIN,
                MOD_PD_NOTIFICATION_IDX_POWER_STATE_TRANSITION),
            .pd_pre_transition_notification_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_POWER_DOMAIN,
                MOD_PD_NOTIFICATION_IDX_POWER_STATE_PRE_TRANSITION),
        },

    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(clock_get_dev_desc_table),
};
