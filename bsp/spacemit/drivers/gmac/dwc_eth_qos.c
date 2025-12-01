/*
 * Copyright (c) 2025, Spacemit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DesignWare Ethernet QoS driver - RT-Thread port
 * Based on the original Synopsys DWC Ethernet QoS driver
 *
 */

#include <rtthread.h>
#include <rtdef.h>
#include <rtdevice.h>
#include <dtb_node.h>
#include <rthw.h>
#include "dwc_eth_qos.h"

void print_packet(const char *title, const rt_uint8_t *packet, int length)
{
	rt_kprintf("%s (length: %d):\n", title, length);
	for (int i = 0; i < length; i++) {
		rt_kprintf("%02X ", packet[i]);
		if ((i + 1) % 16 == 0)
			rt_kprintf("\n");
	}
	rt_kprintf("\n");
}

static void eqos_powerup(struct eqos_device *eqos)
{
	rt_uint32_t pmt;

	pmt = eqos_readl(eqos, EQOS_PMT);
	pmt &= ~GMAC_CONFIG_POWERDOWN;
	eqos_writel(eqos, EQOS_PMT, pmt);
}

static void eqos_powerdown(struct eqos_device *eqos)
{
	rt_uint32_t pmt;

	pmt = eqos_readl(eqos, EQOS_PMT);
	pmt |= GMAC_CONFIG_POWERDOWN;
	eqos_writel(eqos, EQOS_PMT, pmt);
}

phy_interface_t eqos_get_interface(struct eqos_device *eqos)
{
	struct dtb_node *node = eqos->node;
	const char *phy_mode_str;
	rt_uint32_t i;

	phy_mode_str = dtb_node_read_string(node, "phy-mode");
	if (!phy_mode_str) {
		rt_kprintf("phy-mode not found in dts, defaulting to NA\n");
		return PHY_INTERFACE_MODE_NA;
	}

	for (i = 0; i < PHY_INTERFACE_MODE_MAX; i++)
		if (phy_interface_strings[i] &&
		    !rt_strcmp(phy_interface_strings[i], phy_mode_str)) {
			return (phy_interface_t)i;
		}

	rt_kprintf("Unsupported phy-mode: %s, defaulting to NA\n", phy_mode_str);
	return PHY_INTERFACE_MODE_NA;
}

static void *eqos_alloc_descs(struct eqos_device *eqos, rt_uint32_t num)
{
	rt_size_t total_size;

	eqos->desc_size = RT_ALIGN(sizeof(struct eqos_desc), ARCH_DMA_MINALIGN);
	total_size = num * eqos->desc_size;

	void *buf = rt_malloc_align(total_size, eqos->desc_size);

	if (!buf) {
		rt_kprintf("%s: failed to allocate descriptors\n", eqos->node_name);
		return RT_NULL;
	}

	rt_memset(buf, 0, total_size);

	return buf;
}

static void eqos_free_descs(void *descs)
{
	rt_free_align(descs);
}

static struct eqos_desc *eqos_get_desc(struct eqos_device *eqos,
				       rt_uint32_t num, rt_bool_t rx)
{
	return eqos->descs +
		((rx ? EQOS_DESCRIPTORS_TX : 0) + num) * eqos->desc_size;
}

void eqos_inval_desc_generic(void *desc)
{
	rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, desc,
			     RT_ALIGN(sizeof(struct eqos_desc), ARCH_DMA_MINALIGN));
}

void eqos_flush_desc_generic(void *desc)
{
	rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, desc,
			     RT_ALIGN(sizeof(struct eqos_desc), ARCH_DMA_MINALIGN));
}

void eqos_inval_buffer_generic(void *buf, rt_size_t size)
{
	rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, buf, size);
}

void eqos_flush_buffer_generic(void *buf, rt_size_t size)
{
	rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, buf, size);
}

rt_int32_t wait_for_bit_le32(void *addr, rt_uint32_t mask, rt_bool_t set,
			     rt_int32_t timeout_ms, rt_bool_t sleep)
{
	rt_tick_t timeout = rt_tick_from_millisecond(timeout_ms);
	rt_tick_t start = rt_tick_get();
	rt_uint32_t val;

	while ((rt_tick_get() - start) < timeout) {
		val = readl(addr);
		if (!set) {
			if ((val & mask) == 0)
				return RT_EOK;
		} else {
			if ((val & mask) != 0)
				return RT_EOK;
		}

		if (sleep)
			rt_thread_mdelay(5);
	}

	return -RT_ETIMEOUT;
}

static rt_int32_t eqos_mdio_wait_idle(struct eqos_device *eqos)
{
	return wait_for_bit_le32(&eqos->mac_regs->mdio_address,
				 EQOS_MAC_MDIO_ADDRESS_GB, RT_FALSE,
				 1000, RT_TRUE);
}

static rt_int32_t eqos_mdio_read(struct mii_dev *bus, rt_int32_t mdio_addr,
				 rt_int32_t mdio_devad, rt_int32_t mdio_reg)
{
	struct eqos_device *eqos = (struct eqos_device *)(bus->priv);
	rt_uint32_t val;
	rt_int32_t ret;

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		rt_kprintf("MDIO not idle at entry");
		return ret;
	}

	val = readl(&eqos->mac_regs->mdio_address);
	val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
		EQOS_MAC_MDIO_ADDRESS_C45E;
	val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
		(mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
		(eqos->config->config_mac_mdio <<
		 EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
		(EQOS_MAC_MDIO_ADDRESS_GOC_READ <<
		 EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
		EQOS_MAC_MDIO_ADDRESS_GB;
	writel(val, &eqos->mac_regs->mdio_address);

	rt_thread_mdelay(eqos->config->mdio_wait);

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		rt_kprintf("MDIO read didn't complete");
		return ret;
	}

	val = readl(&eqos->mac_regs->mdio_data);
	val &= EQOS_MAC_MDIO_DATA_GD_MASK;

	return val;
}

static rt_int32_t eqos_mdio_write(struct mii_dev *bus, rt_int32_t mdio_addr, rt_int32_t mdio_devad,
				  rt_int32_t mdio_reg, rt_uint16_t mdio_val)
{
	struct eqos_device *eqos = (struct eqos_device *)(bus->priv);
	rt_uint32_t val;
	rt_int32_t ret;

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		rt_kprintf("MDIO not idle at entry");
		return ret;
	}

	writel(mdio_val, &eqos->mac_regs->mdio_data);

	val = readl(&eqos->mac_regs->mdio_address);
	val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
		EQOS_MAC_MDIO_ADDRESS_C45E;
	val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
		(mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
		(eqos->config->config_mac_mdio <<
		 EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
		(EQOS_MAC_MDIO_ADDRESS_GOC_WRITE <<
		 EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
		EQOS_MAC_MDIO_ADDRESS_GB;
	writel(val, &eqos->mac_regs->mdio_address);

	rt_thread_mdelay(eqos->config->mdio_wait);

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		rt_kprintf("MDIO write didn't complete");
		return ret;
	}

	return RT_EOK;
}

rt_err_t eqos_set_full_duplex(struct eqos_device *eqos)
{
	setbits_le32(&eqos->mac_regs->configuration, EQOS_MAC_CONFIGURATION_DM);

	return RT_EOK;
}

rt_err_t eqos_set_half_duplex(struct eqos_device *eqos)
{
	clrbits_le32(&eqos->mac_regs->configuration, EQOS_MAC_CONFIGURATION_DM);

	/* WAR: Flush TX queue when switching to half-duplex */
	setbits_le32(&eqos->mtl_regs->txq0_operation_mode,
		     EQOS_MTL_TXQ0_OPERATION_MODE_FTQ);

	return RT_EOK;
}

rt_err_t eqos_set_gmii_speed(struct eqos_device *eqos)
{
	clrbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);

	return RT_EOK;
}

rt_err_t eqos_set_mii_speed_100(struct eqos_device *eqos)
{
	setbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);

	return RT_EOK;
}

