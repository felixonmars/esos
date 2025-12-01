/*
 * Copyright (c) 2025, Spacemit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * dwc_eth_qos_tool.c - Provides a ethernet interface control and test tool.
 */

#include <stdlib.h>
#include <rtthread.h>
#include <rtdef.h>
#include <riscv-ops.h>
#include <rthw.h>
#include <rtdevice.h>
#include <finsh.h>
#include <clint.h>
#include "dwc_eth_qos.h"
#include "genphy.h"

#define EXPECTED_SNPS_VER 0x54
#define EXPECTED_USER_VER 0x10

#define DEFAULT_POLLING_STRESS_COUNT 1024
#define DEFAULT_UPDOWN_STRESS_COUNT 128

#define LOOPBACK_TIMEOUT 100
#define ETH_HEADER_SIZE 14
#define RX_BUF_SIZE 1518

struct eth_hdr {
	rt_uint8_t dest_mac[6];
	rt_uint8_t src_mac[6];
	rt_uint16_t ethertype;
};

static rt_uint32_t spacemit_magic_number = 0x20211102;

static rt_bool_t set_pkt_magic(rt_uint8_t *pkt, rt_size_t len, rt_uint32_t magic)
{
	if (len < 46)
		return RT_FALSE;

	pkt[42] = (magic >> 24) & 0xFF;
	pkt[43] = (magic >> 16) & 0xFF;
	pkt[44] = (magic >> 8) & 0xFF;
	pkt[45] = (magic & 0xFF);

	return RT_TRUE;
}

static rt_bool_t verify_pkt_magic(rt_uint8_t *packet, rt_size_t len, rt_uint32_t magic_number)
{
	if (len < 46)
		return RT_FALSE;

	rt_uint32_t pkt_magic = (packet[42] << 24) | (packet[43] << 16) |
				(packet[44] << 8) | packet[45];

	return (pkt_magic == magic_number);
}

static rt_bool_t link_is_active(rt_device_t dev)
{
	struct eqos_device *eqos;

	eqos = (struct eqos_device *)dev->user_data;
	if (!eqos || !eqos->phy) {
		rt_kprintf("Failed to get PHY device.\n");
		return RT_FALSE;
	}

	return (eqos->phy->link == 1);
}

static rt_err_t reg_access_test(rt_device_t dev)
{
	struct eqos_device *eqos;
	rt_uint32_t val;
	rt_uint8_t snps_ver, user_ver;
	rt_err_t ret = RT_EOK;

	eqos = (struct eqos_device *)dev->user_data;
	if (!eqos) {
		rt_kprintf("%s: failed to get EQOS device.\n", __func__);
		return -RT_ERROR;
	}

	val = eqos_readl(eqos, EQOS_VER_OFFSET);
	snps_ver = val & 0xFF;
	user_ver = (val >> 8) & 0xFF;

	if (snps_ver != EXPECTED_SNPS_VER) {
		rt_kprintf("%s: SNPS Ver error, expected=0x%02X, got=0x%02X\n", __func__,
			EXPECTED_SNPS_VER, snps_ver);
		ret = -RT_ERROR;
	}

	if (user_ver != EXPECTED_USER_VER) {
		rt_kprintf("%s: USER Ver error, expected=0x%02X, got=0x%02X\n", __func__,
			EXPECTED_USER_VER, user_ver);
		ret = -RT_ERROR;
	}

	return ret;
}

