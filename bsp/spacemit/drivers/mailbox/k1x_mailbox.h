/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RT_MAILBOX_H__
#define __RT_MAILBOX_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#define SPACEMIT_NUM_CHANNELS   4
#define SPACEMIT_TX_ACK_OFFSET  4

typedef volatile struct mbox_reg_desc {
	rt_uint32_t ipc_dw;
	rt_uint32_t ipc_wdr;
	rt_uint32_t ipc_isrw;
	rt_uint32_t ipc_icr;
	rt_uint32_t ipc_iir;
	rt_uint32_t ipc_rdr;
} mbox_reg_desc_t;

struct spacemit_mailbox {
	struct mbox_controller controller;
	struct reset_control *reset;
	mbox_reg_desc_t *regs;
	struct rt_spinlock lock;
};

struct spacemit_mb_con_priv {
	struct spacemit_mailbox *smb;
};

#endif /* __RT_MAILBOX_H__ */
