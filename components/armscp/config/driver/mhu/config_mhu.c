/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "juno_mhu.h"
#include <riscv-clic.h>
#include <mod_mhu.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>

#include <register_defination.h>

static const struct fwk_element element_table[] = {
    [JUNO_MHU_DEVICE_IDX] = {
        .name = "",
        .sub_element_count = 4,
        .data = &(struct mod_mhu_device_config) {
            .irq = (unsigned int) MBOX_IRQn,
            .in = MHU_CPU_INTR_BASE,
            .out = MHU_SCP_INTR_BASE,
        },
    },

    [JUNO_MHU_DEVICE_IDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_mhu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
