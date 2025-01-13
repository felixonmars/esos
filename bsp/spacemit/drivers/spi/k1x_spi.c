// SPDX-License-Identifier: GPL-2.0
/*
 * Support for Spacemit k1x spi controller
 *
 * Copyright (c) 2023, spacemit Corporation.
 *
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/spi.h>
#include <ipc/workqueue.h>
#include "k1x_spi.h"

#define TIMEOUT 		100000

#define SSP_DATA_8BIT		8
#define SSP_DATA_16BIT		16
#define SSP_DATA_24BIT		24
#define SSP_DATA_32BIT		32

#define SSP_DATA_8_BIT		(7 << 5)
#define SSP_DATA_16_BIT		(15 << 5)
#define SSP_DATA_24_BIT		(24 << 5)
#define SSP_DATA_32_BIT		(31 << 5)

struct k1x_spi {
	void *base;
	char *name;
	struct rt_spi_bus bus;
	struct clk *clk;
	struct clk *reset;
	unsigned int width;
	unsigned int freq;
	unsigned int mode;
	int flags;
	int n_bytes;
	int (*write)(struct k1x_spi *priv);
	int (*read)(struct k1x_spi *priv);
	int data_length;
	void *tx;
	void *tx_end;
	void *rx;
	void *rx_end;
	int len;
	struct rt_completion	complete;
	struct rt_spi_message *msg;
	int irq;
};

static inline struct k1x_spi *to_k1x_spi(struct rt_spi_bus *bus)
{
	return rt_container_of(bus, struct k1x_spi, bus);
}

static bool k1x_spi_txfifo_full(struct k1x_spi *priv)
{
	return !(readl(priv->base + REG_SSP_STATUS) & BIT_SSP_TNF);
}

int k1x_spi_flush(struct k1x_spi *priv)
{
	unsigned long limit = 1 << 13;

	do {
		while (readl(priv->base + REG_SSP_STATUS) & BIT_SSP_RNE)
			readl(priv->base + REG_SSP_DATAR);
	} while ((readl(priv->base + REG_SSP_STATUS) & BIT_SSP_BSY) && --limit);
	writel(BIT_SSP_ROR, priv->base + REG_SSP_STATUS);

	return limit;
}

static int null_writer(struct k1x_spi *priv)
{
	rt_uint8_t n_bytes = priv->n_bytes;

	if (k1x_spi_txfifo_full(priv)
		|| (priv->tx == priv->tx_end))
		return 0;

	writel(0, priv->base + REG_SSP_DATAR);
	priv->tx += n_bytes;

	return 1;
}

static int null_reader(struct k1x_spi *priv)
{
	rt_uint8_t n_bytes = priv->n_bytes;

	while ((readl(priv->base + REG_SSP_STATUS) & BIT_SSP_RNE)
		&& (priv->rx < priv->rx_end)) {
		readl(priv->base + REG_SSP_DATAR);
		priv->rx += n_bytes;
	}

	return priv->rx == priv->rx_end;
}

static int u8_writer(struct k1x_spi *priv)
{
	if (k1x_spi_txfifo_full(priv)
		|| (priv->tx == priv->tx_end))
		return 0;

	writel(*(rt_uint8_t *)(priv->tx), priv->base + REG_SSP_DATAR);
	++priv->tx;

	return 1;
}

static int u8_reader(struct k1x_spi *priv)
{
	while ((readl(priv->base + REG_SSP_STATUS) & BIT_SSP_RNE)
		&& (priv->rx < priv->rx_end)) {
		*(rt_uint8_t *)(priv->rx) = readl(priv->base + REG_SSP_DATAR);
		++priv->rx;
	}
	return priv->rx == priv->rx_end;
}

static int u16_writer(struct k1x_spi *priv)
{
	if (k1x_spi_txfifo_full(priv)
		|| (priv->tx == priv->tx_end))
		return 0;

	writel(*(rt_uint16_t *)(priv->tx), priv->base + REG_SSP_DATAR);
	priv->tx += 2;

	return 1;
}

static int u16_reader(struct k1x_spi *priv)
{
	while ((readl(priv->base + REG_SSP_STATUS) & BIT_SSP_RNE)
		&& (priv->rx < priv->rx_end)) {
		*(rt_uint16_t *)(priv->rx) = readl(priv->base + REG_SSP_DATAR);
		priv->rx += 2;
	}

	return priv->rx == priv->rx_end;
}

static int u32_writer(struct k1x_spi *priv)
{
	if (k1x_spi_txfifo_full(priv)
		|| (priv->tx == priv->tx_end))
		return 0;

	writel(*(rt_uint32_t *)(priv->tx), priv->base + REG_SSP_DATAR);
	priv->tx += 4;

	return 1;
}

static int u32_reader(struct k1x_spi *priv)
{
	while ((readl(priv->base + REG_SSP_STATUS) & BIT_SSP_RNE)
		&& (priv->rx < priv->rx_end)) {
		*(rt_uint32_t *)(priv->rx) = readl(priv->base + REG_SSP_DATAR);
		priv->rx += 4;
	}

	return priv->rx == priv->rx_end;
}

static void k1x_stop_ssp(struct k1x_spi *priv)
{
	writel(BIT_SSP_ROR | BIT_SSP_TINT, priv->base + REG_SSP_STATUS);
	writel(BITS_SSP_RFT(9) | BITS_SSP_TFT(8), priv->base + REG_SSP_FIFO_CTRL);
	writel(0, priv->base + REG_SSP_TO);

}

rt_uint32_t k1x_spi_pio_xfer(struct k1x_spi *priv)
{
	rt_uint32_t status, count;

	status = readl(priv->base + REG_SSP_STATUS);
	if (priv->tx)
		count = (SSP_SSSR_TFL_MSK >> 7) - ((status & SSP_SSSR_TFL_MSK) >> 7);
	else if (priv->rx)
		count = ((status & SSP_SSSR_RFL_MSK) >> 15);

	do {
		if (priv->read(priv)) {
			k1x_stop_ssp(priv);
			break;
		}
		if (!priv->write(priv))
			break;
		count--;
	} while (count > 0);

	return 0;
}

static int k1x_spi_transfer_config(struct k1x_spi *priv)
{
	rt_uint32_t top_ctrl;
	rt_uint32_t fifo_ctrl;

	priv->n_bytes = 1;

	/* empty read buffer */
	if (k1x_spi_flush(priv) == 0) {
		rt_kprintf("k1x spi flush failed\n");
		return -1;
	}

	top_ctrl = readl(priv->base + REG_SSP_TOP_CTRL);
	fifo_ctrl = readl(priv->base + REG_SSP_FIFO_CTRL);

	priv->width = 0x0;
	switch (priv->data_length) {
	case 16:
		priv->width |= SSP_DATA_16_BIT;
		break;
	case 32:
		priv->width |= SSP_DATA_32_BIT;
		break;
	case 8:
	default:
		priv->width |= SSP_DATA_8_BIT;
	}
	top_ctrl |= priv->width;

	writel(BIT_SSP_ROR | BIT_SSP_TINT, priv->base + REG_SSP_STATUS);
	writel(0xbb8, priv->base + REG_SSP_TO);

	top_ctrl |= BIT_SSP_HOLD_FRAME_LOW;

	top_ctrl &= ~BIT_SSP_SSE;
	writel(top_ctrl, priv->base + REG_SSP_TOP_CTRL);
	writel(fifo_ctrl, priv->base + REG_SSP_FIFO_CTRL);

	top_ctrl |= BIT_SSP_SSE;

	writel(top_ctrl, priv->base + REG_SSP_TOP_CTRL);

	return 0;
}

