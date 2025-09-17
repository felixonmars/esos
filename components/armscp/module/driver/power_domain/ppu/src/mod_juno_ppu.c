/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "juno_ppu.h"
#include "juno_ppu_idx.h"

#include <mod_juno_ppu.h>
#include <mod_power_domain.h>
#include <mod_system_power.h>
#include <mod_timer.h>

#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PPU_SET_STATE_AND_WAIT_TIMEOUT_US (100U * 1000U)

#define PPU_ALARM_TIMEOUT_SUSPEND_MS 5U

#define CPU_WAKEUP_COMPOSITE_STATE \
    MOD_PD_COMPOSITE_STATE( \
        (unsigned int)MOD_PD_LEVEL_2, \
        0U, \
        (unsigned int)MOD_PD_STATE_ON, \
        (unsigned int)MOD_PD_STATE_ON, \
        (unsigned int)MOD_PD_STATE_ON)

static struct module_ctx juno_ppu_ctx;

static void get_ctx(fwk_id_t id, struct ppu_ctx **ppu_ctx)
{
    if (!fwk_module_is_valid_element_id(id)) {
    	rt_kprintf("%s:%d \n", __func__, __LINE__);
	while (1);
    }

    *ppu_ctx = &juno_ppu_ctx.ppu_ctx_table[fwk_id_get_element_idx(id)];
}

static void juno_ppu_alarm_callback(uintptr_t param)
{
	/* TODO: do something later ? */
}

/*
 * Power Domain driver API
 */
static int pd_set_state(fwk_id_t ppu_id, unsigned int state)
{
    int status;
    struct ppu_ctx *ppu_ctx;

    get_ctx(ppu_id, &ppu_ctx);

    if (state >= MOD_PD_STATE_COUNT) {
        return FWK_E_PARAM;
    }

    switch ((enum mod_pd_state)state) {
    case MOD_PD_STATE_ON:
    case MOD_PD_STATE_OFF:

	ppu_ctx->status = state;

	status = FWK_SUCCESS;
	/* do something later ? */
        break;

    default:
        return FWK_E_SUPPORT;
    }

    status = ppu_ctx->pd_api->report_power_state_transition(ppu_ctx->bound_id,
        state);
    if (status != FWK_SUCCESS) {
        return FWK_E_PANIC;
    }

    return FWK_SUCCESS;
}

static int pd_get_state(fwk_id_t ppu_id, unsigned int *state)
{
    struct ppu_ctx *ppu_ctx;

    if (state == NULL) {
        return FWK_E_PARAM;
    }

    get_ctx(ppu_id, &ppu_ctx);

    *state = ppu_ctx->status;

    /* TODO: do something later ? */
    return FWK_SUCCESS;
}

static int pd_reset(fwk_id_t ppu_id)
{
    struct ppu_ctx *ppu_ctx;

    get_ctx(ppu_id, &ppu_ctx);

    /* TODO: do something later ? */

    return FWK_SUCCESS;
}

static int pd_prepare_core_for_system_suspend(fwk_id_t ppu_id)
{
    return FWK_E_SUPPORT;
}

static const struct mod_pd_driver_api pd_driver_api = {
    .set_state = pd_set_state,
    .get_state = pd_get_state,
    .reset = pd_reset,
    .prepare_core_for_system_suspend = pd_prepare_core_for_system_suspend,
};

/*
 * CSS API
 */
static int css_set_state(fwk_id_t ppu_id, unsigned int state)
{
    int status;
    struct ppu_ctx *ppu_ctx;

    get_ctx(ppu_id, &ppu_ctx);

    if (state >= MOD_SYSTEM_POWER_POWER_STATE_COUNT) {
        return FWK_E_PARAM;
    }

    switch (state) {
    case (unsigned int)MOD_PD_STATE_ON:
        juno_ppu_ctx.css_state = state;
        break;

    case (unsigned int)MOD_PD_STATE_OFF:
        /* Fall through */

    case (unsigned int)MOD_SYSTEM_POWER_POWER_STATE_SLEEP0:
        juno_ppu_ctx.css_state = state;

        break;

    case (unsigned int)MOD_SYSTEM_POWER_POWER_STATE_SLEEP1:
        return FWK_E_SUPPORT;

    default:
        return FWK_E_PARAM;
    }

    status = ppu_ctx->pd_api->report_power_state_transition(ppu_ctx->bound_id,
        state);
    if (status != FWK_SUCCESS) {
        return FWK_E_PANIC;
    }

    return FWK_SUCCESS;
}

