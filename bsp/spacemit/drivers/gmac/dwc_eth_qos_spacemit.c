/*
 * Copyright (c) 2025, Spacemit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Spacemit EQOS GMAC driver - RT-Thread port
 *
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/misc.h>
#include "dwc_eth_qos.h"

#define CLK_PHASE_CNT			256
#define CLK_PHASE_REVERT		180

#define TXCLK_PHASE_DEFAULT		0
#define RXCLK_PHASE_DEFAULT		0

#define TX_PHASE			1
#define RX_PHASE			0

enum clk_tuning_way {
	/* fpga clk tuning register */
	CLK_TUNING_BY_REG,
	/* zebu/evb rgmii delayline register */
	CLK_TUNING_BY_DLINE,
	/* evb rmii only revert tx/rx clock for clk tuning */
	CLK_TUNING_BY_CLK_REVERT,
	CLK_TUNING_MAX,
};

struct spacemit_plat_data {
	struct eqos_device *eqos;
	const struct dtb_node *node;
	void *ctrl_reg;
	void *dline_reg;
	phy_interface_t phy_iface;
	rt_uint32_t phy_reset_gpio;
	rt_uint8_t tx_clk_phase;
	rt_uint8_t rx_clk_phase;
	rt_uint8_t clk_tuning_way;
	struct clk *clk_phy;
	struct clk *clk_rst;
	rt_bool_t clk_tuning_enable;
	rt_bool_t tx_clk_from_soc;
	rt_bool_t phy_clk_from_soc;
};

static inline void dev_set_plat_priv(struct eqos_device *eqos,
				     struct spacemit_plat_data *priv)
{
	struct eth_pdata *plat = eqos->plat_data;

	plat->priv_pdata = priv;
}

static inline void *dev_get_plat_priv(struct eqos_device *eqos)
{
	struct eth_pdata *plat = eqos->plat_data;

	return plat->priv_pdata;
}

/**
 * K3 SoC-specific macros/ops
 */
#define EMAC_AXI_CLK_ENABLE		BIT(0)
#define EMAC_AXI_CLK_RESET		BIT(1)

#define PHY_INTF_RGMII			BIT(3)
#define PHY_INTF_MII			BIT(4)

/* only valid for rmii, invert tx clk */
#define RMII_TX_CLK_SEL			BIT(6)

/* only valid for rmii, invert rx clk */
#define RMII_RX_CLK_SEL			BIT(7)


#define PHY_IRQ_EN			BIT(12)
#define AXI_SINGLE_ID			BIT(13)

#define RMII_TX_PHASE_OFFSET		(16)
#define RMII_TX_PHASE_MASK		RT_GENMASK(18, 16)
#define RMII_RX_PHASE_OFFSET		(20)
#define RMII_RX_PHASE_MASK		RT_GENMASK(22, 20)

#define RGMII_TX_PHASE_OFFSET		(24)
#define RGMII_TX_PHASE_MASK		RT_GENMASK(26, 24)
#define RGMII_RX_PHASE_OFFSET		(20)
#define RGMII_RX_PHASE_MASK		RT_GENMASK(22, 20)

#define EMAC_RX_DLINE_EN		BIT(0)
#define EMAC_RX_DLINE_STEP_OFFSET	(4)
#define EMAC_RX_DLINE_STEP_MASK		RT_GENMASK(5, 4)
#define EMAC_RX_DLINE_CODE_OFFSET	(8)
#define EMAC_RX_DLINE_CODE_MASK		RT_GENMASK(15, 8)

#define EMAC_TX_DLINE_EN		BIT(16)
#define EMAC_TX_DLINE_STEP_OFFSET	(20)
#define EMAC_TX_DLINE_STEP_MASK		RT_GENMASK(21, 20)
#define EMAC_TX_DLINE_CODE_OFFSET	(24)
#define EMAC_TX_DLINE_CODE_MASK		RT_GENMASK(31, 24)