static rt_uint32_t k1x_spi_transfer_enable(struct k1x_spi *priv)
{
	struct rt_spi_message *msg = priv->msg;
	rt_uint32_t ret = 0;

	priv->len = msg->length >> 3;
	priv->tx = (void *)msg->send_buf;
	priv->tx_end = priv->tx + priv->len;
	priv->rx = msg->recv_buf;
	priv->rx_end = priv->rx + priv->len;
	if (priv->tx) {
		switch (priv->data_length) {
			case 16:
				priv->write = u16_writer;
				break;
			case 32:
				priv->write = u32_writer;
				break;
			case 8:
			default:
				priv->write = u8_writer;
		}
	} else {
		priv->write = null_writer;
	}

	if (priv->rx) {
		switch (priv->data_length) {
			case 16:
				priv->read = u16_reader;
				break;
			case 32:
				priv->read = u32_reader;
				break;
			case 8:
			default:
				priv->read = u8_reader;
		}
	} else {
		priv->read = null_reader;
	}

	writel(BIT_SSP_TIE | BIT_SSP_RIE | BIT_SSP_TINTE, priv->base + REG_SSP_INT_EN);

	return ret;
}

rt_err_t k1x_spi_config(struct rt_spi_device *dev, struct rt_spi_configuration *config)
{
	struct rt_spi_bus *bus = dev->bus;
	struct k1x_spi *priv = to_k1x_spi(bus);

	rt_uint32_t sscr0 = 0;
	rt_uint32_t top_ctrl;

	priv->mode = config->mode;
	priv->data_length = config->data_width;
	if (priv->freq != config->max_hz) {
		priv->freq = config->max_hz;
		clk_set_rate(priv->clk, priv->freq);
	}

	top_ctrl = readl(priv->base + REG_SSP_TOP_CTRL);
	switch (priv->mode) {
	case RT_SPI_MODE_0:
		break;
	case RT_SPI_MODE_1:
		sscr0 |= BIT_SSP_SPH;
		break;
	case RT_SPI_MODE_2:
		sscr0 |= BIT_SSP_SPO;
		break;
	case RT_SPI_MODE_3:
		sscr0 |= BIT_SSP_SPO | BIT_SSP_SPH;
		break;
	default:
		return -1;
	}
	top_ctrl |= sscr0;
	writel(top_ctrl, priv->base + REG_SSP_TOP_CTRL);

	top_ctrl |= BIT_SSP_SSE;
	writel(top_ctrl, priv->base + REG_SSP_TOP_CTRL);

	return 0;
}

