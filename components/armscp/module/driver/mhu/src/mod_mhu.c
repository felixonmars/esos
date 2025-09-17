/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      Message Handling Unit (MHU) Device Driver.
 */

#include <internal/mhu.h>
#include <rtdef.h>
#include <mod_mhu.h>
#include <mod_transport.h>

#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#include <register_defination.h>

#include <stddef.h>
#include <stdint.h>


struct mhu_transport_channel {
    fwk_id_t id;
    struct mod_transport_driver_input_api *api;
};

/* MHU device context */
struct mhu_device_ctx {
    /* Pointer to the device configuration */
    const struct mod_mhu_device_config *config;

    /* Number of slots (represented by sub-elements) */
    unsigned int slot_count;

    /* Mask of slots that are bound to a TRANSPORT/SMT channel */
    uint32_t bound_slots;

    /* Table of TRANSPORT channels bound to the device */
    struct mhu_transport_channel *transport_channel_table;
};

/* MHU context */
struct mod_mhu_ctx {
    /* Table of device contexts */
    struct mhu_device_ctx *device_ctx_table;

    /* Number of devices in the device context table*/
    unsigned int device_count;

    /* event waiting for the mhu irq */
    rt_event_t event;
};

enum mod_mhu_event {
    MHU_EVENT_DEVICE_0 = 1,
    MHU_EVENT_DEVICE_1 = 2,
    MHU_EVENT_DEVICE_2 = 4,
    MHU_EVENT_DEVICE_3 = 8,
    MHU_EVENT_DEVICE_MAX_COUNT,
};

#define MHU_SLOT_COUNT_MAX 4

static struct mod_mhu_ctx mhu_ctx;

static void mhu_isr(int interrupt, void *param)
{
    unsigned int status, ret;
    unsigned int device_idx;
    unsigned int mhu_event_type = 0;
    struct mhu_device_ctx *device_ctx;
    struct mhu_reg *reg;
    unsigned int slot;
    struct mhu_transport_channel *transport_channel;

    for (device_idx = 0; device_idx < mhu_ctx.device_count; device_idx++) {
        device_ctx = &mhu_ctx.device_ctx_table[device_idx];
	reg = (struct mhu_reg *)device_ctx->config->in;

	status = reg->mbox_irq[1].irq_status.val & reg->mbox_irq[1].irq_en_set.val;
	if (!(status & 0xff))
		continue;

	for (slot = 0; slot < MHU_SLOT_COUNT_MAX; ++slot) {
		if ((device_ctx->bound_slots & (uint32_t)(1U << slot)) != (uint32_t)0) {
			if (status & (1 << (2 * slot))) {
				transport_channel = &device_ctx->transport_channel_table[slot];
				ret = transport_channel->api->signal_message(transport_channel->id);
				if (ret != FWK_SUCCESS) {
					rt_kprintf("[MHU] %s @%d", __func__, __LINE__);
				}

				mhu_event_type |= (1 << slot);

				/* clear the fifo  */
				while (reg->msg_status[slot].bits.num_msg) { ret = reg->mbox_msg[slot].val ; }

				/* clear the pending */
				ret = reg->mbox_irq[1].irq_status_clr.val;
				ret |= (1 << (slot * 2));
				reg->mbox_irq[1].irq_status_clr.val = ret;
			}
		}
	}
    }

    /* wake up the waiting thread */
    rt_event_send(mhu_ctx.event, mhu_event_type);
}

void fwk_waiting_for_event(void)
{
    rt_uint32_t e;
    rt_err_t err;

    if (mhu_ctx.event == RT_NULL) {
        rt_kprintf("%s: %d, the mhu event has not initialized\n", __func__, __LINE__);
	while (1);
    }

    err = rt_event_recv(mhu_ctx.event,
		    MHU_EVENT_DEVICE_0 |
		    MHU_EVENT_DEVICE_1 |
		    MHU_EVENT_DEVICE_2 |
		    MHU_EVENT_DEVICE_3,
		    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
		    10, &e);
    if (err != RT_EOK) {
        if (err ==  -RT_ETIMEOUT)
		return;
	else {
		rt_kprintf("%s:%d, waiting error\n", __func__, __LINE__);
		while (1);
	}
    }

    return;
}

/*
 * TRANSPORT module driver API
 */

static int raise_interrupt(fwk_id_t slot_id)
{
    struct mhu_device_ctx *device_ctx;
    unsigned int slot;
    struct mhu_reg *reg;

    device_ctx = &mhu_ctx.device_ctx_table[fwk_id_get_element_idx(slot_id)];
    slot = fwk_id_get_sub_element_idx(slot_id);
    reg = (struct mhu_reg *)device_ctx->config->out;

    reg->mbox_msg[slot + 2].val = 0xc;

    return FWK_SUCCESS;
}

const struct mod_transport_driver_api mhu_mod_transport_driver_api = {
    .trigger_event = raise_interrupt,
};

/*
 * Framework handlers
 */