static rt_bool_t phy_iface_is_rmii(struct spacemit_plat_data *priv)
{
	return priv->phy_iface == PHY_INTERFACE_MODE_RMII;
}

static rt_err_t clk_phase_rmii_set(struct spacemit_plat_data *priv, rt_bool_t is_tx)
{
	rt_uint32_t val;

	switch (priv->clk_tuning_way) {
	case CLK_TUNING_BY_REG:
		val = readl(priv->ctrl_reg);
		if (is_tx) {
			val &= ~RMII_TX_PHASE_MASK;
			val |= (priv->tx_clk_phase & 0x7) << RMII_TX_PHASE_OFFSET;
		} else {
			val &= ~RMII_RX_PHASE_MASK;
			val |= (priv->rx_clk_phase & 0x7) << RMII_RX_PHASE_OFFSET;
		}
		writel(val, priv->ctrl_reg);
		break;

	case CLK_TUNING_BY_CLK_REVERT:
		val = readl(priv->ctrl_reg);
		if (is_tx) {
			if (priv->tx_clk_phase == CLK_PHASE_REVERT)
				val |= RMII_TX_CLK_SEL;
			else
				val &= ~RMII_TX_CLK_SEL;
		} else {
			if (priv->rx_clk_phase == CLK_PHASE_REVERT)
				val |= RMII_RX_CLK_SEL;
			else
				val &= ~RMII_RX_CLK_SEL;
		}
		writel(val, priv->ctrl_reg);
		break;
	default:
		rt_kprintf("%s: invalid clk tuning way: %d !!\n",
			   __func__, priv->clk_tuning_way);
		return -RT_ERROR;
	}

	return RT_EOK;
}

static rt_err_t clk_phase_rgmii_set(struct spacemit_plat_data *priv, rt_bool_t is_tx)
{
	rt_uint32_t val;

	switch (priv->clk_tuning_way) {
	case CLK_TUNING_BY_REG:
		val = readl(priv->ctrl_reg);
		if (is_tx) {
			val &= ~RGMII_TX_PHASE_MASK;
			val |= (priv->tx_clk_phase & 0x7) << RGMII_TX_PHASE_OFFSET;
		} else {
			val &= ~RGMII_RX_PHASE_MASK;
			val |= (priv->rx_clk_phase & 0x7) << RGMII_RX_PHASE_OFFSET;
		}
		writel(val, priv->ctrl_reg);
		break;

	case CLK_TUNING_BY_DLINE:
		val = readl(priv->dline_reg);
		if (is_tx) {
			val &= ~EMAC_TX_DLINE_CODE_MASK;
			val |= (priv->tx_clk_phase & 0xff) << EMAC_TX_DLINE_CODE_OFFSET;
			val |= EMAC_TX_DLINE_EN;
		} else {
			val &= ~EMAC_RX_DLINE_CODE_MASK;
			val |= (priv->rx_clk_phase & 0xff) << EMAC_RX_DLINE_CODE_OFFSET;
			val |= EMAC_RX_DLINE_EN;
		}
		writel(val, priv->dline_reg);
		break;

	default:
		rt_kprintf("%s: invalid clk tuning way: %d !!\n",
			   __func__, priv->clk_tuning_way);
		return -RT_ERROR;
	}

	return RT_EOK;
}

static rt_err_t clk_phase_set(struct spacemit_plat_data *priv, rt_bool_t is_tx)
{
	if (priv->clk_tuning_enable) {
		if (phy_iface_is_rmii(priv))
			clk_phase_rmii_set(priv, is_tx);
		else
			clk_phase_rgmii_set(priv, is_tx);
	}
	return RT_EOK;
}

