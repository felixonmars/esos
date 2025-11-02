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
#include <rtdbg.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#ifdef RT_USING_ETHERCAT
#include <mini_netdev.h>
#include <ecdev.h>
#endif
#include "genphy.h"

#define EQOS_MAC_REGS_BASE 0x000
struct eqos_mac_regs {
	rt_uint32_t configuration;			/* 0x000 */
	rt_uint32_t unused_004[(0x070 - 0x004) / 4];	/* 0x004 */
	rt_uint32_t q0_tx_flow_ctrl;			/* 0x070 */
	rt_uint32_t unused_070[(0x090 - 0x074) / 4];	/* 0x074 */
	rt_uint32_t rx_flow_ctrl;			/* 0x090 */
	rt_uint32_t unused_094;				/* 0x094 */
	rt_uint32_t txq_prty_map0;			/* 0x098 */
	rt_uint32_t unused_09c;				/* 0x09c */
	rt_uint32_t rxq_ctrl0;				/* 0x0a0 */
	rt_uint32_t unused_0a4;				/* 0x0a4 */
	rt_uint32_t rxq_ctrl2;				/* 0x0a8 */
	rt_uint32_t unused_0ac[(0x0dc - 0x0ac) / 4];	/* 0x0ac */
	rt_uint32_t us_tic_counter;			/* 0x0dc */
	rt_uint32_t unused_0e0[(0x11c - 0x0e0) / 4];	/* 0x0e0 */
	rt_uint32_t hw_feature0;			/* 0x11c */
	rt_uint32_t hw_feature1;			/* 0x120 */
	rt_uint32_t hw_feature2;			/* 0x124 */
	rt_uint32_t unused_128[(0x200 - 0x128) / 4];	/* 0x128 */
	rt_uint32_t mdio_address;			/* 0x200 */
	rt_uint32_t mdio_data;				/* 0x204 */
	rt_uint32_t unused_208[(0x300 - 0x208) / 4];	/* 0x208 */
	rt_uint32_t address0_high;			/* 0x300 */
	rt_uint32_t address0_low;			/* 0x304 */
};

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

#define EQOS_MAC_CONFIGURATION_GPSLCE			BIT(23)
#define EQOS_MAC_CONFIGURATION_CST			BIT(21)
#define EQOS_MAC_CONFIGURATION_ACS			BIT(20)
#define EQOS_MAC_CONFIGURATION_WD			BIT(19)
#define EQOS_MAC_CONFIGURATION_JD			BIT(17)
#define EQOS_MAC_CONFIGURATION_JE			BIT(16)
#define EQOS_MAC_CONFIGURATION_PS			BIT(15)
#define EQOS_MAC_CONFIGURATION_FES			BIT(14)
#define EQOS_MAC_CONFIGURATION_DM			BIT(13)
#define EQOS_MAC_CONFIGURATION_LM			BIT(12)
#define EQOS_MAC_CONFIGURATION_TE			BIT(1)
#define EQOS_MAC_CONFIGURATION_RE			BIT(0)

#define EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT		16
#define EQOS_MAC_Q0_TX_FLOW_CTRL_PT_MASK		0xffff
#define EQOS_MAC_Q0_TX_FLOW_CTRL_TFE			BIT(1)

#define EQOS_MAC_RX_FLOW_CTRL_RFE			BIT(0)

#define EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT		0
#define EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK		0xff

#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT			0
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_MASK			3
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_NOT_ENABLED		0
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB		2
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV		1

#define EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT			0
#define EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK			0xff

#define EQOS_MAC_HW_FEATURE0_MMCSEL_SHIFT		8
#define EQOS_MAC_HW_FEATURE0_HDSEL_SHIFT		2
#define EQOS_MAC_HW_FEATURE0_GMIISEL_SHIFT		1
#define EQOS_MAC_HW_FEATURE0_MIISEL_SHIFT		0

#define EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT		6
#define EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK		0x1f
#define EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT		0
#define EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK		0x1f

#define EQOS_MAC_HW_FEATURE3_ASP_SHIFT			28
#define EQOS_MAC_HW_FEATURE3_ASP_MASK			0x3

