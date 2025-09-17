/*
 * Arm SCP/MCP Software
 * Copyright (c) 2017-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <rthw.h>
#include <riscv-clic.h>
#include <rtthread.h>
#include "gtimer_reg.h"

#include <mod_clock.h>
#include <mod_gtimer.h>
#include <mod_timer.h>

#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_notification.h>
#include <fwk_status.h>
#include <fwk_time.h>

#include <stddef.h>
#include <stdint.h>

#define GTIMER_FREQUENCY_MIN_HZ  UINT32_C(1)
#define GTIMER_FREQUENCY_MAX_HZ  UINT32_C(1000000000)
#define GTIMER_MIN_TIMESTAMP 2000

/* Device content */
struct gtimer_dev_ctx {
    dw_timer_reg_t *hw_timer;
    const struct mod_gtimer_dev_config *config;
};

static struct mod_gtimer_mod_ctx {
    bool initialized; /* Whether the device context has been initialized */

    struct gtimer_dev_ctx *table; /* Device context table */
} mod_gtimer_ctx;

static uint64_t mod_gtimer_get_counter(const dw_timer_reg_t *hw_timer)
{
    return hw_timer->MTIME;
}

/*
 * Functions fulfilling the Timer module's driver interface
 */
static int enable(fwk_id_t dev_id)
{
    rt_hw_interrupt_umask(CORET_IRQn);

    return FWK_SUCCESS;
}

static int disable(fwk_id_t dev_id)
{
    rt_hw_interrupt_mask(CORET_IRQn);
    
    return FWK_SUCCESS;
}

static int get_counter(fwk_id_t dev_id, uint64_t *value)
{
    const struct gtimer_dev_ctx *ctx;

    ctx = mod_gtimer_ctx.table + fwk_id_get_element_idx(dev_id);

    *value = mod_gtimer_get_counter(ctx->hw_timer);

    return FWK_SUCCESS;
}

static int set_timer(fwk_id_t dev_id, uint64_t timestamp)
{
    struct gtimer_dev_ctx *ctx;
    uint64_t counter;
    int status;

    status = get_counter(dev_id, &counter);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /*
     * If an alarm's period is very small, the timer device could be configured
     * to interrupt on a timestamp that is "in the past". In this case, with the
     * current FVP implementation an interrupt will not be generated. To avoid
     * this issue, the minimum timestamp is GTIMER_MIN_TIMESTAMP ticks from now.
     *
     * It  is assumed here that the 64-bit counter will never loop back during
     * the course of the execution (@1GHz it would loop back after ~585 years).
     */
    timestamp = FWK_MAX(counter + GTIMER_MIN_TIMESTAMP, timestamp);

    ctx = mod_gtimer_ctx.table + fwk_id_get_element_idx(dev_id);

    ctx->hw_timer->MTIMECMP = timestamp;

    return FWK_SUCCESS;
}

static int get_timer(fwk_id_t dev_id, uint64_t *timestamp)
{
    struct gtimer_dev_ctx *ctx;

    ctx = mod_gtimer_ctx.table + fwk_id_get_element_idx(dev_id);

    /* Read 64-bit timer value */
    *timestamp = ctx->hw_timer->MTIMECMP;

    return FWK_SUCCESS;
}

static int get_frequency(fwk_id_t dev_id, uint32_t *frequency)
{
    struct gtimer_dev_ctx *ctx;

    if (frequency == NULL) {
        return FWK_E_PARAM;
    }

    ctx = mod_gtimer_ctx.table + fwk_id_get_element_idx(dev_id);

    *frequency = ctx->config->frequency;

    return FWK_SUCCESS;
}

static const struct mod_timer_driver_api module_api = {
    .enable = enable,
    .disable = disable,
    .set_timer = set_timer,
    .get_timer = get_timer,
    .get_counter = get_counter,
    .get_frequency = get_frequency,
};

/*
 * Functions fulfilling the framework's module interface
 */

