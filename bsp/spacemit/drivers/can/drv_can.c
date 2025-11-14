/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-06-27     misonyo     the first version.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_can.h"
#include "fsl_flexcan.h"

#define RX_MB_COUNT     12
static flexcan_fd_frame_t frame[RX_MB_COUNT];    /* one frame buffer per RX MB */
static rt_uint32_t filter_mask = 0;

#define BSWAP32(x)      ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) << 8) | \
                         (((x) & 0x00ff0000) >> 8) | \
                         (((x) & 0xff000000) >> 24))

struct flexcan_chip
{
    char *name;
    CAN_Type *base;
    rt_int32_t irq;
    struct clk *clk;
    struct clk *rst;
    rt_uint32_t freq;
    flexcan_handle_t handle;
    struct rt_can_device can_dev;
};

static void flexcan_callback(CAN_Type *base, flexcan_handle_t *handle, int32_t status, uint32_t result, void *userData)
{
    struct flexcan_chip *can;
    flexcan_mb_transfer_t rxXfer;

    can = (struct flexcan_chip *)userData;

    switch (status)
    {
    case kStatus_FLEXCAN_RxIdle:
        rt_hw_can_isr(&can->can_dev, RT_CAN_EVENT_RX_IND | result << 8);
        rxXfer.framefd = &frame[result - 1];
        rxXfer.mbIdx = result;
        FLEXCAN_TransferFDReceiveNonBlocking(can->base, &can->handle, &rxXfer);
        break;

    case kStatus_FLEXCAN_TxIdle:
        rt_hw_can_isr(&can->can_dev, RT_CAN_EVENT_TX_DONE | (13 - result) << 8);
        break;

    case kStatus_FLEXCAN_WakeUp:
    case kStatus_FLEXCAN_ErrorStatus:
        break;

    case kStatus_FLEXCAN_TxSwitchToRx:
        break;

    default:
        break;
    }
}

