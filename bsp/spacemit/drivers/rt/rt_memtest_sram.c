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

#define SRAM_BUFFER_BASE	(0x0)
#define SRAM_SIZE		(512 * 1024 * 1024)

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

static int rt_memtester_sram(int argc, char **argv)
{
	int loops;
	char *buffer;

	rt_kprintf("Attantion please\n");
	rt_kprintf("When start this test, The Sram must not be used\n");

	if (argc != 2) {
		rt_kprintf("Usage: rt_memtester_sram <loops>\n");
		return -RT_ERROR;
	}

	loops = atoi(argv[1]);

	buffer = (char *)rt_calloc(SRAM_SIZE, sizeof(char));
	if (!buffer) {
		rt_kprintf("%s:%d, Alloc memory failed\n", __func__, __LINE__);
		return -RT_ENOMEM;
	}

	/* save the origin data of sram */
	rt_memcpy(buffer, SRAM_BUFFER_BASE, SRAM_SIZE);

	while (loops-- >= 0) {
		mem_test((rt_uint64_t)SRAM_BUFFER_BASE, SRAM_SIZE);
		rt_memset(SRAM_BUFFER_BASE, 0, SRAM_SIZE);
		rt_thread_delay(10);
	}

	/* restore the sram data */
	rt_memcpy(SRAM_BUFFER_BASE, buffer, SRAM_SIZE);
	asm volatile ("fence.i");

	rt_free(buffer);

}

MSH_CMD_EXPORT_ALIAS(rt_memtester_sram, rt_memtester_sram, rt_memtester_sram <loops>);

#endif