rt_err_t eqos_set_mii_speed_10(struct eqos_device *eqos)
{
	clrsetbits_le32(&eqos->mac_regs->configuration,
			EQOS_MAC_CONFIGURATION_FES, EQOS_MAC_CONFIGURATION_PS);

	return RT_EOK;
}

static rt_err_t eqos_adjust_link(void *netdev)
{
	struct eqos_device *eqos = (struct eqos_device *)netdev;
	rt_bool_t en_calibration;
	rt_err_t ret;

	if (!eqos->phy->link) {
#ifdef RT_USING_ETHERCAT
		ecdev_set_link(eqos->ecdev, 0);
#endif
		rt_kprintf("No link\n");
		return RT_EOK;
	}

	if (eqos->phy->duplex)
		ret = eqos_set_full_duplex(eqos);
	else
		ret = eqos_set_half_duplex(eqos);
	if (ret < 0) {
		rt_kprintf("eqos_set_*_duplex() failed: %d", ret);
		return ret;
	}

	switch (eqos->phy->speed) {
	case SPEED_1000:
		en_calibration = RT_TRUE;
		ret = eqos_set_gmii_speed(eqos);
		break;
	case SPEED_100:
		en_calibration = RT_TRUE;
		ret = eqos_set_mii_speed_100(eqos);
		break;
	case SPEED_10:
		en_calibration = RT_FALSE;
		ret = eqos_set_mii_speed_10(eqos);
		break;
	default:
		rt_kprintf("invalid speed %d", eqos->phy->speed);
		return -RT_EINVAL;
	}
	if (ret < 0) {
		rt_kprintf("eqos_set_*mii_speed*() failed: %d", ret);
		return ret;
	}

	if (en_calibration) {
		ret = eqos->config->ops->eqos_calibrate_pads(eqos);
		if (ret < 0) {
			rt_kprintf("eqos_calibrate_pads() failed: %d",
			       ret);
			return ret;
		}
	} else {
		ret = eqos->config->ops->eqos_disable_calibration(eqos);
		if (ret < 0) {
			rt_kprintf("eqos_disable_calibration() failed: %d",
			       ret);
			return ret;
		}
	}
	ret = eqos->config->ops->eqos_set_tx_clk_speed(eqos);
	if (ret < 0) {
		rt_kprintf("eqos_set_tx_clk_speed() failed: %d", ret);
		return ret;
	}

	rt_kprintf("%s: link is Up - ", eqos->name);
	rt_kprintf("%dMbps/", eqos->phy->speed);
	rt_kprintf("%s\n", (eqos->phy->duplex == 1) ? "full" : "half");
#ifdef RT_USING_ETHERCAT
	ecdev_set_link(eqos->ecdev, 1);
#endif

	return RT_EOK;
}

static rt_err_t eqos_write_hwaddr(struct eqos_device *eqos)
{
	struct eth_pdata *plat = eqos->plat_data;
	uint32_t val;

	/*
	 * This function may be called before start() or after stop(). At that
	 * time, on at least some configurations of the EQoS HW, all clocks to
	 * the EQoS HW block will be stopped, and a reset signal applied. If
	 * any register access is attempted in this state, bus timeouts or CPU
	 * hangs may occur. This check prevents that.
	 *
	 * A simple solution to this problem would be to not implement
	 * write_hwaddr(), since start() always writes the MAC address into HW
	 * anyway. However, it is desirable to implement write_hwaddr() to
	 * support the case of SW that runs subsequent to U-Boot which expects
	 * the MAC address to already be programmed into the EQoS registers,
	 * which must happen irrespective of whether the U-Boot user (or
	 * scripts) actually made use of the EQoS device, and hence
	 * irrespective of whether start() was ever called.
	 *
	 * Note that this requirement by subsequent SW is not valid for
	 * Tegra186, and is likely not valid for any non-PCI instantiation of
	 * the EQoS HW block. This function is implemented solely as
	 * future-proofing with the expectation the driver will eventually be
	 * ported to some system where the expectation above is RT_TRUE.
	 */
	if (!eqos->config->reg_access_always_ok && !eqos->reg_access_ok)
		return RT_EOK;

	/* Update the MAC address */
	val = (plat->enetaddr[5] << 8) |
		(plat->enetaddr[4]);
	writel(val, &eqos->mac_regs->address0_high);

	val = (plat->enetaddr[3] << 24) |
		(plat->enetaddr[2] << 16) |
		(plat->enetaddr[1] << 8) |
		(plat->enetaddr[0]);
	writel(val, &eqos->mac_regs->address0_low);

	return RT_EOK;
}

static rt_int32_t eqos_get_phy_addr(struct eqos_device *eqos)
{
	const struct dtb_node *phy_node;
	rt_uint32_t phandle;
	rt_int32_t reg;

	if (!dtb_node_read_u32_array(eqos->node, "phy-handle", &phandle, 1)) {
		phy_node = dtb_node_get_by_phandle(phandle);
		if (!phy_node) {
			rt_kprintf("%s: failed to get phy node by phandle\n", eqos->node_name);
			return -RT_EEMPTY;
		}

		eqos->phy_of_node = phy_node;

		if (dtb_node_read_u32_array(phy_node, "reg", &reg, 1)) {
			rt_kprintf("%s: failed to read reg from phy node\n", eqos->node_name);
			return -RT_EEMPTY;
		}

		return reg;
	}

	rt_kprintf("%s: no phy-handle found in dts\n", eqos->node_name);
	return -RT_EEMPTY;
}

