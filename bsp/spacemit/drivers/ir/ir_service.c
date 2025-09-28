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

struct ir_service {
	char ser_name[32];
	unsigned int irq;
	rt_sem_t sem;
	char sem_name[32];
	rt_thread_t tid;
	char tid_name[32];
	struct rpmsg_endpoint lept;
};

struct ir_service _ir_service[] = {
	{
		.ser_name = "rir-service",
		.irq = IRC_IRQn,
		.sem_name = "rir_sem",
		.tid_name = "rir_tid",
	},
};

extern int rpmsg_create_ept(struct rpmsg_endpoint *ept, struct rpmsg_device *rdev,
		     const char *name, uint32_t src, uint32_t dest,
		     rpmsg_ept_cb cb, rpmsg_ns_unbind_cb unbind_cb);

extern struct rpmsg_device *rpdev;

static int irq_handler(int irq, void *arg)
{
	struct ir_service *_ir_service = (struct ir_service *)arg;

	metal_irq_disable(irq);

	rt_sem_release(_ir_service->sem);

	return METAL_IRQ_HANDLED;
}

static int rpmsg_endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src, void *priv)
{
	int ret;
	struct ir_service *_ir_service = container_of(ept, struct ir_service, lept);

	if (strcmp(data, "startup") == 0) {
		/* reqeust i2c irq */
		ret = metal_irq_register(_ir_service->irq, irq_handler,
				(void *)_ir_service);
		if (ret) {
			rt_kprintf("Failed to register ir irq handler\n");
			return -1;
		}

		metal_irq_enable(_ir_service->irq);

		if (rpmsg_send(ept, "startup-ok", 10) < 0) {
			rt_kprintf("rpmsg_send failed\n");
			return -1;
		}
	} else {
		metal_irq_enable(_ir_service->irq);
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
	struct ir_service *_ir_service = (struct ir_service *)parameter;

	/* wait for rproc dev ready */
	while (1) {
		if (!rpdev)
			rt_thread_delay(2);
		else
			break;
	}

	/* create rpmsg endpoint */
	ret = rpmsg_create_ept(&_ir_service->lept, rpdev, _ir_service->ser_name,
			RPMSG_ADDR_ANY, RPMSG_ADDR_ANY,
			rpmsg_endpoint_cb, rpmsg_service_unbind);
	if (ret) {
		rt_kprintf("Failed to create endpoint\n");
		return;
	}

	while (1) {
		rt_sem_take(_ir_service->sem, RT_WAITING_FOREVER);

		if (rpmsg_send(&_ir_service->lept, "#", 1) < 0) {
			rt_kprintf("rpmsg_send failed\n");
		}
	}
}

int rpmsg_ir_service_init(void)
{
	int i;

	for (i = 0; i < sizeof(_ir_service) / sizeof(_ir_service[0]); ++i) {
		_ir_service[i].sem = rt_sem_create(_ir_service[i].sem_name, 0, RT_IPC_FLAG_FIFO);
		if (!_ir_service[i].sem) {
			rt_kprintf("Failed to create ir sem\n");
			return -1;
		}

		/* create the rpmsg trigger irq thread */
		_ir_service[i].tid = rt_thread_create(_ir_service[i].tid_name,
				trigger_irq_thread_entry,
				(void *)&_ir_service[i],
				2048,
				RT_THREAD_PRIORITY_MAX / 5,
				20);
		if (!_ir_service[i].tid) {
			rt_kprintf("Failed to create ir service\n");
			return -1;
		}

		rt_thread_startup(_ir_service[i].tid);
	}

	return 0;
}
INIT_APP_EXPORT(rpmsg_ir_service_init);
