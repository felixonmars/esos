/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
/*
 * K1X DMA 内存到内存自测 (自测版本)
 *
 * 面向 RT-Thread DMA 框架的简单 K1X DMA 测试用例，
 * 对应驱动文件: bsp/spacemit/drivers/dma/dma_k1x.c。
 *
 * 命令行用法 (FinSH/MSH):
 *     dma_k1x_test [mode] [channel] [width_bytes] [burst_bytes] [count]
 *
 * mode    : 0 顺序测试(16 通道依次跑)
 *           1 并行测试(16 通道同时跑)
 *           2 单通道测试(只跑 channel 指定通道)
 * channel : mode=2 时使用，指定要测试的通道号 (0~15)，mode=0/1 时忽略
 * width   : 1 / 2 / 4 字节         (默认 4)
 * burst   : 8 / 16 / 32 / 64 字节  (默认 32)
 * count   : 整体测试重复次数       (默认 1)
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <drivers/dma.h>
#include <stdlib.h>

#define DMA_TEST_MAX_CHANNELS 16

extern struct rt_device *g_dev;

struct dma_test_ctx
{
    struct rt_dma_chan *chan;
    struct rt_completion comp;
};

static struct dma_test_ctx g_dma_test_ctx[DMA_TEST_MAX_CHANNELS];
static rt_uint32_t g_multi_chan_num;

static rt_err_t dma_alloc_channels(rt_uint32_t chan_count);
static void dma_release_channels(void);
static rt_err_t dma_run_seq_multi(rt_uint32_t len,
                                  rt_uint32_t burst,
                                  enum rt_dma_slave_buswidth width,
                                  rt_uint32_t chan_start,
                                  rt_uint32_t chan_count);
static rt_err_t dma_run_parallel_multi(rt_uint32_t len,
                                       rt_uint32_t burst,
                                       enum rt_dma_slave_buswidth width,
                                       rt_uint32_t chan_count);

static enum rt_dma_slave_buswidth dma_width_to_enum(int width_bytes)
{
    switch (width_bytes)
    {
    case 1:
        return RT_DMA_SLAVE_BUSWIDTH_1_BYTE;
    case 2:
        return RT_DMA_SLAVE_BUSWIDTH_2_BYTES;
    case 4:
        return RT_DMA_SLAVE_BUSWIDTH_4_BYTES;
    default:
        return RT_DMA_SLAVE_BUSWIDTH_UNDEFINED;
    }
}

static rt_err_t dma_prepare_memcpy(struct rt_dma_chan *chan,
                                         rt_ubase_t src_addr,
                                         rt_ubase_t dst_addr,
                                         rt_uint32_t len,
                                         rt_uint32_t burst_bytes,
                                         enum rt_dma_slave_buswidth width)
{
    rt_err_t result;
    struct rt_dma_slave_config config;
    struct rt_dma_slave_transfer transfer;

    if (!chan)
    {
        return -RT_EINVAL;
    }

    rt_memset(&config, 0, sizeof(config));
    config.direction      = RT_DMA_MEM_TO_MEM;
    config.src_addr       = src_addr;
    config.dst_addr       = dst_addr;
    config.src_addr_width = width;
    config.dst_addr_width = width;
    config.src_maxburst   = burst_bytes;
    config.dst_maxburst   = burst_bytes;

    result = rt_dma_chan_config(chan, &config);
    if (result != RT_EOK)
    {
        return result;
    }

    rt_memset(&transfer, 0, sizeof(transfer));
    transfer.src_addr   = src_addr;
    transfer.dst_addr   = dst_addr;
    transfer.buffer_len = len;

    result = rt_dma_prep_memcpy(chan, &transfer);

    return result;
}



int dma_k1x_test(int argc, char **argv)
{
    int mode = 0;                 /* 0=顺序, 1=并行, 2=单通道 */
    int channel = 0;              /* mode=2 时使用，0~15 */
    int width_bytes = 4;          /* 默认 4 字节 */
    rt_uint32_t burst = 32;       /* 默认 32 字节 */
    rt_uint32_t count = 1;        /* 默认跑 1 轮 */
    rt_uint32_t len = 256;        /* 每次传输长度 */
    rt_uint32_t i;
    enum rt_dma_slave_buswidth width_enum;
    rt_err_t result = RT_EOK;

    if (argc > 1)
    {
        mode = atoi(argv[1]);
    }
    if (argc > 2)
    {
        channel = atoi(argv[2]);
    }
    if (argc > 3)
    {
        width_bytes = atoi(argv[3]);
    }
    if (argc > 4)
    {
        burst = (rt_uint32_t)atoi(argv[4]);
    }
    if (argc > 5)
    {
        count = (rt_uint32_t)atoi(argv[5]);
    }

    if (mode < 0 || mode > 2)
    {
        rt_kprintf("[%s:%d] invalid mode=%d, must be 0/1/2.\n", __func__, __LINE__, mode);
        return -RT_EINVAL;
    }

    if (burst != 8 && burst != 16 && burst != 32 && burst != 64)
    {
        rt_kprintf("[%s:%d] invalid burst=%u, must be 8/16/32/64.\n", __func__, __LINE__, burst);
        return -RT_EINVAL;
    }

    width_enum = dma_width_to_enum(width_bytes);
    if (width_enum == RT_DMA_SLAVE_BUSWIDTH_UNDEFINED)
    {
        rt_kprintf("[%s:%d] invalid width=%d, must be 1/2/4 bytes.\n",
                   __func__, __LINE__, width_bytes);
        return -RT_EINVAL;
    }

    if (count == 0)
    {
        count = 1;
    }

    if (mode == 2)
    {
        if (channel < 0 || channel >= DMA_TEST_MAX_CHANNELS)
        {
            rt_kprintf("[%s:%d] invalid channel=%d, must be 0~15.\n", __func__, __LINE__, channel);
            return -RT_EINVAL;
        }

        for (i = 0; i < count; i++)
        {
            rt_kprintf("[%s:%d] single ch=%d loop %u/%u\n",
                       __func__, __LINE__, channel, (unsigned)(i + 1), (unsigned)count);

            result = dma_alloc_channels((rt_uint32_t)(channel + 1));
            if (result != RT_EOK)
            {
                break;
            }

            if ((rt_uint32_t)channel >= g_multi_chan_num)
            {
                rt_kprintf("[%s:%d] channel=%d out of allocated range (0~%u).\n",
                           __func__, __LINE__, channel,
                           (unsigned)(g_multi_chan_num ? (g_multi_chan_num - 1) : 0));
                result = -RT_EINVAL;
                dma_release_channels();
                break;
            }

            result = dma_run_seq_multi(len, burst, width_enum,
                                       (rt_uint32_t)channel,
                                       1);

            dma_release_channels();

            if (result != RT_EOK)
            {
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            result = dma_alloc_channels(DMA_TEST_MAX_CHANNELS);
            if (result != RT_EOK)
            {
                break;
            }

            rt_kprintf("[%s:%d] mode=%d %uch loop %u/%u\n",
                       __func__, __LINE__, mode,
                       (unsigned)g_multi_chan_num,
                       (unsigned)(i + 1), (unsigned)count);

            if (g_multi_chan_num == 0)
            {
                rt_kprintf("[%s:%d] no DMA channels allocated.\n", __func__, __LINE__);
                result = -RT_EINVAL;
                dma_release_channels();
                break;
            }

            if (mode == 0)
            {
                result = dma_run_seq_multi(len, burst, width_enum,
                                           0,
                                           g_multi_chan_num);
            }
            else /* mode == 1 */
            {
                result = dma_run_parallel_multi(len, burst, width_enum,
                                                g_multi_chan_num);
            }

            dma_release_channels();

            if (result != RT_EOK)
            {
                break;
            }
        }
    }

    return result;
}