static int css_get_state(fwk_id_t ppu_id, unsigned int *state)
{
    *state = juno_ppu_ctx.css_state;

    return FWK_SUCCESS;
}

static int css_reset(fwk_id_t ppu_id)
{
    return FWK_E_SUPPORT;
}

static const struct mod_pd_driver_api css_pd_driver_api = {
    .set_state = css_set_state,
    .get_state = css_get_state,
    .reset = css_reset,
};

/*
 * Cluster API
 */
static int cluster_set_state(fwk_id_t ppu_id, unsigned int state)
{
    int status;
    struct ppu_ctx *ppu_ctx;
    struct ppu_reg *ppu_reg;
    const struct mod_juno_ppu_element_config *dev_config;
    unsigned int cluster_id = fwk_id_get_element_idx(ppu_id);

    get_ctx(ppu_id, &ppu_ctx);

    dev_config = ppu_ctx->config;
    ppu_reg = (struct ppu_reg *)dev_config->core_pd_reg_base;

    if (state >= MOD_PD_STATE_COUNT) {
        return FWK_E_PARAM;
    }

    switch ((enum mod_pd_state)state) {
    case MOD_PD_STATE_ON:
	if (cluster_id != JUNO_PPU_DEV_IDX_LITTLE_SSTOP)
		*((unsigned int *)dev_config->core_reg_base) |= 0xf;

	status = FWK_SUCCESS;
        break;

    case MOD_PD_STATE_OFF:
	if (cluster_id == JUNO_PPU_DEV_IDX_LITTLE_SSTOP)
		while (!(!!ppu_reg->c0_pd_st.bits.c0_no_op));
	else
		while (!(!!ppu_reg->c1_pd_st.bits.c1_no_op));

	*((unsigned int *)dev_config->core_reg_base) &= ~0x5;

	status = FWK_SUCCESS;
        break;

    default:
        return FWK_E_PANIC;
    }

    status = ppu_ctx->pd_api->report_power_state_transition(ppu_ctx->bound_id,
        state);
    if (status != FWK_SUCCESS) {
        return FWK_E_PANIC;
    }

    return FWK_SUCCESS;
}

static int cluster_get_state(fwk_id_t ppu_id, unsigned int *state)
{
    struct ppu_ctx *ppu_ctx;
    const struct mod_juno_ppu_element_config *dev_config;

    get_ctx(ppu_id, &ppu_ctx);

    dev_config = ppu_ctx->config;

    unsigned int cluster_id = fwk_id_get_element_idx(ppu_id);

    if (((*((unsigned int *)dev_config->core_reg_base) & 0xf) == 0xf) ||
		    (cluster_id == JUNO_PPU_DEV_IDX_LITTLE_SSTOP))
	    *state = MOD_PD_STATE_ON;
    else
	    *state = MOD_PD_STATE_OFF;

    return FWK_SUCCESS;
}

static const struct mod_pd_driver_api cluster_pd_driver_api = {
    .set_state = cluster_set_state,
    .get_state = cluster_get_state,
    .reset = pd_reset,
};

