/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPACEMIT_MAILBOX_K3_H__
#define __SPACEMIT_MAILBOX_K3_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#define SPACEMIT_NUM_CHANNELS	4

/* mailbox register description */
typedef volatile union mbox_msg {
	rt_uint32_t val;
	struct {
		rt_uint32_t msg:32;
	} bits;
} mbox_msg_t;

typedef volatile union mbox_fifo_status {
	rt_uint32_t val;
	struct {
		rt_uint32_t is_full:1;
		rt_uint32_t is_empty:1;
		rt_uint32_t reserved0:26;
		rt_uint32_t writefull:1;
		rt_uint32_t readempty:1;
		rt_uint32_t reserved1:2;
	} bits;
} mbox_fifo_status_t;


typedef volatile union mbox_msg_status {
	rt_uint32_t val;
	struct {
		rt_uint32_t num_msg:4;
		rt_uint32_t reserved0:24;
		rt_uint32_t writefull:1;
		rt_uint32_t readempty:1;
		rt_uint32_t reserved1:2;
	} bits;
} mbox_msg_status_t;

typedef volatile union mbox_irq_status {
	rt_uint32_t val;
	struct {
		rt_uint32_t new_msg0_status:1;
		rt_uint32_t not_msg0_full:1;
		rt_uint32_t new_msg1_status:1;
		rt_uint32_t not_msg1_full:1;
		rt_uint32_t new_msg2_status:1;
		rt_uint32_t not_msg2_full:1;
		rt_uint32_t new_msg3_status:1;
		rt_uint32_t not_msg3_full:1;
		rt_uint32_t reserved:24;
	} bits;
} mbox_irq_status_t;

typedef volatile union mbox_irq_status_clr {
	rt_uint32_t val;
	struct {
		rt_uint32_t new_msg0_clr:1;
		rt_uint32_t not_msg0_full_clr:1;
		rt_uint32_t new_msg1_clr:1;
		rt_uint32_t not_msg1_full_clr:1;
		rt_uint32_t new_msg2_clr:1;
		rt_uint32_t not_msg2_full_clr:1;
		rt_uint32_t new_msg3_clr:1;
		rt_uint32_t not_msg3_full_clr:1;
		rt_uint32_t reserved:24;
	} bits;
} mbox_irq_status_clr_t;

typedef volatile union mbox_irq_enable_set {
	rt_uint32_t val;
	struct {
		rt_uint32_t new_msg0_irq_en:1;
		rt_uint32_t not_msg0_full_irq_en:1;
		rt_uint32_t new_msg1_irq_en:1;
		rt_uint32_t not_msg1_full_irq_en:1;
		rt_uint32_t new_msg2_irq_en:1;
		rt_uint32_t not_msg2_full_irq_en:1;
		rt_uint32_t new_msg3_irq_en:1;
		rt_uint32_t not_msg3_full_irq_en:1;
		rt_uint32_t reserved:24;
	} bits;
} mbox_irq_enable_set_t;

typedef volatile union mbox_irq_enable_clr {
	rt_uint32_t val;
	struct {
		rt_uint32_t new_msg0_irq_clr:1;
		rt_uint32_t not_msg0_full_irq_clr:1;
		rt_uint32_t new_msg1_irq_clr:1;
		rt_uint32_t not_msg1_full_irq_clr:1;
		rt_uint32_t new_msg2_irq_clr:1;
		rt_uint32_t not_msg2_full_irq_clr:1;
		rt_uint32_t new_msg3_irq_clr:1;
		rt_uint32_t not_msg3_full_irq_clr:1;
		rt_uint32_t reserved:24;
	} bits;
} mbox_irq_enable_clr_t;

typedef volatile struct mbox_irqthresh {
	rt_uint32_t val;
	struct {
		unsigned long mbox0_new_msg_thresh:4;
		unsigned long mbox0_not_full_thresh:4;
		unsigned long mbox1_new_msg_thresh:4;
		unsigned long mbox1_not_full_thresh:4;
		unsigned long mbox2_new_msg_thresh:4;
		unsigned long mbox2_not_full_thresh:4;
		unsigned long mbox3_new_msg_thresh:4;
		unsigned long mbox3_not_full_thresh:4;
	} bits;
} mbox_irqthresh_t;

typedef volatile struct mbox_thresh {
	mbox_irqthresh_t thresh0;
	rt_uint32_t reserved[3];
} mbox_thresh_t;

typedef volatile struct mbox_irq {
	mbox_irq_status_t irq_status;
	mbox_irq_status_clr_t irq_status_clr;
	mbox_irq_enable_set_t irq_en_set;
	mbox_irq_enable_clr_t irq_en_clr;
} mbox_irq_t;

typedef volatile struct mbox_reg_desc {
	rt_uint32_t mbox_version; /* 0x00 */
	rt_uint32_t reseved0[3]; /* 0x4, 0x8, 0xc */
	rt_uint32_t mbox_sysconfig; /* 0x10 */
	rt_uint32_t reserved1[11]; /* 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c */
	mbox_msg_t mbox_msg[4]; /* 0x40, 0x44, 0x48, 0x4c */
	rt_uint32_t reserved2[12]; /* 0x50, 0x54, 0x58, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x78, 0x7c */
	mbox_fifo_status_t fifo_status[4]; /* 0x80, 0x84, 0x88, 0x8c */
	rt_uint32_t reserved3[12]; /* 0x90, 0x94, 0x98, 0x9c, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc */
	mbox_msg_status_t msg_status[4]; /* 0xc0, 0xc4, 0xc8, 0xcc */
	rt_uint32_t reserved4[12]; /* 0xd0, 0xd4, 0xd8, 0xdc, 0xe0, 0xe4, 0xe8, 0xec, 0xf0, 0xf4, 0xf8, 0xfc */
	mbox_irq_t mbox_irq[2]; /* 0x100 --- 0x11c */
	rt_uint32_t reserved5[24]; /* 0x120, 0x124, 0x128, 0x12c, 0x130, 0x134, 0x138, 0x13c, 0x140, 0x144, 0x148, 0x14c,
				   * 0x150, 0x154, 0x158, 0x15c, 0x160, 0x164, 0x168, 0x16c, 0x170, 0x174, 0x178, 0x17c
				   * */
	mbox_thresh_t mbox_thresh[2]; /* 0x180 */
} mbox_reg_desc_t;

struct spacemit_mailbox {
	struct mbox_controller controller;
	mbox_reg_desc_t *regs;
	struct rt_spinlock lock;
	bool rcpu_communicate;
};

struct spacemit_mb_con_priv {
	struct spacemit_mailbox *smb;
};

#define USER0_MBOX_OFFSET	((mbox->rcpu_communicate) ? 1:0)
#define USER1_MBOX_OFFSET	((mbox->rcpu_communicate) ? 0:1)

#endif /* __SPACEMIT_MAILBOX_K3_H__ */