static void k3_eqos_iface_config(struct spacemit_plat_data *priv)
{
	phy_interface_t iface;
	rt_uint32_t val, mask;

	iface = priv->phy_iface;
	val = readl(priv->ctrl_reg);
	mask = PHY_INTF_MII | PHY_INTF_RGMII;
	val &= ~mask;

	switch (iface) {
	case PHY_INTERFACE_MODE_MII:
		val |= PHY_INTF_MII;
		break;

	case PHY_INTERFACE_MODE_RMII:
		break;

	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		val |= PHY_INTF_RGMII;
		break;

	default:
		rt_kprintf("%s: unsupported phy-mode=%s\n",
			   __func__, phy_interface_strings[iface]);
		return;
	}
	writel(val, priv->ctrl_reg);
}

static rt_err_t k3_eqos_phy_reset(struct spacemit_plat_data *priv)
{
	rt_err_t ret;
	rt_uint32_t gpio_num = priv->phy_reset_gpio;

	ret = gpio_direction_output(gpio_num, 1);
	if (ret) {
		rt_kprintf("%s: failed to set gpio %u direction\n",
			   __func__, gpio_num);
		return ret;
	}

	rt_thread_mdelay(2);

	gpio_set_value(gpio_num, 0);

	rt_thread_mdelay(10);

	gpio_set_value(gpio_num, 1);

	rt_thread_mdelay(10);

	return RT_EOK;
}

static rt_err_t k3_validate_iface_and_refclk(struct spacemit_plat_data *priv)
{
	switch (priv->phy_iface) {
	case PHY_INTERFACE_MODE_MII:
		return RT_EOK;

	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		return RT_EOK;

	case PHY_INTERFACE_MODE_RMII:
		/* Only accept RMII with TX clock comes from PHY */
		return priv->tx_clk_from_soc ? -RT_EINVAL : RT_EOK;

	default:
		return -RT_EINVAL;
	}
}

static void k3_eqos_set_clk_phase(struct spacemit_plat_data *priv)
{
	phy_interface_t iface = priv->phy_iface;

	if (!priv->clk_tuning_enable)
		return;

	switch (iface) {
	case PHY_INTERFACE_MODE_RGMII_ID:
		/* PHY already provides TX+RX delay */
		return;
	case PHY_INTERFACE_MODE_RGMII_TXID:
		/* PHY provides TX delay; only adjust RX */
		clk_phase_set(priv, RX_PHASE);
		return;
	case PHY_INTERFACE_MODE_RGMII_RXID:
		/* PHY provides RX delay; only adjust TX */
		clk_phase_set(priv, TX_PHASE);
		return;
	case PHY_INTERFACE_MODE_RMII:
	case PHY_INTERFACE_MODE_RGMII:
		/* rgmii/rmii: adjust both TX and RX phases */
		clk_phase_set(priv, TX_PHASE);
		clk_phase_set(priv, RX_PHASE);
		return;
	default:
		rt_kprintf("%s: clk tuning skipped for phy-mode=%s\n",
			   __func__, phy_interface_strings[iface]);
		return;
	}
}

