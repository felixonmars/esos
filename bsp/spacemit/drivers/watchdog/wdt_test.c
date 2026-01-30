/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * spacemit test driver for watchdog
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/watchdog.h>

#define TEST_WDT	"k3-wdt"

static void watchdog_ping_test(void)
{
	struct rt_device *dev;
	rt_err_t err;
	rt_uint32_t timeout, value;
	rt_int32_t loop = 10;
	rt_uint16_t flag;

	dev = rt_device_find(TEST_WDT);
	if (dev == RT_NULL) {
		rt_kprintf("can not find watchdog device\n");
		return;
	} else
		rt_kprintf("find wdt device success\n");

	if (rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
		rt_kprintf("Open wdt Fail\n");
	err = rt_device_init(dev);
	if (err) {
		rt_kprintf("wdt device init error\n");
		return;
	}

	timeout = 100;
	err = rt_device_control(dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, (void *)&timeout);
	if (err) {
		rt_kprintf("wdt device set timeout failed\n");
		return;
	}

	err = rt_device_control(dev, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, (void *)&value);
	if (err) {
		rt_kprintf("wdt device get timeout failed\n");
		return;
	}
	rt_kprintf("timeout:%u, value:%u\n", timeout, value);
	err = rt_device_control(dev, RT_DEVICE_CTRL_WDT_START, (void *)&value);
	if (err) {
		rt_kprintf("wdt device start failed\n");
		return;
	}
	do {
		err = rt_device_control(dev, RT_DEVICE_CTRL_WDT_GET_TIMELEFT, (void *)&value);
		if (err) {
			rt_kprintf("wdt device get timeleft failed\n");
			rt_device_control(dev, RT_DEVICE_CTRL_WDT_STOP, (void *)&value);
			return;
		}
		rt_kprintf("wdt timeleft:%u\n", value);
		rt_hw_us_delay(2 * 1000 * 1000);
		loop--;
		if (loop == 5) {
			 err = rt_device_control(dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, (void *)&value);
			 if (err) {
			 	rt_kprintf("wdt device get keepalive failed\n");
				rt_device_control(dev, RT_DEVICE_CTRL_WDT_STOP, (void *)&value);
				return;
			 }
			 rt_kprintf("wdt device kick dog\n");
		}
	} while (loop > 0);

	rt_device_control(dev, RT_DEVICE_CTRL_WDT_STOP, (void *)&value);
	return;
}

static void watchdog_reset_test(void)
{
	struct rt_device *dev;
	rt_err_t err;
	rt_uint32_t timeout, value;
	rt_int32_t loop = 10;
	rt_uint16_t flag;

	dev = rt_device_find(TEST_WDT);
	if (dev == RT_NULL) {
		rt_kprintf("can not find watchdog device\n");
		return;
	} else
		rt_kprintf("find wdt device success\n");

	if (rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
		rt_kprintf("Open wdt Fail\n");
	err = rt_device_init(dev);
	if (err) {
		rt_kprintf("wdt device init error\n");
		return;
	}

	timeout = 1;
	err = rt_device_control(dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, (void *)&timeout);
	if (err) {
		rt_kprintf("wdt device set timeout failed\n");
		return;
	}

	err = rt_device_control(dev, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, (void *)&value);
	if (err) {
		rt_kprintf("wdt device get timeout failed\n");
		return;
	}
	rt_kprintf("timeout:%u, value:%u\n", timeout, value);
	err = rt_device_control(dev, RT_DEVICE_CTRL_WDT_START, (void *)&value);
	if (err) {
		rt_kprintf("wdt device start failed\n");
		return;
	}
	rt_hw_us_delay(5 * 1000 * 1000);

	return;
}

void wdt_test(int argc, char *argv[])
{
	if(argc != 2) {
		rt_kprintf("wdt test arg num error\n");
		return;
	}

	if (!strncmp(argv[1], "start", 5))
		watchdog_ping_test();
	else if (!strncmp(argv[1], "reset", 5))
		watchdog_reset_test();
}
MSH_CMD_EXPORT(wdt_test, spacemit wdt test);