static void set_core_power_off(fwk_id_t ppu_id)
{
	unsigned int core_id;
	struct ppu_ctx *ppu_ctx;
	struct ppu_reg *ppu_reg;
	const struct mod_juno_ppu_element_config *dev_config;
	
	get_ctx(ppu_id, &ppu_ctx);
	
	dev_config = ppu_ctx->config;
	ppu_reg = (struct ppu_reg *)ppu_ctx->config->core_pd_reg_base;
	core_id = fwk_id_get_element_idx(ppu_id);

	switch (core_id) {
		case JUNO_PPU_DEV_IDX_LITTLE_CPU0 :
			ppu_reg->c0_pd_ua.bits.c0_unavail_core0 = 1;
			while (ppu_reg->c0_pd_st.bits.c0_core0_lpmd_b == 1);
			break;
		
		case JUNO_PPU_DEV_IDX_LITTLE_CPU1 :
			ppu_reg->c0_pd_ua.bits.c0_unavail_core1 = 1;
			while (ppu_reg->c0_pd_st.bits.c0_core1_lpmd_b == 1);
			break;
		
		case JUNO_PPU_DEV_IDX_LITTLE_CPU2 :
			ppu_reg->c0_pd_ua.bits.c0_unavail_core2 = 1;
			while (ppu_reg->c0_pd_st.bits.c0_core2_lpmd_b == 1);
			break;
		
		case JUNO_PPU_DEV_IDX_LITTLE_CPU3 :
			ppu_reg->c0_pd_ua.bits.c0_unavail_core3 = 1;
			while (ppu_reg->c0_pd_st.bits.c0_core3_lpmd_b == 1);
			break;
		
		case JUNO_PPU_DEV_IDX_BIG_CPU0:
			ppu_reg->c1_pd_ua.bits.c1_unavail_core0 = 1;
			while (ppu_reg->c1_pd_st.bits.c1_core0_lpmd_b == 1);
			break;
		
		case JUNO_PPU_DEV_IDX_BIG_CPU1:
			ppu_reg->c1_pd_ua.bits.c1_unavail_core1 = 1;
			while (ppu_reg->c1_pd_st.bits.c1_core1_lpmd_b == 1);
			break;
		
		case JUNO_PPU_DEV_IDX_BIG_CPU2:
			ppu_reg->c1_pd_ua.bits.c1_unavail_core2 = 1;
			while (ppu_reg->c1_pd_st.bits.c1_core2_lpmd_b == 1);
			break;
		
		case JUNO_PPU_DEV_IDX_BIG_CPU3:
			ppu_reg->c1_pd_ua.bits.c1_unavail_core3 = 1;
			while (ppu_reg->c1_pd_st.bits.c1_core3_lpmd_b == 1);
			break;
		
		default :
			break;
	}
	
	if (core_id > JUNO_PPU_DEV_IDX_LITTLE_SSTOP) {
		core_id -= JUNO_PPU_DEV_IDX_BIG_CPU0;
	}
	
	*((unsigned int *)dev_config->core_reg_base) &= ~(1 << (core_id + 4));
}

static void set_core_power_on(fwk_id_t ppu_id)
{
	unsigned int core_id;
	struct ppu_ctx *ppu_ctx;
	struct ppu_reg *ppu_reg;
	const struct mod_juno_ppu_element_config *dev_config;

	get_ctx(ppu_id, &ppu_ctx);
	dev_config = ppu_ctx->config;
	ppu_reg = (struct ppu_reg *)ppu_ctx->config->core_pd_reg_base;
	core_id = fwk_id_get_element_idx(ppu_id);

	switch (core_id) {
		case JUNO_PPU_DEV_IDX_LITTLE_CPU0 :
			ppu_reg->c0_pd_ua.bits.c0_unavail_core0 = 0;
		       break;

		case JUNO_PPU_DEV_IDX_LITTLE_CPU1 :
			ppu_reg->c0_pd_ua.bits.c0_unavail_core1 = 0;
		       break;

		case JUNO_PPU_DEV_IDX_LITTLE_CPU2 :
			ppu_reg->c0_pd_ua.bits.c0_unavail_core2 = 0;
		       break;

		case JUNO_PPU_DEV_IDX_LITTLE_CPU3 :
			ppu_reg->c0_pd_ua.bits.c0_unavail_core3 = 0;
		       break;

		case JUNO_PPU_DEV_IDX_BIG_CPU0:
			ppu_reg->c1_pd_ua.bits.c1_unavail_core0 = 0;
			break;

		case JUNO_PPU_DEV_IDX_BIG_CPU1:
			ppu_reg->c1_pd_ua.bits.c1_unavail_core1 = 0;
			break;

		case JUNO_PPU_DEV_IDX_BIG_CPU2:
			ppu_reg->c1_pd_ua.bits.c1_unavail_core2 = 0;
			break;

		case JUNO_PPU_DEV_IDX_BIG_CPU3:
			ppu_reg->c1_pd_ua.bits.c1_unavail_core3 = 0;
			break;

		default :
			break;
	}

	if (core_id > JUNO_PPU_DEV_IDX_LITTLE_SSTOP) {
		core_id -= JUNO_PPU_DEV_IDX_BIG_CPU0;
	}

	*((unsigned int *)dev_config->core_reg_base) |= (1 << (core_id + 4));
}

