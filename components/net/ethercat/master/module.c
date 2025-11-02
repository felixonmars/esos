/*****************************************************************************
 *
 *  Copyright (C) 2006-2008  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  -------------------------------------------------------------------------
 *
 *  Copyright (c) 2025, Spacemit Corporation
 *
 *  This file is the RT-Thread version developed from the original files.
 *
 *  SPDX-License-Identifier: GPL-2.0
 *
 ****************************************************************************/

/**
 * \file
 * EtherCAT datagram structure.
 */

/****************************************************************************/

#include <dtb_node.h>
#include <rtdef.h>
#include "globals.h"
#include "master.h"
#include "device.h"

/****************************************************************************/

static uint32_t master_count; /**< Number of masters. */
static uint32_t debug_level;  /**< Debug level parameter. */
static uint32_t run_on_cpu = 0xffffffff; /**< Bind created kernel threads
					   * to a cpu. Default do not bind.
					   */
static ec_master_t *masters; /**< Array of masters. */
static semaphore master_sem; /**< Master semaphore. */

static uint8_t macs[EC_MAX_MASTERS][2][ETH_ALEN]; /**< MAC addresses. */

char *ec_master_version_str = EC_MASTER_VERSION; /**< Version string. */

/****************************************************************************/

int ec_parse_mac_address(
	struct dtb_node *eth_node, /** EtherCAT device node */
	uint8_t *mac_addr /** Output buffer for MAC address */
	)
{
	int mac_len;
	const void *mac;

	mac = dtb_node_get_property(eth_node, "mac-address", &mac_len);
	if (!mac) {
		EC_ERR("MAC address not found for node: %s.\n", eth_node->name);
		return -EINVAL;
	}

	if (mac_len != ETH_ALEN) {
		EC_ERR("Invalid MAC address for node: %s.\n", eth_node->name);
		return -EINVAL;
	}

	rt_memcpy(mac_addr, mac, ETH_ALEN);

	return 0;
}

static int ec_parse_eth_node(
	struct dtb_node *eth_node, /** EtherCAT device node */
	uint32_t device_id,  /** ID of the EtherCAT device to bind to */
	const char *mode /** device mode */
	)
{
	int ret;

	if (device_id >= master_count) {
		EC_ERR("Device id must be between 0 and %d.\n", master_count - 1);
		return -EINVAL;
	}

	if (rt_strcmp(mode, "ec_main") == 0) {
		ret = ec_parse_mac_address(eth_node, macs[device_id][0]);
		if (ret)
			return ret;
	} else if (rt_strcmp(mode, "ec_backup") == 0) {
		ret = ec_parse_mac_address(eth_node, macs[device_id][1]);
		if (ret)
			return ret;
	} else {
		rt_kprintf("Unknown ethercat device mode '%s'\n", mode);
		return -EINVAL;
	}
	return 0;
}

int ec_check_config(void)
{
	if (master_count > EC_MAX_MASTERS) {
		EC_ERR("master-count must be between 0 and %d\n", EC_MAX_MASTERS);
		return -EINVAL;
	}

	if (debug_level > 2) {
		EC_ERR("debug-level must be between 0 and 2");
		return -EINVAL;
	}

	return 0;
}

static int ec_parse_dt(
	struct dtb_node *master_node  /** EtherCAT master node */
	)
{
	struct dtb_node *child;
	struct dtb_node *eth_node;
	int ret;
	int i;

	if (dtb_node_read_u32(master_node, "master-count", &master_count)) {
		rt_kprintf("Missing master-count\n");
		return -EINVAL;
	}

	if (dtb_node_read_u32(master_node, "debug-level", &debug_level)) {
		rt_kprintf("Missing debug-level\n");
		return -EINVAL;
	}

	ret = ec_check_config();
	if (ret < 0)
		return -EINVAL;

	child = dtb_node_first_subnode(master_node);
	for (i = 0; i < master_count; i++) {
		if (!child) {
			rt_kprintf("Missing master@%d subnode\n", i);
			return -EINVAL;
		}

		eth_node = dtb_node_parse_phandle(child, "main-device", 0);
		if (!eth_node) {
			rt_kprintf("Missing main-device in master@%d\n", i);
			return -EINVAL;
		}

		ret = ec_parse_eth_node(eth_node, i, "ec_main");
		if (ret) {
			rt_kprintf("Failed to parse main-device for master %d\n", i);
			return -EINVAL;
		}

		eth_node = dtb_node_parse_phandle(child, "backup-device", 0);
		if (eth_node) {
			ret = ec_parse_eth_node(eth_node, i, "ec_backup");
			if (ret) {
				rt_kprintf("Failed to parse backup-device for master %d\n", i);
				return -EINVAL;
			}
		}

		child = dtb_node_next_subnode(child);
	}

	return 0;
}