static rt_err_t eqos_start(struct eqos_device *eqos)
{
	rt_err_t ret;
	rt_int32_t i;
	rt_uint64_t rate;
	rt_uint32_t val, tx_fifo_sz, rx_fifo_sz, tqs, rqs, pbl;
	rt_uint32_t last_rx_desc;
	rt_uint32_t desc_pad;

	if (eqos->started)
		return RT_EOK;

	eqos->tx_desc_idx = 0;
	eqos->rx_desc_idx = 0;

	ret = eqos->config->ops->eqos_start_resets(eqos);
	if (ret < 0) {
		rt_kprintf("%s: eqos_start_resets() failed\n", eqos->name);
		goto err;
	}
	rt_thread_mdelay(2);

	eqos_powerup(eqos);

	eqos->reg_access_ok = RT_TRUE;

	ret = wait_for_bit_le32(&eqos->dma_regs->mode,
				EQOS_DMA_MODE_SWR, RT_FALSE,
				eqos->config->swr_wait, RT_FALSE);
	if (ret) {
		rt_kprintf("%s: EQOS_DMA_MODE_SWR stuck\n", eqos->name);
		goto err_stop_resets;
	}

	ret = eqos->config->ops->eqos_calibrate_pads(eqos);
	if (ret < 0) {
		rt_kprintf("%s: eqos_calibrate_pads() failed\n", eqos->name);
		goto err_stop_resets;
	}
	rate = eqos->config->ops->eqos_get_tick_clk_rate(eqos);

	val = (rate / 1000000) - 1;
	writel(val, &eqos->mac_regs->us_tic_counter);

	/*
	 * if PHY was already connected and configured,
	 * don't need to reconnect/reconfigure again
	 */
	if (!eqos->phy) {
		rt_int32_t addr = -1;

		addr = eqos_get_phy_addr(eqos);
		eqos->phy = phy_connect(eqos->mii, addr, eqos,
					eqos->config->get_interface(eqos));
		if (!eqos->phy) {
			rt_kprintf("%s: phy_connect() failed\n", eqos->name);
			goto err_stop_resets;
		}

		if (eqos->max_speed) {
			ret = phy_set_supported(eqos->phy, eqos->max_speed);
			if (ret) {
				rt_kprintf("%s: phy_set_supported() failed\n", eqos->name);
				goto err_shutdown_phy;
			}
		}

		eqos->phy->node = eqos->phy_of_node;
		ret = phy_config(eqos->phy);
		if (ret < 0) {
			rt_kprintf("%s: phy_config() failed", eqos->name);
			goto err_shutdown_phy;
		}
	}

	ret = phy_startup(eqos->phy);
	if (ret < 0 && ret != -RT_ETIMEOUT) {
		rt_kprintf("%s: phy_startup() failed\n", eqos->name);
		goto err_shutdown_phy;
	}

	phy_set_link_notify(eqos->phy, eqos_adjust_link);

	ret = eqos_adjust_link(eqos);
	if (ret < 0) {
		rt_kprintf("%s: eqos_adjust_link() failed\n", eqos->name);
		goto err_shutdown_phy;
	}

	/* Configure MTL */

	/* Enable Store and Forward mode for TX */
	/* Program Tx operating mode */
	setbits_le32(&eqos->mtl_regs->txq0_operation_mode,
		     EQOS_MTL_TXQ0_OPERATION_MODE_TSF |
		     (EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED <<
		      EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT));

	/* Transmit Queue weight */
	writel(0x10, &eqos->mtl_regs->txq0_quantum_weight);

	/* Enable Store and Forward mode for RX, since no jumbo frame */
	setbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
		     EQOS_MTL_RXQ0_OPERATION_MODE_RSF);

	/* Transmit/Receive queue fifo size; use all RAM for 1 queue */
	val = readl(&eqos->mac_regs->hw_feature1);
	tx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT) &
		EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK;
	rx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT) &
		EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK;

	/*
	 * r/tx_fifo_sz is encoded as log2(n / 128). Undo that by shifting.
	 * r/tqs is encoded as (n / 256) - 1.
	 */
	tqs = (128 << tx_fifo_sz) / 256 - 1;
	rqs = (128 << rx_fifo_sz) / 256 - 1;

	clrsetbits_le32(&eqos->mtl_regs->txq0_operation_mode,
			EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK <<
			EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT,
			tqs << EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT);
	clrsetbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
			EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK <<
			EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT,
			rqs << EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT);

	/* Flow control used only if each channel gets 4KB or more FIFO */
	if (rqs >= ((4096 / 256) - 1)) {
		rt_uint32_t rfd, rfa;

		setbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
			     EQOS_MTL_RXQ0_OPERATION_MODE_EHFC);

		/*
		 * Set Threshold for Activating Flow Contol space for min 2
		 * frames ie, (1500 * 1) = 1500 bytes.
		 *
		 * Set Threshold for Deactivating Flow Contol for space of
		 * min 1 frame (frame size 1500bytes) in receive fifo
		 */
		if (rqs == ((4096 / 256) - 1)) {
			/*
			 * This violates the above formula because of FIFO size
			 * limit therefore overflow may occur inspite of this.
			 */
			rfd = 0x3;	/* Full-3K */
			rfa = 0x1;	/* Full-1.5K */
		} else if (rqs == ((8192 / 256) - 1)) {
			rfd = 0x6;	/* Full-4K */
			rfa = 0xa;	/* Full-6K */
		} else if (rqs == ((16384 / 256) - 1)) {
			rfd = 0x6;	/* Full-4K */
			rfa = 0x12;	/* Full-10K */
		} else {
			rfd = 0x6;	/* Full-4K */
			rfa = 0x1E;	/* Full-16K */
		}

		clrsetbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
				(EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
				(EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT),
				(rfd <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
				(rfa <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT));
	}

	/* Configure MAC */
	clrsetbits_le32(&eqos->mac_regs->rxq_ctrl0,
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_MASK <<
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT,
			eqos->config->config_mac <<
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT);

	/* Multicast and Broadcast Queue Enable */
	setbits_le32(&eqos->mac_regs->unused_0a4,
		     0x00100000);
	/* enable promise mode */
	setbits_le32(&eqos->mac_regs->unused_004[1],
		     0x1);

	/* Set TX flow control parameters */
	/* Set Pause Time */
	setbits_le32(&eqos->mac_regs->q0_tx_flow_ctrl,
		     0xffff << EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);
	/* Assign priority for TX flow control */
	clrbits_le32(&eqos->mac_regs->txq_prty_map0,
		     EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK <<
		     EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT);
	/* Assign priority for RX flow control */
	clrbits_le32(&eqos->mac_regs->rxq_ctrl2,
		     EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK <<
		     EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT);
	/* Enable flow control */
	setbits_le32(&eqos->mac_regs->q0_tx_flow_ctrl,
		     EQOS_MAC_Q0_TX_FLOW_CTRL_TFE);
	setbits_le32(&eqos->mac_regs->rx_flow_ctrl,
		     EQOS_MAC_RX_FLOW_CTRL_RFE);

	clrsetbits_le32(&eqos->mac_regs->configuration,
			EQOS_MAC_CONFIGURATION_GPSLCE |
			EQOS_MAC_CONFIGURATION_WD |
			EQOS_MAC_CONFIGURATION_JD |
			EQOS_MAC_CONFIGURATION_JE,
			EQOS_MAC_CONFIGURATION_CST |
			EQOS_MAC_CONFIGURATION_ACS);

	/* Configure DMA */

	/* Enable OSP mode */
	setbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_OSP);

	/* RX buffer size. Must be a multiple of bus width */
	clrsetbits_le32(&eqos->dma_regs->ch0_rx_control,
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_MASK <<
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT,
			EQOS_MAX_PACKET_SIZE <<
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT);

	desc_pad = (eqos->desc_size - sizeof(struct eqos_desc)) /
		   eqos->config->axi_bus_width;

	setbits_le32(&eqos->dma_regs->ch0_control,
		     EQOS_DMA_CH0_CONTROL_PBLX8 |
		     (desc_pad << EQOS_DMA_CH0_CONTROL_DSL_SHIFT));

	/*
	 * Burst length must be < 1/2 FIFO size.
	 * FIFO size in tqs is encoded as (n / 256) - 1.
	 * Each burst is n * 8 (PBLX8) * 16 (AXI width) == 128 bytes.
	 * Half of n * 256 is n * 128, so pbl == tqs, modulo the -1.
	 */
	pbl = tqs + 1;
	if (pbl > 32)
		pbl = 32;
	clrsetbits_le32(&eqos->dma_regs->ch0_tx_control,
			EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK <<
			EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT,
			pbl << EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT);

	clrsetbits_le32(&eqos->dma_regs->ch0_rx_control,
			EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK <<
			EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT,
			8 << EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);

	/* DMA performance configuration */
	val = (2 << EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
		EQOS_DMA_SYSBUS_MODE_EAME | EQOS_DMA_SYSBUS_MODE_BLEN16 |
		EQOS_DMA_SYSBUS_MODE_BLEN8 | EQOS_DMA_SYSBUS_MODE_BLEN4;
	writel(val, &eqos->dma_regs->sysbus_mode);

	/* Set up descriptors */

	rt_memset(eqos->descs, 0, eqos->desc_size * EQOS_DESCRIPTORS_NUM);

	for (i = 0; i < EQOS_DESCRIPTORS_TX; i++) {
		struct eqos_desc *tx_desc = eqos_get_desc(eqos, i, RT_FALSE);

		eqos->config->ops->eqos_flush_desc(tx_desc);
	}

	for (i = 0; i < EQOS_DESCRIPTORS_RX; i++) {
		struct eqos_desc *rx_desc = eqos_get_desc(eqos, i, RT_TRUE);

		rx_desc->des0 = lower_32_bits((uintptr_t)(eqos->rx_dma_buf +
					     (i * EQOS_MAX_PACKET_SIZE)));
		rx_desc->des1 = upper_32_bits((uintptr_t)(eqos->rx_dma_buf +
					     (i * EQOS_MAX_PACKET_SIZE)));
		rx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_BUF1V | BIT(30);
		eqos_mb();
		eqos->config->ops->eqos_flush_desc(rx_desc);
		eqos->config->ops->eqos_inval_buffer(eqos->rx_dma_buf +
						(i * EQOS_MAX_PACKET_SIZE),
						EQOS_MAX_PACKET_SIZE);
	}

	val = upper_32_bits((uintptr_t)eqos_get_desc(eqos, 0, RT_FALSE));
	writel(val, &eqos->dma_regs->ch0_txdesc_list_haddress);
	writel((uintptr_t)eqos_get_desc(eqos, 0, RT_FALSE),
		&eqos->dma_regs->ch0_txdesc_list_address);
	writel(EQOS_DESCRIPTORS_TX - 1,
	       &eqos->dma_regs->ch0_txdesc_ring_length);

	val = upper_32_bits((uintptr_t)eqos_get_desc(eqos, 0, RT_TRUE));
	writel(val, &eqos->dma_regs->ch0_rxdesc_list_haddress);
	writel((uintptr_t)eqos_get_desc(eqos, 0, RT_TRUE),
		&eqos->dma_regs->ch0_rxdesc_list_address);
	writel(EQOS_DESCRIPTORS_RX - 1,
	       &eqos->dma_regs->ch0_rxdesc_ring_length);

	/* Enable everything */
	setbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_ST);
	setbits_le32(&eqos->dma_regs->ch0_rx_control,
		     EQOS_DMA_CH0_RX_CONTROL_SR);
	setbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_TE | EQOS_MAC_CONFIGURATION_RE);

	eqos_write_hwaddr(eqos);
	/* TX tail pointer not written until we need to TX a packet */
	/*
	 * Point RX tail pointer at last descriptor. Ideally, we'd point at the
	 * first descriptor, implying all descriptors were available. However,
	 * that's not distinguishable from none of the descriptors being
	 * available.
	 */
	last_rx_desc = (uintptr_t)eqos_get_desc(eqos, EQOS_DESCRIPTORS_RX - 1, RT_TRUE);
	writel(last_rx_desc, &eqos->dma_regs->ch0_rxdesc_tail_pointer);

	eqos->started = RT_TRUE;

	return 0;