/*
 * Cores API
 */
static int core_set_state(fwk_id_t ppu_id, unsigned int state)
{
    int status;
    struct ppu_ctx *ppu_ctx;

    get_ctx(ppu_id, &ppu_ctx);

    if (state >= MOD_PD_STATE_COUNT) {
        return FWK_E_PARAM;
    }

    switch ((enum mod_pd_state)state) {
    case MOD_PD_STATE_OFF:
	/* power off cpu */
	set_core_power_off(ppu_id);

	status = FWK_SUCCESS;
        break;

    case MOD_PD_STATE_SLEEP:
	/* power sleep cpu */
	set_core_power_off(ppu_id);
	/* clear wakeup pending */
    	status = fwk_interrupt_clear_pending(ppu_ctx->config->wakeup_irq);
	/* enable the wakeup irq */
    	status = fwk_interrupt_enable(ppu_ctx->config->wakeup_irq);
	
	status = FWK_SUCCESS;
        break;

    case MOD_PD_STATE_ON:
	/* power on cpu */
	set_core_power_on(ppu_id);

	status = FWK_SUCCESS;
        break;

    default:
        rt_kprintf("%s:%d\n", __func__, __LINE__);
        status = FWK_E_PANIC;

        break;
    }

    if (status != FWK_SUCCESS) {
        return status;
    }

    status = ppu_ctx->pd_api->report_power_state_transition(ppu_ctx->bound_id,
        state);
    if (status != FWK_SUCCESS) {
        return FWK_E_PANIC;
    }

    return FWK_SUCCESS;
}

static int core_get_state(fwk_id_t ppu_id, unsigned int *state)
{
    struct ppu_ctx *ppu_ctx;
    const struct mod_juno_ppu_element_config *dev_config;

    get_ctx(ppu_id, &ppu_ctx);

    dev_config = ppu_ctx->config;

    unsigned int core_id = fwk_id_get_element_idx(ppu_id);

    if (core_id == JUNO_PPU_DEV_IDX_LITTLE_CPU0) {
	    *state = MOD_PD_STATE_ON;
	    return FWK_SUCCESS;
    }

    if ((*((unsigned int *)dev_config->core_reg_base) >> 4) & (1 << core_id))
	    *state = MOD_PD_STATE_ON;
    else
	    *state = MOD_PD_STATE_OFF;

    return FWK_SUCCESS;
}

static int core_reset(fwk_id_t ppu_id)
{
	/* TODO: do something later ? */
	return FWK_SUCCESS;
}

static int core_prepare_core_for_system_suspend(fwk_id_t ppu_id)
{
    struct ppu_ctx *ppu_ctx;
    const struct mod_juno_ppu_config *config;

    get_ctx(ppu_id, &ppu_ctx);
    config = fwk_module_get_data(fwk_module_id_juno_ppu);

    /* TODO: do somehting later ? */
    /* Start the timer alarm to poll for the power policy change */
    return juno_ppu_ctx.alarm_api->start(
        config->timer_alarm_id,
        PPU_ALARM_TIMEOUT_SUSPEND_MS,
        MOD_TIMER_ALARM_TYPE_PERIODIC,
        juno_ppu_alarm_callback,
        (uintptr_t)ppu_ctx);
}

static const struct mod_pd_driver_api core_pd_driver_api = {
    .set_state = core_set_state,
    .get_state = core_get_state,
    .reset = core_reset,
    .prepare_core_for_system_suspend = core_prepare_core_for_system_suspend,
};

