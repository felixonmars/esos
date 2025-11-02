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
//checked by ZengYu on 2025-10-10
/**
 * \file
 * EtherCAT datagram structure.
 */

/****************************************************************************/

#include <mini_netdev.h>
#include "device.h"
#include "master.h"

/****************************************************************************/

uint16_t ec_htons(uint16_t hostshort)
{
	uint16_t result;

#if __BYTE_ORDER == __LITTLE_ENDIAN
	result = (hostshort >> 8) | (hostshort << 8);
#else
	result = hostshort;
#endif

	return result;
}

/** Constructor.
 *
 * \return 0 in case of success, else < 0
 */
int ec_device_init(
	ec_device_t *device, /**< EtherCAT device */
	ec_master_t *master /**< master owning the device */
	)
{
	int ret;
	unsigned int i;
	struct ethhdr *eth;

	device->master = master;
	device->dev = NULL;
	device->poll = NULL;

	device->open = 0;
	device->link_state = 0;
	for (i = 0; i < EC_TX_RING_SIZE; i++)
		device->tx_buf[i] = NULL;

	device->tx_ring_index = 0;
#ifdef EC_HAVE_CYCLES
	device->cycles_poll = 0;
#endif
	device->jiffies_poll = 0;

	ec_device_clear_stats(device);

	for (i = 0; i < EC_TX_RING_SIZE; i++) {
		device->tx_buf[i] = rt_malloc_align(ECAT_BUF_SIZE, ARCH_DMA_MINALIGN);
		if (!device->tx_buf[i]) {
			EC_MASTER_ERR(master, "Error allocating device socket buffer!\n");
			ret = -ENOMEM;
			goto out_tx_ring;
		}

		eth = (struct ethhdr *) (device->tx_buf[i]);
		eth->h_proto = ec_htons(0x88A4);
		rt_memset(eth->h_dest, 0xFF, ETH_ALEN);
	}

	return 0;

out_tx_ring:
	for (i = 0; i < EC_TX_RING_SIZE; i++) {
		if (device->tx_buf[i])
			rt_free_align(device->tx_buf[i]);
	}

	return ret;
}

/****************************************************************************/

/** Destructor.
 */
void ec_device_clear(ec_device_t *device)
{
	unsigned int i;

	if (device->open)
		ec_device_close(device);

	for (i = 0; i < EC_TX_RING_SIZE; i++)
		rt_free_align(device->tx_buf[i]);
}

/****************************************************************************/

/** Associate with net_device.
 */
void ec_device_attach(
	ec_device_t *device, /**< EtherCAT device */
	struct mini_netdev *net_dev, /**< net_device structure */
	ec_pollfunc_t poll /**< pointer to device's poll function */
	)
{
	unsigned int i;
	struct ethhdr *eth;

	ec_device_detach(device); // resets fields

	device->dev = net_dev;
	device->poll = poll;

	for (i = 0; i < EC_TX_RING_SIZE; i++) {
		eth = (struct ethhdr *) (device->tx_buf[i]);
		rt_memcpy(eth->h_source, net_dev->mac_addr, ETH_ALEN);
	}
}

/****************************************************************************/

/** Disconnect from net_device.
 */
void ec_device_detach(
	ec_device_t *device /**< EtherCAT device */
	)
{
	unsigned int i;

	device->dev = NULL;
	device->poll = NULL;
	device->open = 0;
	device->link_state = 0; // down

	ec_device_clear_stats(device);
}

/****************************************************************************/

/** Opens the EtherCAT device.
 *
 * \return 0 in case of success, else < 0
 */
int ec_device_open(
	ec_device_t *device /**< EtherCAT device */
	)
{
	int ret;

	if (!device->dev) {
		EC_MASTER_ERR(device->master, "No net_device to open!\n");
		return -ENODEV;
	}

	if (device->open) {
		EC_MASTER_WARN(device->master, "Device already opened!\n");
		return 0;
	}

	device->link_state = 0;

	ec_device_clear_stats(device);

	ret = device->dev->open(device->dev);
	if (!ret)
		device->open = 1;

	return ret;
}

/****************************************************************************/

/** Stops the EtherCAT device.
 *
 * \return 0 in case of success, else < 0
 */
int ec_device_close(
	ec_device_t *device
	)
{
	int ret;

	if (!device->dev) {
		EC_MASTER_ERR(device->master, "No device to close!\n");
		return -ENODEV;
	}

	if (!device->open) {
		EC_MASTER_WARN(device->master, "Device already closed!\n");
		return 0;
	}

	ret = device->dev->close(device->dev);
	if (!ret)
		device->open = 0;

	return ret;
}

