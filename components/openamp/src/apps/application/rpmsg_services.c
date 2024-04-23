/*
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <rtdef.h>
#include <rtthread.h>
#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include <openamp/rpmsg_virtio.h>
#include <metal/irq.h>
#include <riscv-clic.h>

extern int platform_init(int argc, char *argv[], void **platform);
extern struct  rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev),
			   rpmsg_ns_bind_cb ns_bind_cb);

extern int platform_poll(void *priv);

static rt_thread_t tid;
static void *platform;
struct rpmsg_device *rpdev;

static void rpmsg_poll_thread_entry(void *parameter)
{
	/* loop here */
	while (1) {
		platform_poll(platform);
	}
}

int rpmsg_service_init(void)
{
	int ret;
	int argc = 3;
	char *argv[] = {"./rpmsg_service_init", 0, 0};

	/* initialize the openamp framework */
	ret = platform_init(argc, argv, &platform);
	if (ret) {
		rt_kprintf("Failed to initialize platform.\n");
		return -1;
	}

	rpdev = platform_create_rpmsg_vdev(platform, 0, VIRTIO_DEV_DEVICE, NULL, NULL);
	if (!rpdev) {
		rt_kprintf("Failed to create rpmsg virtio device\n");
		return -1;
	}

	/* create the rpmsg poll thread */
	tid = rt_thread_create("rpmsg_poll_serivce",
			rpmsg_poll_thread_entry,
			NULL,
			2048,
			RT_THREAD_PRIORITY_MAX / 3,
			20);
	if (!tid) {
		rt_kprintf("Failed to create adma service\n");
		return -1;
	}

	rt_thread_startup(tid);

	return 0;
}

INIT_COMPONENT_EXPORT(rpmsg_service_init);
