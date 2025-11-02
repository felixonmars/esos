/*****************************************************************************
 *
 *  Copyright (C) 2006-2024  Florian Pose, Ingenieurgemeinschaft IgH
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
 * EtherCAT slave configuration structure.
 */

/****************************************************************************/

#ifndef __EC_SLAVE_CONFIG_H__
#define __EC_SLAVE_CONFIG_H__

#include "base.h"

#include "globals.h"
#include "slave.h"
#include "sync_config.h"
#include "fmmu_config.h"
#include "coe_emerg_ring.h"
#include "flag.h"

/****************************************************************************/

#define EC_CONFIG_INFO(sc, fmt, args...) \
	rt_kprintf("EtherCAT %u %u:%u: " fmt, sc->master->index, \
	    sc->alias, sc->position, ##args)

#define EC_CONFIG_ERR(sc, fmt, args...) \
	rt_kprintf("EtherCAT ERROR %u %u:%u: " fmt, sc->master->index, \
	    sc->alias, sc->position, ##args)

#define EC_CONFIG_WARN(sc, fmt, args...) \
	rt_kprintf("EtherCAT WARNING %u %u:%u: " fmt, \
	    sc->master->index, sc->alias, sc->position, ##args)

#define EC_CONFIG_DBG(sc, level, fmt, args...) \
	do { \
		if (sc->master->debug_level >= level) { \
			rt_kprintf("EtherCAT DEBUG %u %u:%u: " fmt, \
			    sc->master->index, sc->alias, sc->position, ##args); \
		} \
	} while (0)

/****************************************************************************/

/** EtherCAT slave configuration.
 */
struct ec_slave_config {
	list_head list; /**< List item. */
	ec_master_t *master; /**< Master owning the slave configuration. */

	uint16_t alias; /**< Slave alias. */
	uint16_t position; /**< Index after alias. If alias is zero, this is the
			     ring position. */
	uint32_t vendor_id; /**< Slave vendor ID. */
	uint32_t product_code; /**< Slave product code. */

	uint16_t watchdog_divider; /**< Watchdog divider as a number of 40ns
				     intervals (see spec. reg. 0x0400). */
	uint16_t watchdog_intervals; /**< Process data watchdog intervals (see
				       spec. reg. 0x0420). */

	ec_slave_t *slave; /**< Slave pointer. This is \a NULL, if the slave is
			     offline. */

	ec_sync_config_t sync_configs[EC_MAX_SYNC_MANAGERS]; /**< Sync manager
							       configurations. */
	ec_fmmu_config_t fmmu_configs[EC_MAX_FMMUS]; /**< FMMU configurations. */
	uint8_t used_fmmus; /**< Number of FMMUs used. */
	uint16_t dc_assign_activate; /**< Vendor-specific AssignActivate word. */
	ec_sync_signal_t dc_sync[EC_SYNC_SIGNAL_COUNT]; /**< DC sync signals. */

	list_head sdo_configs; /**< List of SDO configurations. */
	list_head sdo_requests; /**< List of SDO requests. */
	list_head soe_requests; /**< List of SoE requests. */
	list_head voe_handlers; /**< List of VoE handlers. */
	list_head reg_requests; /**< List of register requests. */
	list_head soe_configs; /**< List of SoE configurations. */
	list_head flags; /**< List of feature flags. */
	list_head al_timeouts; /**< List of specific AL state timeouts. */

	ec_coe_emerg_ring_t emerg_ring; /**< CoE emergency ring buffer. */
};

/****************************************************************************/

void ec_slave_config_init(ec_slave_config_t *, ec_master_t *, uint16_t,
	uint16_t, uint32_t, uint32_t);
void ec_slave_config_clear(ec_slave_config_t *);

int ec_slave_config_attach(ec_slave_config_t *);
void ec_slave_config_detach(ec_slave_config_t *);

void ec_slave_config_load_default_sync_config(ec_slave_config_t *);

unsigned int ec_slave_config_sdo_count(const ec_slave_config_t *);
const ec_sdo_request_t *ec_slave_config_get_sdo_by_pos_const(
	const ec_slave_config_t *, unsigned int);
unsigned int ec_slave_config_idn_count(const ec_slave_config_t *);
const ec_soe_request_t *ec_slave_config_get_idn_by_pos_const(
	const ec_slave_config_t *, unsigned int);
unsigned int ec_slave_config_flag_count(const ec_slave_config_t *);
const ec_flag_t *ec_slave_config_get_flag_by_pos_const(
	const ec_slave_config_t *, unsigned int);
ec_sdo_request_t *ec_slave_config_find_sdo_request(ec_slave_config_t *,
	unsigned int);
ec_soe_request_t *ec_slave_config_find_soe_request(ec_slave_config_t *,
	unsigned int);
ec_reg_request_t *ec_slave_config_find_reg_request(ec_slave_config_t *,
	unsigned int);
ec_voe_handler_t *ec_slave_config_find_voe_handler(ec_slave_config_t *,
	unsigned int);
ec_flag_t *ec_slave_config_find_flag(ec_slave_config_t *, const char *);

ec_sdo_request_t *ecrt_slave_config_create_sdo_request_err(
	ec_slave_config_t *, uint16_t, uint8_t, size_t);
ec_soe_request_t *ecrt_slave_config_create_soe_request_err(
	ec_slave_config_t *, uint8_t, uint16_t, size_t);
ec_voe_handler_t *ecrt_slave_config_create_voe_handler_err(
	ec_slave_config_t *, size_t);
ec_reg_request_t *ecrt_slave_config_create_reg_request_err(
	ec_slave_config_t *, size_t);

unsigned int ec_slave_config_al_timeout(const ec_slave_config_t *,
	ec_slave_state_t, ec_slave_state_t);

/****************************************************************************/

#endif