static rt_err_t mac_loopback_test(rt_device_t dev)
{
	struct eth_hdr eth_hdr;
	rt_uint8_t *tx_buf;
	rt_size_t tx_buf_len;
	rt_size_t tx_pkt_len;
	rt_size_t rx_pkt_len;
	rt_uint8_t rx_buf[RX_BUF_SIZE];
	rt_int32_t timeout;
	rt_err_t ret;

	rt_memset(&eth_hdr, 0, sizeof(eth_hdr));
	rt_memcpy(eth_hdr.dest_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);
	rt_memcpy(eth_hdr.src_mac, "\x02\x11\x22\x33\x44\x55", 6);
	eth_hdr.ethertype = 0xa488;

	tx_buf_len = ETH_HEADER_SIZE + 50;
	tx_buf = (rt_uint8_t *)rt_malloc(tx_buf_len);
	if (!tx_buf) {
		rt_kprintf("%s: failed to allocate tx buffer!\n", __func__);
		return -RT_ENOMEM;
	}

	rt_memcpy(tx_buf, &eth_hdr, ETH_HEADER_SIZE);
	rt_memset(tx_buf + ETH_HEADER_SIZE, 0, tx_buf_len - ETH_HEADER_SIZE);

	++spacemit_magic_number;
	set_pkt_magic(tx_buf, tx_buf_len, spacemit_magic_number);

	ret = rt_device_control(dev, CMD_ENABLE_MAC_LOOPBACK, RT_NULL);
	if (ret != RT_EOK) {
		rt_kprintf("%s: failed to enable MAC loopback!\n", __func__);
		goto free_pkt;
	}

	/* send loop */
	timeout = LOOPBACK_TIMEOUT;
	while (timeout > 0) {
		tx_pkt_len = rt_device_write(dev, 0, tx_buf, tx_buf_len);
		/* Send successful */
		if (tx_pkt_len == tx_buf_len) {
			ret = RT_EOK;
			break;
		}
		--timeout;
		rt_thread_mdelay(1);
	}

	if (timeout == 0) {
		ret = -RT_ETIMEOUT;
		rt_kprintf("%s: send error!\n", __func__);
		goto disable_macloopback;
	}

	/* recv loop */
	timeout = LOOPBACK_TIMEOUT;
	while (timeout > 0) {
		rx_pkt_len = rt_device_read(dev, 0, rx_buf, RX_BUF_SIZE);
		if (rx_pkt_len > 0) {
			if (verify_pkt_magic(rx_buf, rx_pkt_len, spacemit_magic_number)) {
				ret = RT_EOK;
				break;
			}
			rt_kprintf("%s: receive unmatched packet!\n", __func__);
			/* Do not consume timeout for unrelated packets to tolerate noisy traffic. */
			continue;
		}
		--timeout;
		rt_thread_mdelay(1);
	}

	if (timeout == 0) {
		ret = -RT_ETIMEOUT;
		rt_kprintf("%s: receive timeout\n", __func__);
	}

disable_macloopback:
	if (rt_device_control(dev, CMD_DISABLE_MAC_LOOPBACK, RT_NULL))
		rt_kprintf("%s: failed to disable MAC loopback!\n", __func__);
free_pkt:
	rt_free(tx_buf);
	return ret;
}

static rt_err_t phy_loopback_test(rt_device_t dev)
{
	struct eth_hdr eth_hdr;
	rt_uint8_t *tx_buf;
	rt_size_t tx_buf_len;
	rt_size_t tx_pkt_len;
	rt_size_t rx_pkt_len;
	rt_uint8_t rx_buf[RX_BUF_SIZE];
	rt_int32_t timeout;
	rt_err_t ret;

	if (!link_is_active(dev)) {
		rt_kprintf("%s: link is not active!\n", __func__);
		return -RT_ERROR;
	}

	rt_memset(&eth_hdr, 0, sizeof(eth_hdr));
	rt_memcpy(eth_hdr.dest_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);
	rt_memcpy(eth_hdr.src_mac, "\x02\x11\x22\x33\x44\x55", 6);
	eth_hdr.ethertype = 0xa488;

	tx_buf_len = ETH_HEADER_SIZE + 50;
	tx_buf = (rt_uint8_t *)rt_malloc(tx_buf_len);
	if (!tx_buf) {
		rt_kprintf("%s: failed to allocate tx buffer!\n", __func__);
		return -RT_ENOMEM;
	}

	rt_memcpy(tx_buf, &eth_hdr, ETH_HEADER_SIZE);
	rt_memset(tx_buf + ETH_HEADER_SIZE, 0, tx_buf_len - ETH_HEADER_SIZE);

	++spacemit_magic_number;
	set_pkt_magic(tx_buf, tx_buf_len, spacemit_magic_number);

	ret = rt_device_control(dev, CMD_ENABLE_PHY_LOOPBACK, RT_NULL);
	if (ret != RT_EOK) {
		rt_kprintf("%s: failed to enable PHY loopback!\n", __func__);
		goto free_pkt;
	}

	/* send loop */
	timeout = LOOPBACK_TIMEOUT;
	while (timeout > 0) {
		tx_pkt_len = rt_device_write(dev, 0, tx_buf, tx_buf_len);
		/* Send successful */
		if (tx_pkt_len == tx_buf_len) {
			ret = RT_EOK;
			break;
		}
		--timeout;
		rt_thread_mdelay(1);
	}

	if (timeout == 0) {
		ret = -RT_ETIMEOUT;
		rt_kprintf("%s: send error!\n", __func__);
		goto disable_phyloopback;
	}

	/* recv loop */
	timeout = LOOPBACK_TIMEOUT;
	while (timeout > 0) {
		rx_pkt_len = rt_device_read(dev, 0, rx_buf, RX_BUF_SIZE);
		if (rx_pkt_len > 0) {
			if (verify_pkt_magic(rx_buf, rx_pkt_len, spacemit_magic_number)) {
				ret = RT_EOK;
				break;
			}
			rt_kprintf("%s: receive unmatched packet!\n", __func__);
			/* Do not consume timeout for unrelated packets to tolerate noisy traffic. */
			continue;
		}
		--timeout;
		rt_thread_mdelay(1);
	}

	if (timeout == 0) {
		ret = -RT_ETIMEOUT;
		rt_kprintf("%s: receive timeout\n", __func__);
	}

disable_phyloopback:
	if (rt_device_control(dev, CMD_DISABLE_PHY_LOOPBACK, RT_NULL))
		rt_kprintf("%s: failed to disable PHY loopback!\n", __func__);
free_pkt:
	rt_free(tx_buf);
	return ret;
}