static rt_err_t can_cfg(struct rt_can_device *can_dev, struct can_configure *cfg)
{
    struct flexcan_chip *can;
    flexcan_config_t config;
    rt_uint32_t res = RT_EOK;
    flexcan_rx_mb_config_t mbConfig;
    flexcan_mb_transfer_t rxXfer;
    rt_uint8_t i, mailbox;

    RT_ASSERT(can_dev != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    can = (struct flexcan_chip *)can_dev->parent.user_data;
    RT_ASSERT(can != RT_NULL);

    FLEXCAN_GetDefaultConfig(&config);
    config.baudRate = cfg->baud_rate;
    if (cfg->enable_canfd == 1)
        config.baudRateFD = cfg->baud_rate_fd;
    config.maxMbNum = 14;               /* all series have 14 MB */
    config.enableIndividMask = true;    /* one filter per MB */
    switch (cfg->mode)
    {
    case RT_CAN_MODE_NORMAL:
        /* default mode */
        break;
    case RT_CAN_MODE_LISTEN:
        break;
    case RT_CAN_MODE_LOOPBACK:
        config.enableLoopBack = true;
        break;
    case RT_CAN_MODE_LOOPBACKANLISTEN:
        break;
    }

    if (cfg->enable_canfd == 1) {
        config.baudRateFD = cfg->baud_rate_fd;
        FLEXCAN_FDInit(can->base, &config, can->freq, true);
    } else
        FLEXCAN_Init(can->base, &config, can->freq);
    FLEXCAN_TransferCreateHandle(can->base, &can->handle, flexcan_callback, can);
    /* init RX_MB_COUNT RX MB to default status */
    mbConfig.format = kFLEXCAN_FrameFormatStandard;  /* standard ID */
    mbConfig.type = kFLEXCAN_FrameTypeData;          /* data frame */
    mbConfig.id = FLEXCAN_ID_STD(0);                 /* default ID is 0 */
    for (i = 0; i < RX_MB_COUNT; i++)
    {
        /* the used MB index from 1 to RX_MB_COUNT */
        mailbox = i + 1;
        /* all ID bit in the filter is "don't care" */
        FLEXCAN_SetRxIndividualMask(can->base, mailbox, FLEXCAN_RX_MB_STD_MASK(0, 0, 0));
        FLEXCAN_SetFDRxMbConfig(can->base, mailbox, &mbConfig, true);
        /* one frame buffer per MB */
        rxXfer.framefd = &frame[i];
        rxXfer.mbIdx = mailbox;
        FLEXCAN_TransferFDReceiveNonBlocking(can->base, &can->handle, &rxXfer);
    }

    return res;
}

static rt_err_t can_control(struct rt_can_device *can_dev, int cmd, void *arg)
{
    struct flexcan_chip *spcan;
    rt_uint32_t argval, mask;
    rt_uint32_t res = RT_EOK;
    flexcan_rx_mb_config_t mbConfig;
    struct rt_can_filter_config  *cfg;
    struct rt_can_filter_item *item;
    rt_uint8_t i, count, index;

    RT_ASSERT(can_dev != RT_NULL);

    spcan = (struct flexcan_chip *)can_dev->parent.user_data;
    RT_ASSERT(spcan != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_SET_INT:
        argval = *(rt_uint32_t *)arg;
        if (argval == RT_DEVICE_FLAG_INT_RX)
        {
            mask = kFLEXCAN_RxWarningInterruptEnable;
        }
        else if (argval == RT_DEVICE_FLAG_INT_TX)
        {
            mask = kFLEXCAN_TxWarningInterruptEnable;
        }
        else if (argval == RT_DEVICE_CAN_INT_ERR)
        {
            mask = kFLEXCAN_ErrorInterruptEnable;
        }
        FLEXCAN_EnableInterrupts(spcan->base, mask);
        break;
    case RT_DEVICE_CTRL_CLR_INT:
        /* each CAN device have one IRQ number. */
        rt_hw_interrupt_mask(spcan->irq);
        break;
    case RT_CAN_CMD_SET_FILTER:
        cfg = (struct rt_can_filter_config *)arg;
        item = cfg->items;
        count = cfg->count;

        if (filter_mask == 0xffffffff)
        {
            rt_kprintf("%s filter is full!\n", spcan->name);
            res = RT_ERROR;
            break;
        }
        else if (filter_mask == 0)
        {
            /* deinit all init RX MB */
            for (i = 0; i < RX_MB_COUNT; i++)
            {
                FLEXCAN_SetRxMbConfig(spcan->base, i + 1, RT_NULL, false);
            }
        }

        while (count)
        {
            if (item->ide)
            {
                mbConfig.format = kFLEXCAN_FrameFormatExtend;
                mbConfig.id = FLEXCAN_ID_EXT(item->id);
                mask = FLEXCAN_RX_MB_EXT_MASK(item->mask, 0, 0);
            }
            else
            {
                mbConfig.format = kFLEXCAN_FrameFormatStandard;
                mbConfig.id = FLEXCAN_ID_STD(item->id);
                mask = FLEXCAN_RX_MB_STD_MASK(item->mask, 0, 0);
            }

            if (item->rtr)
            {
                mbConfig.type = kFLEXCAN_FrameTypeRemote;
            }
            else
            {
                mbConfig.type = kFLEXCAN_FrameTypeData;
            }

            /* user does not specify hdr index,set hdr from RX MB 1 */
            if (item->hdr == -1)
            {

                for (i = 0; i < 32; i++)
                {
                    if (!(filter_mask & (1 << i)))
                    {
                        index = i;
                        break;
                    }
                }
            }
            else    /* use user specified hdr */
            {
                if (filter_mask & (1 << item->hdr))
                {
                    res = RT_ERROR;
                    rt_kprintf("%s hdr%d filter already set!\n", spcan->name, item->hdr);
                    break;
                }
                else
                {
                    index = item->hdr;
                }
            }

            /* RX MB index from 1 to 32,hdr index 0~31 map RX MB index 1~32. */
            FLEXCAN_SetRxIndividualMask(spcan->base, index + 1, mask);
            FLEXCAN_SetRxMbConfig(spcan->base, index + 1, &mbConfig, true);
            filter_mask |= 1 << index;

            item++;
            count--;
        }

        break;

    case RT_CAN_CMD_SET_BAUD:
        res = RT_ERROR;
        break;
    case RT_CAN_CMD_SET_MODE:
        res = RT_ERROR;
        break;

    case RT_CAN_CMD_SET_PRIV:
        res = RT_ERROR;
        break;
    case RT_CAN_CMD_GET_STATUS:
        FLEXCAN_GetBusErrCount(spcan->base, (rt_uint8_t *)(&spcan->can_dev.status.snderrcnt), (rt_uint8_t *)(&spcan->can_dev.status.rcverrcnt));
        rt_memcpy(arg, &spcan->can_dev.status, sizeof(spcan->can_dev.status));
        break;
    default:
        res = RT_ERROR;
        break;
    }

    return res;
}

static int can_send(struct rt_can_device *can_dev, const void *buf, rt_uint32_t boxno)
{
    struct flexcan_chip *can;
    struct rt_can_msg *msg;
    int32_t ret;
    flexcan_fd_frame_t frame;
    flexcan_mb_transfer_t txXfer;
    rt_uint8_t sendMB;

    RT_ASSERT(can_dev != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    can = (struct flexcan_chip *)can_dev->parent.user_data;
    msg = (struct rt_can_msg *) buf;
    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(msg != RT_NULL);

    /* use the last 16 MB to send msg */
    sendMB = 13 - boxno;
    FLEXCAN_SetFDTxMbConfig(can->base, sendMB, true);

    if (RT_CAN_STDID == msg->ide)
    {
        frame.id = FLEXCAN_ID_STD(msg->id);
        frame.format = kFLEXCAN_FrameFormatStandard;
    }
    else if (RT_CAN_EXTID == msg->ide)
    {
        frame.id = FLEXCAN_ID_EXT(msg->id);
        frame.format = kFLEXCAN_FrameFormatExtend;
    }

    if (RT_CAN_DTR == msg->rtr)
    {
        frame.type = kFLEXCAN_FrameTypeData;
    }
    else if (RT_CAN_RTR == msg->rtr)
    {
        frame.type = kFLEXCAN_FrameTypeRemote;
    }

    frame.length = msg->len;
    frame.brs = msg->brs;
    for (int i = 0; i < msg->len; i += 4)
    {
        frame.dataWord[i / 4] = BSWAP32(*((uint32_t *)&msg->data[i]));
    }

    txXfer.mbIdx = sendMB;
    txXfer.framefd = &frame;

    ret = FLEXCAN_TransferFDSendNonBlocking(can->base, &can->handle, &txXfer);
    switch (ret)
    {
    case kStatus_Success:
        ret = RT_EOK;
        break;
    case kStatus_Fail:
        ret = RT_ERROR;
        break;
    case kStatus_FLEXCAN_TxBusy:
        ret = RT_EBUSY;
        break;
    }

    return ret;
}

static int can_recv(struct rt_can_device *can_dev, void *buf, rt_uint32_t boxno)
{
    struct flexcan_chip *can;
    struct rt_can_msg *pmsg;
    rt_uint8_t index;

    RT_ASSERT(can_dev != RT_NULL);

    can = (struct flexcan_chip *)can_dev->parent.user_data;
    pmsg = (struct rt_can_msg *) buf;
    RT_ASSERT(can != RT_NULL);

    index = boxno - 1;

    if (frame[index].format == kFLEXCAN_FrameFormatStandard)
    {
        pmsg->ide = RT_CAN_STDID;
        pmsg->id = frame[index].id >> CAN_ID_STD_SHIFT;
    }
    else
    {
        pmsg->ide = RT_CAN_EXTID;
        pmsg->id = frame[index].id >> CAN_ID_EXT_SHIFT;
    }

    if (frame[index].type == kFLEXCAN_FrameTypeData)
    {
        pmsg->rtr = RT_CAN_DTR;
    }
    else if (frame[index].type == kFLEXCAN_FrameTypeRemote)
    {
        pmsg->rtr = RT_CAN_RTR;
    }
    pmsg->hdr_index = index;      /* one hdr filter per MB */
    pmsg->len = frame[index].length;
    for (int i = 0; i < pmsg->len; i += 4)
    {
        *(uint32_t *)&pmsg->data[i] = BSWAP32(frame[index].dataWord[i / 4]);
    }

    return 0;
}

static struct rt_can_ops spacemit_can_ops =
{
    .configure    = can_cfg,
    .control      = can_control,
    .sendmsg      = can_send,
    .recvmsg      = can_recv,
};

static struct dtb_compatible_array __compatible[] = {
    { .compatible = "spacemit,flexcan0", .data = "flexcan0" },
    { .compatible = "spacemit,flexcan1", .data = "flexcan1" },
    { .compatible = "spacemit,flexcan2", .data = "flexcan2" },
    { .compatible = "spacemit,flexcan3", .data = "flexcan3" },
    { .compatible = "spacemit,flexcan4", .data = "flexcan4" },
    {},
};

static int spacemit_flexcan_init(void)
{
    int i;
    struct flexcan_chip *spcan;
    struct dtb_node *compatible_node;
    struct dtb_node *dtb_head_node = get_dtb_node_head();
    struct can_configure config = CANDEFAULTCONFIG;
    rt_err_t ret = RT_EOK;

    config.privmode = 0;
    config.ticks = 50;
    config.sndboxnumber = 16;
    config.msgboxsz = RX_MB_COUNT;

    for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
        compatible_node = dtb_node_find_compatible_node(dtb_head_node,
            __compatible[i].compatible);
        if (compatible_node == RT_NULL)
            break;
        spcan = (struct flexcan_chip *)rt_calloc(1, sizeof(struct flexcan_chip));
        if (!spcan) {
            rt_kprintf("%s:%d, calloc failed\n", __func__, __LINE__);
            return -RT_ENOMEM;
        }
        spcan->name = (char *)__compatible[i].data;
        spcan->base = (CAN_Type *)dtb_node_get_addr_index(compatible_node, 0);
        if (spcan->base == RT_NULL) {
            rt_kprintf("get reg of flexcan error\n");
            return -RT_ERROR;
        }
        spcan->clk = of_clk_get(compatible_node, 0);
        if (IS_ERR(spcan->clk)) {
            rt_kprintf("get flexcan clk failed\n");
            return -RT_ERROR;
        }

        spcan->rst = of_clk_get(compatible_node, 1);
        if (IS_ERR(spcan->rst)) {
            rt_kprintf("get flexcan rst failed\n");
            return -RT_ERROR;
        }

        spcan->irq = dtb_node_irq_get(compatible_node, 0);
        if (spcan->irq <= 0) {
            rt_kprintf("get irq of uart error\n");
            return -RT_ERROR;
        }

        clk_prepare_enable(spcan->clk);
        clk_prepare_enable(spcan->rst);

        if (dtb_node_read_u32(compatible_node, "clock-frequency", &spcan->freq)) {
            rt_kprintf("get frequency failed\n");
            return RT_ERROR;
        }
        clk_set_rate(spcan->clk, spcan->freq);

        spcan->can_dev.config = config;

        rt_hw_interrupt_install(spcan->irq, CAN_DriverIRQHandler, (void *)spcan->base, RT_NULL);
        rt_hw_interrupt_umask(spcan->irq);
        ret = rt_hw_can_register(&spcan->can_dev, spcan->name, &spacemit_can_ops, spcan);
        if (ret) {
            rt_kprintf("register can dev failed\n");
            return ret;
        }
    }

    return 0;
}
INIT_DEVICE_EXPORT(spacemit_flexcan_init);