err_shutdown_phy:
	phy_shutdown(eqos->phy);
	eqos->phy = RT_NULL;
err_stop_resets:
	eqos->config->ops->eqos_stop_resets(eqos);
err:
	return ret;
}

static rt_err_t eqos_stop(struct eqos_device *eqos)
{
	rt_int32_t i;

	if (!eqos->started)
		return RT_EOK;
	eqos->started = RT_FALSE;
	eqos->reg_access_ok = RT_FALSE;

	/* Disable TX DMA */
	clrbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_ST);

	/* Wait for TX all packets to drain out of MTL */
	for (i = 0; i < 1000000; i++) {
		rt_uint32_t val = readl(&eqos->mtl_regs->txq0_debug);
		rt_uint32_t trcsts = (val >> EQOS_MTL_TXQ0_DEBUG_TRCSTS_SHIFT) &
			EQOS_MTL_TXQ0_DEBUG_TRCSTS_MASK;
		rt_uint32_t txqsts = val & EQOS_MTL_TXQ0_DEBUG_TXQSTS;

		if ((trcsts != 1) && (!txqsts))
			break;
	}

	/* Turn off MAC TX and RX */
	clrbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_TE | EQOS_MAC_CONFIGURATION_RE);

	/* Wait for all RX packets to drain out of MTL */
	for (i = 0; i < 1000000; i++) {
		rt_uint32_t val = readl(&eqos->mtl_regs->rxq0_debug);
		rt_uint32_t prxq = (val >> EQOS_MTL_RXQ0_DEBUG_PRXQ_SHIFT) &
			EQOS_MTL_RXQ0_DEBUG_PRXQ_MASK;
		rt_uint32_t rxqsts = (val >> EQOS_MTL_RXQ0_DEBUG_RXQSTS_SHIFT) &
			EQOS_MTL_RXQ0_DEBUG_RXQSTS_MASK;
		if ((!prxq) && (!rxqsts))
			break;
	}

	/* Turn off RX DMA */
	clrbits_le32(&eqos->dma_regs->ch0_rx_control,
		     EQOS_DMA_CH0_RX_CONTROL_SR);

	if (eqos->phy) {
		phy_shutdown(eqos->phy);
		eqos->phy = RT_NULL;
	}

	eqos_powerdown(eqos);

