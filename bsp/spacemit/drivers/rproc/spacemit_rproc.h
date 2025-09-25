/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PLATFORM_INFO_H_
#define PLATFORM_INFO_H_

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include <openamp/open_amp.h>
#include <rtdevice.h>
#include <rtdef.h>

#if defined __cplusplus
extern "C" {
#endif

/* memory attributes: not used  */
#define DEVICE_SHARED		0x00000001U /* device, shareable */
#define DEVICE_NONSHARED	0x00000010U /* device, non shareable */
#define NORM_NSHARED_NCACHE	0x00000008U /* Non cacheable  non shareable */
#define NORM_SHARED_NCACHE	0x0000000CU /* Non cacheable shareable */
#define PRIV_RW_USER_RW		(0x00000003U<<8U) /* Full Access */

struct remoteproc_priv {
	struct mbox_chan *chan;
	struct mbox_client client;
	rt_event_t event;
	int evtype;
};

#define NO_RESOURCE_ENTRIES         8

/* Resource table for the given remote */
struct remote_resource_table {
	rt_uint32_t version;
	rt_uint32_t num;
	rt_uint32_t reserved[2];
	rt_uint32_t offset[NO_RESOURCE_ENTRIES];
	/* rpmsg vdev entry */
	struct fw_rsc_vdev rpmsg_vdev;
	struct fw_rsc_vdev_vring rpmsg_vring0;
	struct fw_rsc_vdev_vring rpmsg_vring1;
}__attribute__((packed, aligned(0x100)));

#if defined __cplusplus
}
#endif

#endif /* PLATFORM_INFO_H_ */