int ec_master_probe(void)
{
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	static char *compatibe = "spacemit,k3-ec-master";
	struct dtb_node *node;
	int ret;
	int i;

	EC_INFO("Master driver %s\n", EC_MASTER_VERSION);
	master_sem = ec_sema_init(1);

	node = dtb_node_find_compatible_node(dtb_head_node, compatibe);
	if (!node)
		return 0;

	if (!dtb_node_device_is_available(node))
		return 0;

	rt_memset(macs, 0x00, sizeof(uint8_t) * EC_MAX_MASTERS * 2 * ETH_ALEN);

	ret = ec_parse_dt(node);
	if (ret) {
		EC_ERR("Failed to parse device tree.\n");
		master_count = 0;
	}
	// initialize static master variables
	ec_master_init_static();

	if (master_count) {
		masters = rt_malloc(sizeof(ec_master_t) * master_count);
		if (!masters) {
			EC_ERR("Failed to allocate memory for EtherCAT masters.\n");
			ret = -ENOMEM;
			goto out_return;
		}
	}

	for (i = 0; i < master_count; i++) {
		ret = ec_master_init(&masters[i], i, macs[i][0], macs[i][1],
				debug_level, run_on_cpu);
		if (ret)
			goto out_free_masters;
	}

	EC_INFO("%u master%s waiting for devices.\n",
		master_count, (master_count > 1 ? "" : "s"));
	return ret;

out_free_masters:
	for (i--; i >= 0; i--)
		ec_master_clear(&masters[i]);
	rt_free(masters);
out_return:
	return ret;
}
INIT_PREV_EXPORT(ec_master_probe);

/****************************************************************************/

/** Module cleanup.
 *
 * Clears all master instances.
 */
void ec_master_remove(void)
{
	unsigned int i;

	for (i = 0; i < master_count; i++)
		ec_master_clear(&masters[i]);

	if (master_count)
		rt_free(masters);

	EC_INFO("Master module cleaned up.\n");
}

/****************************************************************************/

/** Get the number of masters.
 */
uint32_t ec_master_count(void)
{
	return master_count;
}

/*****************************************************************************
 * MAC address functions
 ****************************************************************************/

/**
 * \return true, if two MAC addresses are equal.
 */
int ec_mac_equal(
	const uint8_t *mac1, /**< First MAC address. */
	const uint8_t *mac2 /**< Second MAC address. */
	)
{
	unsigned int i;

	for (i = 0; i < ETH_ALEN; i++)
		if (mac1[i] != mac2[i])
			return 0;

	return 1;
}

/****************************************************************************/

/** Maximum MAC string size.
 */
#define EC_MAX_MAC_STRING_SIZE (3 * ETH_ALEN)

/** Print a MAC address to a buffer.
 *
 * The buffer size must be at least EC_MAX_MAC_STRING_SIZE.
 *
 * \return number of bytes written.
 */
int ec_mac_print(
	const uint8_t *mac, /**< MAC address */
	char *buffer /**< Target buffer. */
	)
{
	int32_t off = 0;
	int i;

	for (i = 0; i < ETH_ALEN; i++) {
		off += rt_sprintf(buffer + off, "%02X", mac[i]);
		if (i < ETH_ALEN - 1)
			off += rt_sprintf(buffer + off, ":");
	}

	return off;
}

/****************************************************************************/

/**
 * \return true, if the MAC address is all-zero.
 */
int ec_mac_is_zero(
	const uint8_t *mac /**< MAC address. */
	)
{
	unsigned int i;

	for (i = 0; i < ETH_ALEN; i++)
		if (mac[i])
			return 0;

	return 1;
}

/****************************************************************************/

/**
 * \return true, if the given MAC address is the broadcast address.
 */
int ec_mac_is_broadcast(
	const uint8_t *mac /**< MAC address. */
	)
{
	unsigned int i;

	for (i = 0; i < ETH_ALEN; i++)
		if (mac[i] != 0xff)
			return 0;

	return 1;
}

/****************************************************************************/

/** Outputs frame contents for debugging purposes.
 * If the data block is larger than 256 bytes, only the first 128
 * and the last 128 bytes will be shown
 */
