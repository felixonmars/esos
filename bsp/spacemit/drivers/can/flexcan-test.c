/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * spacemit test driver for flexcan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>

#ifdef RT_USING_FINSH
static struct rt_semaphore rx_sem;
static uint32_t rx_status = -1;
static rt_err_t can_rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

static void can_rx_thread(void *parameter)
{
    int i;
    rt_err_t res;
    struct rt_can_msg rx_msg = {0};
    rt_device_t can_dev = (rt_device_t)parameter;

    rx_status = 0xaa;
    rt_device_set_rx_indicate(can_dev, can_rx_callback);

    while (1)
    {
        /* wait for rx done */
        if (rt_sem_take(&rx_sem, RT_WAITING_FOREVER) == RT_EOK)
        {
            rt_device_read(can_dev, 0, &rx_msg, sizeof(rx_msg));

            rt_kprintf("ID:%0*x", rx_msg.ide ? 8 : 3, rx_msg.id);
            rt_kprintf(" data:");
            for (i = 0; i < rx_msg.len; i++)
            {
                rt_kprintf("%02x ", rx_msg.data[i]);
            }
            rt_kprintf("\n");
        }
    }
}

void can_loopback_sample(int argc, char *argv[])
{
    struct rt_can_msg tx_msg = {0};
    rt_err_t res;
    rt_size_t size;
    rt_thread_t thread;
    rt_device_t can_dev;
    char can_name[RT_NAME_MAX];

    struct can_configure can_cfg = {0};

    if (argc == 2)
    {
        rt_strncpy(can_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(can_name, "can1", RT_NAME_MAX);
    }

    can_dev = rt_device_find(can_name);
    if (!can_dev)
    {
        rt_kprintf("find %s failed!\n", can_name);
        return;
    }

    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

    can_cfg.baud_rate = CAN500kBaud;
    can_cfg.mode = RT_CAN_MODE_LOOPBACK;
    can_cfg.msgboxsz = 14;
    can_cfg.privmode = RT_CAN_MODE_NOPRIV;
    can_cfg.baud_rate_fd = CAN1MBaud;
    can_cfg.enable_canfd = 1;

    res = rt_device_control(can_dev, RT_DEVICE_CTRL_CONFIG, (void *)&can_cfg);
    if (res != RT_EOK)
    {
        rt_kprintf("set configure failed! code: %d\n", res);
        return;
    }

    res = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    if (res != RT_EOK)
    {
        rt_kprintf("open %s failed! code: %d\n", can_name, res);
        return;
    }

    thread = rt_thread_create("can_rx", can_rx_thread, can_dev, 1024, 1, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        rt_kprintf("create can_rx thread failed!\n");
        goto exit;
    }

    tx_msg.id = 0x123;
    tx_msg.ide = RT_CAN_STDID;
    tx_msg.rtr = RT_CAN_DTR;
    tx_msg.len = 12;
    tx_msg.fd_frame = 1;
    tx_msg.brs = 1;

    tx_msg.data[0] = 0x00;
    tx_msg.data[1] = 0x11;
    tx_msg.data[2] = 0x22;
    tx_msg.data[3] = 0x33;
    tx_msg.data[4] = 0x44;
    tx_msg.data[5] = 0x55;
    tx_msg.data[6] = 0x66;
    tx_msg.data[7] = 0x77;
    tx_msg.data[8] = 0x88;
    tx_msg.data[9] = 0x99;
    tx_msg.data[10] = 0xaa;
    tx_msg.data[11] = 0xbb;

    rt_kprintf("Start CAN loopback test...\n");
    rt_kprintf("Configuration: baud_rate=%d, mode=loopback\n", can_cfg.baud_rate);
    rt_kprintf("Send data: ID=0x%x, data=", tx_msg.id);
    for (int i = 0; i < 12; i++)
    {
        rt_kprintf("%02x ", tx_msg.data[i]);
    }
    rt_kprintf("\n");

    size = rt_device_write(can_dev, 0, &tx_msg, sizeof(tx_msg));
    if (size == 0)
    {
        rt_kprintf("can dev write data failed!\n");
    }
    else
    {
        rt_kprintf("send success, waiting receive...\n");
    }

    return;

exit:
    rt_device_close(can_dev);
}

MSH_CMD_EXPORT(can_loopback_sample, CAN loopback test sample using struct can_configure);
#endif
