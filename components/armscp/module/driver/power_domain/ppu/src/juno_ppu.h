/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef JUNO_PPU_H
#define JUNO_PPU_H

#include <rtdef.h>

#include <mod_juno_ppu.h>

#include <fwk_macros.h>

#include <mod_timer.h>

#include <stdint.h>

typedef union c0_pd_unavail {
	unsigned int val;
	struct {
		unsigned int c0_unavail_core0:1;
		unsigned int c0_unavail_core1:1;
		unsigned int c0_unavail_core2:1;
		unsigned int c0_unavail_core3:1;
		unsigned int reserved:28;
	} bits;
} c0_pd_unavail_t;

typedef union c0_pd_flush {
	unsigned int val;
	struct {
		unsigned int c0_flush_req:1;
		unsigned int reserved:31;
	} bits;
} c0_pd_flush_t;

typedef union c0_pd_status {
	unsigned int val;
	struct {
		unsigned int c0_core0_lpmd_b:1;
		unsigned int c0_core1_lpmd_b:1;
		unsigned int c0_core2_lpmd_b:1;
		unsigned int c0_core3_lpmd_b:1;
		unsigned int c0_no_op:1;
		unsigned int reserved:27;
	} bits;
} c0_pd_status_t;

typedef union c1_pd_unavail {
	unsigned int val;
	struct {
		unsigned int c1_unavail_core0:1;
		unsigned int c1_unavail_core1:1;
		unsigned int c1_unavail_core2:1;
		unsigned int c1_unavail_core3:1;
		unsigned int reserved:28;
	} bits;
} c1_pd_unavail_t;

typedef union c1_pd_flush {
	unsigned int val;
	struct {
		unsigned int c1_flush_req:1;
		unsigned int reserved:31;
	} bits;
} c1_pd_flush_t;

typedef union c1_pd_status {
	unsigned int val;
	struct {
		unsigned int c1_core0_lpmd_b:1;
		unsigned int c1_core1_lpmd_b:1;
		unsigned int c1_core2_lpmd_b:1;
		unsigned int c1_core3_lpmd_b:1;
		unsigned int c1_no_op:1;
		unsigned int reserved:27;
	} bits;
} c1_pd_status_t;

struct ppu_reg {
    c0_pd_unavail_t c0_pd_ua; /* 0x500 */
    c0_pd_flush_t c0_flush; /* 0x504 */
    c0_pd_status_t c0_pd_st; /* 0x508 */
    unsigned int reserved; /* 0x50c */
    c1_pd_unavail_t c1_pd_ua; /* 0x510 */
    c1_pd_flush_t c1_flush;
    c1_pd_status_t c1_pd_st;
};

struct ppu_ctx {
    /* Pointer to the element's configuration data */
    const struct mod_juno_ppu_element_config *config;

    /* Identifier of the entity that bound to this PPU element */
    fwk_id_t bound_id;

    /* Timer API */
    const struct mod_timer_api *timer_api;

    /* Power domain driver input API */
    const struct mod_pd_driver_input_api *pd_api;

    /* Power domain status, will delete later */
    int status;
};

struct module_ctx {
    /* Table of element context structures */
    struct ppu_ctx *ppu_ctx_table;

    /* Timer alarm API */
    const struct mod_timer_alarm_api *alarm_api;

    /* CSS power state */
    unsigned int css_state;
};

#endif /* JUNO_PPU_H */