typedef struct
{
	rt_uint8_t addr[6];
} ndev_addr_t;

static rt_err_t write_hwaddr_test(rt_device_t dev, ndev_addr_t *mac)
{
	ndev_addr_t current_mac;
	rt_err_t ret = -1;

	ret = rt_device_close(dev);
	if (ret != RT_EOK) {
		rt_kprintf("%s: failed to close device!\n", __func__);
		return ret;
	}

	ret = rt_device_control(dev, CMD_SET_MAC, mac);
	if (ret != RT_EOK) {
		rt_kprintf("%s: failed to set MAC address!\n", __func__);
		return ret;
	}

	ret = rt_device_open(dev, RT_DEVICE_FLAG_RDWR);
	if (ret != RT_EOK) {
		rt_kprintf("%s: failed to reopen device!\n", __func__);
		return ret;
	}

	ret = rt_device_control(dev, CMD_GET_MAC, &current_mac);
	if (ret != RT_EOK) {
		rt_kprintf("%s: failed to get MAC address after setting!\n", __func__);
		return ret;
	}

	if (rt_memcmp(current_mac.addr, mac->addr, sizeof(mac->addr)) != 0) {
		rt_kprintf("%s: MAC address mismatch after setting!\n", __func__);
		return ret;
	}

	rt_kprintf("%s: success to set MAC address to: %02X:%02X:%02X:%02X:%02X:%02X\n", __func__,
		current_mac.addr[0], current_mac.addr[1], current_mac.addr[2],
		current_mac.addr[3], current_mac.addr[4], current_mac.addr[5]);

	return RT_EOK;
}

static rt_err_t txrx_polling_test(rt_device_t dev, int count)
{
	struct eth_hdr eth_hdr;
	rt_uint8_t *tx_buf;
	rt_size_t tx_buf_len;
	rt_size_t tx_pkt_len;
	rt_size_t rx_pkt_len;
	rt_uint8_t rx_buf[RX_BUF_SIZE];
	rt_int32_t timeout;
	rt_err_t ret = -RT_ERROR;
	int i;

	if (!link_is_active(dev)) {
		rt_kprintf("%s: link is not active!\n", __func__);
		return -RT_ERROR;
	}

	rt_memset(&eth_hdr, 0, sizeof(eth_hdr));
	rt_memcpy(eth_hdr.dest_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);
	rt_memcpy(eth_hdr.src_mac, "\x02\x11\x22\x33\x44\x55", 6);
	eth_hdr.ethertype = 0xa488;

	tx_buf_len = ETH_HEADER_SIZE + 50;
	tx_buf = (rt_uint8_t *)rt_malloc(tx_buf_len);
	if (!tx_buf) {
		rt_kprintf("%s: failed to allocate packet buffer!\n", __func__);
		return -RT_ENOMEM;
	}

	rt_memcpy(tx_buf, &eth_hdr, ETH_HEADER_SIZE);
	rt_memset(tx_buf + ETH_HEADER_SIZE, 0, tx_buf_len - ETH_HEADER_SIZE);

	ret = rt_device_control(dev, CMD_ENABLE_PHY_LOOPBACK, RT_NULL);
	if (ret != RT_EOK) {
		rt_kprintf("%s: failed to enable PHY loopback!\n", __func__);
		goto free_pkt;
	}

	for (i = 0; i < count; i++) {

		++spacemit_magic_number;
		set_pkt_magic(tx_buf, tx_buf_len, spacemit_magic_number);

		/* send loop */
		timeout = LOOPBACK_TIMEOUT;
		while (timeout > 0) {
			tx_pkt_len = rt_device_write(dev, 0, tx_buf, tx_buf_len);
			if (tx_pkt_len == tx_buf_len) {
				ret = RT_EOK;
				break;
			}
			--timeout;
			rt_thread_mdelay(1);
		}

		if (timeout == 0) {
			ret = -RT_ETIMEOUT;
			rt_kprintf("[%d] %s: Send error!\n", i, __func__);
			goto disable_phyloopback;
		}

		/* recv loop */
		timeout = LOOPBACK_TIMEOUT;
		while (timeout > 0) {
			rx_pkt_len = rt_device_read(dev, 0, rx_buf, RX_BUF_SIZE);

			if (rx_pkt_len > 0) {
				if (verify_pkt_magic(rx_buf, rx_pkt_len, spacemit_magic_number)) {
					ret = RT_EOK;
					break;
				}
				rt_kprintf("%s: receive unmatched packet!\n", __func__);
				/* Ignore unrelated packets to tolerate noisy traffic */
				continue;
			}
			--timeout;
			rt_thread_mdelay(1);
		}

		if (timeout == 0) {
			ret = -RT_ETIMEOUT;
			rt_kprintf("[%d] %s: receive timeout!\n", i, __func__);
			goto disable_phyloopback;
		}
	}

disable_phyloopback:
	if (rt_device_control(dev, CMD_DISABLE_PHY_LOOPBACK, RT_NULL))
		rt_kprintf("%s: failed to disable PHY loopback!\n", __func__);

free_pkt:
	rt_free(tx_buf);
	return ret;
}