MSH_CMD_EXPORT_ALIAS(dma_k1x_test,
                     dma_k1x_test,
                     dma_k1x_test [mode] [channel] [width] [burst] [count]);

static void dma_multi_callback(struct rt_dma_chan *chan, rt_size_t size RT_UNUSED)
{
	rt_uint32_t i;

	for (i = 0; i < g_multi_chan_num; i++)
    {
        if (g_dma_test_ctx[i].chan == chan)
        {
            rt_completion_done(&g_dma_test_ctx[i].comp);
            break;
        }
    }
}

static rt_err_t dma_alloc_channels(rt_uint32_t chan_count)
{
    rt_uint32_t i;
    rt_err_t result = RT_EOK;

    if (g_dev == RT_NULL)
    {
        rt_kprintf("[%s:%d] DMA controller (g_dev) not initialized.\n", __func__, __LINE__);
        return -RT_ENOSYS;
    }

    if (chan_count > DMA_TEST_MAX_CHANNELS)
    {
        chan_count = DMA_TEST_MAX_CHANNELS;
    }

    g_multi_chan_num = 0;

    for (i = 0; i < chan_count; i++)
    {
        struct rt_dma_chan *chan = rt_dma_chan_request(g_dev, RT_NULL);
        if (!chan)
        {
            break;
        }

        g_dma_test_ctx[g_multi_chan_num].chan = chan;
        g_multi_chan_num++;
    }

    if (g_multi_chan_num < chan_count)
    {
        rt_kprintf("[%s:%d] only allocated %u/%u channels, testing with %u channels.\n",
                   __func__, __LINE__,
                   (unsigned)g_multi_chan_num,
                   (unsigned)chan_count,
                   (unsigned)g_multi_chan_num);
    }

    return result;
}