#ifdef RT_USING_ETHERCAT
	ecdev_set_link(eqos->ecdev, 0);
#endif

	eqos->config->ops->eqos_stop_resets(eqos);

	return RT_EOK;
}

static rt_err_t eqos_send_native(struct eqos_device *eqos, void *packet, rt_int32_t length)
{
	struct eqos_desc *tx_desc;
	rt_int32_t i;

	eqos->config->ops->eqos_flush_buffer(packet, length);

	tx_desc = eqos_get_desc(eqos, eqos->tx_desc_idx, RT_FALSE);
	eqos->config->ops->eqos_inval_desc(tx_desc);
	if ((readl(&tx_desc->des3) & EQOS_DESC3_OWN))
		return -RT_EBUSY;

	eqos->tx_desc_idx++;
	eqos->tx_desc_idx %= EQOS_DESCRIPTORS_TX;

	tx_desc->des0 = lower_32_bits((uintptr_t)packet);
	tx_desc->des1 = upper_32_bits((uintptr_t)packet);
	tx_desc->des2 = length;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	eqos_mb();

	tx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_FD | EQOS_DESC3_LD | length;
	eqos->config->ops->eqos_flush_desc(tx_desc);

	writel((uintptr_t)eqos_get_desc(eqos, eqos->tx_desc_idx, RT_FALSE),
		&eqos->dma_regs->ch0_txdesc_tail_pointer);

	return RT_EOK;
}

static rt_err_t eqos_send(struct eqos_device *eqos, void *packet, rt_size_t length)
{
	struct eqos_desc *tx_desc;
	rt_int32_t i;

	rt_memcpy(eqos->tx_dma_buf, packet, length);

	eqos->config->ops->eqos_flush_buffer(eqos->tx_dma_buf, length);

	tx_desc = eqos_get_desc(eqos, eqos->tx_desc_idx, RT_FALSE);
	eqos->tx_desc_idx++;
	eqos->tx_desc_idx %= EQOS_DESCRIPTORS_TX;

	tx_desc->des0 = lower_32_bits((uintptr_t)eqos->tx_dma_buf);
	tx_desc->des1 = upper_32_bits((uintptr_t)eqos->tx_dma_buf);
	tx_desc->des2 = length;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	eqos_mb();

	tx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_FD | EQOS_DESC3_LD | length;
	eqos->config->ops->eqos_flush_desc(tx_desc);

	writel((uintptr_t)eqos_get_desc(eqos, eqos->tx_desc_idx, RT_FALSE),
		&eqos->dma_regs->ch0_txdesc_tail_pointer);

	/**
	 * Waits up to 2 ms to allow hardware sufficient time to complete the transmission.
	 * Maximum TX delay for a 1518-byte packet at 10 Mbps is approximately 1.2 ms.
	 */
	for (i = 0; i < 2; i++) {
		eqos->config->ops->eqos_inval_desc(tx_desc);
		if (!(readl(&tx_desc->des3) & EQOS_DESC3_OWN))
			return RT_EOK;
		rt_thread_mdelay(1);
	}

	rt_kprintf("%s: send timeout\n", eqos->name);

	return -RT_ETIMEOUT;
}

static rt_int32_t eqos_recv(struct eqos_device *eqos, rt_int32_t flags, rt_uint8_t **packetp)
{
	struct eqos_desc *rx_desc;
	rt_int32_t length;

	rx_desc = eqos_get_desc(eqos, eqos->rx_desc_idx, RT_TRUE);
	eqos->config->ops->eqos_inval_desc(rx_desc);
	if (rx_desc->des3 & EQOS_DESC3_OWN)
		return -RT_EBUSY;

	*packetp = eqos->rx_dma_buf +
		(eqos->rx_desc_idx * EQOS_MAX_PACKET_SIZE);
	length = rx_desc->des3 & 0x7fff;

	eqos->config->ops->eqos_inval_buffer(*packetp, length);

	return length;
}

