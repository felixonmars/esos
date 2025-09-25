/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/dma.h>
#include <rthw.h>

#define BIT(x)		(1 << x)

#define DMA_DCR			(0x000)
#define DMA_SR			(0x004)
#define DMA_DIMR		(0x008)
#define DMA_SAR(n)		(0x080 + (n * 0x40))
#define DMA_DAR(n)		(0x084 + (n * 0x40))
#define DMA_CNTR(n)		(0x088 + (n * 0x40))
#define DMA_CCR(n)		(0x08c + (n * 0x40))
#define DMA_RSSR(n)		(0x090 + (n * 0x40)) //ok
#define DMA_BLR(n)		(0x094 + (n * 0x40))
#define DMA_TRSFCNT(n)		(0x09c + (n * 0x40))
#define DMA_BTYPE(n)		(0x0a0 + (n * 0x40))
#define DMA_WR_CNT(n)		(0x0a4 + (n * 0x40))
#define DMA_RD_CNT(n)		(0x0a8 + (n * 0x40))

/* DMA_DCR */
#define DMA_DCR_EN		(0x1 << 0)
#define DMA_DCR_RESET		(0x1 << 1)
#define DMA_DCR_AMODE		(0x1 << 2)

/* DMA_CNTR */
#define DMA_CNT_MSK		(0xffffff)
#define DMA_MAX_LEN		(0xffffff)

/* DMA_CCR */
#define DMA_CCR_CH_EN       (0x1 << 0)
#define DMA_CCR_REQ_EN      (0x1 << 3)
#define DMA_CCR_SSIZE_MSK   (0x3 << 4)
#define DMA_CCR_SSIZE_32BIT (0x0 << 4)
#define DMA_CCR_SSIZE_8BIT  (0x1 << 4)
#define DMA_CCR_SSIZE_16BIT (0x2 << 4)
#define DMA_CCR_DSIZE_MSK   (0x3 << 6)
#define DMA_CCR_DSIZE_32BIT (0x0 << 6)
#define DMA_CCR_DSIZE_8BIT  (0x1 << 6)
#define DMA_CCR_DSIZE_16BIT (0x2 << 6)
#define DMA_CCR_SMODE_MSK   (0x3 << 10)
#define DMA_CCR_SMODE_MEM   (0x0 << 10)
#define DMA_CCR_SMODE_FIFO  (0x2 << 10)
#define DMA_CCR_DMODE_MSK   (0x3 << 12)
#define DMA_CCR_DMODE_MEM   (0x0 << 12)
#define DMA_CCR_DMODE_FIFO  (0x2 << 12)

/* DMA_BLR */
#define DMA_BLR_MSK		(0x3f)
#define DMA_BLRN_BURST64	(0)		/* BLRN 64 byte burst */
#define DMA_BLRN_BURST8		(8)		/* BLRN 8 byte burst */
#define DMA_BLRN_BURST16	(16)	/* BLRN 16 byte burst */
#define DMA_BLRN_BURST32	(32)	/* BLRN 32 byte burst */

#define DMA_BURST8	(8)	/* 8 byte burst */
#define DMA_BURST16	(16)	/* 16 byte burst */
#define DMA_BURST32	(32)	/* 32 byte burst */
#define DMA_BURST64	(64)	/* 64 byte burst */
#define DMA_BURST64_MAX		DMA_BURST64
#define DMA_BURST_INVALID	(-1)

/* DMA_BTYPE */
#define DMA_BTYPE_MSK		(0x3)

#define DMA_CHANNEL_MAX		(16)

/**
 * enum dma_status - DMA transaction status
 * @DMA_COMPLETE: transaction completed
 * @DMA_IN_PROGRESS: transaction not yet processed
 * @DMA_PAUSED: transaction is paused
 * @DMA_ERROR: transaction failed
 */
enum dma_status {
	DMA_COMPLETE,
	DMA_IN_PROGRESS,
	DMA_PAUSED,
	DMA_ERROR,
	DMA_OUT_OF_ORDER,
};

struct mmp_pdma_phy;
struct mmp_pdma_device;

struct mmp_pdma_chan {
	struct rt_device *dev;
	struct rt_dma_chan chan;
	struct mmp_pdma_phy *phy;
	struct mmp_pdma_device *device;
	enum rt_dma_transfer_direction dir;
	struct rt_dma_slave_config slave_config;
	/* channel's basic info */
	rt_uint32_t dccr;		//channel control register
	rt_uint32_t dcrs;		//channel request source
	rt_uint32_t dev_addr;

