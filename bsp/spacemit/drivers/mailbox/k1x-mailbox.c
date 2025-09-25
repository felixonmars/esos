/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include "k1x_mailbox.h"

static void spacemit_mbox_irq(rt_int32_t irq, void *dev_id)
{
	struct spacemit_mailbox *mbox = dev_id;
	struct mbox_chan *chan;
	rt_uint32_t status, msg = 0;
	rt_int32_t i;

	writel(0, (void *)&mbox->regs->ipc_iir);

	status = readl((void *)&mbox->regs->ipc_iir);
	if (!(status & 0xff))
		return;

	for (i = SPACEMIT_TX_ACK_OFFSET; i < SPACEMIT_NUM_CHANNELS + SPACEMIT_TX_ACK_OFFSET; ++i) {
		chan = &mbox->controller.chans[i - SPACEMIT_TX_ACK_OFFSET];
		/* new msg irq */
		if (!(status & (1 << i)))
			continue;

		/* clear the irq pending */
		writel(1 << i, (void *)&mbox->regs->ipc_icr);

		if (chan->txdone_method & TXDONE_BY_IRQ)
			mbox_chan_txdone(chan, 0);
	}

	for (i = 0; i < SPACEMIT_NUM_CHANNELS; ++i) {
		chan = &mbox->controller.chans[i];

		/* new msg irq */
		if (!(status & (1 << i)))
			continue;

		mbox_chan_received_data(chan, &msg);

		/* clear the irq pending */
		writel(1 << i, (void *)&mbox->regs->ipc_icr);

#if 0
		/* debug code for k1 rpmi */
		if (i != 3)
#endif
			/* then send an ack */
			writel(1 << (i + SPACEMIT_TX_ACK_OFFSET), (void *)&mbox->regs->ipc_isrw);
	}

	return;
}

static rt_int32_t spacemit_chan_send_data(struct mbox_chan *chan, void *data)
{
	rt_uint64_t flag;
	struct spacemit_mailbox *mbox = ((struct spacemit_mb_con_priv *)chan->con_priv)->smb;
	rt_uint32_t chan_num = chan - mbox->controller.chans;

	flag = rt_spin_lock_irqsave(&mbox->lock);

	writel(1 << chan_num, (void *)&mbox->regs->ipc_isrw);

	rt_spin_unlock_irqrestore(&mbox->lock, flag);

        // mbox_dbg(mbox, "Channel %d sent 0x%08x\n", chan_num, *((rt_uint32_t *)data));

        return 0;
}

static rt_int32_t spacemit_chan_startup(struct mbox_chan *chan)
{
	return 0;
}

static void spacemit_chan_shutdown(struct mbox_chan *chan)
{
	rt_uint32_t j;
	rt_uint64_t flag;
	struct spacemit_mailbox *mbox = ((struct spacemit_mb_con_priv *)chan->con_priv)->smb;
	rt_uint32_t chan_num = chan - mbox->controller.chans;

	flag = rt_spin_lock_irqsave(&mbox->lock);

	/* clear pending */
	j = 0;
	j |= (1 << chan_num);
	writel(j, (void *)&mbox->regs->ipc_icr);

	rt_spin_unlock_irqrestore(&mbox->lock, flag);
}

static bool spacemit_chan_last_tx_done(struct mbox_chan *chan)
{
	return 0;
}

static bool spacemit_chan_peek_data(struct mbox_chan *chan)
{
	struct spacemit_mailbox *mbox = ((struct spacemit_mb_con_priv *)chan->con_priv)->smb;
	rt_uint32_t chan_num = chan - mbox->controller.chans;

	return readl((void *)&mbox->regs->ipc_rdr) & (1 << chan_num);
}

static const struct mbox_chan_ops spacemit_chan_ops = {
	.send_data    = spacemit_chan_send_data,
	.startup      = spacemit_chan_startup,
	.shutdown     = spacemit_chan_shutdown,
	.last_tx_done = spacemit_chan_last_tx_done,
	.peek_data    = spacemit_chan_peek_data,
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k1-x-mailbox", },
	{},
};

rt_int32_t spacemit_mailbox_init(void)
{
	rt_int32_t i, j, irq, ret;
	struct mbox_chan *chans;
	struct spacemit_mailbox *mbox;
	struct spacemit_mb_con_priv *con_priv;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			mbox = rt_calloc(1, sizeof(struct spacemit_mailbox));
			if (!mbox) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			con_priv = rt_calloc(SPACEMIT_NUM_CHANNELS, sizeof(struct spacemit_mb_con_priv));
			if (!con_priv) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			chans = rt_calloc(SPACEMIT_NUM_CHANNELS, sizeof(struct mbox_chan));
			if (!chans) {
				rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			for (j = 0; j < SPACEMIT_NUM_CHANNELS; ++j) {
				con_priv[j].smb = mbox;
				chans[j].con_priv = con_priv + j;
			}

			mbox->regs = (mbox_reg_desc_t *)dtb_node_get_addr_index(compatible_node, 0);
			if (!mbox->regs) {
				rt_kprintf("%s:%d, No reg found\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			irq = dtb_node_irq_get(compatible_node, 0);

			rt_hw_interrupt_install(irq, spacemit_mbox_irq, (void *)mbox, "spt-mbox");

			/* register the mailbox controller */
			mbox->controller.dev           = compatible_node;
			mbox->controller.ops           = &spacemit_chan_ops;
			mbox->controller.chans         = chans;
			mbox->controller.num_chans     = SPACEMIT_NUM_CHANNELS;
			mbox->controller.txdone_irq    = true;
			mbox->controller.txdone_poll   = false;
			mbox->controller.txpoll_period = 5;

			rt_spin_lock_init(&mbox->lock);

			ret = mbox_controller_register(&mbox->controller);
			if (ret) {
				rt_kprintf("%s:%d, Failed to register controller: %d\n", __func__, __LINE__, ret);
				return -RT_EINVAL;
			}
		}
	}

	return 0;
}
INIT_DEVICE_EXPORT(spacemit_mailbox_init);
