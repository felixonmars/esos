/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Framework API for the architecture layer.
 */

#include <rtthread.h>
#include <fwk_arch.h>
#include <internal/fwk_module.h>
#include <internal/fwk_core.h>
#include <fwk_module.h>
#include <fwk_status.h>

int fwk_arch_init(void)
{
    int status;

    fwk_module_init();

    status = fwk_module_start();
    if (status != FWK_SUCCESS) {
        return FWK_E_PANIC;
    }

    return FWK_SUCCESS;
}

int fwk_arch_deinit(void)
{
    int status;

    status = fwk_module_stop();
    if (status != FWK_SUCCESS) {
        return FWK_E_PANIC;
    }

    return FWK_SUCCESS;
}
