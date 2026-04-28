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

static void mem_test(rt_uint64_t address, rt_uint32_t size)
{
    rt_uint32_t i;

    rt_kprintf("thread:%s, memtest,address: 0x%08X size: 0x%08X\r\n", rt_thread_self()->name, address, size);

    /**< 8bit test */
    {
        rt_uint8_t * p_uint8_t = (rt_uint8_t *)address;
        for(i=0; i<size/sizeof(rt_uint8_t); i++)
        {
            *p_uint8_t++ = (rt_uint8_t)i;
        }

        p_uint8_t = (rt_uint8_t *)address;
        for(i=0; i<size/sizeof(rt_uint8_t); i++)
        {
            if( *p_uint8_t != (rt_uint8_t)i )
            {
                rt_kprintf("thread:%s, 8bit test fail @ 0x%08X\r\nsystem halt!!!!!", rt_thread_self()->name, (rt_uint32_t)p_uint8_t);
                while(1);
            }
            p_uint8_t++;
        }
        rt_kprintf("thread:%s, 8bit test pass!!\r\n", rt_thread_self()->name);
    }

    /**< 16bit test */
    {
        rt_uint16_t * p_uint16_t = (rt_uint16_t *)address;
        for(i=0; i<size/sizeof(rt_uint16_t); i++)
        {
            *p_uint16_t++ = (rt_uint16_t)i;
        }

        p_uint16_t = (rt_uint16_t *)address;
        for(i=0; i<size/sizeof(rt_uint16_t); i++)
        {
            if( *p_uint16_t != (rt_uint16_t)i )
            {
                rt_kprintf("thread:%s, 16bit test fail @ 0x%08X\r\nsystem halt!!!!!", rt_thread_self()->name, (rt_uint32_t)p_uint16_t);
                while(1);
            }
            p_uint16_t++;
        }
        rt_kprintf("thread:%s, 16bit test pass!!\r\n", rt_thread_self()->name);
    }

    /**< 32bit test */
    {
        rt_uint32_t * p_uint32_t = (rt_uint32_t *)address;
        for(i=0; i<size/sizeof(rt_uint32_t); i++)
        {
            *p_uint32_t++ = (rt_uint32_t)i;
        }

        p_uint32_t = (rt_uint32_t *)address;
        for(i=0; i<size/sizeof(rt_uint32_t); i++)
        {
            if( *p_uint32_t != (rt_uint32_t)i )
            {
                rt_kprintf("thread:%s, 32bit test fail @ 0x%08X\r\nsystem halt!!!!!", rt_thread_self()->name, (rt_uint32_t)p_uint32_t);
                while(1);
            }
            p_uint32_t++;
        }
        rt_kprintf("thread:%s, 32bit test pass!!\r\n", rt_thread_self()->name);
    }

    /**< 32bit Loopback test */
    {
        rt_uint32_t * p_uint32_t = (rt_uint32_t *)address;
        for(i=0; i<size/sizeof(rt_uint32_t); i++)
        {
            *p_uint32_t  = (rt_uint32_t)p_uint32_t;
            p_uint32_t++;
        }

        p_uint32_t = (rt_uint32_t *)address;
        for(i=0; i<size/sizeof(rt_uint32_t); i++)
        {
            if( *p_uint32_t != (rt_uint32_t)p_uint32_t )
            {
                rt_kprintf("thread:%s, 2bit Loopback test fail @ 0x%08X", rt_thread_self()->name, (rt_uint32_t)p_uint32_t);
                rt_kprintf("thread:%s, data:0x%08X \r\n", rt_thread_self()->name, (rt_uint32_t)*p_uint32_t);
                rt_kprintf("thread:%s, system halt!!!!!", rt_thread_self()->name, (rt_uint32_t)p_uint32_t);
                while(1);
            }
            p_uint32_t++;
        }
        rt_kprintf("thread:%s, 32bit Loopback test pass!!\r\n", rt_thread_self()->name);
    }
}

static void rt_memtester_thread(void *priv)
{
	char *buffer;
	char **argv = (char **)priv;

	int mem, loops;

	loops = atoi(argv[2]);
	mem = atoi(argv[3]);

	buffer = (char *)rt_calloc(mem, sizeof(char));
	if (!buffer) {
		rt_kprintf("%s:%d, thread:%s, Alloc memory failed\n", __func__, __LINE__, rt_thread_self()->name);
		return;
	}

	while (loops-- >= 0) {
		mem_test((rt_uint64_t)buffer, mem);
		rt_kprintf("loops: %d\n",loops);
		rt_thread_delay(100);
	}

	rt_free(buffer);
}

static int rt_memtester(int argc, char **argv)
{
	char string[32];
	rt_thread_t tid;
	int i, num_threads;

	if (argc != 4) {
		rt_kprintf("Usage: rt_memtester <number_threads> <loops> <mem:bytes>\n");
		return -RT_ERROR;
	}

	num_threads = atoi(argv[1]);

	for (i = 0; i < num_threads; ++i) {
		rt_sprintf(string, "rt_mem:%d\n", i);
		tid = rt_thread_create(string, rt_memtester_thread, (void *)argv, 2048, RT_THREAD_PRIORITY_MAX / 3, 20);
		if (!tid) {
			rt_kprintf("%s:%d, create thread: %s, error\n", __func__, __LINE__, string);
			return -RT_ERROR;
		}

		rt_thread_startup(tid);
	}

	return 0;
}


MSH_CMD_EXPORT_ALIAS(rt_memtester, rt_memtester, rt_memtester <number_threads> <loops> <mem:bytes>);

#endif