static rt_err_t eqos_free_pkt(struct eqos_device *eqos, rt_uint8_t *packet, rt_int32_t length)
{
	rt_uint8_t *packet_expected;
	struct eqos_desc *rx_desc;

	packet_expected = eqos->rx_dma_buf +
		(eqos->rx_desc_idx * EQOS_MAX_PACKET_SIZE);
	if (packet != packet_expected) {
		rt_kprintf("%s: unexpected packet\n", eqos->name);
		return -RT_ERROR;
	}

	eqos->config->ops->eqos_inval_buffer(packet, length);

	rx_desc = eqos_get_desc(eqos, eqos->rx_desc_idx, RT_TRUE);

	rx_desc->des0 = 0;
	eqos_mb();
	eqos->config->ops->eqos_flush_desc(rx_desc);
	eqos->config->ops->eqos_inval_buffer(packet, length);
	rx_desc->des0 = lower_32_bits((uintptr_t)packet);
	rx_desc->des1 = upper_32_bits((uintptr_t)packet);
	rx_desc->des2 = 0;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	eqos_mb();
	rx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_BUF1V;
	eqos->config->ops->eqos_flush_desc(rx_desc);

	writel((uintptr_t)rx_desc, &eqos->dma_regs->ch0_rxdesc_tail_pointer);

	eqos->rx_desc_idx++;
	eqos->rx_desc_idx %= EQOS_DESCRIPTORS_RX;

	return 0;
}

static rt_err_t eqos_get_hwaddr(struct eqos_device *eqos, rt_uint8_t *mac)
{
	struct eth_pdata *plat = eqos->plat_data;
	rt_err_t ret;

	ret = eqos->config->ops->eqos_get_enetaddr(eqos);
	if (ret < 0)
		return ret;

	if (!is_valid_ethaddr(plat->enetaddr))
		return -RT_ERROR;

	rt_memcpy(mac, plat->enetaddr, ETH_ALEN);

	return RT_EOK;
}

static rt_err_t eqos_set_hwaddr(struct eqos_device *eqos, const rt_uint8_t *mac)
{
	struct eth_pdata *plat = eqos->plat_data;

	if (eqos->started)
		return RT_EBUSY;

	rt_memcpy(plat->enetaddr, mac, ETH_ALEN);

	return RT_EOK;
}

rt_err_t eqos_set_mac_loopback(struct eqos_device *eqos, rt_bool_t enable)
{
	rt_uint32_t val = eqos_readl(eqos, EQOS_MAC_REGS_BASE);

	if (!eqos->started)
		return -RT_EINVAL;

	if (enable)
		val |= GMAC_CONFIG_LM;
	else
		val &= ~GMAC_CONFIG_LM;

	eqos_writel(eqos, EQOS_MAC_REGS_BASE, val);

	return RT_EOK;
}

rt_err_t eqos_set_phy_loopback(struct eqos_device *eqos, rt_bool_t enable)
{
	if (!eqos->started || !eqos->phy)
		return -RT_EINVAL;

	phy_loopback(eqos->phy, enable);

	return RT_EOK;
}

static rt_err_t eqos_probe_resources_core(struct eqos_device *eqos)
{
	rt_err_t ret;

	eqos->descs = eqos_alloc_descs(eqos, EQOS_DESCRIPTORS_NUM);
	if (!eqos->descs) {
		rt_kprintf("%s: eqos_alloc_descs() failed\n", eqos->node_name);
		ret = -RT_ENOMEM;
		goto err;
	}

	eqos->tx_dma_buf = rt_malloc_align(EQOS_MAX_PACKET_SIZE, EQOS_BUFFER_ALIGN);
	if (!eqos->tx_dma_buf) {
		rt_kprintf("%s: rt_malloc_align(tx_dma_buf) failed\n", eqos->node_name);
		ret = -RT_ENOMEM;
		goto err_free_descs;
	}

	eqos->rx_dma_buf = rt_malloc_align(EQOS_RX_BUFFER_SIZE, EQOS_BUFFER_ALIGN);
	if (!eqos->rx_dma_buf) {
		rt_kprintf("%s: rt_malloc_align(rx_dma_buf) failed\n", eqos->node_name);
		ret = -RT_ENOMEM;
		goto err_free_tx_dma_buf;
	}

	eqos->rx_pkt = rt_malloc_align(EQOS_MAX_PACKET_SIZE, EQOS_BUFFER_ALIGN);
	if (!eqos->rx_pkt) {
		rt_kprintf("%s: rt_malloc_align(rx_pkt) failed\n", eqos->node_name);
		ret = -RT_ENOMEM;
		goto err_free_rx_dma_buf;
	}

	if (eqos->config && eqos->config->ops && eqos->config->ops->eqos_inval_buffer) {
		eqos->config->ops->eqos_inval_buffer(eqos->rx_dma_buf,
						EQOS_MAX_PACKET_SIZE * EQOS_DESCRIPTORS_RX);
	}

	return RT_EOK;

err_free_rx_dma_buf:
	rt_free_align(eqos->rx_dma_buf);
	eqos->rx_dma_buf = RT_NULL;

err_free_tx_dma_buf:
	rt_free_align(eqos->tx_dma_buf);
	eqos->tx_dma_buf = RT_NULL;

err_free_descs:
	eqos_free_descs(eqos->descs);
	eqos->descs = RT_NULL;

err:
	return ret;
}

static rt_err_t eqos_remove_resources_core(struct eqos_device *eqos)
{
	if (eqos->rx_pkt) {
		rt_free_align(eqos->rx_pkt);
		eqos->rx_pkt = RT_NULL;
	}

	if (eqos->rx_dma_buf) {
		rt_free_align(eqos->rx_dma_buf);
		eqos->rx_dma_buf = RT_NULL;
	}

	if (eqos->tx_dma_buf) {
		rt_free_align(eqos->tx_dma_buf);
		eqos->tx_dma_buf = RT_NULL;
	}

	if (eqos->descs) {
		eqos_free_descs(eqos->descs);
		eqos->descs = RT_NULL;
	}

	return RT_EOK;
}

#ifdef RT_USING_ETHERCAT
static rt_err_t ec_eqos_open(struct mini_netdev *netdev)
{
	struct eqos_device *eqos = netdev->user_data;
	rt_err_t ret;

	ret = eqos_start(eqos);

	return ret;
}

static rt_err_t ec_eqos_close(struct mini_netdev *netdev)
{
	struct eqos_device *eqos = netdev->user_data;

	ecdev_close(eqos->ecdev);
	eqos_stop(eqos);

	return RT_EOK;
}

static rt_err_t ec_eqos_send(struct mini_netdev *netdev, void *buffer, rt_size_t size)
{
	struct eqos_device *eqos = netdev->user_data;
	rt_err_t ret;

	if (size > EQOS_MAX_PACKET_SIZE)
		return -RT_EINVAL;

	ret = eqos_send_native(eqos, buffer, size);

	return ret;
}