static rt_uint32_t k1x_spi_xfer(struct rt_spi_device *dev, struct rt_spi_message *msg)
{
	struct rt_spi_bus *bus = dev->bus;
	struct k1x_spi *priv = to_k1x_spi(bus);
	rt_uint32_t val = 0;
	int ret = 0;

	priv->msg = msg;
	k1x_spi_transfer_config(priv);
	k1x_spi_transfer_enable(priv);

	ret = rt_completion_wait(&priv->complete, RT_WAITING_FOREVER);
		if (ret != 0) {
			rt_kprintf("msg completion timeout\n");
			ret = -RT_ETIMEOUT;
		}

	if (msg->cs_release)
	{
		//release bus
		val = readl(priv->base + REG_SSP_TOP_CTRL);
		val &= ~(BIT_SSP_SSE | BIT_SSP_HOLD_FRAME_LOW);
		writel(val, priv->base + REG_SSP_TOP_CTRL);
	}
	return !ret;
}

static struct rt_spi_ops spacemit_spi_ops = {
	.configure = k1x_spi_config,
	.xfer	= k1x_spi_xfer,
};

static void spacemit_spi_int_handler(int irq, void *devid)
{
	rt_uint32_t int_en, status;
	struct k1x_spi *priv = devid;

	//disable interrupts
	int_en = readl(priv->base + REG_SSP_INT_EN);
	int_en &= ~(BIT_SSP_TIE | BIT_SSP_RIE | BIT_SSP_TINTE);
	writel(int_en, priv->base + REG_SSP_INT_EN);

	status = readl(priv->base + REG_SSP_STATUS);
	if (!status)
		return;

	if (status & BIT_SSP_TINT)
	{
		writel(BIT_SSP_TINT, priv->base + REG_SSP_STATUS);
		return;
	}

	k1x_spi_pio_xfer(priv);

	if ((priv->tx && priv->tx == priv->tx_end)
		|| (priv->rx && priv->rx == priv->rx_end))
	{
		//transmit or receive complete
		rt_completion_done(&priv->complete);
	} else {
		//enable interrupts
		int_en = readl(priv->base + REG_SSP_INT_EN);
		writel(int_en | BIT_SSP_TIE | BIT_SSP_RIE | BIT_SSP_TINTE, priv->base + REG_SSP_INT_EN);
	}
}

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k1x-rspi0", "rspi0"},
	{}
};

