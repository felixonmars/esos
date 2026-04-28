/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <clint.h>
#include <stdlib.h>

#ifdef RT_USING_FINSH

static void rt_float_case(void)
{
        int i;
        float a = 1.1, c = 0.0;
        double b = 2.010, d = 0.0;
        float e = 204.224, f = 2.0;
        double g = 128.12000, h = 2.00;

        for (i = 0; i < 4096; ++i) {
                c += a * i;
        }

        if ((unsigned int)c != 9225226)
                rt_kprintf("thread:%s, (float *):error\n", rt_thread_self()->name);
        else
                rt_kprintf("thread:%s, (float *):success\n", rt_thread_self()->name);

        for (i = 0; i < 4096; ++i) {
                d += b * i;
        }

        if ((unsigned int)d != 16856985)
                rt_kprintf("thread:%s, (double *):error\n", rt_thread_self()->name);
        else
                rt_kprintf("thread:%s, (double *):sucess\n", rt_thread_self()->name);

        e = e / f * 1000.0;
        if ((unsigned int)e != 102112)
                rt_kprintf("thread:%s, (float '/'):error\n", rt_thread_self()->name);
        else
                rt_kprintf("thread:%s, (float '/'):success\n", rt_thread_self()->name);

        g = g / h * 1000.000;
        if ((unsigned int)g != 64060)
                rt_kprintf("thread:%s, (double '/'):error\n", rt_thread_self()->name);
        else
                rt_kprintf("thread:%s, (double '/'):sucess\n", rt_thread_self()->name);

}

static void rt_float_thread(void *priv)
{
	char **argv = (char **)priv;

	int mem, loops;

	loops = atoi(argv[2]);

	while (loops-- >= 0) {
		rt_float_case();
		rt_kprintf("loops: %d\n",loops);
		rt_thread_delay(100);
	}
}

static int rt_float(int argc, char **argv)
{
	char string[32];
	rt_thread_t tid;
	int i, num_threads;

	if (argc != 3) {
		rt_kprintf("Usage: rt_float <number_threads> <loops>\n");
		return -RT_ERROR;
	}

	num_threads = atoi(argv[1]);

	for (i = 0; i < num_threads; ++i) {
		rt_sprintf(string, "rt_float:%d\n", i);
		tid = rt_thread_create(string, rt_float_thread, (void *)argv, 2048, RT_THREAD_PRIORITY_MAX / 3, 20);
		if (!tid) {
			rt_kprintf("%s:%d, create thread: %s, error\n", __func__, __LINE__, string);
			return -RT_ERROR;
		}

		rt_thread_startup(tid);
	}

	return 0;
}


MSH_CMD_EXPORT_ALIAS(rt_float, rt_float, rt_float <number_threads> <loops>);
#endif