static void dma_release_channels(void)
{
    rt_uint32_t i;

    for (i = 0; i < g_multi_chan_num; i++)
    {
        if (g_dma_test_ctx[i].chan)
        {
            rt_dma_chan_release(g_dma_test_ctx[i].chan);
            g_dma_test_ctx[i].chan = RT_NULL;
        }
    }

    g_multi_chan_num = 0;
}

static rt_err_t dma_run_seq_multi(rt_uint32_t len,
                                  rt_uint32_t burst,
                                  enum rt_dma_slave_buswidth width,
                                  rt_uint32_t chan_start,
                                  rt_uint32_t chan_count)
{
    rt_uint32_t i;
    rt_err_t result = RT_EOK;
    rt_uint32_t end;

    if (g_multi_chan_num == 0)
    {
        rt_kprintf("[%s:%d] no DMA channels allocated.\n", __func__, __LINE__);
        return -RT_EINVAL;
    }

    if (chan_start >= g_multi_chan_num)
    {
        rt_kprintf("[%s:%d] chan_start=%u out of range (0~%u).\n",
                   __func__, __LINE__, (unsigned)chan_start,
                   (unsigned)(g_multi_chan_num - 1));
        return -RT_EINVAL;
    }

    end = chan_start + chan_count;
    if (end > g_multi_chan_num)
    {
        end = g_multi_chan_num;
    }

    for (i = chan_start; i < end; i++)
    {
        struct rt_dma_chan *chan = g_dma_test_ctx[i].chan;
        void *src = RT_NULL;
        void *dst = RT_NULL;

        src = rt_malloc(len);
        dst = rt_malloc(len);
        if (!src || !dst)
        {
            rt_kprintf("[%s:%d] seq: alloc buffer failed on ch%u.\n",
                       __func__, __LINE__, (unsigned)i);
            if (src)
            {
                rt_free(src);
            }
            if (dst)
            {
                rt_free(dst);
            }
            result = -RT_ENOMEM;
            break;
        }

        memset(src, 0xA5, len);
        memset(dst, 0x5A, len);
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, src, len);
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, dst, len);

        rt_completion_init(&g_dma_test_ctx[i].comp);
        chan->callback = dma_multi_callback;

        result = dma_prepare_memcpy(chan,
                                          (rt_ubase_t)src,
                                          (rt_ubase_t)dst,
                                          len,
                                          burst,
                                          width);
        if (result != RT_EOK)
        {
            rt_kprintf("[%s:%d] seq: prepare_memcpy failed on ch%u: %d\n",
                       __func__, __LINE__, (unsigned)i, result);
            rt_free(src);
            rt_free(dst);
            break;
        }

        result = rt_dma_chan_start(chan);
        if (result != RT_EOK)
        {
            rt_kprintf("[%s:%d] seq: start failed on ch%u: %d\n",
                       __func__, __LINE__, (unsigned)i, result);
            rt_free(src);
            rt_free(dst);
            break;
        }

        rt_kprintf("[%s:%d] seq: ch%u started, waiting...\n",
                   __func__, __LINE__, (unsigned)i);

        result = rt_completion_wait(&g_dma_test_ctx[i].comp,
                                    rt_tick_from_millisecond(1000));
        if (result != RT_EOK)
        {
            rt_kprintf("[%s:%d] seq: timeout on ch%u: %d\n",
                       __func__, __LINE__, (unsigned)i, result);
            rt_dma_chan_stop(chan);
            rt_free(src);
            rt_free(dst);
            break;
        }

        rt_dma_chan_stop(chan);

        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, dst, len);

        if (memcmp(src, dst, len) == 0)
        {
            rt_kprintf("[%s:%d] seq: ch%u SUCCESS.\n", __func__, __LINE__, (unsigned)i);
        }
        else
        {
            rt_kprintf("[%s:%d] seq: ch%u data mismatch.\n",
                       __func__, __LINE__, (unsigned)i);
            result = -RT_ERROR;
            rt_free(src);
            rt_free(dst);
            break;
        }

        rt_free(src);
        rt_free(dst);
    }

    return result;
}