static rt_err_t updown_stress_test(rt_device_t dev, int count)
{
	rt_err_t ret = -RT_ERROR;
	int i;

	for (i = 0; i < count; i++) {

		/* Bring up device */
		ret = rt_device_open(dev, RT_DEVICE_FLAG_RDWR);
		if (ret != RT_EOK) {
			rt_kprintf("%s: failed to bring up device at iteration %d\n", __func__, i);
			goto free_device;
		}

		/* Wait link is up */
		rt_thread_mdelay(5000);

		/* Run PHY loopback test */
		ret = phy_loopback_test(dev);
		if (ret != RT_EOK) {
			rt_kprintf("%s: failed at iteration %d\n", __func__, i);
			goto close_device;
		}

		rt_kprintf("%s: success at iteration %d\n", __func__, i);

		/* Bring down device */
		ret = rt_device_close(dev);
		if (ret != RT_EOK) {
			rt_kprintf("%s: failed to bring down device at iteration %d\n", __func__, i);
			/* Bring down device again */
			goto close_device;
		}
	}

	return RT_EOK;

close_device:
	rt_device_close(dev);
free_device:
	return ret;
}

static rt_err_t macctl_up(rt_device_t dev, int argc, char **argv)
{
	int ret;

	if (dev->ref_count > 0) {
		rt_kprintf("Device already opened.\n");
		return RT_EOK;
	}

	ret = rt_device_open(dev, RT_DEVICE_FLAG_RDWR);
	if (ret != RT_EOK) {
		rt_kprintf("Failed to open device: %s\n", dev->parent.name);
	}
	return ret;
}

static rt_err_t macctl_down(rt_device_t dev, int argc, char **argv)
{
	int ret;

	if (dev->ref_count == 0) {
		rt_kprintf("Device already closed.\n");
		return RT_EOK;
	}

	ret = rt_device_close(dev);
	if (ret != RT_EOK) {
		rt_kprintf("Failed to close device: %s\n", dev->parent.name);
	}
	return ret;
}

static rt_err_t macctl_recv(rt_device_t dev, int argc, char **argv)
{
	int count = 1;

	if (argc != 3 && argc != 5) {
		rt_kprintf("Invalid number of arguments.\n");
		return -RT_ERROR;
	}

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	if (argc == 5) {
		if (rt_strcmp(argv[3], "-c") == 0) {
			count = atoi(argv[4]);
			if (count <= 0 || count >= EQOS_DESCRIPTORS_RX) {
				rt_kprintf("Invalid parameter: -c value must be a positive integer less than %d.\n", EQOS_DESCRIPTORS_RX);
				return -RT_ERROR;
			}
		} else {
			rt_kprintf("Invalid command: Unknown option %s.\n", argv[3]);
			return -RT_ERROR;
		}
	}

	if (!link_is_active(dev)) {
		rt_kprintf("%s: link is not active!\n", __func__);
		return -RT_ERROR;
	}

	for (int i = 0; i < count; i++) {
		rt_uint8_t rx_buf[RX_BUF_SIZE];
		int rx_pkt_len = rt_device_read(dev, 0, rx_buf, RX_BUF_SIZE);
		if (rx_pkt_len > 0) {
			print_packet("Received Packet", rx_buf, rx_pkt_len);
		}
	}

	return RT_EOK;
}