/*
 * Interrupt handlers
 */

static void core_wakeup_handler(int vector, void *param)
{
    int status;
    struct ppu_ctx *ppu_ctx = (struct ppu_ctx *)param;

    /* disable wakeup irq */
    status = fwk_interrupt_disable(ppu_ctx->config->wakeup_irq);

    /* clear wakeup pending */
    status = fwk_interrupt_clear_pending(ppu_ctx->config->wakeup_irq);

    /* power-on the releated cpu */
    status = ppu_ctx->pd_api->set_state(
        ppu_ctx->bound_id, false, CPU_WAKEUP_COMPOSITE_STATE);
    if (status != FWK_SUCCESS) {
    	rt_kprintf("%s:%d\n", __func__, __LINE__);
	while (1);
    }
}

/**
 * static void core_warm_reset_handler(int vector, void *param)
 * {
 * }
 */

/*
 * Framework API
 */
static int juno_ppu_module_init(fwk_id_t module_id,
                                unsigned int element_count,
                                const void *data)
{
    if (element_count <= 0) {
        return FWK_E_PANIC;
    }

    juno_ppu_ctx.ppu_ctx_table = fwk_mm_calloc(element_count,
        sizeof(struct ppu_ctx));

    juno_ppu_ctx.css_state = (unsigned int)MOD_PD_STATE_ON;

#ifdef BUILD_HAS_MOD_TIMER
    if (data == NULL) {
    	rt_kprintf("%s:%d\n", __func__, __LINE__);
	while (1);
    }
    #endif

    return FWK_SUCCESS;
}

static int juno_ppu_element_init(fwk_id_t ppu_id,
                                 unsigned int subelement_count,
                                 const void *data)
{
    const struct mod_juno_ppu_element_config *dev_config = data;
    struct ppu_ctx *ppu_ctx;

    ppu_ctx = juno_ppu_ctx.ppu_ctx_table + fwk_id_get_element_idx(ppu_id);

    ppu_ctx->config = dev_config;
    ppu_ctx->bound_id = FWK_ID_NONE;

    if (dev_config->pd_type == MOD_PD_TYPE_SYSTEM) {
	    /* Do something later ? */
    }

    return FWK_SUCCESS;
}

static int juno_ppu_bind(fwk_id_t id, unsigned int round)
{
#if (defined BUILD_HAS_MOD_TIMER) || (defined BUILD_HAS_MOD_POWER_DOMAIN) || \
    (defined BUILD_HAS_MOD_SYSTEM_POWER)
    int status;
#endif
    struct ppu_ctx *ppu_ctx;
    const struct mod_juno_ppu_element_config *dev_config;
#ifdef BUILD_HAS_MOD_TIMER
    const struct mod_juno_ppu_config *config;
#endif
    (void)dev_config;

    /* Bind in the second round only */
    if (round == 0) {
        return FWK_SUCCESS;
    }

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
#ifdef BUILD_HAS_MOD_TIMER
        config = fwk_module_get_data(fwk_module_id_juno_ppu);

        status = fwk_module_bind(
            config->timer_alarm_id,
            MOD_TIMER_API_ID_ALARM,
            &juno_ppu_ctx.alarm_api);
        if (status != FWK_SUCCESS) {
            return FWK_E_HANDLER;
        }
#endif

        return FWK_SUCCESS;
    }

    ppu_ctx = juno_ppu_ctx.ppu_ctx_table + fwk_id_get_element_idx(id);
    dev_config = ppu_ctx->config;

