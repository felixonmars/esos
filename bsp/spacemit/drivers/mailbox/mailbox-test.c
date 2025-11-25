/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include "k3_mailbox.h"

#ifdef RT_USING_FINSH

#define MAILBOX_TEST_COUNT	30

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,mailbox-test", },
	{},
};

struct k3_mbox_test {
	struct mbox_chan *tx_chan;
	struct mbox_chan *rx_chan;
	struct mbox_client tx_client;
	struct mbox_client rx_client;
	rt_thread_t rx_tid;
	rt_sem_t rx_sem;
	struct dtb_node *node;
	bool master;
	unsigned int rx_data;
};

static void mbox_rx_callback(struct mbox_client *cl, void *data)
{
	struct k3_mbox_test *test= rt_container_of(cl, struct k3_mbox_test, rx_client);

	test->rx_data = *(unsigned int *)data;
	if (!test->master) {
		rt_kprintf("rx_data:->%u\n", *(unsigned int *)data);
	}

	rt_sem_release(test->rx_sem);
}

static unsigned int __tx_count;

static void mbox_test_poll(void *priv)
{
	struct k3_mbox_test *ppriv = (struct k3_mbox_test *)priv;

	while(1) {
		/* wait the tick or 20ms polling */
		rt_sem_take(ppriv->rx_sem, RT_WAITING_FOREVER);

		rt_thread_delay(10);

		if (ppriv->master) {
			(ppriv->rx_data)++;
			if (ppriv->rx_data >= MAILBOX_TEST_COUNT)
				break;
		}

		mbox_send_message(ppriv->tx_chan, &ppriv->rx_data);

		if (!ppriv->master) {
			if (ppriv->rx_data == (MAILBOX_TEST_COUNT - 1))
				break;
		}
	}

	if (ppriv->master)
		rt_kprintf("%s:%d, master exit\n", __func__, __LINE__);
	else
		rt_kprintf("%s:%d, slave exit\n", __func__, __LINE__);

}

static void mbox_test_start(void *priv)
{
	struct k3_mbox_test *ppriv = (struct k3_mbox_test *)priv;

	rt_thread_delay(50);
	mbox_send_message(ppriv->tx_chan, &__tx_count);
}

static int mailbox_test(void)
{
	rt_thread_t tid;
	rt_int32_t i, ret;
	struct k3_mbox_test *test;
	char *string, stringtmp;
	rt_int32_t size;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;

			test = (struct k3_mbox_test *)rt_calloc(1, sizeof(struct k3_mbox_test));
			if (!test) {
				rt_kprintf("%s:%d, No memroy\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			test->node = compatible_node;
			test->master = dtb_node_read_bool(compatible_node, "master");

			for_each_property_string_extend(compatible_node, "mbox-names", string, stringtmp, size) {
				if (rt_strcmp(string, "tx_test")) {
					/* initialize the client */
					test->tx_client.tx_block = true;
					test->tx_client.rx_callback = mbox_rx_callback;
					test->tx_client.dev = compatible_node;
					test->tx_chan = mbox_request_channel_byname(&test->tx_client, string);
				} else if (rt_strcmp(string, "rx_test")) {
					/* initialize the client */
					test->rx_client.tx_block = true;
					test->rx_client.rx_callback = mbox_rx_callback;
					test->rx_client.dev = compatible_node;
					test->rx_chan = mbox_request_channel_byname(&test->rx_client, string);

					/* initialize the sem */
					test->rx_sem = rt_sem_create(string, 0, RT_IPC_FLAG_FIFO);

					/* initialize the rx thread */
					test->rx_tid = rt_thread_create("mbox_test_thr",
							mbox_test_poll,
							(void *)test,
							2048,
							RT_THREAD_PRIORITY_MAX / 3,
							20);
					if (!test->rx_tid) {
						rt_kprintf("Failed to create adma service\n");
						return -RT_EINVAL;
					}

					rt_thread_startup(test->rx_tid);
				} else {
					rt_kprintf("%s:%d, Wrong mbox name\n", __func__, __LINE__);
					return -RT_EINVAL;
				}
			};

			if (test->master) {
				/* initialize the rx thread */
				tid = rt_thread_create("start_mbox_thr",
						mbox_test_start,
						(void *)test,
						2048,
						RT_THREAD_PRIORITY_MAX / 3,
						20);
				rt_thread_startup(tid);
			}
		}
	}

	return 0;
}

#include <finsh.h>
MSH_CMD_EXPORT(mailbox_test, mailbox test driver(rcpu<->rcpu));
#endif