static rt_err_t macctl_send(rt_device_t dev, int argc, char **argv)
{
	struct eth_hdr eth_hdr;
	rt_int32_t tx_buf_len = 64;
	rt_uint8_t tx_buf[64];
	rt_int32_t timeout = LOOPBACK_TIMEOUT;
	int ret = -1;

	if (argc != 3) {
		rt_kprintf("Invalid arguments. Usage: macctl <interface> send\n");
		return -RT_ERROR;
	}

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	if (!link_is_active(dev)) {
		rt_kprintf("%s: link is not active!\n", __func__);
		return -RT_ERROR;
	}

	rt_memset(&eth_hdr, 0, sizeof(eth_hdr));
	rt_memcpy(eth_hdr.dest_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);
	rt_memcpy(eth_hdr.src_mac, "\x02\x11\x22\x33\x44\x55", 6);
	eth_hdr.ethertype = 0xa488;

	rt_memcpy(tx_buf, &eth_hdr, ETH_HEADER_SIZE);
	rt_memset(tx_buf + ETH_HEADER_SIZE, 0, tx_buf_len - ETH_HEADER_SIZE);

	++spacemit_magic_number;
	set_pkt_magic(tx_buf, tx_buf_len, spacemit_magic_number);

	while (timeout > 0) {
		ret = rt_device_write(dev, 0, tx_buf, tx_buf_len);
		if (ret == tx_buf_len) {
			print_packet("Sent Packet", tx_buf, tx_buf_len);
			break;
		} else {
			--timeout;
			rt_thread_mdelay(1);
		}
	}

	if (timeout == 0) {
		rt_kprintf("Send error! Timeout.\n");
		ret = -RT_ETIMEOUT;
	}

	return ret;
}

static rt_err_t macctl_get_mac(rt_device_t dev, int argc, char **argv)
{
	ndev_addr_t current_mac;
	int ret;

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	ret = rt_device_control(dev, CMD_GET_MAC, &current_mac);
	if (ret != RT_EOK) {
		rt_kprintf("Failed to get mac address for device.\n");
		return ret;
	}

	rt_kprintf("Current mac address: %02X:%02X:%02X:%02X:%02X:%02X\n",
		current_mac.addr[0], current_mac.addr[1], current_mac.addr[2],
		current_mac.addr[3], current_mac.addr[4], current_mac.addr[5]);

	return RT_EOK;
}

static int parse_mac_address(const char *mac_str, ndev_addr_t *mac)
{
	rt_size_t i = 0;
	rt_uint8_t temp;
	const char *pos = mac_str;

	while (*pos && i < 6) {
		if (*pos == ':' || *pos == '-') {
			pos++;
			continue;
		}

		if (*pos >= '0' && *pos <= '9') {
			temp = *pos - '0';
		} else if (*pos >= 'A' && *pos <= 'F') {
			temp = *pos - 'A' + 10;
		} else if (*pos >= 'a' && *pos <= 'f') {
			temp = *pos - 'a' + 10;
		} else {
			return -1;
		}

		mac->addr[i] = temp << 4;
		pos++;

		if (*pos >= '0' && *pos <= '9') {
			temp = *pos - '0';
		} else if (*pos >= 'A' && *pos <= 'F') {
			temp = *pos - 'A' + 10;
		} else if (*pos >= 'a' && *pos <= 'f') {
			temp = *pos - 'a' + 10;
		} else {
			return -1;
		}

		mac->addr[i] |= temp;
		pos++;

		i++;
	}

	if (i != 6) {
		return -1;
	}

	return 0;
}

static rt_err_t macctl_set_mac(rt_device_t dev, int argc, char **argv)
{
	int ret;
	ndev_addr_t new_mac;

	if (argc != 4) {
		rt_kprintf("Invalid arguments. Usage: macctl <interface> set-mac <new-mac>\n");
		return -RT_ERROR;
	}

	if (dev->open_flag & RT_DEVICE_OFLAG_OPEN) {
		rt_kprintf("Device is open. Please use 'macctl <interface> down' to close the device first.\n");
		return -RT_ERROR;
	}

	ret = parse_mac_address(argv[3], &new_mac);
	if (ret != 0) {
		rt_kprintf("Invalid MAC address format.\n");
		return ret;
	}

	ret = rt_device_control(dev, CMD_SET_MAC, &new_mac);
	if (ret != RT_EOK) {
		rt_kprintf("Failed to set MAC address.\n");
		return ret;
	}

	return RT_EOK;
}