#define EQOS_MAC_MDIO_ADDRESS_PA_SHIFT			21
#define EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT			16
#define EQOS_MAC_MDIO_ADDRESS_CR_SHIFT			8
#define EQOS_MAC_MDIO_ADDRESS_CR_20_35			2
#define EQOS_MAC_MDIO_ADDRESS_CR_250_300		5
#define EQOS_MAC_MDIO_ADDRESS_SKAP			BIT(4)
#define EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT			2
#define EQOS_MAC_MDIO_ADDRESS_GOC_READ			3
#define EQOS_MAC_MDIO_ADDRESS_GOC_WRITE			1
#define EQOS_MAC_MDIO_ADDRESS_C45E			BIT(1)
#define EQOS_MAC_MDIO_ADDRESS_GB			BIT(0)

#define EQOS_MAC_MDIO_DATA_GD_MASK			0xffff

#define EQOS_MTL_REGS_BASE 0xd00
struct eqos_mtl_regs {
	rt_uint32_t txq0_operation_mode;		/* 0xd00 */
	rt_uint32_t unused_d04;				/* 0xd04 */
	rt_uint32_t txq0_debug;				/* 0xd08 */
	rt_uint32_t unused_d0c[(0xd18 - 0xd0c) / 4];	/* 0xd0c */
	rt_uint32_t txq0_quantum_weight;		/* 0xd18 */
	rt_uint32_t unused_d1c[(0xd30 - 0xd1c) / 4];	/* 0xd1c */
	rt_uint32_t rxq0_operation_mode;		/* 0xd30 */
	rt_uint32_t unused_d34;				/* 0xd34 */
	rt_uint32_t rxq0_debug;				/* 0xd38 */
};

#define EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT		16
#define EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK		0x1ff
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT	2
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_MASK		3
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED	2
#define EQOS_MTL_TXQ0_OPERATION_MODE_TSF		BIT(1)
#define EQOS_MTL_TXQ0_OPERATION_MODE_FTQ		BIT(0)

#define EQOS_MTL_TXQ0_DEBUG_TXQSTS			BIT(4)
#define EQOS_MTL_TXQ0_DEBUG_TRCSTS_SHIFT		1
#define EQOS_MTL_TXQ0_DEBUG_TRCSTS_MASK			3

#define EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT		20
#define EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK		0x3ff
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT		14
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK		0x3f
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT		8
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK		0x3f
#define EQOS_MTL_RXQ0_OPERATION_MODE_EHFC		BIT(7)
#define EQOS_MTL_RXQ0_OPERATION_MODE_RSF		BIT(5)

#define EQOS_MTL_RXQ0_DEBUG_PRXQ_SHIFT			16
#define EQOS_MTL_RXQ0_DEBUG_PRXQ_MASK			0x7fff
#define EQOS_MTL_RXQ0_DEBUG_RXQSTS_SHIFT		4
#define EQOS_MTL_RXQ0_DEBUG_RXQSTS_MASK			3

#define EQOS_DMA_REGS_BASE 0x1000
struct eqos_dma_regs {
	rt_uint32_t mode;				/* 0x1000 */
	rt_uint32_t sysbus_mode;			/* 0x1004 */
	rt_uint32_t unused_1008[(0x1100 - 0x1008) / 4];	/* 0x1008 */
	rt_uint32_t ch0_control;			/* 0x1100 */
	rt_uint32_t ch0_tx_control;			/* 0x1104 */
	rt_uint32_t ch0_rx_control;			/* 0x1108 */
	rt_uint32_t unused_110c;			/* 0x110c */
	rt_uint32_t ch0_txdesc_list_haddress;		/* 0x1110 */
	rt_uint32_t ch0_txdesc_list_address;		/* 0x1114 */
	rt_uint32_t ch0_rxdesc_list_haddress;		/* 0x1118 */
	rt_uint32_t ch0_rxdesc_list_address;		/* 0x111c */
	rt_uint32_t ch0_txdesc_tail_pointer;		/* 0x1120 */
	rt_uint32_t unused_1124;			/* 0x1124 */
	rt_uint32_t ch0_rxdesc_tail_pointer;		/* 0x1128 */
	rt_uint32_t ch0_txdesc_ring_length;		/* 0x112c */
	rt_uint32_t ch0_rxdesc_ring_length;		/* 0x1130 */
};

#define EQOS_DMA_MODE_SWR				BIT(0)

#define EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT		16
#define EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK		0xf
#define EQOS_DMA_SYSBUS_MODE_EAME			BIT(11)
#define EQOS_DMA_SYSBUS_MODE_BLEN16			BIT(3)
#define EQOS_DMA_SYSBUS_MODE_BLEN8			BIT(2)
#define EQOS_DMA_SYSBUS_MODE_BLEN4			BIT(1)

