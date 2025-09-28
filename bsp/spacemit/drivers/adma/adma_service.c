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
#include <metal/irq.h>
#include <riscv-clic.h>

#define RPMSG_SERV_NAME         "adma-service"

extern int rpmsg_create_ept(struct rpmsg_endpoint *ept, struct rpmsg_device *rdev,
		     const char *name, uint32_t src, uint32_t dest,
		     rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb);

static rt_thread_t trigger_tid;
static rt_sem_t trigger_sem;
extern struct rpmsg_device *rpdev;
static struct rpmsg_endpoint lept;

static int adma_irq_handler(int irq, void *arg)
{
	unsigned int pending;

	/* clear pending */
	pending = *((volatile unsigned long *)(0xc08838a0));
	*((volatile unsigned long *)(0xc08838a0)) = 0x0;

	/* finish transfer */
	if (pending & (1 << 0)) {
		rt_sem_release(trigger_sem);
	}

	return METAL_IRQ_HANDLED;
}

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	int ret;

	if (strcmp(data, "startup") == 0) {

		/* enable hdmi interrupt */
		 *((volatile unsigned long *)(0xc0883880)) = 0x1;

		/* reqeust adma irq */
		ret = metal_irq_register(HDMI_ADMA_CH_IRQn, adma_irq_handler,
				NULL);
		if (ret) {
			rt_kprintf("Failed to register adma irq handler\n");
			return -1;
		}

		metal_irq_enable(HDMI_ADMA_CH_IRQn);

		if (rpmsg_send(ept, "startup-ok", 10) < 0) {
			rt_kprintf("rpmsg_send failed\n");
			return -1;
		}
	}

	return 0;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	/* do nothing */
}

static void adma_trigger_irq_thread_entry(void *parameter)
{
	int ret;

	/* wait for rproc dev ready */
	while (1) {
		if (!rpdev)
			rt_thread_delay(2);
		else
			break;
	}

	/* create rpmsg endpoint */
	ret = rpmsg_create_ept(&lept, rpdev, RPMSG_SERV_NAME,
			RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			rpmsg_endpoint_cb, rpmsg_service_unbind);
	if (ret) {
		rt_kprintf("Failed to create endpoint\n");
		return;
	}

	while (1) {
		rt_sem_take(trigger_sem, RT_WAITING_FOREVER);

		if (rpmsg_send(&lept, "#", 1) < 0) {
			rt_kprintf("rpmsg_send failed\n");
		}
	}
}

int rpmsg_adma_service_init(void)
{
	trigger_sem = rt_sem_create("adma_trigger_sem", 0, RT_IPC_FLAG_FIFO);
	if (!trigger_sem) {
		rt_kprintf("Failed to create adm sem\n");
		return -1;
	}

	/* create the rpmsg trigger irq thread */
	trigger_tid = rt_thread_create("adma_trigger_irq_serivce",
			adma_trigger_irq_thread_entry,
			NULL,
			2048,
			RT_THREAD_PRIORITY_MAX / 5,
			20);
	if (!trigger_tid) {
		rt_kprintf("Failed to create adma service\n");
		return -1;
	}

	rt_thread_startup(trigger_tid);

	return 0;
}
INIT_APP_EXPORT(rpmsg_adma_service_init);