static rt_err_t k3_eqos_plat_probe(struct eqos_device *eqos)
{
	struct eth_pdata *plat = eqos->plat_data;
	struct dtb_node *node = eqos->node;
	struct spacemit_plat_data *priv;
	rt_uint32_t ctrl_reg, dline_reg;
	/* Use default MAC; overridden if DTS provides a valid address */
	unsigned char default_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
	rt_err_t ret;

	priv = rt_calloc(1, sizeof(*priv));
	if (!priv)
		return -RT_ENOMEM;

	priv->node = node;
	dev_set_plat_priv(eqos, priv);

	/* Get PHY interface from config */
	priv->phy_iface = eqos->config->get_interface(eqos);
	if (priv->phy_iface == PHY_INTERFACE_MODE_NA) {
		rt_kprintf("%s: invalid PHY interface\n", eqos->node_name);
		ret = -RT_EINVAL;
		goto err_free_pdata;
	}

	/* tx clock source select */
	priv->tx_clk_from_soc = dtb_node_read_bool(node, "tx-clock-from-soc");
	/* phy clock source select */
	priv->phy_clk_from_soc = dtb_node_read_bool(node, "phy-clock-from-soc");

	ret = k3_validate_iface_and_refclk(priv);
	if (ret) {
		rt_kprintf("%s: unsupported phy-mode=%s with tx clk from %s\n",
			   __func__, phy_interface_strings[priv->phy_iface],
			   priv->tx_clk_from_soc ? "soc" : "phy");
		goto err_free_pdata;
	}

	rt_memcpy(plat->enetaddr, default_mac, ETH_ALEN);

	/* PHY reset pin (GPIO ID or abstracted integer) */
	ret = dtb_node_read_u32(node, "phy-reset-pin", &priv->phy_reset_gpio);
	if (ret) {
		rt_kprintf("%s: phy-reset-pin not found in dts\n", eqos->node_name);
		goto err_free_pdata;
	}

	ret = gpio_request(priv->phy_reset_gpio, "phy-reset");
	if (ret) {
		rt_kprintf("%s: failed to request GPIO %d\n",
			   eqos->node_name, priv->phy_reset_gpio);
		goto err_free_pdata;
	}

	/* Control register */
	ret = dtb_node_read_u32(node, "ctrl-reg", &ctrl_reg);
	if (ret) {
		rt_kprintf("%s: ctrl-reg not found in dts\n", eqos->node_name);
		goto err_free_gpio;
	}
	priv->ctrl_reg = TO_PTR(ctrl_reg);

	/* Clock tuning related options */
	priv->clk_tuning_enable = dtb_node_read_bool(node, "clk-tuning-enable");
	if (priv->clk_tuning_enable) {
		if (dtb_node_read_bool(node, "clk-tuning-by-reg")) {
			priv->clk_tuning_way = CLK_TUNING_BY_REG;
		} else if (dtb_node_read_bool(node, "clk-tuning-by-clk-revert")) {
			priv->clk_tuning_way = CLK_TUNING_BY_CLK_REVERT;
		} else if (dtb_node_read_bool(node, "clk-tuning-by-delayline")) {
			priv->clk_tuning_way = CLK_TUNING_BY_DLINE;
			ret = dtb_node_read_u32(node, "dline-reg", &dline_reg);
			if (ret) {
				rt_kprintf("%s: dline-reg missing for delayline tuning\n", eqos->node_name);
				goto err_free_gpio;
			}
			priv->dline_reg = TO_PTR(dline_reg);
		} else {
			priv->clk_tuning_way = CLK_TUNING_BY_REG;
		}

		priv->tx_clk_phase = dtb_node_read_u32_default(node, "tx-phase", TXCLK_PHASE_DEFAULT);
		priv->rx_clk_phase = dtb_node_read_u32_default(node, "rx-phase", RXCLK_PHASE_DEFAULT);
	}

	priv->clk_rst = of_clk_get_by_name(node, "mac_reset");
	if (IS_ERR(priv->clk_rst) || !priv->clk_rst) {
		rt_kprintf("%s: of_clk_get_by_name(mac_reset) failed\n", eqos->node_name);
		ret = PTR_ERR(priv->clk_rst);
		goto err_free_gpio;
	}

	eqos->clk_master_bus = of_clk_get_by_name(node, "master_bus_clk");
	if (IS_ERR(eqos->clk_master_bus) || !eqos->clk_master_bus) {
		rt_kprintf("%s: of_clk_get_by_name(master_bus_clk) failed\n", eqos->node_name);
		ret = PTR_ERR(eqos->clk_master_bus);
		goto err_free_reset;
	}

	if (priv->tx_clk_from_soc) {
		eqos->clk_tx = of_clk_get_by_name(node, "tx_clk");
		if (IS_ERR(eqos->clk_tx) || !eqos->clk_tx) {
			rt_kprintf("%s: clk_get_by_name(tx_clk) failed\n", eqos->node_name);
			ret = PTR_ERR(eqos->clk_tx);
			goto err_free_gpio;
		}
	}

	if (priv->phy_clk_from_soc) {
		priv->clk_phy = of_clk_get_by_name(node, "phy_clk");
		if (IS_ERR(priv->clk_phy) || !priv->clk_phy) {
			rt_kprintf("%s: clk_get_by_name(phy_clk) failed\n", eqos->node_name);
			ret = PTR_ERR(priv->clk_phy);
			goto err_free_tx_clk;
		}
		/* Provide the clock before resetting the PHY */
		ret = clk_prepare_enable(priv->clk_phy);
		if (ret) {
			rt_kprintf("%s: clk_prepare_enable(phy_clk) failed\n", eqos->node_name);
			goto err_free_phy_clk;
		}
	}

	ret = k3_eqos_phy_reset(priv);
	if (ret) {
		rt_kprintf("%s: k3_eqos_phy_reset() failed\n", eqos->node_name);
		goto err_disable_phy_clk;
	}

	k3_eqos_iface_config(priv);
	k3_eqos_set_clk_phase(priv);

	return RT_EOK;

/**
 * The current clk/gpio driver has not yet implemented a resource-release interface.
 */
err_disable_phy_clk:
	if (priv->phy_clk_from_soc)
		clk_disable_unprepare(priv->clk_phy);
err_free_phy_clk:
//	if (priv->phy_clk_from_soc)
//		clk_put(priv->clk_phy);
err_free_tx_clk:
//	if (priv->tx_clk_from_soc)
//		clk_put(eqos->clk_tx);
err_free_clk_master_bus:
//	clk_put(eqos->clk_master_bus);
err_free_reset:
//	clk_put(priv->clk_rst);
err_free_gpio:
//	gpio_free(priv->phy_reset_gpio);
err_free_pdata:
	rt_free(priv);
	dev_set_plat_priv(eqos, NULL);
	return ret;
}