	enum dma_status status; /* channel state machine */
};

struct mmp_pdma_phy {
	int idx;
	void *base;
	struct mmp_pdma_chan *vchan;
};

struct mmp_pdma_device {
	int			dma_channels;	//max support channel numbers
	rt_list_t		channels;		//channel list
	int			max_burst_size;
	void			*base;
	struct rt_device	dev;
	struct mmp_pdma_phy	*phy;
#ifdef RT_USING_SMP
	struct rt_spinlock	phy_lock; /* protect alloc/free phy channels */
#else
	rt_ubase_t		phy_lock;
#endif
	struct rt_dma_controller ctrl;
	int			irq;
};

#define to_mmp_pdma_chan(dchan)					\
	rt_container_of(dchan, struct mmp_pdma_chan, chan)
#define to_mmp_pdma_dev(dmadev)					\
	rt_container_of(dmadev, struct mmp_pdma_device, device)
#define ctrl_to_mmp_pdma_device(dctrl)					\
	rt_container_of(dctrl, struct mmp_pdma_device, ctrl)

static void enable_chan(struct mmp_pdma_phy *phy)
{
	rt_uint32_t reg;
	rt_uint32_t val;
	unsigned long flags;
	struct mmp_pdma_device *pdev;

	if (!phy)
		return;

	if (!phy->vchan)
		return;

	pdev = phy->vchan->device;
	flags = rt_spin_lock_irqsave(&pdev->phy_lock);

	reg = DMA_RSSR(phy->idx);
	writel(phy->vchan->dcrs, phy->base + reg);

	reg = DMA_CCR(phy->idx);
	val = readl(phy->base + reg);
	writel(val | DMA_CCR_CH_EN, phy->base + reg);

	val = readl(phy->base + DMA_DIMR);
	val &= (~BIT(phy->idx));
	writel(val, phy->base + DMA_DIMR);

	rt_spin_unlock_irqrestore(&pdev->phy_lock, flags);
}

static void disable_chan(struct mmp_pdma_phy *phy)
{
	rt_uint32_t reg;
	rt_uint32_t val;
	unsigned long flags;
	struct mmp_pdma_device *pdev;

	if (!phy)
		return;

	pdev = phy->vchan->device;
	flags = rt_spin_lock_irqsave(&pdev->phy_lock);

	reg = DMA_RSSR(phy->idx);
	writel(0, phy->base + reg);

	reg = DMA_CCR(phy->idx);
	val = readl(phy->base + reg);
	writel((val & (~DMA_CCR_CH_EN)), phy->base + reg);

	val = readl(phy->base + DMA_DIMR);
	val |= (BIT(phy->idx));
	writel(val, phy->base + DMA_DIMR);

	rt_spin_unlock_irqrestore(&pdev->phy_lock, flags);
}

static int clear_chan_irq(struct mmp_pdma_phy *phy)
{
	rt_uint32_t sr = readl(phy->base + DMA_SR);

	if (!(sr & BIT(phy->idx)))
		return -RT_EINVAL;

	/* clear irq */
	writel(BIT(phy->idx), phy->base + DMA_SR);
	return 0;
}

static int mmp_pdma_chan_handler(int irq, void *dev_id)
{
	struct mmp_pdma_phy *phy = dev_id;
	struct mmp_pdma_chan *pchan = phy->vchan;

	if (clear_chan_irq(phy) != 0)
		return 1;

	if (pchan) {
		if (pchan->chan.callback != RT_NULL) {
			//callback
			pchan->chan.callback(&pchan->chan, readl(phy->base + DMA_RD_CNT(phy->idx)));
		}
	}

	return 0;
}

static void mmp_pdma_int_handler(int irq, void *dev_id)
{
	struct mmp_pdma_device *pdev = dev_id;
	struct mmp_pdma_phy *phy;
	rt_uint32_t sr = readl(pdev->base + DMA_SR);
	int i = 0, ret = 0;
	unsigned long flags;

	while (sr) {
		i = __rt_ffs(sr) - 1;
		if (i >= pdev->dma_channels)
			break;

		sr &= (sr - 1);
		phy = &pdev->phy[i];
		flags = rt_spin_lock_irqsave(&pdev->phy_lock);

		ret = mmp_pdma_chan_handler(irq, phy);

		rt_spin_unlock_irqrestore(&pdev->phy_lock, flags);
	}

}