void ec_print_data(const uint8_t *data, /**< pointer to data */
		   size_t size /**< number of bytes to output */
		   )
{
	unsigned int i;

	EC_DBG("");
	for (i = 0; i < size; i++) {
		rt_kprintf("%02X ", data[i]);

		if ((i + 1) % 16 == 0 && i < size - 1) {
			rt_kprintf("\n");
			EC_DBG("");
		}

		if (i + 1 == 128 && size > 256) {
			rt_kprintf("dropped %u bytes\n", size - 128 - i);
			i = size - 128;
			EC_DBG("");
		}
	}

	rt_kprintf("\n");
}

/****************************************************************************/

/** Outputs frame contents and differences for debugging purposes.
 */
void ec_print_data_diff(
	const uint8_t *d1, /**< first data */
	const uint8_t *d2, /**< second data */
	size_t size /** number of bytes to output */
	)
{
	unsigned int i;

	EC_DBG("");
	for (i = 0; i < size; i++) {
		if (d1[i] == d2[i])
			rt_kprintf(".. ");
		else
			rt_kprintf("%02X ", d2[i]);

		if ((i + 1) % 16 == 0) {
			rt_kprintf("\n");
			EC_DBG("");
		}
	}

	rt_kprintf("\n");
}

/****************************************************************************/

/** Prints slave states in clear text.
 *
 * \return Size of the created string.
 */
size_t ec_state_string(
	uint8_t states, /**< slave states */
	char *buffer, /**< target buffer
			(min. EC_STATE_STRING_SIZE bytes) */
	uint8_t multi /**< Show multi-state mask. */
	)
{
	int32_t off = 0;
	unsigned int first = 1;

	if (!states) {
		off += rt_sprintf(buffer + off, "(unknown)");
		return off;
	}

	if (multi) { // multiple slaves
		if (states & EC_SLAVE_STATE_INIT) {
			off += rt_sprintf(buffer + off, "INIT");
			first = 0;
		}
		if (states & EC_SLAVE_STATE_PREOP) {
			if (!first)
				off += rt_sprintf(buffer + off, ", ");
			off += rt_sprintf(buffer + off, "PREOP");
			first = 0;
		}
		if (states & EC_SLAVE_STATE_SAFEOP) {
			if (!first)
				off += rt_sprintf(buffer + off, ", ");
			off += rt_sprintf(buffer + off, "SAFEOP");
			first = 0;
		}
		if (states & EC_SLAVE_STATE_OP) {
			if (!first)
				off += rt_sprintf(buffer + off, ", ");
			off += rt_sprintf(buffer + off, "OP");
		}
	} else { // single slave
		if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_INIT)
			off += rt_sprintf(buffer + off, "INIT");
		else if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_PREOP)
			off += rt_sprintf(buffer + off, "PREOP");
		else if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_BOOT)
			off += rt_sprintf(buffer + off, "BOOT");
		else if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_SAFEOP)
			off += rt_sprintf(buffer + off, "SAFEOP");
		else if ((states & EC_SLAVE_STATE_MASK) == EC_SLAVE_STATE_OP)
			off += rt_sprintf(buffer + off, "OP");
		else
			off += rt_sprintf(buffer + off, "(invalid)");

		first = 0;
	}

	if (states & EC_SLAVE_STATE_ACK_ERR) {
		if (!first)
			off += rt_sprintf(buffer + off, " + ");
		off += rt_sprintf(buffer + off, "ERROR");
	}

	return (size_t)off;
}

/*****************************************************************************
 *  Device interface
 ****************************************************************************/

/** Device names.
 */
const char *ec_device_names[2] = {
	"main",
	"backup"
};

/** Offers an EtherCAT device to a certain master.
 *
 * The master decides, if it wants to use the device for EtherCAT operation,
 * or not. It is important, that the offered net_device is not used by the
 * kernel IP stack. If the master, accepted the offer, the address of the
 * newly created EtherCAT device is returned, else \a NULL is returned.
 *
 * \return Pointer to device, if accepted, or NULL if declined.
 * \ingroup DeviceInterface
 */
ec_device_t *ecdev_offer(
	struct mini_netdev *net_dev, /**< net_device to offer */
	ec_pollfunc_t poll /**< device poll function */
	)
{
	ec_master_t *master;
	char str[EC_MAX_MAC_STRING_SIZE];
	unsigned int i, dev_idx;

	for (i = 0; i < master_count; i++) {
		master = &masters[i];
		ec_mac_print(net_dev->mac_addr, str);

		if (down_interruptible(master->device_sem)) {
			EC_MASTER_WARN(master, "%s() interrupted!\n", __func__);
			return NULL;
		}

		for (dev_idx = EC_DEVICE_MAIN;
			    dev_idx < ec_master_num_devices(master); dev_idx++) {
			if (!master->devices[dev_idx].dev
			    && (ec_mac_equal(master->macs[dev_idx], net_dev->mac_addr)
				|| ec_mac_is_broadcast(master->macs[dev_idx]))) {

				EC_INFO("Accepting %s as %s device for master %u.\n",
					str, ec_device_names[dev_idx != 0], master->index);

				ec_device_attach(&master->devices[dev_idx],
						net_dev, poll);
				up(master->device_sem);

				rt_snprintf(net_dev->name, sizeof(net_dev->name), "ec%c%u",
					ec_device_names[dev_idx != 0][0], master->index);

				return &master->devices[dev_idx]; // offer accepted
			}
		}

		up(master->device_sem);

		EC_MASTER_DBG(master, 1, "Master declined device %s.\n", str);
	}

	return NULL; // offer declined
}

