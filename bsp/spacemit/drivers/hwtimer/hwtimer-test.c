/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>

#ifdef RT_USING_FINSH

#ifdef RT_USING_HWTIMER

#define TIMER   "timer:0:0"

static rt_err_t timer_timeout_cb(rt_device_t dev, rt_size_t size)
{
	rt_kprintf("enter hardware timer isr\n");

	return 0;
}

static int hwtimer_test(void)
{
	int err;
	rt_hwtimerval_t val;
	rt_hwtimer_mode_t mode;
	rt_device_t dev = RT_NULL;
	int t = 5;

	if ((dev = rt_device_find(TIMER)) == RT_NULL) {
		rt_kprintf("No Device: %s\n", TIMER);
		return -1;
	}

	if (rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK) {
		rt_kprintf("Open %s Fail\n", TIMER);
		return -1;
	}

	/* 定时执行回调函数 -- 单次模式 */
	/* 设置超时回调函数 */
	rt_device_set_rx_indicate(dev, timer_timeout_cb);

	/* 单次模式 */
	mode = HWTIMER_MODE_ONESHOT;
	err = rt_device_control(dev, HWTIMER_CTRL_MODE_SET, &mode);

	/* 设置定时器超时值并启动定时器 */
	val.sec = t;
	val.usec = 0;

	rt_kprintf("SetTime: Sec %d, Usec %d\n", val.sec, val.usec);

	if (rt_device_write(dev, 0, &val, sizeof(val)) != sizeof(val)) {
		rt_kprintf("SetTime Fail\n");
		goto EXIT;
	}

	/* 等待回调函数执行 */
	rt_thread_delay((t + 1) * RT_TICK_PER_SECOND);

EXIT:
	/* 停止定时器 */
	err = rt_device_control(dev, HWTIMER_CTRL_STOP, RT_NULL);

	err = rt_device_close(dev);

	rt_kprintf("Close %s\n", TIMER);

	return err;
}

#include <finsh.h>
MSH_CMD_EXPORT(hwtimer_test, hwtimer test driver);
#endif
#endif