/* lookup free phy channel as descending priority */
static struct mmp_pdma_phy *lookup_phy(struct mmp_pdma_chan *pchan)
{
	int i;
	struct mmp_pdma_device *dev = pchan->device;
	struct mmp_pdma_phy *phy, *found = RT_NULL;
	unsigned long flags;

	flags = rt_spin_lock_irqsave(&dev->phy_lock);

	for (i = 0; i < dev->dma_channels; i++) {
		phy = &dev->phy[i];

		if (!phy->vchan) {
			phy->vchan = pchan;
			found = phy;
			goto out_unlock;
		}
	}

out_unlock:
	rt_spin_unlock_irqrestore(&pdev->phy_lock, flags);
	return found;
}

static void mmp_pdma_free_phy(struct mmp_pdma_chan *pchan)
{
	struct mmp_pdma_device *pdev = pchan->device;
	unsigned long flags;

	if (!pchan->phy)
		return;

	flags = rt_spin_lock_irqsave(&pdev->phy_lock);

	pchan->phy->vchan = RT_NULL;
	pchan->phy = RT_NULL;

	rt_spin_unlock_irqrestore(&pdev->phy_lock, flags);
}

static int get_valid_burst_setting(unsigned int max_burst_size)
{
	switch (max_burst_size) {
	case DMA_BURST8:
		return DMA_BLRN_BURST8;
	case DMA_BURST16:
		return DMA_BLRN_BURST16;
	case DMA_BURST32:
		return DMA_BLRN_BURST32;
	case DMA_BURST64:
		return DMA_BLRN_BURST64;
	default:
		return DMA_BURST_INVALID;
	}
}

static int mmp_pdma_config_write(struct rt_dma_chan *dchan,
			   struct rt_dma_slave_config *cfg,
			   enum rt_dma_transfer_direction direction)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);
	rt_uint32_t width = RT_DMA_SLAVE_BUSWIDTH_UNDEFINED;
	rt_int32_t burst = 0;

	if (!dchan)
		return -RT_EINVAL;

	chan->dccr = 0;
	switch (direction) {
	case RT_DMA_DEV_TO_MEM:
		chan->dccr = DMA_CCR_SMODE_FIFO
				| DMA_CCR_DMODE_MEM
				| DMA_CCR_REQ_EN;
		burst = cfg->src_maxburst;
		width = cfg->src_addr_width;
		chan->dev_addr = cfg->src_addr;
		break;

	case RT_DMA_MEM_TO_DEV:
		chan->dccr = DMA_CCR_SMODE_MEM
				| DMA_CCR_DMODE_FIFO
				| DMA_CCR_REQ_EN;
		burst = cfg->dst_maxburst;
		width = cfg->dst_addr_width;
		chan->dev_addr = cfg->dst_addr;
		break;

	case RT_DMA_MEM_TO_MEM:
		chan->dccr = DMA_CCR_SMODE_MEM
				| DMA_CCR_DMODE_MEM;
		burst = cfg->src_maxburst;
		width = cfg->src_addr_width;
		break;

	default:
		return -RT_EINVAL;
	}

	switch (width) {
	case RT_DMA_SLAVE_BUSWIDTH_1_BYTE:
		chan->dccr |= DMA_CCR_SSIZE_8BIT | DMA_CCR_DSIZE_8BIT;
		break;

	case RT_DMA_SLAVE_BUSWIDTH_2_BYTES:
		chan->dccr |= DMA_CCR_SSIZE_16BIT | DMA_CCR_DSIZE_16BIT;
		break;

	case RT_DMA_SLAVE_BUSWIDTH_4_BYTES:
		chan->dccr |= DMA_CCR_SSIZE_32BIT | DMA_CCR_DSIZE_32BIT;
		break;

	default:
		return -RT_EINVAL;

	}

	writel(chan->dccr, chan->phy->base + DMA_CCR(chan->phy->idx));

	burst = get_valid_burst_setting(burst);
	if (burst == DMA_BURST_INVALID)
		return -RT_EINVAL;

	writel(burst, chan->phy->base + DMA_BLR(chan->phy->idx));
	chan->dir = direction;

	return 0;
}

rt_err_t mmp_pdma_prep_memcpy(struct rt_dma_chan *dchan,
		rt_ubase_t dma_dst, rt_ubase_t dma_src, rt_size_t len)
{
	struct mmp_pdma_chan *chan;

	if (!dchan)
		return RT_NULL;

	if (!len)
		return RT_NULL;

	chan = to_mmp_pdma_chan(dchan);

	if (!chan->dir) {
		chan->dir = RT_DMA_MEM_TO_MEM;

		mmp_pdma_config_write(dchan, &chan->slave_config, chan->dir);

		writel(dma_src, chan->phy->base + DMA_SAR(chan->phy->idx));
		writel(dma_dst, chan->phy->base + DMA_DAR(chan->phy->idx));
		writel(len, chan->phy->base + DMA_CNTR(chan->phy->idx));
	}

	return 0;
}