static int gtimer_init(fwk_id_t module_id,
                       unsigned int element_count,
                       const void *data)
{
    mod_gtimer_ctx.table =
        fwk_mm_alloc(element_count, sizeof(struct gtimer_dev_ctx));

    return FWK_SUCCESS;
}

static int gtimer_device_init(fwk_id_t element_id, unsigned int unused,
                              const void *data)
{
    int status;
    struct gtimer_dev_ctx *ctx;

    ctx = mod_gtimer_ctx.table + fwk_id_get_element_idx(element_id);

    ctx->config = data;
    if (ctx->config->hw_timer == 0   ||
        ctx->config->frequency < GTIMER_FREQUENCY_MIN_HZ ||
        ctx->config->frequency > GTIMER_FREQUENCY_MAX_HZ) {

        return FWK_E_DEVICE;
    }

    ctx->hw_timer   = (dw_timer_reg_t *)ctx->config->hw_timer;

    status = disable(element_id);
    if (status != FWK_SUCCESS) {
    	rt_kprintf("%s:%d, disbale error\n", __func__, __LINE__);
	while (1);
    }

    return FWK_SUCCESS;
}

static int gtimer_process_bind_request(fwk_id_t requester_id,
                                       fwk_id_t id,
                                       fwk_id_t api_type,
                                       const void **api)
{
    /* No binding to the module */
    if (fwk_module_is_valid_module_id(id)) {
        return FWK_E_ACCESS;
    }

    *api = &module_api;

    return FWK_SUCCESS;
}

static void gtimer_control_init(struct gtimer_dev_ctx *ctx)
{
    /* Set primary counter update frequency and enable counter. */
}

static int gtimer_start(fwk_id_t id)
{
    struct gtimer_dev_ctx *ctx;

    if (!fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_SUCCESS;
    }

    ctx = mod_gtimer_ctx.table + fwk_id_get_element_idx(id);

    if (!fwk_id_is_type(ctx->config->clock_id, FWK_ID_TYPE_NONE)) {
        /* Register for clock state notifications */
        return fwk_notification_subscribe(
            mod_clock_notification_id_state_changed,
            ctx->config->clock_id,
            id);
    } else {
        gtimer_control_init(ctx);
    }

    return FWK_SUCCESS;
}

static int gtimer_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    struct clock_notification_params *params;
    struct gtimer_dev_ctx *ctx;

    if (!fwk_id_is_equal(event->id, mod_clock_notification_id_state_changed)) {
    	rt_kprintf("%s:%d, err\n", __func__, __LINE__);
	while (1);	
    }

    if (!fwk_id_is_type(event->target_id, FWK_ID_TYPE_ELEMENT)) {
    	rt_kprintf("%s:%d, err\n", __func__, __LINE__);
	while (1);
    }

    params = (struct clock_notification_params *)event->params;

    if (params->new_state == MOD_CLOCK_STATE_RUNNING) {
        ctx = mod_gtimer_ctx.table + fwk_id_get_element_idx(event->target_id);
        gtimer_control_init(ctx);
    }

    return FWK_SUCCESS;
}

/*
 * Module descriptor
 */
const struct fwk_module module_gtimer = {
    .api_count = 1,
    .event_count = 0,
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = gtimer_init,
    .element_init = gtimer_device_init,
    .start = gtimer_start,
    .process_bind_request = gtimer_process_bind_request,
    .process_notification = gtimer_process_notification,
};

static fwk_timestamp_t mod_gtimer_timestamp(const void *ctx)
{
    fwk_timestamp_t timestamp;

    const struct mod_gtimer_dev_config *cfg = ctx;
    const dw_timer_reg_t *hw_timer = (const void *)cfg->hw_timer;

    uint32_t frequency = cfg->frequency;
    uint64_t counter = mod_gtimer_get_counter(hw_timer);

    timestamp = (FWK_S(1) / frequency) * counter;

    return timestamp;
}

struct fwk_time_driver mod_gtimer_driver(
    const void **ctx,
    const struct mod_gtimer_dev_config *cfg)
{
    *ctx = cfg;

    return (struct fwk_time_driver){
        .timestamp = mod_gtimer_timestamp,
    };
}
