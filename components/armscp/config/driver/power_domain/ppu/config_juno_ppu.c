/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "juno_ppu_idx.h"

#include <juno_alarm_idx.h>
#include <mod_juno_ppu.h>
#include <mod_power_domain.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <riscv-clic.h>

static struct fwk_element element_table[] = {
    [JUNO_PPU_DEV_IDX_LITTLE_SSTOP] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CLUSTER,
	    .core_reg_base = 0x2F024000,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
        },
    },
    [JUNO_PPU_DEV_IDX_LITTLE_CPU0] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CORE,
	    .core_reg_base = 0x2F024000,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .wakeup_irq = (unsigned int) C0C0_WAKEUP_IRQn,
            .warm_reset_irq = (unsigned int) /* LITTLE_0_WARM_RST_REQ_IRQ */ 0,
        },
    },
    [JUNO_PPU_DEV_IDX_LITTLE_CPU1] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CORE,
	    .core_reg_base = 0x2F024000,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .wakeup_irq = (unsigned int) C0C1_WAKEUP_IRQn,
            .warm_reset_irq = (unsigned int) /* LITTLE_1_WARM_RST_REQ_IRQ */ 0,
        },
    },
    [JUNO_PPU_DEV_IDX_LITTLE_CPU2] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CORE,
	    .core_reg_base = 0x2F024000,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .wakeup_irq = (unsigned int) C0C2_WAKEUP_IRQn,
            .warm_reset_irq = (unsigned int) /* LITTLE_2_WARM_RST_REQ_IRQ */ 0,
        },
    },
    [JUNO_PPU_DEV_IDX_LITTLE_CPU3] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CORE,
	    .core_reg_base = 0x2F024000,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .wakeup_irq = (unsigned int) C0C3_WAKEUP_IRQn,
            .warm_reset_irq = (unsigned int) /* LITTLE_3_WARM_RST_REQ_IRQ */ 0,
        },
    },
    [JUNO_PPU_DEV_IDX_BIG_SSTOP] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CLUSTER,
	    .core_reg_base = 0x2F024004,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
        },
    },
    [JUNO_PPU_DEV_IDX_BIG_CPU0] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CORE,
	    .core_reg_base = 0x2F024004,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .wakeup_irq = (unsigned int) C1C0_WAKEUP_IRQn,
            .warm_reset_irq = (unsigned int) /* BIG_0_WARM_RST_REQ_IRQ */ 0,
        },
    },
    [JUNO_PPU_DEV_IDX_BIG_CPU1] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CORE,
	    .core_reg_base = 0x2F024004,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .wakeup_irq = (unsigned int) C1C1_WAKEUP_IRQn,
            .warm_reset_irq = (unsigned int) /* BIG_1_WARM_RST_REQ_IRQ */ 0,
        },
    },
    [JUNO_PPU_DEV_IDX_BIG_CPU2] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CORE,
	    .core_reg_base = 0x2F024004,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .wakeup_irq = (unsigned int) C1C2_WAKEUP_IRQn,
            .warm_reset_irq = (unsigned int) /* BIG_2_WARM_RST_REQ_IRQ */ 0,
        },
    },
    [JUNO_PPU_DEV_IDX_BIG_CPU3] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_CORE,
	    .core_reg_base = 0x2F024004,
	    .core_pd_reg_base = 0x2F804500,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .wakeup_irq = (unsigned int) C1C3_WAKEUP_IRQn,
            .warm_reset_irq = (unsigned int) /* BIG_2_WARM_RST_REQ_IRQ */ 0,
        },
    },
    [JUNO_PPU_DEV_IDX_GPUTOP] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_DEVICE,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
	    .core_reg_base = 0x2F024008,
        },
    },
    [JUNO_PPU_DEV_IDX_SYSTOP] = {
        .name = "",
        .data = &(const struct mod_juno_ppu_element_config) {
            .pd_type = MOD_PD_TYPE_SYSTEM,
	    .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
	    .core_reg_base = 0x2F02400c,
        },
    },
    [JUNO_PPU_DEV_IDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_juno_ppu = {
    .data =
        &(struct mod_juno_ppu_config){
            .timer_alarm_id = FWK_ID_SUB_ELEMENT_INIT(
                FWK_MODULE_IDX_TIMER,
                0,
                JUNO_PPU_ALARM_IDX),
        },
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