static int mhu_init(fwk_id_t module_id, unsigned int device_count,
                    const void *unused)
{
    if (device_count == 0) {
        return FWK_E_PARAM;
    }

    mhu_ctx.device_ctx_table = fwk_mm_calloc(device_count,
        sizeof(mhu_ctx.device_ctx_table[0]));

    mhu_ctx.device_count = device_count;

    mhu_ctx.event = rt_event_create("mhu_event", RT_IPC_FLAG_FIFO);
    if (mhu_ctx.event == RT_NULL) {
	    rt_kprintf("%s:%d, create sem error\n", __func__, __LINE__);
	    while (1);
    }

    return FWK_SUCCESS;
}

static int mhu_device_init(fwk_id_t device_id, unsigned int slot_count,
                           const void *data)
{
    struct mod_mhu_device_config *config = (struct mod_mhu_device_config *)data;
    struct mhu_device_ctx *device_ctx;

    if ((config->in == 0) || (config->out == 0)) {
        return FWK_E_PARAM;
    }

    device_ctx = &mhu_ctx.device_ctx_table[fwk_id_get_element_idx(device_id)];

    device_ctx->transport_channel_table = fwk_mm_calloc(
        slot_count, sizeof(device_ctx->transport_channel_table[0]));

    device_ctx->config = config;
    device_ctx->slot_count = slot_count;

    return FWK_SUCCESS;
}

static int mhu_bind(fwk_id_t id, unsigned int round)
{
    int status;
    struct mhu_device_ctx *device_ctx;
    unsigned int slot;
    struct mhu_transport_channel *transport_channel;

    if ((round == 1U) && fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        device_ctx = &mhu_ctx.device_ctx_table[fwk_id_get_element_idx(id)];

        for (slot = 0; slot < MHU_SLOT_COUNT_MAX; slot++) {
            if ((device_ctx->bound_slots & (uint32_t)(1U << slot)) ==
                (uint32_t)0) {
                continue;
            }

            transport_channel = &device_ctx->transport_channel_table[slot];

            status = fwk_module_bind(
                transport_channel->id,
                FWK_ID_API(
                    FWK_MODULE_IDX_TRANSPORT,
                    MOD_TRANSPORT_API_IDX_DRIVER_INPUT),
                &transport_channel->api);
            if (status != FWK_SUCCESS) {
                return status;
            }
        }
    }

    return FWK_SUCCESS;
}

static int mhu_process_bind_request(fwk_id_t source_id, fwk_id_t target_id,
                                    fwk_id_t api_id, const void **api)
{
    struct mhu_device_ctx *device_ctx;
    unsigned int slot;

    if (!fwk_id_is_type(target_id, FWK_ID_TYPE_SUB_ELEMENT)) {
        return FWK_E_ACCESS;
    }

    device_ctx = &mhu_ctx.device_ctx_table[fwk_id_get_element_idx(target_id)];
    slot = fwk_id_get_sub_element_idx(target_id);

    if (device_ctx->bound_slots & (1U << slot)) {
        return FWK_E_ACCESS;
    }

    device_ctx->transport_channel_table[slot].id = source_id;
    device_ctx->bound_slots |= 1U << slot;

    *api = &mhu_mod_transport_driver_api;

    return FWK_SUCCESS;
}

static int mhu_start(fwk_id_t id)
{
    int status;
    struct mhu_device_ctx *device_ctx;
    struct mhu_reg *regs;
    unsigned int j, slot;
    rcpu_clk_en_t *rcpu_clk_en = (rcpu_clk_en_t *)RCPU_CLK_EN_BASE;
    rcpu_sw_reset_t *rcpu_rst_en = (rcpu_sw_reset_t *)RCPU_SW_RST_BASE;

    if (fwk_id_get_type(id) == FWK_ID_TYPE_MODULE) {

	/* enable the clk */
	rcpu_clk_en->bits.mcu_mbox_clken = 1;

	/* reset the module */
	rcpu_rst_en->bits.mcu_mbox_rstn = 1;

        return FWK_SUCCESS;
    }

    device_ctx = &mhu_ctx.device_ctx_table[fwk_id_get_element_idx(id)];
    regs = (struct mhu_reg *)device_ctx->config->in;

    if (device_ctx->bound_slots != 0) {
        /* all the devices using the same mailbox controller, so we initialize below only once */
        status = fwk_interrupt_set_isr_param(device_ctx->config->irq, &mhu_isr, NULL);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = fwk_interrupt_enable(device_ctx->config->irq);
        if (status != FWK_SUCCESS) {
            return status;
        }

	/* check which channel number */
        for (slot = 0; slot < MHU_SLOT_COUNT_MAX; slot++) {
		if ((device_ctx->bound_slots & (uint32_t)(1U << slot)) == 0)
			continue;

		/* clear the fifo */
		while (regs->msg_status[slot].bits.num_msg) { j = regs->mbox_msg[slot].val; }

		/* clear pending */
		j = regs->mbox_irq[1].irq_status_clr.val;
		j |= (1 << (slot * 2));
		regs->mbox_irq[1].irq_status_clr.val = j;

		/* enable new msg irq */
		j = regs->mbox_irq[1].irq_en_set.val;
		j |= (1 << (slot * 2));
		regs->mbox_irq[1].irq_en_set.val = j;
	}
    }

    return FWK_SUCCESS;
}

/* MHU module definition */
const struct fwk_module module_mhu = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = 1,
    .init = mhu_init,
    .element_init = mhu_device_init,
    .bind = mhu_bind,
    .start = mhu_start,
    .process_bind_request = mhu_process_bind_request,
};