#define EQOS_DMA_CH0_CONTROL_DSL_SHIFT			18
#define EQOS_DMA_CH0_CONTROL_PBLX8			BIT(16)

#define EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT		16
#define EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK		0x3f
#define EQOS_DMA_CH0_TX_CONTROL_OSP			BIT(4)
#define EQOS_DMA_CH0_TX_CONTROL_ST			BIT(0)

#define EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT		16
#define EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK		0x3f
#define EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT		1
#define EQOS_DMA_CH0_RX_CONTROL_RBSZ_MASK		0x3fff
#define EQOS_DMA_CH0_RX_CONTROL_SR			BIT(0)

#define EQOS_SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD	BIT(31)

#define EQOS_AUTO_CAL_CONFIG_START			BIT(31)
#define EQOS_AUTO_CAL_CONFIG_ENABLE			BIT(29)

#define EQOS_AUTO_CAL_STATUS_ACTIVE			BIT(31)

#define EQOS_VER_OFFSET		0x110
#define EQOS_PMT		0xc0

#define GMAC_CONFIG_LM		BIT(12)
#define GMAC_CONFIG_POWERDOWN	BIT(1)

/* Descriptors */
#define ARCH_DMA_MINALIGN 64
#define EQOS_DESCRIPTORS_TX	128
#define EQOS_DESCRIPTORS_RX	128
#define EQOS_DESCRIPTORS_NUM	(EQOS_DESCRIPTORS_TX + EQOS_DESCRIPTORS_RX)
#define EQOS_BUFFER_ALIGN	ARCH_DMA_MINALIGN
#define EQOS_MAX_PACKET_SIZE	RT_ALIGN(1568, ARCH_DMA_MINALIGN)
#define EQOS_RX_BUFFER_SIZE	(EQOS_DESCRIPTORS_RX * EQOS_MAX_PACKET_SIZE)

struct eqos_desc {
	rt_uint32_t des0;
	rt_uint32_t des1;
	rt_uint32_t des2;
	rt_uint32_t des3;
};

#define EQOS_DESC3_OWN		BIT(31)
#define EQOS_DESC3_FD		BIT(29)
#define EQOS_DESC3_LD		BIT(28)
#define EQOS_DESC3_BUF1V	BIT(24)

#define EQOS_AXI_WIDTH_32	4
#define EQOS_AXI_WIDTH_64	8
#define EQOS_AXI_WIDTH_128	16

#define EQOS_NAME_MAX 32
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#ifndef ETH_ZLEN
#define ETH_ZLEN 60
#endif

/* Network device control commands */
enum {
	CMD_SET_MAC = 0,
	CMD_GET_MAC,
	CMD_ENABLE_MAC_LOOPBACK,
	CMD_DISABLE_MAC_LOOPBACK,
	CMD_ENABLE_PHY_LOOPBACK,
	CMD_DISABLE_PHY_LOOPBACK,
};

struct eth_pdata {
	uintptr_t iobase;
	rt_uint8_t enetaddr[ETH_ALEN];
	phy_interface_t phy_iface;
	int max_speed;
	void *priv_pdata;
};

struct eqos_device {
	char name[RT_NAME_MAX];
#ifdef RT_USING_ETHERCAT
	struct mini_netdev *parent;
	ec_device_t *ecdev;
#else
	rt_device_t parent;
#endif
	const struct eqos_config *config;
	struct eth_pdata *plat_data;
	struct dtb_node *node;
	const char *node_name;
	uintptr_t regs;

	struct eqos_mac_regs *mac_regs;
	struct eqos_mtl_regs *mtl_regs;
	struct eqos_dma_regs *dma_regs;

	rt_uint32_t phy_reset_gpio;
	struct clk *clk_master_bus;
	struct clk *clk_rx;
	struct clk *clk_ptp_ref;
	struct clk *clk_tx;
	struct clk *clk_ck;
	struct clk *clk_slave_bus;
	struct mii_dev *mii;
	struct phy_device *phy;
	const struct dtb_node *phy_of_node;
	rt_uint32_t max_speed;
	void *descs;
	rt_uint32_t tx_desc_idx, rx_desc_idx;
	rt_size_t desc_size;
	void *tx_dma_buf;
	void *rx_dma_buf;
	void *rx_pkt;
	rt_bool_t started;
	rt_bool_t reg_access_ok;
	rt_bool_t clk_ck_enabled;
};

