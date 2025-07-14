#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include "k3_mailbox.h"

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k3-mailbox3", },
	{ .compatible = "spacemit,k3-mailbox4", },
	{ .compatible = "spacemit,k3-mailbox5", },
	{ .compatible = "spacemit,k3-mailbox6", },
	{ .compatible = "spacemit,k3-mailbox7", },
	{},
};

static void spacemit_mbox_irq(rt_int32_t irq, void *dev_id)
{
	struct spacemit_mailbox *mbox = dev_id;
	struct mbox_chan *chan;
	rt_uint32_t status, msg;
	rt_int32_t i, j;

	status = readl((void *)&mbox->regs->mbox_irq[1].irq_status)
		& readl((void *)&mbox->regs->mbox_irq[1].irq_en_set);

	if (!(status & 0xff))
		return;

	rt_spin_lock(&mbox->lock);

	for (i = 0; i < SPACEMIT_NUM_CHANNELS; ++i) {
		chan = &mbox->controller.chans[i];

		/* not full irq */
		if (status & (1 << (i * 2 + 1))) {
			j = readl((void *)&mbox->regs->mbox_irq[1].irq_status_clr);
			j |= (1 << (i * 2 + 1));
			writel(j, (void *)&mbox->regs->mbox_irq[1].irq_status_clr);

			/* disable not full irq */
			j = readl((void *)&mbox->regs->mbox_irq[1].irq_en_clr);
			j |= (1 << (i * 2 + 1));
			writel(j, (void *)&mbox->regs->mbox_irq[1].irq_en_clr);

			if (chan->txdone_method & TXDONE_BY_IRQ)
				mbox_chan_txdone(chan, 0);
		}

		/* new msg irq */
		if (status & (1 << (i * 2))) {

			/* clear the fifo */
			while (readl((void *)&mbox->regs->msg_status[i])) {
				msg = readl((void *)&mbox->regs->mbox_msg[i]);
				mbox_chan_received_data(chan, &msg);
			}

			/* clear the irq pending */
			j = readl((void *)&mbox->regs->mbox_irq[1].irq_status_clr);
			j |= (1 << (i * 2));
			writel(j, (void *)&mbox->regs->mbox_irq[1].irq_status_clr);
		}
        }

	rt_spin_unlock(&mbox->lock);
}

static rt_int32_t spacemit_chan_send_data(struct mbox_chan *chan, void *data)
{
	rt_uint32_t j;
	struct spacemit_mailbox *mbox = ((struct spacemit_mb_con_priv *)chan->con_priv)->smb;
	rt_uint32_t chan_num = chan - mbox->controller.chans;

        /* send data */
	writel('c', (void *)&mbox->regs->mbox_msg[chan_num]);

	/* enable the not full interrupt */
	rt_spin_lock(&mbox->lock);

	/* set not full thresh */
	j = readl((void *)&mbox->regs->mbox_thresh[1].thresh0);
	j |= 1 << (chan_num * 8 + 4);
	writel(j, (void *)&mbox->regs->mbox_thresh[1].thresh0);

	/* enable not full irq */
	j = readl((void *)&mbox->regs->mbox_irq[1].irq_en_set);
	j |= (1 << (chan_num * 2 + 1));
	writel(j, (void *)&mbox->regs->mbox_irq[1].irq_en_set);

	rt_spin_unlock(&mbox->lock);
}

static rt_int32_t spacemit_chan_startup(struct mbox_chan *chan)
{
	struct spacemit_mailbox *mbox = ((struct spacemit_mb_con_priv *)chan->con_priv)->smb;
	rt_uint32_t chan_num = chan - mbox->controller.chans;
	rt_uint32_t msg, j;

	/* clear the fifo */
	while (readl((void *)&mbox->regs->msg_status[chan_num])) {
		msg = readl((void *)&mbox->regs->mbox_msg[chan_num]);
	}

	rt_spin_lock(&mbox->lock);

	/* clear pending */
	j = readl((void *)&mbox->regs->mbox_irq[1].irq_status_clr);
	j |= (1 << (chan_num * 2));
	writel(j, (void *)&mbox->regs->mbox_irq[1].irq_status_clr);

	/* enable new msg irq */
	j = readl((void *)&mbox->regs->mbox_irq[1].irq_en_set);
	j |= (1 << (chan_num * 2));
	writel(j, (void *)&mbox->regs->mbox_irq[1].irq_en_set);

	rt_spin_unlock(&mbox->lock);

        return 0;
}

static void spacemit_chan_shutdown(struct mbox_chan *chan)
{
	struct spacemit_mailbox *mbox = ((struct spacemit_mb_con_priv *)chan->con_priv)->smb;
	rt_uint32_t chan_num = chan - mbox->controller.chans;
	rt_uint32_t msg, j;

	if (chan->cl->tx_prepare != NULL)
		return;

	rt_spin_lock(&mbox->lock);

	/* disable new msg irq */
	j = readl((void *)&mbox->regs->mbox_irq[1].irq_en_clr);
	j |= (1 << (chan_num * 2));
	writel(j, (void *)&mbox->regs->mbox_irq[1].irq_en_clr);

	/* flush the fifo */
	while (readl((void *)&mbox->regs->msg_status[chan_num])) {
		msg = readl((void *)&mbox->regs->mbox_msg[chan_num]);
	}

	/* clear pending */
	j = readl((void *)&mbox->regs->mbox_irq[1].irq_status_clr);
	j |= (1 << (chan_num * 2));
	writel(j, (void *)&mbox->regs->mbox_irq[1].irq_status_clr);

	rt_spin_unlock(&mbox->lock);
}

static bool spacemit_chan_last_tx_done(struct mbox_chan *chan)
{
	/* TODO */
	return true;
}

static bool spacemit_chan_peek_data(struct mbox_chan *chan)
{
	struct spacemit_mailbox *mbox = chan->con_priv;
	rt_uint32_t chan_num = chan - mbox->controller.chans;

	return readl((void *)&mbox->regs->msg_status[chan_num]);
}

static const struct mbox_chan_ops spacemit_chan_ops = {
	.send_data    = spacemit_chan_send_data,
	.startup      = spacemit_chan_startup,
	.shutdown     = spacemit_chan_shutdown,
	.last_tx_done = spacemit_chan_last_tx_done,
	.peek_data    = spacemit_chan_peek_data,
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

			mbox->regs = (mbox_reg_desc_t *)dtb_node_get_addr_index(compatible_node, 0);
			if (!mbox->regs) {
				rt_kprintf("%s:%d, No reg found\n", __func__, __LINE__);
				return -RT_EINVAL;
			}

			for (j = 0; j < SPACEMIT_NUM_CHANNELS; ++j) {
				con_priv[j].smb = mbox;
				chans[j].con_priv = con_priv + j;
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

			rt_hw_interrupt_umask(irq);
		}
	}

	return 0;
}
INIT_DEVICE_EXPORT(spacemit_mailbox_init);