rt_err_t mmp_pdma_prep_single(struct rt_dma_chan *dchan,
	rt_ubase_t dma_buf_addr, rt_size_t buf_len,
	enum rt_dma_transfer_direction dir)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);

	if ((dma_buf_addr == RT_NULL) || (buf_len == 0))
		return RT_NULL;

	mmp_pdma_config_write(dchan, &chan->slave_config, dir);

	if (dir == RT_DMA_DEV_TO_MEM) {
		writel(chan->dev_addr, chan->phy->base + DMA_SAR(chan->phy->idx));
		writel(dma_buf_addr, chan->phy->base + DMA_DAR(chan->phy->idx));

	} else if (dir == RT_DMA_MEM_TO_DEV) {
		writel(dma_buf_addr, chan->phy->base + DMA_SAR(chan->phy->idx));
		writel(chan->dev_addr, chan->phy->base + DMA_DAR(chan->phy->idx));
	}
	writel(buf_len, chan->phy->base + DMA_CNTR(chan->phy->idx));

	chan->dir = dir;

	return 0;
}

rt_err_t mmp_pdma_config(struct rt_dma_chan *dchan, struct rt_dma_slave_config *cfg)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);

	memcpy(&chan->slave_config, cfg, sizeof(*cfg));

	return 0;
}

static rt_err_t mmp_pdma_terminate_all(struct rt_dma_chan *dchan)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);

	if (!dchan)
		return -RT_EINVAL;

	disable_chan(chan->phy);
	chan->status = DMA_COMPLETE;

	return 0;
}

rt_err_t mmp_pdma_stop(struct rt_dma_chan *chan)
{
	mmp_pdma_terminate_all(chan);
	return 0;
}

rt_err_t mmp_pdma_start(struct rt_dma_chan *dchan)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);

	/* still in running*/
	if (chan->status == DMA_IN_PROGRESS) {
		rt_kprintf("DMA controller still running\n");
		return -RT_ERROR;
	}

	/*
	 * start the DMA transaction
	 */
	enable_chan(chan->phy);
	chan->status = DMA_IN_PROGRESS;

	return 0;
}

static int mmp_pdma_chan_init(struct mmp_pdma_device *pdev, int idx)
{
	struct mmp_pdma_phy *phy  = &pdev->phy[idx];
	struct mmp_pdma_chan *chan;

	chan = (struct mmp_pdma_chan *)rt_calloc(1, sizeof(struct mmp_pdma_chan));
	if (chan == RT_NULL)
		return -RT_ENOMEM;

	phy->idx = idx;
	phy->base = pdev->base;
	chan->dev = &pdev->dev;
	chan->device = pdev;
	chan->status = DMA_COMPLETE;

	rt_list_insert_before(&pdev->channels, &chan->chan.device_node);

	return 0;
}

rt_err_t mmp_pdma_release_chan(struct rt_dma_chan *dchan)
{
	struct mmp_pdma_chan *chan = to_mmp_pdma_chan(dchan);

	if (!dchan->used)
		return 0;

	dchan->used = false;
	chan->status = DMA_COMPLETE;
	chan->dir = 0;
	chan->dccr = 0;
	chan->dev_addr = 0;

	mmp_pdma_free_phy(chan);

	return 0;
}

static struct rt_dma_chan *dma_get_any_slave_channel(struct mmp_pdma_device *dev)
{
	struct rt_dma_chan *chan = RT_NULL;
	struct mmp_pdma_chan *pchan;

	rt_list_for_each_entry(chan, &dev->channels, device_node) {
		if (chan->used)
			continue;

		chan->used = true;
		break;
	}

	if (chan) {
		pchan = to_mmp_pdma_chan(chan);
		pchan->status = DMA_COMPLETE;
		pchan->dir = 0;
		pchan->dccr = 0;
		pchan->dev_addr = 0;
		mmp_pdma_free_phy(pchan);
	}

	if (!pchan->phy) {
		pchan->phy = lookup_phy(pchan);
		if (!pchan->phy) {
			rt_kprintf("no free dma channel\n");
			return RT_NULL;
		}
	}
	return chan;
}

