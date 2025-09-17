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

#ifdef BSP_USING_PM
#include <drivers/pm.h>
#endif

#define RPMSG_SERV_NAME         "adma-service"
#define RPMSG_LOW_PWR_SERV_NAME         "rcpu-pwr-management-service"

extern int platform_init(int argc, char *argv[], void **platform);
extern struct  rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
			   unsigned int role,
			   void (*rst_cb)(struct virtio_device *vdev),
			   rpmsg_ns_bind_cb ns_bind_cb);

extern int rpmsg_create_ept(struct rpmsg_endpoint *ept, struct rpmsg_device *rdev,
		     const char *name, uint32_t src, uint32_t dest,
		     rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb);

extern int platform_poll(void *priv);

static rt_thread_t tid;
static rt_thread_t trigger_tid;
static rt_sem_t trigger_sem;
static void *platform;
static struct rpmsg_device *rpdev;
static struct rpmsg_endpoint lept;
#ifdef BSP_USING_PM
static struct rpmsg_endpoint lpwept;
#endif

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

#ifdef BSP_USING_PM
static int rpmsg_lpw_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	if (strcmp(data, "pwr_management") == 0) {
		rpmsg_send(ept, "pwr_management_ok", 17);
		return 0;
	}

	if (strcmp(data, "enter-low-pwr-mode") == 0) {
		/* Big-cpu will want us to enter low power mode */

		/* 1. release the DEFAULT_SLEEP_MODE */
		rt_pm_release(RT_PM_DEFAULT_SLEEP_MODE);

		/* 2. send ack ? */
		rpmsg_send(ept, "enter-low-pwr-mode-reply", 24);
	}

	if (strcmp(data, "exit-low-pwr-mode") == 0) {
		/* the rcpu has wakedup by Big-cpu */
		/* 1. send ack ? */
		rpmsg_send(ept, "exit-low-pwr-mode-reply", 23);
	}

	return 0;
}

static void rpmsg_lpw_service_unbind(struct rpmsg_endpoint *ept)
{
	/* do nothing */
}
#endif

static void adma_trigger_irq_thread_entry(void *parameter)
{
	while (1) {
		rt_sem_take(trigger_sem, RT_WAITING_FOREVER);

		if (rpmsg_send(&lept, "#", 1) < 0) {
			rt_kprintf("rpmsg_send failed\n");
		}
	}
}

static void adma_poll_thread_entry(void *parameter)
{
	/* loop here */
	while (1) {
		platform_poll(platform);
	}
}

int rpmsg_adma_service_init(void)
{
	int ret;
	int argc = 3;
	char *argv[] = {"./rpmsg_adma_service_init", 0, 0};

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

	/* create rpmsg endpoint */
	ret = rpmsg_create_ept(&lept, rpdev, RPMSG_SERV_NAME,
			RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			rpmsg_endpoint_cb, rpmsg_service_unbind);
	if (ret) {
		rt_kprintf("Failed to create endpoint\n");
		return -1;
	}

#ifdef BSP_USING_PM
	/* create lowpower mode endpoint */
	ret = rpmsg_create_ept(&lpwept, rpdev, RPMSG_LOW_PWR_SERV_NAME,
			RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			rpmsg_lpw_endpoint_cb, rpmsg_lpw_service_unbind);
	if (ret) {
		rt_kprintf("Failed to create endpoint\n");
		return -1;
	}
#endif

	/* create the rpmsg poll thread */
	tid = rt_thread_create("adma_poll_serivce",
			adma_poll_thread_entry,
			NULL,
			2048,
			RT_THREAD_PRIORITY_MAX / 3,
			20);
	if (!tid) {
		rt_kprintf("Failed to create adma service\n");
		return -1;
	}

	rt_thread_startup(tid);

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

INIT_COMPONENT_EXPORT(rpmsg_adma_service_init);