rt_err_t k3_eqos_plat_remove(struct eqos_device *eqos)
{
	struct spacemit_plat_data *priv;

	priv = (struct spacemit_plat_data *)(dev_get_plat_priv(eqos));
	if (!priv)
		return RT_EOK;

	if (priv->phy_clk_from_soc) {
		clk_disable_unprepare(priv->clk_phy);
//		clk_put(priv->clk_phy);
	}

//	if (priv->tx_clk_from_soc)
//		clk_put(eqos->clk_tx);
//	clk_put(eqos->clk_master_bus);
//	clk_put(priv->clk_rst);
//	gpio_free(priv->phy_reset_gpio);
	rt_free(priv);
	dev_set_plat_priv(eqos, NULL);

	return RT_EOK;
}

static rt_err_t k3_eqos_start_resets(struct eqos_device *eqos)
{
	struct spacemit_plat_data *priv;
	rt_err_t ret;

	priv = (struct spacemit_plat_data *)(dev_get_plat_priv(eqos));
	/**
	 * Reset PHY on each interface up.
	 * The sequence is: eqos_start -> k3_eqos_start_resets -> k3_eqos_phy_reset.
	 */
	ret = k3_eqos_phy_reset(priv);
	if (ret) {
		rt_kprintf("%s: k3_eqos_phy_reset() failed\n", eqos->node_name);
		return -RT_ERROR;
	}

	ret = clk_prepare_enable(priv->clk_rst);
	if (ret) {
		rt_kprintf("%s: clk_enable(clk_rst) failed\n", eqos->node_name);
		return -RT_ERROR;
	}

	return RT_EOK;
}

static rt_err_t k3_eqos_stop_resets(struct eqos_device *eqos)
{
	struct spacemit_plat_data *priv;

	priv = (struct spacemit_plat_data *)(dev_get_plat_priv(eqos));

	clk_disable_unprepare(priv->clk_rst);

	return RT_EOK;
}