/****************************************************************************/

/** Returns a pointer to the device's transmit memory.
 *
 * \return pointer to the TX socket buffer
 */
uint8_t *ec_device_tx_data(
	ec_device_t *device /**< EtherCAT device */
	)
{
	/*
	 * cycle through socket buffers, because otherwise there is a race
	 * condition, if multiple frames are sent and the DMA is not scheduled in
	 * between.
	 */
	device->tx_ring_index++;
	device->tx_ring_index %= EC_TX_RING_SIZE;
	return device->tx_buf[device->tx_ring_index] + ETH_HLEN;
}

/****************************************************************************/

/** Sends the content of the transmit socket buffer.
 *
 * Cuts the socket buffer content to the (now known) size, and calls the
 * start_xmit() function of the assigned net_device.
 */
void ec_device_send(
	ec_device_t *device, /**< EtherCAT device */
	size_t size /**< number of bytes to send */
)
{
	void *buffer = device->tx_buf[device->tx_ring_index];
	// set the right length for the data
	rt_size_t len = (rt_size_t)(ETH_HLEN + size);

	if (unlikely(device->master->debug_level > 1)) {
		EC_MASTER_DBG(device->master, 2, "Sending frame:\n");
		ec_print_data(buffer, ETH_HLEN + size);
	}

	// start sending
	if (device->dev->send(device->dev, buffer, len) == RT_EOK) {
		device->tx_count++;
		device->master->device_stats.tx_count++;
		device->tx_bytes += ETH_HLEN + size;
		device->master->device_stats.tx_bytes += ETH_HLEN + size;
	} else {
		device->tx_errors++;
	}
}

/****************************************************************************/

/** Clears the frame statistics.
 */
void ec_device_clear_stats(ec_device_t *device)
{
	unsigned int i;

	// zero frame statistics
	device->tx_count = 0;
	device->last_tx_count = 0;
	device->rx_count = 0;
	device->last_rx_count = 0;
	device->tx_bytes = 0;
	device->last_tx_bytes = 0;
	device->rx_bytes = 0;
	device->last_rx_bytes = 0;
	device->tx_errors = 0;

	for (i = 0; i < EC_RATE_COUNT; i++) {
		device->tx_frame_rates[i] = 0;
		device->rx_frame_rates[i] = 0;
		device->tx_byte_rates[i] = 0;
		device->rx_byte_rates[i] = 0;
	}
}

/****************************************************************************/

/** Calls the poll function of the assigned net_device.
 *
 * The master itself works without using interrupts. Therefore the processing
 * of received data and status changes of the network device has to be
 * done by the master calling the ISR "manually".
 */
void ec_device_poll(ec_device_t *device)
{
#ifdef EC_HAVE_CYCLES
	device->cycles_poll = get_cycles();
#endif
	device->jiffies_poll = rt_tick_get();

	device->poll(device->dev);
}

/****************************************************************************/

/** Update device statistics.
 */
void ec_device_update_stats(ec_device_t *device)
{
	unsigned int i;

	int32_t tx_frame_rate = (device->tx_count - device->last_tx_count) * 1000;
	int32_t rx_frame_rate = (device->rx_count - device->last_rx_count) * 1000;
	int32_t tx_byte_rate = (device->tx_bytes - device->last_tx_bytes);
	int32_t rx_byte_rate = (device->rx_bytes - device->last_rx_bytes);

	/* Low-pass filter:
	 *      Y_n = y_(n - 1) + T / tau * (x - y_(n - 1))   | T = 1
	 *   -> Y_n += (x - y_(n - 1)) / tau
	 */
	for (i = 0; i < EC_RATE_COUNT; i++) {
		int32_t n = rate_intervals[i];

		device->tx_frame_rates[i] +=
			(tx_frame_rate - device->tx_frame_rates[i]) / n;
		device->rx_frame_rates[i] +=
			(rx_frame_rate - device->rx_frame_rates[i]) / n;
		device->tx_byte_rates[i] +=
			(tx_byte_rate - device->tx_byte_rates[i]) / n;
		device->rx_byte_rates[i] +=
			(rx_byte_rate - device->rx_byte_rates[i]) / n;
	}

	device->last_tx_count = device->tx_count;
	device->last_rx_count = device->rx_count;
	device->last_tx_bytes = device->tx_bytes;
	device->last_rx_bytes = device->rx_bytes;
}

/*****************************************************************************
 *  Device interface
 ****************************************************************************/

