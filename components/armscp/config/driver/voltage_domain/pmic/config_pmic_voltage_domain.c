/*
 * Arm SCP/MCP Software
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <config_pmic_voltage_domain.h>

#include <mod_pmic_voltage_domain.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>

#include <stdbool.h>
#include <stddef.h>

static const int32_t little_discrete_voltage_values[] = {
    820000,
    850000,
    900000,
    950000,
    1000000,
};

static const int32_t gpu_discrete_voltage_values[] = {
    820000,
    825000,
    850000,
    875000,
    900000,
};

static const struct fwk_element element_table[] = {
    [CONFIG_MOCK_VOLTAGE_DOMAIN_ELEMENT_IDX_VLITTLE] = {
        .name = "LITTLE_VOLTD",
        .data = &(const struct mod_mock_voltage_domain_element_cfg) {
            .async_alarm_id = FWK_ID_NONE_INIT,
            .async_alarm_api_id = FWK_ID_NONE_INIT,

            .async_response_id = FWK_ID_NONE_INIT,
            .async_response_api_id = FWK_ID_NONE_INIT,

            .default_mode_id = MOD_VOLTD_MODE_ID_OFF,
            .default_voltage = 900000,

            .level_type = MOD_VOLTD_VOLTAGE_LEVEL_DISCRETE,
            .level_count = FWK_ARRAY_SIZE(little_discrete_voltage_values),
            .voltage_levels = little_discrete_voltage_values,
        },
    },
    [CONFIG_MOCK_VOLTAGE_DOMAIN_ELEMENT_IDX_VGPU] = {
        .name = "GPU_VOLTD",
        .data = &(const struct mod_mock_voltage_domain_element_cfg) {
            .async_alarm_id = FWK_ID_NONE_INIT,
            .async_alarm_api_id = FWK_ID_NONE_INIT,

            .async_response_id = FWK_ID_NONE_INIT,
            .async_response_api_id = FWK_ID_NONE_INIT,

            .default_mode_id = MOD_VOLTD_MODE_ID_OFF,
            .default_voltage = 875000,

            .level_type = MOD_VOLTD_VOLTAGE_LEVEL_DISCRETE,
            .level_count = FWK_ARRAY_SIZE(gpu_discrete_voltage_values),
            .voltage_levels = gpu_discrete_voltage_values,
        },
    },
    { 0 }
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_pmic_voltage_domain = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