static rt_int32_t ec_eqos_recv(struct mini_netdev *netdev, void *buffer, rt_size_t size)
{
	struct eqos_device *eqos = netdev->user_data;
	rt_uint8_t *pkt;
	rt_int32_t len, ret;

	ret = eqos_recv(eqos, 0, &pkt);
	if (ret == -RT_EBUSY)
		return ret;

	len = (ret > size) ? size : ret;
	rt_memcpy(buffer, pkt, len);
	eqos_free_pkt(eqos, pkt, ret);

	return ret;
}

void eqos_ec_poll(struct mini_netdev *netdev)
{
	struct eqos_device *eqos = netdev->user_data;
	rt_uint8_t *pkt;
	rt_int32_t len;

	for (int i = 0; i < 128; ++i) {
		len = eqos_recv(eqos, 0, &pkt);
		if (len == -RT_EBUSY)
			continue;
		if (len > 0 && len < ETH_ZLEN) {
			eqos_free_pkt(eqos, pkt, len);
			break;
		}
		ecdev_receive(eqos->ecdev, pkt, (size_t)len);
		eqos_free_pkt(eqos, pkt, len);
	}
}
#else
static rt_err_t rt_eqos_open(rt_device_t dev, rt_uint16_t oflag)
{
	struct eqos_device *eqos = (struct eqos_device *)dev->user_data;

	return eqos_start(eqos);
}

static rt_err_t rt_eqos_close(rt_device_t dev)
{
	struct eqos_device *eqos = (struct eqos_device *)dev->user_data;

	return eqos_stop(eqos);
}

static rt_size_t rt_eqos_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	struct eqos_device *eqos = (struct eqos_device *)dev->user_data;
	rt_uint8_t *pkt = RT_NULL;
	rt_int32_t len, ret;

	ret = eqos_recv(eqos, 0, &pkt);
	if (ret == -RT_EBUSY)
		return 0;

	len = (ret > size) ? size : ret;
	rt_memcpy(buffer, pkt, len);
	eqos_free_pkt(eqos, pkt, ret);

	return len;
}

static rt_size_t rt_eqos_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
	struct eqos_device *eqos = (struct eqos_device *)dev->user_data;

	if (size > EQOS_MAX_PACKET_SIZE)
		return 0;
	if (eqos_send_native(eqos, (void *)buffer, size) != RT_EOK)
		return 0;
	return size;
}

static rt_err_t rt_eqos_control(rt_device_t dev, int cmd, void *args)
{
	struct eqos_device *eqos = dev->user_data;
	rt_err_t ret = -RT_EINVAL;

	switch (cmd) {
	case CMD_SET_MAC:
		ret = eqos_set_hwaddr(eqos, args);
		break;
	case CMD_GET_MAC:
		ret = eqos_get_hwaddr(eqos, args);
		break;
	case CMD_ENABLE_MAC_LOOPBACK:
		ret = eqos_set_mac_loopback(eqos, RT_TRUE);
		break;
	case CMD_DISABLE_MAC_LOOPBACK:
		ret = eqos_set_mac_loopback(eqos, RT_FALSE);
		break;
	case CMD_ENABLE_PHY_LOOPBACK:
		ret = eqos_set_phy_loopback(eqos, RT_TRUE);
		break;
	case CMD_DISABLE_PHY_LOOPBACK:
		ret = eqos_set_phy_loopback(eqos, RT_FALSE);
		break;
	default:
		ret = -RT_EINVAL;
		break;
	}

	return ret;
}
#endif

static rt_err_t eqos_parent_init(struct eqos_device *eqos)
{
	rt_err_t ret = RT_EOK;

	if (!eqos)
		return -RT_EINVAL;
#ifdef RT_USING_ETHERCAT
	eqos->parent = rt_calloc(1, sizeof(struct mini_netdev));
	if (!eqos->parent) {
		rt_kprintf("%s: failed to allocate mini_netdev\n", eqos->node_name);
		return -RT_ENOMEM;
	}

	eqos->parent->open    = ec_eqos_open;
	eqos->parent->close   = ec_eqos_close;
	eqos->parent->send    = ec_eqos_send;
	eqos->parent->recv    = ec_eqos_recv;

	eqos->parent->user_data = eqos;

	rt_memcpy(eqos->parent->mac_addr, eqos->plat_data->enetaddr, ETH_ALEN);

	eqos->ecdev = ecdev_offer(eqos->parent, eqos_ec_poll);
	if (!eqos->ecdev) {
		rt_kprintf("%s: Failed to register EtherCAT device\n", eqos->node_name);
		rt_free(eqos->parent);
		return -RT_ERROR;
	}

	ret = ecdev_open(eqos->ecdev);
	if (ret) {
		ecdev_withdraw(eqos->ecdev);
		rt_free(eqos->parent);
		return -RT_ERROR;
	}
	rt_kprintf("%s: EtherCAT device registered\n", eqos->node_name);

#else
	eqos->parent = rt_calloc(1, sizeof(struct rt_device));
	if (!eqos->parent) {
		rt_kprintf("%s: failed to allocate rt_device\n", eqos->node_name);
		return -RT_ENOMEM;
	}

	eqos->parent->type = RT_Device_Class_NetIf;
	eqos->parent->flag = RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX;
	eqos->parent->open_flag = 0;

#ifdef RT_USING_DEVICE_OPS
	static const struct rt_device_ops rt_eqos_dev_ops = {
		.init = RT_NULL,
		.open = rt_eqos_open,
		.close = rt_eqos_close,
		.read = rt_eqos_read,
		.write = rt_eqos_write,
		.control = rt_eqos_control,
	};
	eqos->parent->ops = &rt_eqos_dev_ops;
#else
	eqos->parent->init = RT_NULL;
	eqos->parent->open = rt_eqos_open;
	eqos->parent->close = rt_eqos_close;
	eqos->parent->read = rt_eqos_read;
	eqos->parent->write = rt_eqos_write;
	eqos->parent->control = rt_eqos_control;
#endif
	eqos->parent->user_data = eqos;

	ret = rt_device_register(eqos->parent, eqos->name, eqos->parent->flag);
	if (ret != RT_EOK) {
		rt_free(eqos->parent);
		eqos->parent = RT_NULL;
		return ret;
	}
#endif
	return RT_EOK;
}

static void eqos_parent_deinit(struct eqos_device *eqos)
{
	if (!eqos)
		return;
#ifdef RT_USING_ETHERCAT
	if (eqos->parent) {
		ecdev_close(eqos->ecdev);
		ecdev_withdraw(eqos->ecdev);
		rt_free(eqos->parent);
		eqos->parent = RT_NULL;
	}
#else
	if (eqos->parent) {
		rt_device_unregister(eqos->parent);
		rt_free(eqos->parent);
		eqos->parent = RT_NULL;
	}
#endif
}