/** Withdraws an EtherCAT device from the master.
 *
 * The device is disconnected from the master and all device ressources
 * are freed.
 *
 * \attention Before calling this function, the ecdev_stop() function has
 *            to be called, to be sure that the master does not use the device
 *            any more.
 * \ingroup DeviceInterface
 */
void ecdev_withdraw(ec_device_t *device /**< EtherCAT device */)
{
	ec_master_t *master = device->master;
	char dev_str[20], mac_str[20];

	ec_mac_print(device->dev->mac_addr, mac_str);

	if (device == &master->devices[EC_DEVICE_MAIN]) {
		rt_sprintf(dev_str, "main");
	} else if (device == &master->devices[EC_DEVICE_BACKUP]) {
		rt_sprintf(dev_str, "backup");
	} else {
		EC_MASTER_WARN(master, "%s() called with unknown device %s!\n", __func__, mac_str);
		rt_sprintf(dev_str, "UNKNOWN");
	}

	EC_MASTER_INFO(master, "Releasing %s device %s.\n", dev_str, mac_str);

	down(master->device_sem);
	ec_device_detach(device);
	up(master->device_sem);
}

/****************************************************************************/

/** Opens the network device and makes the master enter IDLE phase.
 *
 * \return 0 on success, else < 0
 * \ingroup DeviceInterface
 */
int ecdev_open(ec_device_t *device /**< EtherCAT device */)
{
	int ret;
	ec_master_t *master = device->master;
	unsigned int all_open = 1, dev_idx;

	ret = ec_device_open(device);
	if (ret) {
		EC_MASTER_ERR(master, "Failed to open device: error %d!\n", ret);
		return ret;
	}

	for (dev_idx = EC_DEVICE_MAIN;
		    dev_idx < ec_master_num_devices(device->master); dev_idx++) {
		if (!master->devices[dev_idx].open) {
			all_open = 0;
			break;
		}
	}

	if (all_open) {
		ret = ec_master_enter_idle_phase(device->master);
		if (ret) {
			EC_MASTER_ERR(device->master, "Failed to enter IDLE phase!\n");
			return ret;
		}
	}

	return 0;
}

/****************************************************************************/

/** Makes the master leave IDLE phase and closes the network device.
 *
 * \return 0 on success, else < 0
 * \ingroup DeviceInterface
 */
void ecdev_close(ec_device_t *device)
{
	ec_master_t *master = device->master;

	if (master->phase == EC_IDLE)
		ec_master_leave_idle_phase(master);

	if (ec_device_close(device))
		EC_MASTER_WARN(master, "Failed to close device!\n");

}

/****************************************************************************/

/** Accepts a received frame.
 *
 * Forwards the received data to the master. The master will analyze the frame
 * and dispatch the received commands to the sending instances.
 *
 * The data have to begin with the Ethernet header (target MAC address).
 *
 * \ingroup DeviceInterface
 */
void ecdev_receive(
	ec_device_t *device, /**< EtherCAT device */
	const void *data, /**< pointer to received data */
	size_t size /**< number of bytes received */
	)
{
	const void *ec_data = data + ETH_HLEN;
	size_t ec_size = (size_t)(size - ETH_HLEN);

	if (unlikely(!data)) {
		EC_MASTER_WARN(device->master, "%s() called with NULL data.\n", __func__);
		return;
	}

	device->rx_count++;
	device->master->device_stats.rx_count++;
	device->rx_bytes += size;
	device->master->device_stats.rx_bytes += size;

	if (unlikely(device->master->debug_level > 1)) {
		EC_MASTER_DBG(device->master, 2, "Received frame:\n");
		ec_print_data(data, size);
	}

	ec_master_receive_datagrams(device->master, device, ec_data, ec_size);
}

/****************************************************************************/

/** Sets a new link state.
 *
 * If the device notifies the master about the link being down, the master
 * will not try to send frames using this device.
 *
 * \ingroup DeviceInterface
 */
void ecdev_set_link(ec_device_t *device, uint8_t state)
{
	if (unlikely(!device)) {
		EC_WARN("%s() called with null device!\n", __func__);
		return;
	}

	if (likely(state != device->link_state)) {
		device->link_state = state;
		EC_MASTER_INFO(device->master,
			"Link state of %s changed to %s.\n",
			device->dev->name, (state ? "UP" : "DOWN"));
	}
}

/****************************************************************************/

/** Reads the link state.
 *
 * \ingroup DeviceInterface
 *
 * \return Link state.
 */
uint8_t ecdev_get_link(
	const ec_device_t *device /**< EtherCAT device */
	)
{
	if (unlikely(!device)) {
		EC_WARN("%s() called with null device!\n", __func__);
		return 0;
	}

	return device->link_state;
}