static rt_err_t macctl_mac_loopback(rt_device_t dev, int argc, char **argv)
{
	int ret;

	if (argc != 4) {
		rt_kprintf("Invalid arguments. Usage: macctl <interface> mac-loopback <on/off>\n");
		return -RT_ERROR;
	}

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	if (rt_strcmp(argv[3], "on") == 0) {
		/* Enable MAC loopback mode */
		ret = rt_device_control(dev, CMD_ENABLE_MAC_LOOPBACK, RT_NULL);
		if (ret != RT_EOK) {
			rt_kprintf("Failed to enable MAC loopback mode.\n");
			return ret;
		}
		rt_kprintf("MAC loopback mode enabled.\n");
	} else if (rt_strcmp(argv[3], "off") == 0) {
		/* Disable MAC loopback mode */
		ret = rt_device_control(dev, CMD_DISABLE_MAC_LOOPBACK, RT_NULL);
		if (ret != RT_EOK) {
			rt_kprintf("Failed to disable MAC loopback mode.\n");
			return ret;
		}
		rt_kprintf("MAC loopback mode disabled.\n");
	} else {
		rt_kprintf("Invalid argument for mac-loopback. Use 'on' or 'off'.\n");
		return -RT_ERROR;
	}

	return RT_EOK;
}

static rt_err_t macctl_phy_loopback(rt_device_t dev, int argc, char **argv)
{
	int ret;

	if (argc != 4) {
		rt_kprintf("Invalid arguments. Usage: macctl <interface> phy-loopback <on/off>\n");
		return -RT_ERROR;
	}

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	if (rt_strcmp(argv[3], "on") == 0) {
		/* Enable PHY loopback mode */
		ret = rt_device_control(dev, CMD_ENABLE_PHY_LOOPBACK, RT_NULL);
		if (ret != RT_EOK) {
			rt_kprintf("Failed to enable PHY loopback mode.\n");
			return ret;
		}
		rt_kprintf("PHY loopback mode enabled.\n");
	} else if (rt_strcmp(argv[3], "off") == 0) {
		/* Disable PHY loopback mode */
		ret = rt_device_control(dev, CMD_DISABLE_PHY_LOOPBACK, RT_NULL);
		if (ret != RT_EOK) {
			rt_kprintf("Failed to disable PHY loopback mode.\n");
			return ret;
		}
		rt_kprintf("PHY loopback mode disabled.\n");
	} else {
		rt_kprintf("Invalid argument for phy-loopback. Use 'on' or 'off'.\n");
		return -RT_ERROR;
	}

	return RT_EOK;
}

static rt_err_t macctl_link_status(rt_device_t dev, int argc, char **argv)
{
	struct eqos_device *eqos;

	if (argc != 3) {
		rt_kprintf("Invalid arguments. Usage: macctl <interface> status\n");
		return -RT_ERROR;
	}

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	eqos = (struct eqos_device *)dev->user_data;
	if (!eqos || !eqos->phy) {
		rt_kprintf("Failed to get PHY device.\n");
		return -RT_ERROR;
	}

	rt_kprintf("link is %s - ", eqos->phy->link ? "up" : "down");
	rt_kprintf("%dMbps/", eqos->phy->speed);
	rt_kprintf("%s\n", (eqos->phy->duplex == 1) ? "full" : "half");

	return RT_EOK;
}

static rt_err_t macctl_phy_regs(rt_device_t dev, int argc, char **argv)
{
	struct eqos_device *eqos_dev;

	if (argc != 3) {
		rt_kprintf("Invalid arguments. Usage: macctl <interface> phy-regs\n");
		return -RT_ERROR;
	}

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	eqos_dev = (struct eqos_device *)dev->user_data;
	if (!eqos_dev || !eqos_dev->phy) {
		rt_kprintf("Failed to get PHY device.\n");
		return -RT_ERROR;
	}

	phy_dump_registers(eqos_dev->phy);

	return RT_EOK;
}

