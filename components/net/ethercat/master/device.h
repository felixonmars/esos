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

#ifndef __EC_DEVICE_H__
#define __EC_DEVICE_H__

#include <mini_netdev.h>
#include "../device/ecdev.h"
#include "globals.h"

/**
 * Size of the transmit ring.
 * This memory ring is used to transmit frames. It is necessary to use
 * different memory regions, because otherwise the network device DMA could
 * send the same data twice, if it is called twice.
 */
#define EC_TX_RING_SIZE 2

/****************************************************************************/

/**
 * EtherCAT device.
 * An EtherCAT device is a network interface card, that is owned by an
 * EtherCAT master to send and receive EtherCAT frames with.
 */

struct ec_device {
	ec_master_t *master; /**< EtherCAT master */
	struct mini_netdev *dev; /**< pointer to the assigned net_device */
	ec_pollfunc_t poll; /**< pointer to the device's poll function */

	uint8_t open; /**< true, if the net_device has been opened */
	uint8_t link_state; /**< device link state */
	void *tx_buf[EC_TX_RING_SIZE]; /**< transmit skb ring */
	unsigned int tx_ring_index; /**< last ring entry used to transmit */
#ifdef EC_HAVE_CYCLES
	uint64_t cycles_poll; /**< cycles of last poll */
#endif
	unsigned long jiffies_poll; /**< jiffies of last poll */

	// Frame statistics
	uint64_t tx_count; /**< Number of frames sent. */
	uint64_t last_tx_count; /**< Number of frames sent of last statistics cycle. */
	uint64_t rx_count; /**< Number of frames received. */
	uint64_t last_rx_count; /**< Number of frames received of last statistics cycle. */

	uint64_t tx_bytes; /**< Number of bytes sent. */
	uint64_t last_tx_bytes; /**< Number of bytes sent of last statistics cycle. */
	uint64_t rx_bytes; /**< Number of bytes received. */
	uint64_t last_rx_bytes; /**< Number of bytes received of last statistics cycle. */

	uint64_t tx_errors; /**< Number of transmit errors. */
	int32_t tx_frame_rates[EC_RATE_COUNT]; /*< Transmit rates in frames/s for
						* different statistics cycle periods.
						*/
	int32_t rx_frame_rates[EC_RATE_COUNT]; /*< Receive rates in frames/s for
						* different statistics cycle periods.
						*/
	int32_t tx_byte_rates[EC_RATE_COUNT];  /*< Transmit rates in byte/s for
						* different statistics cycle periods.
						*/
	int32_t rx_byte_rates[EC_RATE_COUNT];  /*< Receive rates in byte/s for
						* different statistics cycle periods
						*/
};

/****************************************************************************/

int ec_device_init(ec_device_t *device, ec_master_t *master);
void ec_device_clear(ec_device_t *device);

void ec_device_attach(ec_device_t *device, struct mini_netdev *net_dev, ec_pollfunc_t poll);
void ec_device_detach(ec_device_t *device);

int ec_device_open(ec_device_t *device);
int ec_device_close(ec_device_t *device);

void ec_device_poll(ec_device_t *device);
uint8_t *ec_device_tx_data(ec_device_t *device);
void ec_device_send(ec_device_t *device, size_t size);
void ec_device_clear_stats(ec_device_t *device);
void ec_device_update_stats(ec_device_t *device);

/****************************************************************************/

#endif
