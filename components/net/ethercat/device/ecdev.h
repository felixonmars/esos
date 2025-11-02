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

#ifndef __ECDEV_H__
#define __ECDEV_H__

#include "mini_netdev.h"
#include "../master/base.h"

/****************************************************************************/

struct ec_device;
typedef struct ec_device ec_device_t; /**< \see ec_device */

struct ethhdr {
	uint8_t h_dest[6];
	uint8_t h_source[6];
	uint16_t h_proto;
};

/** Device poll function type.
 */
typedef void (*ec_pollfunc_t)(struct mini_netdev *);

/*****************************************************************************
 * Offering/withdrawal functions
 ****************************************************************************/

ec_device_t *ecdev_offer(struct mini_netdev *net_dev, ec_pollfunc_t poll);
void ecdev_withdraw(ec_device_t *device);

/*****************************************************************************
 * Device methods
 ****************************************************************************/

int ecdev_open(ec_device_t *device);
void ecdev_close(ec_device_t *device);
void ecdev_receive(ec_device_t *device, const void *data, size_t size);
void ecdev_set_link(ec_device_t *device, uint8_t state);
uint8_t ecdev_get_link(const ec_device_t *device);

/****************************************************************************/

#endif