static rt_err_t dma_run_parallel_multi(rt_uint32_t len,
                                              rt_uint32_t burst,
                                              enum rt_dma_slave_buswidth width,
                                              rt_uint32_t chan_count)
{
    rt_uint32_t i;
    rt_err_t result = RT_EOK;
    void *src[DMA_TEST_MAX_CHANNELS];
    void *dst[DMA_TEST_MAX_CHANNELS];

    if (chan_count > g_multi_chan_num)
    {
        chan_count = g_multi_chan_num;
    }

    for (i = 0; i < chan_count; i++)
    {
        src[i] = RT_NULL;
        dst[i] = RT_NULL;
    }

    /* allocate and prepare all channels */
    for (i = 0; i < chan_count; i++)
    {
        struct rt_dma_chan *chan = g_dma_test_ctx[i].chan;

        src[i] = rt_malloc(len);
        dst[i] = rt_malloc(len);
        if (!src[i] || !dst[i])
        {
            rt_kprintf("[%s:%d] par: alloc buffer failed on ch%u.\n",
                       __func__, __LINE__, (unsigned)i);
            result = -RT_ENOMEM;
            goto cleanup;
        }

        memset(src[i], 0xA5, len);
        memset(dst[i], 0x5A, len);
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, src[i], len);
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, dst[i], len);

        rt_completion_init(&g_dma_test_ctx[i].comp);
        chan->callback = dma_multi_callback;

        result = dma_prepare_memcpy(chan,
                                          (rt_ubase_t)src[i],
                                          (rt_ubase_t)dst[i],
                                          len,
                                          burst,
                                          width);
        if (result != RT_EOK)
        {
            rt_kprintf("[%s:%d] par: prepare_memcpy failed on ch%u: %d\n",
                       __func__, __LINE__, (unsigned)i, result);
            goto cleanup;
        }
    }

    /* start all */
    for (i = 0; i < chan_count; i++)
    {
        result = rt_dma_chan_start(g_dma_test_ctx[i].chan);
        if (result != RT_EOK)
        {
            rt_kprintf("[%s:%d] par: start failed on ch%u: %d\n",
                       __func__, __LINE__, (unsigned)i, result);
            goto cleanup;
        }
    }

    rt_kprintf("[%s:%d] par: started %u channels, waiting...\n",
               __func__, __LINE__, (unsigned)chan_count);

    /* wait all */
    for (i = 0; i < chan_count; i++)
    {
        rt_err_t r = rt_completion_wait(&g_dma_test_ctx[i].comp,
                                        rt_tick_from_millisecond(1000));
        if (r != RT_EOK)
        {
            rt_kprintf("[%s:%d] par: timeout on ch%u: %d\n",
                       __func__, __LINE__, (unsigned)i, r);
            result = r;
        }
    }

    /* stop and verify all */
    for (i = 0; i < chan_count; i++)
    {
        struct rt_dma_chan *chan = g_dma_test_ctx[i].chan;

        rt_dma_chan_stop(chan);

        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, dst[i], len);

        if (memcmp(src[i], dst[i], len) == 0)
        {
            rt_kprintf("[%s:%d] par: ch%u SUCCESS.\n", __func__, __LINE__, (unsigned)i);
        }
        else
        {
            rt_kprintf("[%s:%d] par: ch%u data mismatch.\n",
                       __func__, __LINE__, (unsigned)i);
            result = -RT_ERROR;
        }
    }

cleanup:
    for (i = 0; i < chan_count; i++)
    {
        if (src[i])
        {
            rt_free(src[i]);
        }
        if (dst[i])
        {
            rt_free(dst[i]);
        }
    }

    return result;
}