struct rt_dma_chan *mmp_pdma_request_chan(struct rt_dma_controller *ctrl,
	struct rt_device *dev, void *fw_data)
{
	struct rt_dma_chan *chan;
	struct fdt_phandle_args *args = fw_data;
	struct mmp_pdma_device *dma_dev = ctrl_to_mmp_pdma_device(ctrl);

	chan = dma_get_any_slave_channel(dma_dev);
	if (!chan)
		return RT_NULL;

	to_mmp_pdma_chan(chan)->dcrs = args->args[0];

	return chan;
}

static struct rt_dma_controller_ops dma_ctrl_ops = {
	.request_chan = mmp_pdma_request_chan,
	.release_chan = mmp_pdma_release_chan,
	.start = mmp_pdma_start,
	.stop = mmp_pdma_stop,
	.config = mmp_pdma_config,
	.prep_memcpy = mmp_pdma_prep_memcpy,
	.prep_single = mmp_pdma_prep_single,
};

static int
mmp_pdma_parse_dt(struct dtb_node *dnode, struct mmp_pdma_device *dev)
{
	int property_size;
	rt_uint32_t value;
	rt_uint32_t *property_ptr;

	dev->dma_channels = DMA_CHANNEL_MAX;
	dev->max_burst_size = DMA_BURST64_MAX;

	for_each_property_cell(dnode, "#dma-channels", value, property_ptr, property_size) {

		dev->dma_channels = value;
	}

	for_each_property_cell(dnode, "max-burst-size", value, property_ptr, property_size) {

		if (get_valid_burst_setting(value) == DMA_BURST_INVALID)
			dev->max_burst_size = DMA_BURST64_MAX;
		else
			dev->max_burst_size = value;

	}

	return 0;
}

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k1x-rdma", "rdma"},
	{}
};

static int spacemit_k1x_dma_probe(void)
{
	int i;
	struct mmp_pdma_device *dma_dev;
	struct rt_dma_controller *ctrl;
	struct dtb_node *dnode;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	int ret = 0;

	for (i = 0; i < ARRAY_SIZE(__compatible); ++i) {
		dnode = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);

		if (dnode != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(dnode))
				continue;

			dma_dev = (struct mmp_pdma_device *)rt_calloc(1, sizeof(struct mmp_pdma_device));
			if (!dma_dev) {
				rt_kprintf("%s:%d, calloc failed\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}
			memset(dma_dev, 0, sizeof(struct mmp_pdma_device));

			dma_dev->dev.node = dnode;

			dma_dev->base = (void *)dtb_node_get_addr_index(dnode, 0);
			if (dma_dev->base < 0) {
				rt_kprintf("get dma base failed\n");
				return -RT_ERROR;
			}

			ret = mmp_pdma_parse_dt(dnode, dma_dev);
			if (ret)
				goto err_out;

			dma_dev->irq = dtb_node_irq_get(dnode, 0);
			if (dma_dev->irq < 0) {
				rt_kprintf("get irq of dma error\n");
				return -RT_ERROR;
			}

			dma_dev->phy = (struct mmp_pdma_phy *)rt_calloc(dma_dev->dma_channels, sizeof(struct mmp_pdma_phy));
			if (!dma_dev->phy) {
				rt_kprintf("%s:%d, calloc failed\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			rt_hw_interrupt_install(dma_dev->irq, mmp_pdma_int_handler, (void *)dma_dev, "rdma-irq");
			rt_hw_interrupt_umask(dma_dev->irq);

			rt_list_init(&dma_dev->channels);
			rt_spin_lock_init(&dma_dev->phy_lock);

			for (i = 0; i < dma_dev->dma_channels; i++) {
				ret = mmp_pdma_chan_init(dma_dev, i);
				if (ret)
					return ret;
			}

			ctrl = &dma_dev->ctrl;
			ctrl->ops = &dma_ctrl_ops;
			ctrl->dev = &dma_dev->dev;

			rt_bitmap_set_bit(ctrl->dir_cap, RT_DMA_MEM_TO_MEM);
			rt_bitmap_set_bit(ctrl->dir_cap, RT_DMA_MEM_TO_DEV);
			rt_bitmap_set_bit(ctrl->dir_cap, RT_DMA_DEV_TO_MEM);

			rt_dma_controller_register(ctrl);

			writel(DMA_DCR_EN, dma_dev->base + DMA_DCR);
		}
	}

err_out:
	return ret;
}
INIT_PREV_EXPORT(spacemit_k1x_dma_probe);
