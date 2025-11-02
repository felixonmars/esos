/*
 * Copyright (c) 2025, Spacemit Corporation
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 * mini_netdev.h — Minimal network device abstraction for EtherCAT masters
 * Defines the required open/close and send/receive primitives for EtherCAT,
 * with an optional control hook reserved for extensibility.
 *
 */

#ifndef _MINI_NETDEV_H__
#define _MINI_NETDEV_H__

#include <rtdef.h>

/* Network device */
struct mini_netdev {
	char name[RT_NAME_MAX];
	rt_uint8_t mac_addr[6];
	/* Network device operations */
	rt_err_t (*open)(struct mini_netdev *netdev);
	rt_err_t (*close)(struct mini_netdev *netdev);
	rt_err_t (*send)(struct mini_netdev *netdev, void *buffer, rt_size_t size);
	rt_int32_t (*recv)(struct mini_netdev *netdev, void *buffer, rt_size_t size);
	rt_err_t  (*control)(struct mini_netdev *netdev, int cmd, void *args);
	void *user_data;
};

#endif /* _MINI_NETDEV_H__ */