#ifdef BUILD_HAS_MOD_TIMER
    if (!fwk_id_is_equal(dev_config->timer_id, FWK_ID_NONE)) {
        /* Bind to the timer */
        status = fwk_module_bind(dev_config->timer_id,
            MOD_TIMER_API_ID_TIMER, &ppu_ctx->timer_api);
        if (status != FWK_SUCCESS) {
            return FWK_E_PANIC;
        }
    }
    #endif

    if (!fwk_id_is_equal(ppu_ctx->bound_id, FWK_ID_NONE)) {
        /* Bind back to the entity that bound to us (if any) */
        switch (fwk_id_get_module_idx(ppu_ctx->bound_id)) {
#ifdef BUILD_HAS_MOD_POWER_DOMAIN
        case FWK_MODULE_IDX_POWER_DOMAIN:
            /* Bind back to the PD module */
            status = fwk_module_bind(ppu_ctx->bound_id,
                mod_pd_api_id_driver_input, &ppu_ctx->pd_api);
            if (status != FWK_SUCCESS) {
                return FWK_E_PANIC;
            }

            break;
        #endif

#ifdef BUILD_HAS_MOD_SYSTEM_POWER
        case FWK_MODULE_IDX_SYSTEM_POWER:
            /* Bind back to the System Power module */
            status = fwk_module_bind(ppu_ctx->bound_id,
                mod_system_power_api_id_pd_driver_input, &ppu_ctx->pd_api);
            if (status != FWK_SUCCESS) {
                return FWK_E_PANIC;
            }

            break;
        #endif

        default:
            rt_kprintf("%s:%d\n", __func__, __LINE__);
            return FWK_E_SUPPORT;

            break;
        }
    }

    return FWK_SUCCESS;
}

static int juno_ppu_process_bind_request(fwk_id_t requester_id,
                                         fwk_id_t id,
                                         fwk_id_t api_id,
                                         const void **api)
{
    struct ppu_ctx *ppu_ctx;
    const struct mod_juno_ppu_element_config *dev_config;

    switch ((enum mod_juno_ppu_api_idx)fwk_id_get_api_idx(api_id)) {
    case MOD_JUNO_PPU_API_IDX_PD:
        ppu_ctx = juno_ppu_ctx.ppu_ctx_table + fwk_id_get_element_idx(id);

        ppu_ctx->bound_id = requester_id;

        dev_config = ppu_ctx->config;

        switch (dev_config->pd_type) {
        case MOD_PD_TYPE_CORE:
            *api = &core_pd_driver_api;

            break;

        case MOD_PD_TYPE_CLUSTER:
            *api = &cluster_pd_driver_api;

            break;

        case MOD_PD_TYPE_SYSTEM:
        case MOD_PD_TYPE_DEVICE:
        case MOD_PD_TYPE_DEVICE_DEBUG:
            if (fwk_id_get_element_idx(id) == JUNO_PPU_DEV_IDX_SYSTOP) {
                *api = &css_pd_driver_api;
            } else {
                *api = &pd_driver_api;
            }

            break;

        default:
            return FWK_E_PARAM;
        }

        return FWK_SUCCESS;
    default:
        return FWK_E_SUPPORT;
    }
}

static int juno_ppu_start(fwk_id_t id)
{
    int status;
    struct ppu_ctx *ppu_ctx;
    /* unsigned int warm_reset_irq; */
    unsigned int wakeup_irq;

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    ppu_ctx = juno_ppu_ctx.ppu_ctx_table + fwk_id_get_element_idx(id);

    if (ppu_ctx->config->pd_type != MOD_PD_TYPE_CORE) {
        return FWK_SUCCESS;
    }

    /* warm_reset_irq = ppu_ctx->config->warm_reset_irq; */
    wakeup_irq = ppu_ctx->config->wakeup_irq;

    /*
     * Perform ISR configuration only when interrupts ID is provided
     */
    if (/* (warm_reset_irq == 0) || */ (wakeup_irq == 0)) {
        return FWK_SUCCESS;
    }

    status = fwk_interrupt_set_isr_param(wakeup_irq, core_wakeup_handler, (void *)ppu_ctx);
    if (status != FWK_SUCCESS) {
        return FWK_E_PANIC;
    }

    /* TODO: warm_reset_irq register later ? */

    return FWK_SUCCESS;
}

const struct fwk_module module_juno_ppu = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = (unsigned int)MOD_JUNO_PPU_API_COUNT,
    .init = juno_ppu_module_init,
    .element_init = juno_ppu_element_init,
    .bind = juno_ppu_bind,
    .process_bind_request = juno_ppu_process_bind_request,
    .start = juno_ppu_start,
};