static int spacemit_spi_probe(void)
{
	int i;
	struct k1x_spi *spacemit_spi;
	struct rt_spi_bus *bus;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	int property_size;
	rt_uint32_t u32_value;
	void * property_ptr;

	int ret = 0;

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			spacemit_spi = (struct k1x_spi *)rt_calloc(1, sizeof(struct k1x_spi));
			if (!spacemit_spi) {
				rt_kprintf("%s:%d, calloc failed\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}
			memset(spacemit_spi, 0, sizeof(struct k1x_spi));
			bus = &spacemit_spi->bus;
			spacemit_spi->name = (char *)__compatible[i].data;

			spacemit_spi->base = (void *)dtb_node_get_addr_index(compatible_node, 0);
			if (spacemit_spi->base < 0) {
				rt_kprintf("get spi base failed\n");
				return -RT_ERROR;
			}

			for_each_property_cell(compatible_node, "clock-frequency", u32_value, property_ptr, property_size)
			{
				spacemit_spi->freq = u32_value;
			}
			if (!spacemit_spi->freq) {
				rt_kprintf("Please provide clock-frequency!\n");
				return -RT_EINVAL;
			}

			spacemit_spi->clk = of_clk_get(compatible_node, 0);
			if (IS_ERR(spacemit_spi->clk)) {
				rt_kprintf("get spi clk failed\n");
				return -RT_ERROR;
			}
			spacemit_spi->reset = of_clk_get(compatible_node, 1);
			if (IS_ERR(spacemit_spi->reset)) {
				rt_kprintf("get spi reset failed\n");
				return -RT_ERROR;
			}

			clk_set_rate(spacemit_spi->clk, spacemit_spi->freq);
			clk_prepare_enable(spacemit_spi->clk);
			clk_prepare_enable(spacemit_spi->reset);

			rt_completion_init(&spacemit_spi->complete);

			spacemit_spi->irq = dtb_node_irq_get(compatible_node, 0);
			if (spacemit_spi->irq < 0) {
				rt_kprintf("get irq of spi error\n");
				return -RT_ERROR;
			}

			rt_hw_interrupt_install(spacemit_spi->irq, spacemit_spi_int_handler, (void *)spacemit_spi, "rspi0-irq");
			rt_hw_interrupt_umask(spacemit_spi->irq);

			/* current default settings */
			writel(0, spacemit_spi->base + REG_SSP_TOP_CTRL);
			writel(0, spacemit_spi->base + REG_SSP_FIFO_CTRL);
			writel(BITS_SSP_RFT(9) | BITS_SSP_TFT(8), spacemit_spi->base + REG_SSP_FIFO_CTRL);
			writel(SSP_DATA_8_BIT, spacemit_spi->base + REG_SSP_TOP_CTRL);
			spacemit_spi->data_length = SSP_DATA_8BIT;
			writel(0, spacemit_spi->base + REG_SSP_TO);
			writel(0, spacemit_spi->base + REG_SSP_PSP_CTRL);

			rt_spi_bus_register(bus, spacemit_spi->name, &spacemit_spi_ops);
		}
	}
	return ret;
}
INIT_DEVICE_EXPORT(spacemit_spi_probe);
