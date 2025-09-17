/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Interrupt management.
 */

#include <rthw.h>
#include <rtthread.h>

#include <fwk_interrupt.h>
#include <fwk_status.h>

#include <stdbool.h>
#include <stddef.h>

int fwk_interrupt_is_enabled(unsigned int interrupt, bool *enabled)
{
    if (enabled == NULL) {
        return FWK_E_PARAM;
    }

    *enabled = rt_hw_interrupt_is_enabled(interrupt);

    return FWK_SUCCESS;
}

int fwk_interrupt_enable(unsigned int interrupt)
{
    rt_hw_interrupt_umask(interrupt);

    return FWK_SUCCESS;
}

int fwk_interrupt_disable(unsigned int interrupt)
{
    rt_hw_interrupt_mask(interrupt);

    return FWK_SUCCESS;
}

int fwk_interrupt_is_pending(unsigned int interrupt, bool *pending)
{
    if (pending == NULL) {
        return FWK_E_PARAM;
    }

    *pending = rt_hw_interrupt_is_pending(interrupt);

    return FWK_SUCCESS;
}

int fwk_interrupt_set_pending(unsigned int interrupt)
{
    rt_hw_interrupt_set_pending(interrupt);

    return FWK_SUCCESS;
}

int fwk_interrupt_clear_pending(unsigned int interrupt)
{
    rt_hw_interrupt_clear_pending(interrupt);
    
    return FWK_SUCCESS;
}

int fwk_interrupt_set_isr_param(unsigned int interrupt,
                                void (*isr)(int interrupt, void *param),
                                void *param)
{
    if (isr == NULL) {
        return FWK_E_PARAM;
    }

    rt_hw_interrupt_install(interrupt, isr, param, RT_NULL);

    return FWK_SUCCESS;
}

bool fwk_is_interrupt_context(void)
{
  return rt_interrupt_get_nest();
}