static rt_err_t macctl_test(rt_device_t dev, int argc, char **argv)
{
	const char *test_type;
	rt_err_t ret;

	if (argc < 3) {
		rt_kprintf("Invalid arguments. Usage: macctl <interface> test <test-type>\n");
		return -RT_ERROR;
	}

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	test_type = argv[3];

	if (rt_strcmp(test_type, "reg-access") == 0) {
		ret = reg_access_test(dev);
	} else if (rt_strcmp(test_type, "mac-loopback") == 0) {
		ret = mac_loopback_test(dev);
	} else if (rt_strcmp(test_type, "phy-loopback") == 0) {
		ret = phy_loopback_test(dev);
	} else if (rt_strcmp(test_type, "polling") == 0) {
		int count = 512;
		if (argc == 6 && rt_strcmp(argv[4], "-c") == 0) {
			count = atoi(argv[5]);
		}
		ret = txrx_polling_test(dev, count);
	} else if (rt_strcmp(test_type, "updown") == 0) {
		int count = 512;
		if (argc == 6 && rt_strcmp(argv[4], "-c") == 0) {
			count = atoi(argv[5]);
		}
		/* Turn off the MAC controller first. */
		macctl_down(dev, argc, argv);
		ret = updown_stress_test(dev, count);
	} else {
		rt_kprintf("Unknown test type: %s\n", test_type);
		return -RT_ERROR;
	}

	if (ret == RT_EOK) {
		rt_kprintf("%s test passed.\n", test_type);
	} else {
		rt_kprintf("%s test failed.\n", test_type);
	}

	return ret;
}

static rt_err_t macctl_send_then_recv(rt_device_t dev, int argc, char **argv)
{
	struct eth_hdr eth_hdr;
	rt_size_t tx_buf_len;
	rt_uint8_t *tx_buf;
	rt_size_t tx_pkt_len;
	rt_size_t rx_pkt_len;
	rt_uint8_t rx_buf[RX_BUF_SIZE];
	rt_int32_t timeout;
	rt_err_t ret;
	rt_uint64_t start_time, end_time;
	int i, max_loops = 10000;

	if (!(dev->open_flag & RT_DEVICE_OFLAG_OPEN)) {
		rt_kprintf("Device not opened. Please use 'macctl <interface> up' to open the device first.\n");
		return -RT_ERROR;
	}

	if (argc != 5 || rt_strcmp(argv[3], "-b") != 0) {
		rt_kprintf("Invalid arguments. Usage: macctl <interface> send-recv -b <size>\n");
		return -RT_ERROR;
	}

	tx_buf_len = atoi(argv[4]);
	if (tx_buf_len < 64 || tx_buf_len > RX_BUF_SIZE) {
		rt_kprintf("Invalid packet size: %d. Size should be between 64 and %d.\n", tx_buf_len, RX_BUF_SIZE);
		return -RT_ERROR;
	}

	if (!link_is_active(dev)) {
		rt_kprintf("%s: link is not active!\n", __func__);
		return -RT_ERROR;
	}

	tx_buf = (rt_uint8_t *)rt_malloc(tx_buf_len);
	if (!tx_buf) {
		rt_kprintf("Failed to allocate TX buffer!\n");
		return -RT_ENOMEM;
	}

	rt_memset(&eth_hdr, 0, sizeof(eth_hdr));
	rt_memcpy(eth_hdr.dest_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);
	rt_memcpy(eth_hdr.src_mac, "\x02\x11\x22\x33\x44\x55", 6);
	eth_hdr.ethertype = 0xa488;

	rt_memcpy(tx_buf, &eth_hdr, ETH_HEADER_SIZE);
	rt_memset(tx_buf + ETH_HEADER_SIZE, 0, tx_buf_len - ETH_HEADER_SIZE);

	++spacemit_magic_number;
	set_pkt_magic(tx_buf, tx_buf_len, spacemit_magic_number);

	ret = rt_device_control(dev, CMD_ENABLE_PHY_LOOPBACK, RT_NULL);
	if (ret != RT_EOK) {
		rt_kprintf("Failed to enable PHY loopback mode.\n");
		goto free_pkt;
	}

	start_time = SysTimer_GetLoadValue();

	/* send loop */
	timeout = 1000;
	while (timeout > 0) {
		tx_pkt_len = rt_device_write(dev, 0, tx_buf, tx_buf_len);
		/* Send successful */
		if (tx_pkt_len == tx_buf_len) {
			ret = RT_EOK;
			break;
		}

		--timeout;
	}

	if (timeout == 0) {
		ret = -RT_ETIMEOUT;
		rt_kprintf("Send error!\n", __func__);
		goto disable_phyloopback;
	}

	/* recv loop */
	timeout = 10000;
	while (timeout > 0) {
		rx_pkt_len = rt_device_read(dev, 0, rx_buf, RX_BUF_SIZE);
		if (rx_pkt_len > 0) {
			if (verify_pkt_magic(rx_buf, rx_pkt_len, spacemit_magic_number)) {
				ret = RT_EOK;
				break;
			}
			continue;
		}

		--timeout;
	}

	if (timeout == 0) {
		ret = -RT_ETIMEOUT;
		rt_kprintf("Receive timeout\n", __func__);
		goto disable_phyloopback;
	}

	/* End timestamp after receiving */
	end_time = SysTimer_GetLoadValue();  // Convert to nanoseconds

	rt_kprintf("Sent %d bytes. MAC layer delay: %ld ns\n", tx_buf_len, end_time - start_time);

disable_phyloopback:
	if (rt_device_control(dev, CMD_DISABLE_PHY_LOOPBACK, RT_NULL) != RT_EOK) {
		rt_kprintf("Failed to disable PHY loopback mode.\n");
	}
free_pkt:
	rt_free(tx_buf);
	return ret;
}

