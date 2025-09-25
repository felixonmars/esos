/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtdef.h>
#include <rtthread.h>
#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include <openamp/rpmsg_virtio.h>

#define APPLICATION_NAME	"rpmsg:demo0"
#define RPMSG_ADDR_SRC		888
#define RPMSG_ADDR_DST		666

extern struct rpmsg_device *rpdev;

struct double_os_demo {
	char *service_name;
	struct rpmsg_endpoint endp;
};

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data,
			     size_t len, uint32_t src, void *priv)
{
	int r;
	char val[128];
	static unsigned long long count;

	count++;
	r = rt_snprintf(val, 128, "%s-->%d\n", data, count);

	rpmsg_send(ept, (char *)val, r);

	return 0;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	/* do nothing */
}

void rproc_test_thread(void *parameter)
{
	int ret;
	struct double_os_demo *demo = (struct double_os_demo *)parameter;

	while (1) {
test_0:
		if (rpdev == RT_NULL) {
			rt_thread_delay(10);
			goto test_0;
		}

		demo->service_name = APPLICATION_NAME;

		ret = rpmsg_create_ept(&demo->endp, rpdev, demo->service_name,
					RPMSG_ADDR_SRC, RPMSG_ADDR_DST,
					rpmsg_endpoint_cb, rpmsg_service_unbind);
		if (ret) {
			rt_kprintf("%s:%d, create ept failed\n", __func__, __LINE__);
			return;
		}

		return;
	}
}

int double_os_demo_init(void)
{
	int ret;
	rt_thread_t tid;
	struct double_os_demo *demo;

	demo = rt_calloc(1, sizeof(struct double_os_demo));
	if (!demo) {
		rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
		return -RT_ENOMEM;
	}

	tid = rt_thread_create("rproc-test",
			rproc_test_thread,
			(void *)demo,
			4096,
			RT_THREAD_PRIORITY_MAX / 3,
			20);
	if (!tid) {
		rt_kprintf("Failed to create rproc test service\n");
		return -RT_EINVAL;
	}

	rt_thread_startup(tid);

	return 0;
}
/* INIT_APP_EXPORT(double_os_demo_init); */
