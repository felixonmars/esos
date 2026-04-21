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
#define RT_SCHEDULER_PRIORITY   9


static unsigned long long *timestamp;
static unsigned long long timestamp0, timestamp1;
static unsigned int buffer_count;
static rt_thread_t rtscheduler0, rtscheduler1;

static void rt_scheduler_0(void *priv)
{
	unsigned int i;
	unsigned long long all = 0, min = -1, max = 0;

	/* startup thread1 */
	rt_thread_startup(rtscheduler1);
	rt_thread_yield();

	for (i = 0; i < RT_TEST_NUMBER; ++i) {
		timestamp0 = SysTimer_GetLoadValue();
		timestamp[i] = timestamp0 - timestamp1;
		rt_thread_yield();
	}

	/* printf the timestamp */
	for (i = 0; i < RT_TEST_NUMBER ; ++i) {
		all += timestamp[i];
		if (timestamp[i] < min)
			min = timestamp[i];
		if (timestamp[i] > max)
			max = timestamp[i];
	}

	rt_kprintf("scheduling delay of esos Average:%lldns, min:%lldns, max:%lldns\n",
			all * 1000000000 / SOC_TIMER_FREQ / RT_TEST_NUMBER,
			min * 1000000000 / SOC_TIMER_FREQ,
			max * 1000000000 / SOC_TIMER_FREQ);

	if (timestamp != RT_NULL) {
		rt_free(timestamp);
		timestamp = RT_NULL;
	}
	rtscheduler0 = RT_NULL;
	rtscheduler1 = RT_NULL;
}

static void rt_scheduler_1(void *priv)
{
	unsigned int i;

	for (i = 0; i < RT_TEST_NUMBER; ++i) {
		timestamp1 = SysTimer_GetLoadValue();
		rt_thread_yield();
	}
}

static int rt_scheduler(int argc, char **argv)
{
	if (rtscheduler0 != RT_NULL || rtscheduler1 != RT_NULL) {
		rt_kprintf("Previous test is still running, please wait\n");
		return -RT_ERROR;
	}
	if (timestamp != RT_NULL) {
		rt_free(timestamp);
		timestamp = RT_NULL;
	}
	timestamp = rt_calloc(RT_TEST_NUMBER, sizeof(unsigned long long));
	if (!timestamp) {
		rt_kprintf("%s:%d, No memory\n", __func__, __LINE__);
		return -RT_ERROR;
	}

	rtscheduler1 = rt_thread_create("schduler1", rt_scheduler_1, RT_NULL, 4096, RT_SCHEDULER_PRIORITY, 1000000);
	if (!rtscheduler1) {
		rt_kprintf("%s:%d create schedule1 failed\n", __func__, __LINE__);
		rt_thread_delete(rtscheduler0);
		rtscheduler0 = RT_NULL;
		rt_free(timestamp);
		timestamp = RT_NULL;
		return -RT_ERROR;
	}

	rtscheduler0 = rt_thread_create("schduler0", rt_scheduler_0, RT_NULL, 4096, RT_SCHEDULER_PRIORITY, 1000000);
	if (!rtscheduler0) {
		rt_kprintf("%s:%d create schedule0 failed\n", __func__, __LINE__);
		rt_free(timestamp);
		timestamp = RT_NULL;
		return -RT_ERROR;
	}
	rt_thread_startup(rtscheduler0);
	return 0;
}

MSH_CMD_EXPORT(rt_scheduler, "rt scheduler delay");
#endif