static rt_err_t k3_eqos_start_clks(struct eqos_device *eqos)
{
	struct spacemit_plat_data *priv;
	rt_err_t ret;

	priv = (struct spacemit_plat_data *)(dev_get_plat_priv(eqos));

	ret = clk_prepare_enable(eqos->clk_master_bus);
	if (ret) {
		rt_kprintf("%s: clk_enable(clk_master_bus) failed\n", eqos->node_name);
		return -RT_ERROR;
	}

	if (priv->tx_clk_from_soc) {
		ret = clk_prepare_enable(eqos->clk_tx);
		if (ret) {
			rt_kprintf("%s: clk_enable(clk_tx) failed\n", eqos->node_name);
			clk_disable_unprepare(eqos->clk_master_bus);
			return -RT_ERROR;
		}
	}

	return RT_EOK;
}

static rt_err_t k3_eqos_stop_clks(struct eqos_device *eqos)
{
	struct spacemit_plat_data *priv;

	priv = (struct spacemit_plat_data *)(dev_get_plat_priv(eqos));
	if (priv->tx_clk_from_soc)
		clk_disable_unprepare(eqos->clk_tx);

	clk_disable_unprepare(eqos->clk_master_bus);

	return RT_EOK;
}

static rt_err_t eqos_read_hwaddr(struct eqos_device *eqos)
{
	struct eth_pdata *plat = eqos->plat_data;
	uint32_t val_high, val_low;

	/* Read the MAC address from the hardware registers */
	val_high = readl(&eqos->mac_regs->address0_high);
	val_low = readl(&eqos->mac_regs->address0_low);

	/* Combine the values into the MAC address */
	plat->enetaddr[5] = (val_high >> 8) & 0xFF;
	plat->enetaddr[4] = val_high & 0xFF;
	plat->enetaddr[3] = (val_low >> 24) & 0xFF;
	plat->enetaddr[2] = (val_low >> 16) & 0xFF;
	plat->enetaddr[1] = (val_low >> 8) & 0xFF;
	plat->enetaddr[0] = val_low & 0xFF;

	return RT_EOK;
}

rt_uint64_t k3_eqos_get_tick_clk_rate(struct eqos_device *eqos)
{
	return 100 * 1000000;
}

static struct eqos_priv_ops k3_eqos_ops = {
	.eqos_inval_desc		= eqos_inval_desc_generic,
	.eqos_flush_desc		= eqos_flush_desc_generic,
	.eqos_inval_buffer		= eqos_inval_buffer_generic,
	.eqos_flush_buffer		= eqos_flush_buffer_generic,
	.eqos_probe_resources		= k3_eqos_plat_probe,
	.eqos_remove_resources		= k3_eqos_plat_remove,
	.eqos_stop_resets		= k3_eqos_stop_resets,
	.eqos_start_resets		= k3_eqos_start_resets,
	.eqos_stop_clks			= k3_eqos_stop_clks,
	.eqos_start_clks		= k3_eqos_start_clks,
	.eqos_calibrate_pads		= eqos_null_ops,
	.eqos_disable_calibration	= eqos_null_ops,
	.eqos_set_tx_clk_speed		= eqos_null_ops,
	.eqos_get_enetaddr		= eqos_read_hwaddr,
	.eqos_get_tick_clk_rate		= k3_eqos_get_tick_clk_rate,
};

struct eqos_config k3_eqos_config = {
	.reg_access_always_ok	= RT_FALSE,
	.mdio_wait		= 10,
	.swr_wait		= 50,
	.config_mac		= EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
	.config_mac_mdio	= EQOS_MAC_MDIO_ADDRESS_CR_250_300,
	.axi_bus_width		= EQOS_AXI_WIDTH_64,
	.get_interface		= eqos_get_interface,
	.ops			= &k3_eqos_ops,
};