struct eqos_priv_ops {
	void (*eqos_inval_desc)(void *desc);
	void (*eqos_flush_desc)(void *desc);
	void (*eqos_inval_buffer)(void *buf, rt_size_t size);
	void (*eqos_flush_buffer)(void *buf, rt_size_t size);
	rt_err_t (*eqos_probe_resources)(struct eqos_device *eqos);
	rt_err_t (*eqos_remove_resources)(struct eqos_device *eqos);
	rt_err_t (*eqos_stop_resets)(struct eqos_device *eqos);
	rt_err_t (*eqos_start_resets)(struct eqos_device *eqos);
	rt_err_t (*eqos_stop_clks)(struct eqos_device *eqos);
	rt_err_t (*eqos_start_clks)(struct eqos_device *eqos);
	rt_err_t (*eqos_calibrate_pads)(struct eqos_device *eqos);
	rt_err_t (*eqos_disable_calibration)(struct eqos_device *eqos);
	rt_err_t (*eqos_set_tx_clk_speed)(struct eqos_device *eqos);
	rt_err_t (*eqos_get_enetaddr)(struct eqos_device *eqos);
	rt_uint64_t (*eqos_get_tick_clk_rate)(struct eqos_device *eqos);
};

struct eqos_config {
	rt_bool_t reg_access_always_ok;
	int mdio_wait;
	int swr_wait;
	int config_mac;
	int config_mac_mdio;
	unsigned int axi_bus_width;
	phy_interface_t (*get_interface)(struct eqos_device *eqos);
	struct eqos_priv_ops *ops;
};

static inline rt_err_t eqos_null_ops(struct eqos_device *eqos)
{
	return RT_EOK;
}

void eqos_inval_desc_generic(void *desc);
void eqos_flush_desc_generic(void *desc);
void eqos_inval_buffer_generic(void *buf, rt_size_t size);
void eqos_flush_buffer_generic(void *buf, rt_size_t size);

phy_interface_t eqos_get_interface(struct eqos_device *eqos);

static inline void eqos_writel(struct eqos_device *eqos, rt_uint32_t offset, rt_uint32_t value)
{
	*(rt_uint32_t *)(eqos->regs + offset) = value;
}

static inline rt_uint32_t eqos_readl(struct eqos_device *eqos, rt_uint32_t offset)
{
	return *(rt_uint32_t *)(eqos->regs + offset);
}

#define TO_PTR(x) ((void *)(uintptr_t)(x))

/*
 * Set specific bits in a 32-bit little-endian MMIO register
 * Usage: setbits_le32(0x20000300, BIT(5));
 */
#define setbits_le32(addr, bits) \
	writel(readl(addr) | (bits), addr)

/*
 * Clear specific bits in a 32-bit little-endian MMIO register
 * Usage: clrbits_le32(0x20000300, BIT(5));
 */
#define clrbits_le32(addr, bits) \
	writel(readl(addr) & ~(bits), addr)

/*
 * Clear then set specific bits in a 32-bit little-endian MMIO register
 * Usage: clrsetbits_le32(0x20000300, BIT(5), BIT(7));
 */
#define clrsetbits_le32(addr, clear, set) \
	writel((readl(addr) & ~(clear)) | (set), addr)

#define lower_32_bits(n) ((rt_uint32_t)((n) & 0xFFFFFFFF))

#define upper_32_bits(n) ((rt_uint32_t)(((n) >> 32) & 0xFFFFFFFF))

#define eqos_mb() do { __asm__ __volatile__("fence iorw, iorw" ::: "memory"); } while (0)

void print_packet(const char *title, const rt_uint8_t *packet, int length);

static inline rt_bool_t is_multicast_ethaddr(const uint8_t *addr)
{
	return 0x01 & addr[0];
}

static inline rt_bool_t is_zero_ethaddr(const uint8_t *addr)
{
	rt_uint32_t i;

	for (i = 0; i < ETH_ALEN; i++) {
		if (addr[i] != 0x00)
			return RT_FALSE;
	}

	return RT_TRUE;
}

static inline rt_bool_t is_valid_ethaddr(const uint8_t *addr)
{
	return !is_multicast_ethaddr(addr) && !is_zero_ethaddr(addr);
}

extern struct eqos_config k3_eqos_config;
