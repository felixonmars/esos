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

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#undef container_of
#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

struct uart_service {
	char ser_name[32];
	rt_uint32_t irq;
	rt_sem_t sem;
	char sem_name[32];
	rt_thread_t tid;
	char tid_name[32];
	struct rpmsg_endpoint lept;
};

struct uart_service _uart_service[] = {
	{
		.ser_name = "ruart-service0",
		.irq = UART_IRQn,
		.sem_name = "ruart0_sem",
		.tid_name = "ruart0_tid",
	},
	{
		.ser_name = "ruart-service1",
		.irq = UART1_INT_REQ_IRQn,
		.sem_name = "ruart1_sem",
		.tid_name = "ruart1_tid",
	},
};

extern int rpmsg_create_ept(struct rpmsg_endpoint *ept, struct rpmsg_device *rdev,
		     const char *name, uint32_t src, uint32_t dest,
		     rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb);

extern struct rpmsg_device *rpdev;

static int irq_handler(int irq, void *arg)
{
	struct uart_service *_uart_service = (struct uart_service *)arg;

	metal_irq_disable(irq);

	rt_sem_release(_uart_service->sem);

	return METAL_IRQ_HANDLED;
}

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	int ret;
	struct uart_service *_uart_service = container_of(ept, struct uart_service, lept);

	if (strcmp(data, "startup") == 0) {
		/* reqeust uart irq */
		ret = metal_irq_register(_uart_service->irq, irq_handler,
				(void *)_uart_service);
		if (ret) {
			rt_kprintf("Failed to register uart irq handler\n");
			return -1;
		}

		metal_irq_enable(_uart_service->irq);

		if (rpmsg_send(ept, "startup-ok", 10) < 0) {
			rt_kprintf("rpmsg_send failed\n");
			return -1;
		}
	} else {
		metal_irq_enable(_uart_service->irq);
	}

	return 0;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	/* do nothing */
}

static void trigger_irq_thread_entry(void *parameter)
{
	int ret;
	struct uart_service *_uart_service = (struct uart_service *)parameter;

	/* wait for rproc dev ready */
	while (1) {
		if (!rpdev)
			rt_thread_delay(2);
		else
			break;
	}

	/* create rpmsg endpoint */
	ret = rpmsg_create_ept(&_uart_service->lept, rpdev, _uart_service->ser_name,
			RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			rpmsg_endpoint_cb, rpmsg_service_unbind);
	if (ret) {
		rt_kprintf("Failed to create endpoint\n");
		return;
	}

	while (1) {
		rt_sem_take(_uart_service->sem, RT_WAITING_FOREVER);

		if (rpmsg_send(&_uart_service->lept, "#", 1) < 0) {
			rt_kprintf("rpmsg_send failed\n");
		}
	}
}

int rpmsg_uart_service_init(void)
{
	int i;

	for (i = 0; i < sizeof(_uart_service) / sizeof(_uart_service[0]); ++i) {
		_uart_service[i].sem = rt_sem_create(_uart_service[i].sem_name, 0, RT_IPC_FLAG_FIFO);
		if (!_uart_service[i].sem) {
			rt_kprintf("Failed to create uart sem\n");
			return -1;
		}

		/* create the rpmsg trigger irq thread */
		_uart_service[i].tid = rt_thread_create(_uart_service[i].tid_name,
				trigger_irq_thread_entry,
				(void *)&_uart_service[i],
				2048,
				RT_THREAD_PRIORITY_MAX / 5,
				20);
		if (!_uart_service[i].tid) {
			rt_kprintf("Failed to create uart service\n");
			return -1;
		}

		rt_thread_startup(_uart_service[i].tid);
	}

	return 0;
}
INIT_APP_EXPORT(rpmsg_uart_service_init);
