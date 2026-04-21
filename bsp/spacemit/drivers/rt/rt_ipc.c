/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <clint.h>
#include <spacemit_sdk_soc.h>

#ifdef RT_USING_FINSH

#define RT_TEST_NUMBER		500

static unsigned long long *timestamp;

static unsigned long long timestamp0, timestamp1;
static unsigned int buffer_count;
static rt_thread_t rtscheduler0, rtscheduler1;
static rt_sem_t rtipc_sem;

static void rt_scheduler_0(void *priv)
{
	unsigned int i;

	/* startup thread1 */
	rt_thread_startup(rtscheduler1);

	for (i = 0; i < RT_TEST_NUMBER; ++i) {
		timestamp0 = SysTimer_GetLoadValue();
		rt_sem_release(rtipc_sem);
	}
}

static void rt_scheduler_1(void *priv)
{
	unsigned int i;
	unsigned long long all = 0, min = -1, max = 0;

	for (i = 0; i < RT_TEST_NUMBER; ++i) {
		rt_sem_take(rtipc_sem, RT_WAITING_FOREVER);
		timestamp1 = SysTimer_GetLoadValue();
		timestamp[buffer_count++] = timestamp1 - timestamp0;
	}

	/* printf the timestamp */
	for (i = 0; i < RT_TEST_NUMBER; ++i) {
		all += timestamp[i];

		if (timestamp[i] < min)
			min = timestamp[i];

		if (timestamp[i] > max)
			max = timestamp[i];
	}

	all /= RT_TEST_NUMBER;

	rt_kprintf("IPC delay of esos Average:%lldns, min:%lldns, max:%lldns\n",
			all * 1000000000 / SOC_TIMER_FREQ,
			min * 1000000000 / SOC_TIMER_FREQ,
			max * 1000000000 / SOC_TIMER_FREQ);

	if (timestamp != RT_NULL) {
		rt_free(timestamp);
		timestamp = RT_NULL;
	}

	rt_sem_delete(rtipc_sem);
	rtipc_sem = RT_NULL;
	rtscheduler0 = RT_NULL;
	rtscheduler1 = RT_NULL;
}

static int rt_ipc(int argc, char **argv)
{

	timestamp = rt_calloc(RT_TEST_NUMBER, sizeof(unsigned long long));
	if (!timestamp) {
		rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	rtipc_sem = rt_sem_create("ipc_sem", 0, RT_IPC_FLAG_FIFO);

	rtscheduler0 = rt_thread_create("schduler0", rt_scheduler_0, RT_NULL, 1024, 0, 1000000);
	if (!rtscheduler0) {
		rt_kprintf("%s:%d create schedule0 failed\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	rtscheduler1 = rt_thread_create("schduler1", rt_scheduler_1, RT_NULL, 1024, 0, 1000000);
	if (!rtscheduler1) {
		rt_kprintf("%s:%d create schedule1 failed\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	rt_thread_startup(rtscheduler0);

	return 0;
}

MSH_CMD_EXPORT(rt_ipc, "rt ipc delay");
#endif
