/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

struct spacemit_rcan {
	int irq;
	struct dtb_node *node;
	struct mbox_chan *chan;
	struct mbox_client client;
	rt_sem_t trigger_sem;
	rt_thread_t trigger_tid; 
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k1x-rcan0" },
	{}
};

static void rcan_rx_callback(struct mbox_client *cl, void *data)
{
	struct spacemit_rcan *mb = rt_container_of(cl, struct spacemit_rcan, client);
	rt_hw_interrupt_umask(mb->irq);
}

void spacemit_rcan_int_handler(int vector, void *param)
{
	struct spacemit_rcan *can = (struct spacemit_rcan *)param;
	rt_hw_interrupt_mask(can->irq);

	rt_sem_release(can->trigger_sem);
}

static void can_trigger_irq_thread_entry(void *parameter)
{
	char c = 'c';
	struct spacemit_rcan *can = (struct spacemit_rcan *)parameter;

	while (1) {
		rt_sem_take(can->trigger_sem, RT_WAITING_FOREVER);
		mbox_send_message(can->chan, &c);
	}
}

static int spacemit_rcan_probe(void)
{
	int i;
	char *string;
	int size;
	struct spacemit_rcan *rcan;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node)) {
				continue;
			}

			rcan = rt_calloc(1, sizeof(struct spacemit_rcan));
			if (!rcan) {
				rt_kprintf("%s:%d, No memory spacemit\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			rcan->node = compatible_node;

			rcan->irq = dtb_node_irq_get(compatible_node, 0);
			if (rcan->irq < 0) {
				rt_kprintf("%s:%d, get irq failed\n", __func__, __LINE__);
				return -RT_ERROR;
			}

			/* we konw that, we used one mailbox */
			for_each_property_string(rcan->node, "mbox-names", string, size) {
				rcan->client.dev = rcan->node;
				rcan->client.tx_block = true;
				rcan->client.rx_callback = rcan_rx_callback;
				rcan->chan = mbox_request_channel_byname(&rcan->client, string);
			}

			rcan->trigger_sem = rt_sem_create("can_trigger_sem", 0, RT_IPC_FLAG_FIFO);
			if (!rcan->trigger_sem) {
				rt_kprintf("Failed to create adm sem\n");
				return -1;
			}

			/* create the rpmsg trigger irq thread */
			rcan->trigger_tid = rt_thread_create("can_trigger_irq_serivce",
				can_trigger_irq_thread_entry,
				(void *)rcan,
				2048,
				RT_THREAD_PRIORITY_MAX / 5,
				20);
			if (!rcan->trigger_tid) {
				rt_kprintf("Failed to create adma service\n");
				return -1;
			}

			rt_thread_startup(rcan->trigger_tid);

			rt_hw_interrupt_install(rcan->irq, spacemit_rcan_int_handler, (void *)rcan, "rcan0-irq");
			rt_hw_interrupt_umask(rcan->irq);
		}
	}

	return 0;
}
INIT_COMPONENT_EXPORT(spacemit_rcan_probe);