/**
 * @brief Parse the node path to extract the node name.
 *
 * Here, `node->path` is always "/soc/ethernet@20000000/".
 * However, `dtb_node_get_name()` expects the path without the trailing "/",
 * such as "/soc/ethernet@20000000".
 *
 * Therefore, we define this function instead of using `dtb_node_get_name`.
 */
const char *parse_node_name(const struct dtb_node *node)
{
	static char node_name[256];
	rt_int32_t start, end;

	if (!dtb_node_valid(node)) {
		rt_kprintf("%s: node not valid\n", __func__);
		return RT_NULL;
	}

	end = rt_strlen(node->path);
	if (end <= 0 || end > 255) {
		rt_kprintf("%s: length %d exceeds valid range\n", __func__, end);
		return RT_NULL;
	}
	--end;

	if (node->path[end] == '/')
		--end;

	for (start = end; start > 0; --start)
		if (node->path[start] == '/')
			break;

	rt_memcpy(node_name, &node->path[start + 1], end - start);
	node_name[end - start] = '\0';
	return node_name;
}

rt_bool_t mac_is_zero(const rt_uint8_t *mac)
{
	rt_uint8_t i;

	for (i = 0; i < ETH_ALEN; i++)
		if (mac[i])
			return RT_FALSE;

	return RT_TRUE;
}

rt_bool_t mac_is_broadcast(const rt_uint8_t *mac)
{
	rt_uint8_t i;

	for (i = 0; i < ETH_ALEN; i++)
		if (mac[i] != 0xff)
			return RT_FALSE;

	return RT_TRUE;
}

rt_err_t parse_mac_address(struct eqos_device *eqos)
{
	struct eth_pdata *plat = eqos->plat_data;
	int mac_len;
	const void *mac;

	mac = dtb_node_get_property(eqos->node, "mac-address", &mac_len);
	if (!mac) {
		rt_kprintf("Mac address not found for node\n");
		return -RT_EINVAL;
	}

	if (mac_len != ETH_ALEN || mac_is_zero(mac) || mac_is_broadcast(mac)) {
		rt_kprintf("Invalid mac address for node\n");
		return -RT_EINVAL;
	}

	rt_memcpy(plat->enetaddr, mac, ETH_ALEN);

	return RT_EOK;
}

struct eqos_match_table {
	char *compatible;
	struct eqos_config *config;
};

static const struct eqos_match_table eqos_ids[] = {
	{
		.compatible	= "spacemit,k3-gmac",
		.config		= &k3_eqos_config,
	},
	{}
};

static int eqos_probe(void)
{
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	const struct eqos_match_table *match;
	struct dtb_node *node;
	rt_uint8_t eqos_index;
	rt_err_t ret = RT_EOK;

	eqos_index = 0;
	for (match = eqos_ids; match->compatible; ++match) {
		node = dtb_node_find_compatible_node(dtb_head_node, match->compatible);
		if (!node)
			continue;

		if (!dtb_node_device_is_available(node))
			continue;

		struct eqos_device *eqos = rt_calloc(1, sizeof(struct eqos_device));

		if (!eqos)
			return -RT_ENOMEM;

		eqos->node_name = parse_node_name(node);
		eqos->config = match->config;
		eqos->node = node;
		eqos->regs = (uintptr_t)dtb_node_get_addr_index(node, 0);
		if (!eqos->regs) {
			rt_kprintf("%s: failed to get base address\n", eqos->node_name);
			goto err_free_eqos;
		}

		eqos->mac_regs = (void *)(eqos->regs + EQOS_MAC_REGS_BASE);
		eqos->mtl_regs = (void *)(eqos->regs + EQOS_MTL_REGS_BASE);
		eqos->dma_regs = (void *)(eqos->regs + EQOS_DMA_REGS_BASE);
		eqos->max_speed = dtb_node_read_u32_default(node, "max-speed", 0);

		rt_snprintf(eqos->name, RT_NAME_MAX - 1, "eqos%d", eqos_index);
		eqos_index++;

		ret = parse_mac_address(eqos);
		if (ret < 0) {
			rt_kprintf("%s: failed to get mac address: %d\n", eqos->node_name, ret);
			goto err_free_eqos;
		}

		ret = eqos_probe_resources_core(eqos);
		if (ret < 0) {
			rt_kprintf("%s: core resource probe failed: %d\n", eqos->node_name, ret);
			goto err_free_eqos;
		}

		if (eqos->config->ops && eqos->config->ops->eqos_probe_resources) {
			ret = eqos->config->ops->eqos_probe_resources(eqos);
			if (ret < 0) {
				rt_kprintf("%s: platform resource probe failed: %d\n", eqos->node_name, ret);
				goto err_release_core;
			}
		}

		if (eqos->config->ops && eqos->config->ops->eqos_start_clks) {
			ret = eqos->config->ops->eqos_start_clks(eqos);
			if (ret < 0) {
				rt_kprintf("%s: start clks failed: %d\n", eqos->node_name, ret);
				goto err_release_plat;
			}
		}

		if (!eqos->mii) {
			eqos->mii = mdio_alloc();
			if (!eqos->mii) {
				rt_kprintf("%s: mdio alloc failed\n", eqos->node_name);
				ret = -RT_ENOMEM;
				goto err_stop_clks;
			}

			eqos->mii->read = eqos_mdio_read;
			eqos->mii->write = eqos_mdio_write;
			eqos->mii->priv = eqos;
			rt_strncpy(eqos->mii->name, eqos->name, RT_NAME_MAX - 1);
		}

		ret = eqos_parent_init(eqos);
		if (ret != RT_EOK) {
			rt_kprintf("%s: parent init failed: %d\n", eqos->node_name, ret);
			goto err_free_mdio;
		}

		rt_kprintf("%s: probe successfully\n", eqos->node_name);
		continue;

err_free_mdio:
		mdio_free(eqos->mii);
err_stop_clks:
		if (eqos->config->ops->eqos_stop_clks)
			eqos->config->ops->eqos_stop_clks(eqos);
err_release_plat:
		if (eqos->config->ops->eqos_remove_resources)
			eqos->config->ops->eqos_remove_resources(eqos);
err_release_core:
		eqos_remove_resources_core(eqos);
err_free_eqos:
		rt_free(eqos);
	}
	return RT_EOK;
}
INIT_DEVICE_EXPORT(eqos_probe);