typedef struct {
	const char *command;
	rt_err_t (*handler)(rt_device_t dev, int argc, char **argv);
} command_map_t;

static command_map_t command_map[] = {
	{"up", macctl_up},
	{"down", macctl_down},
	{"recv", macctl_recv},
	{"send", macctl_send},
	{"get-mac", macctl_get_mac},
	{"set-mac", macctl_set_mac},
	{"mac-loopback", macctl_mac_loopback},
	{"phy-loopback", macctl_phy_loopback},
	{"link-status", macctl_link_status},
	{"phy-regs", macctl_phy_regs},
	{"test", macctl_test},
	{"send-recv", macctl_send_then_recv},
};

/* macctl command function */
static int macctl(int argc, char **argv)
{
	rt_device_t dev;
	const char *command;

	if (argc < 2) {
		rt_kprintf("Usage: macctl <interface> <command> [options]\n");
		return -RT_ERROR;
	}

	if (rt_strcmp(argv[1], "help") == 0) {
		rt_kprintf("Usage: macctl <interface> <command> [options]\n\n");
		rt_kprintf("macctl - A tool for controlling and testing network interfaces.\n\n");
		rt_kprintf("Commands:\n");
		rt_kprintf("  up                      - Bring the interface up\n");
		rt_kprintf("  down                    - Bring the interface down\n");
		rt_kprintf("  recv [option1]          - Receive packets (default: 1 packet)\n");
		rt_kprintf("  send                    - Send a special packets\n");
		rt_kprintf("  get-mac                 - Show current MAC address\n");
		rt_kprintf("  set-mac <new-mac>       - Set a new MAC address (format: XX:XX:XX:XX:XX:XX)\n");
		rt_kprintf("  mac-loopback <on/off>   - Enable or disable MAC loopback mode\n");
		rt_kprintf("  phy-loopback <on/off>   - Enable or disable PHY loopback mode\n");
		rt_kprintf("  link-status             - Print link status\n");
		rt_kprintf("  phy-regs                - Print PHY register values\n");
		rt_kprintf("  test <test-type>        - Run a specified test\n");
		rt_kprintf("    reg-access            - Test register access\n");
		rt_kprintf("    mac-loopback          - MAC loopback test\n");
		rt_kprintf("    phy-loopback          - PHY loopback test\n");
		rt_kprintf("    polling [option1]     - Perform polling test (default: 512 packets)\n");
		rt_kprintf("    updown [option1]      - Perform up/down test (default: 512 packets)\n");
		rt_kprintf("  send-recv [option2]     - Measure PHY-loopback time\n");
		rt_kprintf("option1:\n");
		rt_kprintf("  -c <count>              - Number of packets to receive or iterations for tests\n");
		rt_kprintf("option2:\n");
		rt_kprintf("  -b <size>               - Packet size for send/recv (default: 512 bytes)\n");
		return RT_EOK;
	}

	dev = rt_device_find(argv[1]);
	if (!dev) {
		rt_kprintf("Failed to find device: %s\n", argv[1]);
		return -RT_ERROR;
	}

	if (argc == 2) {
		rt_kprintf("Usage: macctl <interface> <command> [options]\n");
		return -RT_ERROR;
	}

	command = argv[2];

	for (int i = 0; i < sizeof(command_map) / sizeof(command_map[0]); i++) {
		if (rt_strcmp(command, command_map[i].command) == 0) {
			return command_map[i].handler(dev, argc, argv);
		}
	}

	rt_kprintf("Invalid command: %s\n", command);
	return -RT_ERROR;
}
MSH_CMD_EXPORT(macctl, a gmac tool for debug or selftest. e.g: macctl());