/*****************************************************************************
 * Application interface
 ****************************************************************************/

/** Request a master.
 *
 * Same as ecrt_request_master(), but with err_to_ptr() return value.
 *
 * \return Requested master.
 */
ec_master_t *ecrt_request_master_err(
	unsigned int master_index /**< Master index. */
	)
{
	ec_master_t *master, *errptr = NULL;
	unsigned int dev_idx = EC_DEVICE_MAIN;

	EC_INFO("Requesting master %u...\n", master_index);

	if (master_index >= master_count) {
		EC_ERR("Invalid master index %u.\n", master_index);
		errptr = err_to_ptr(-EINVAL);
		goto out_return;
	}
	master = &masters[master_index];

	if (down_interruptible(master_sem)) {
		errptr = err_to_ptr(-EINTR);
		goto out_return;
	}

	if (master->reserved) {
		up(master_sem);
		EC_MASTER_ERR(master, "Master already in use!\n");
		errptr = err_to_ptr(-EBUSY);
		goto out_return;
	}
	master->reserved = 1;
	up(master_sem);

	if (down_interruptible(master->device_sem)) {
		errptr = err_to_ptr(-EINTR);
		goto out_release;
	}

	if (master->phase != EC_IDLE) {
		up(master->device_sem);
		EC_MASTER_ERR(master, "Master still waiting for devices!\n");
		errptr = err_to_ptr(-ENODEV);
		goto out_release;
	}

	for (; dev_idx < ec_master_num_devices(master); dev_idx++) {
		ec_device_t *device = &master->devices[dev_idx];
		/*
		 * In RT-Thread, the GMAC driver is a permanent system module,
		 * dedicated exclusively to EtherCAT. Thus, `try_module_get` is not
		 * needed. We replace it with a check on `device->dev` to ensure the
		 * device is available without changing functionality.
		 */
		if (!device->dev) {
			up(master->device_sem);
			EC_MASTER_ERR(master, "Device module is unloading!\n");
			errptr = err_to_ptr(-ENODEV);
			goto out_release;
		}
	}

	up(master->device_sem);

	if (ec_master_enter_operation_phase(master)) {
		EC_MASTER_ERR(master, "Failed to enter OPERATION phase!\n");
		errptr = err_to_ptr(-EIO);
		goto out_release;
	}

	EC_INFO("Successfully requested master %u.\n", master_index);
	return master;

out_release:
	master->reserved = 0;
out_return:
	return errptr;
}

/****************************************************************************/

ec_master_t *ecrt_request_master(unsigned int master_index)
{
	ec_master_t *master = ecrt_request_master_err(master_index);

	return ptr_is_err(master) ? NULL : master;
}

/****************************************************************************/

void ecrt_release_master(ec_master_t *master)
{
	unsigned int dev_idx;

	EC_MASTER_INFO(master, "Releasing master...\n");

	if (!master->reserved) {
		EC_MASTER_WARN(master, "%s(): Master was not requested!\n",
			       __func__);
		return;
	}

	ec_master_leave_operation_phase(master);

	master->reserved = 0;

	EC_MASTER_INFO(master, "Released.\n");
}

/****************************************************************************/

unsigned int ecrt_version_magic(void)
{
	return ECRT_VERSION_MAGIC;
}

/****************************************************************************/

/** Global request state type translation table.
 *
 * Translates an internal request state to an external one.
 */
const ec_request_state_t ec_request_state_translation_table[] = {
	EC_REQUEST_UNUSED,  // EC_INT_REQUEST_INIT,
	EC_REQUEST_BUSY,    // EC_INT_REQUEST_QUEUED,
	EC_REQUEST_BUSY,    // EC_INT_REQUEST_BUSY,
	EC_REQUEST_SUCCESS, // EC_INT_REQUEST_SUCCESS,
	EC_REQUEST_ERROR    // EC_INT_REQUEST_FAILURE
};

/****************************************************************************/
