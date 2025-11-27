/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * K1X SPI + DMA 联合测试
 *
 * 依据 25-11-14测试用例.txt 中的代码，
 * 将 SPI Flash 读写 + DMA 通路测试整理为一个可在 MSH 中
 * 通过命令 dma_k1_spi_test 调用的自测用例。
 *
 * 依赖驱动：
 *   - bsp/spacemit/drivers/spi/k1x_spi.c
 *   - bsp/spacemit/drivers/dma/dma_k1x.c
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <rthw.h>
#include <riscv-ops.h>
#include <drivers/spi.h>
#include <drivers/dma.h>

#ifdef RT_USING_SPI

static int k1x_spi_flash_attach(void)
{
    struct rt_spi_device *spi_device = RT_NULL;

    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL)
    {
        rt_kprintf("Failed to malloc the spi device.\n");
        return -RT_ENOMEM;
    }

    if (rt_spi_bus_attach_device(spi_device, "rspi00", "rspi0", RT_NULL) != RT_EOK)
    {
        rt_kprintf("Failed to attach the spi device.\n");
        rt_free(spi_device);
        return -RT_ERROR;
    }

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(k1x_spi_flash_attach);

static int dma_k1_spi_test_run(void)
{
    int result = 0;
    struct rt_spi_device *dev;
    rt_uint8_t *tmp_base = RT_NULL;
    rt_uint8_t *tmp = RT_NULL;
    rt_uint8_t *buf = RT_NULL;
    rt_uint8_t *recv = RT_NULL;
    int i;

    dev = (struct rt_spi_device *)rt_device_find("rspi00");
    if (!dev)
    {
        if (k1x_spi_flash_attach() != RT_EOK)
        {
            rt_kprintf("get rspi00 device failed (attach error)\n");
            return -RT_EIO;
        }

        dev = (struct rt_spi_device *)rt_device_find("rspi00");
        if (!dev)
        {
            rt_kprintf("get rspi00 device failed\n");
            return -RT_EIO;
        }
    }

    tmp_base = (rt_uint8_t *)rt_malloc(sizeof(rt_uint8_t) * 1024);
    if (tmp_base == RT_NULL)
    {
        rt_kprintf("malloc tmp buffer failed\n");
        result = -RT_ENOMEM;
        goto _exit;
    }
    tmp = tmp_base;
    rt_kprintf("tmp=%p\n", tmp);

    buf = (rt_uint8_t *)rt_malloc_align(sizeof(rt_uint8_t) * 8, 32);
    if (buf == RT_NULL)
    {
        rt_kprintf("malloc buf failed\n");
        result = -RT_ENOMEM;
        goto _exit;
    }

    recv = (rt_uint8_t *)rt_malloc_align(sizeof(rt_uint8_t) * 8, 32);
    if (recv == RT_NULL)
    {
        rt_kprintf("malloc recv failed\n");
        result = -RT_ENOMEM;
        goto _exit;
    }

    dev->config.mode       = RT_SPI_MODE_0;
    dev->config.data_width = 8;
    dev->config.max_hz     = 26000000;

    /*
     * 1. 读取 JEDEC ID
     *    指令 0x9F，读回 4 字节 ID。
     */
    rt_memset(buf, 0, 8);
    rt_memset(recv, 0, 8);
    buf[0] = 0x9F;

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, buf, 8);
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, recv, 8);

    if (rt_spi_send_then_recv(dev, buf, 1, recv, 4) == RT_EOK)
    {
        result = RT_EOK;
        rt_kprintf("%s,%d: JEDEC ID read ok\n", __func__, __LINE__);
    }
    else
    {
        result = -RT_ERROR;
        rt_kprintf("%s,%d: JEDEC ID read error\n", __func__, __LINE__);
        goto _exit;
    }

    for (i = 0; i < 4; i++)
    {
        rt_kprintf("ID buf[%d] = 0x%02x, recv[%d] = 0x%02x\n",
                   i, buf[i], i, recv[i]);
    }

    /*
     * 2. 从地址 0x000000 顺序读取 256 字节（将走 SPI 驱动的 DMA 路径）。
     */
    buf[0] = 0x03;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, buf, 8);
    rt_memset(tmp, 0, sizeof(rt_uint8_t) * 1024);
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, tmp, 1024);

    if (rt_spi_send_then_recv(dev, buf, 4, tmp, 256) == RT_EOK)
    {
        result = RT_EOK;
        rt_kprintf("%s,%d: read 256 bytes ok\n", __func__, __LINE__);
    }
    else
    {
        result = -RT_ERROR;
        rt_kprintf("%s,%d: read 256 bytes error\n", __func__, __LINE__);
        goto _exit;
    }

    for (i = 0; i < 30; i++)
    {
        rt_kprintf("R tmp[%d] = 0x%02x\n", i, tmp[i]);
    }

    /*
     * 3. Write Enable (0x06)
     */
    buf[0] = 0x06;
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, buf, 8);
    if (rt_spi_transfer(dev, buf, RT_NULL, 1))
    {
        result = RT_EOK;
        rt_kprintf("%s,%d: WREN ok\n", __func__, __LINE__);
    }
    else
    {
        result = -RT_ERROR;
        rt_kprintf("%s,%d: WREN error\n", __func__, __LINE__);
        goto _exit;
    }

    /*
     * 4. 页写入测试：
     *    先将 tmp 填充 0x12，再在前 4 字节填入写指令 0x02 + 地址 0x000000，
     *    然后通过 rt_spi_transfer 发送 14 字节数据。
     */
    rt_memset(tmp, 0x12, sizeof(rt_uint8_t) * 1024);

    tmp[0] = 0x02;
    tmp[1] = 0x00;
    tmp[2] = 0x00;
    tmp[3] = 0x00;
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, tmp, 1024);

    if (rt_spi_transfer(dev, tmp, RT_NULL, 14))
    {
        result = RT_EOK;
        rt_kprintf("%s,%d: page program ok\n", __func__, __LINE__);
    }
    else
    {
        result = -RT_ERROR;
        rt_kprintf("%s,%d: page program error\n", __func__, __LINE__);
        goto _exit;
    }

    /*
     * 5. Write Disable (0x04)
     */
    buf[0] = 0x04;
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, buf, 8);
    if (rt_spi_transfer(dev, buf, RT_NULL, 1))
    {
        result = RT_EOK;
        rt_kprintf("%s,%d: WRDI ok\n", __func__, __LINE__);
    }
    else
    {
        result = -RT_ERROR;
        rt_kprintf("%s,%d: WRDI error\n", __func__, __LINE__);
        goto _exit;
    }

    /*
     * 6. 再次从 0x000000 读出 256 字节，验证刚才写入的数据。
     */
    buf[0] = 0x03;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, buf, 8);
    rt_memset(tmp, 0xCC, sizeof(rt_uint8_t) * 1024);
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, tmp, 1024);

    tmp = tmp_base + 4;
    rt_kprintf("read-back buf at %p\n", tmp);

    if (rt_spi_send_then_recv(dev, buf, 4, tmp, 256) == RT_EOK)
    {
        result = RT_EOK;
        rt_kprintf("%s,%d: read-back ok tmp=%p\n",
                   __func__, __LINE__, tmp);
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, tmp, 1024);
        for (i = 0; i < 260; i++)
        {
            rt_kprintf("RB tmp[%d] = 0x%02x\n", i, tmp[i]);
        }
    }
    else
    {
        result = -RT_ERROR;
        rt_kprintf("%s,%d: read-back error\n", __func__, __LINE__);
    }

_exit:
    if (buf)
    {
        rt_free(buf);
    }
    if (recv)
    {
        rt_free(recv);
    }
    if (tmp_base)
    {
        rt_free(tmp_base);
    }

    return result;
}

static int dma_k1_spi_test(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	return dma_k1_spi_test_run();
}
MSH_CMD_EXPORT(dma_k1_spi_test, dma_k1_spi_test: K1X SPI+DMA flash test);

#endif /* RT_USING_SPI */
