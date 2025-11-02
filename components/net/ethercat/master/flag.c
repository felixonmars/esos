/*****************************************************************************
 *
 *  Copyright (C) 2021  Florian Pose, Ingenieurgemeinschaft IgH
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

/** \file
 * Slave Configuration Feature Flag.
 */

/****************************************************************************/

#include "base.h"

#include "flag.h"

/****************************************************************************/

/** SDO request constructor.
 */
int ec_flag_init(
	ec_flag_t *flag, /**< Feature flag. */
	const char *key, /**< Feature key. */
	int32_t value /**< Feature value. */
	)
{
	if (!key || rt_strlen(key) == 0) {
		return -EINVAL;
	}

	flag->key = (char *) rt_malloc(rt_strlen(key) + 1);
	if (!flag->key) {
		return -ENOMEM;
	}

	rt_strncpy((char *)flag->key, key, rt_strlen(key) + 1);
	flag->value = value;
	return 0;
}

/****************************************************************************/

/** SDO request destructor.
 */
void ec_flag_clear(
	ec_flag_t *flag /**< Feature flag. */
	)
{
	if (flag->key) {
		rt_free(flag->key);
		flag->key = NULL;
	}
}

/****************************************************************************/
