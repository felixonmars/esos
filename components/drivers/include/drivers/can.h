/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2015-05-14     aubrcool@qq.com   first version
 * 2015-07-06     Bernard           remove RT_CAN_USING_LED.
 */

#ifndef CAN_H_
#define CAN_H_

#include <rtthread.h>

#ifndef RT_CANMSG_BOX_SZ
#define RT_CANMSG_BOX_SZ    16
#endif
#ifndef RT_CANSND_BOX_NUM
#define RT_CANSND_BOX_NUM   1
#endif
enum CAN_DLC
{
    CAN_MSG_0BYTE = 0,
    CAN_MSG_1BYTE,
    CAN_MSG_2BYTES,
    CAN_MSG_3BYTES,
    CAN_MSG_4BYTES,
    CAN_MSG_5BYTES,
    CAN_MSG_6BYTES,
    CAN_MSG_7BYTES,
    CAN_MSG_8BYTES,
    CAN_MSG_12BYTES,
    CAN_MSG_16BYTES,
    CAN_MSG_20BYTES,
    CAN_MSG_24BYTES,
    CAN_MSG_32BYTES,
    CAN_MSG_48BYTES,
    CAN_MSG_64BYTES,
};

enum CANBAUD
{
    CAN1MBaud   = 1000UL * 1000,/* 1 MBit/sec   */
    CAN800kBaud = 1000UL * 800, /* 800 kBit/sec */
    CAN500kBaud = 1000UL * 500, /* 500 kBit/sec */
    CAN250kBaud = 1000UL * 250, /* 250 kBit/sec */
    CAN125kBaud = 1000UL * 125, /* 125 kBit/sec */
    CAN100kBaud = 1000UL * 100, /* 100 kBit/sec */
    CAN50kBaud  = 1000UL * 50,  /* 50 kBit/sec  */
    CAN20kBaud  = 1000UL * 20,  /* 20 kBit/sec  */
    CAN10kBaud  = 1000UL * 10   /* 10 kBit/sec  */
};

#define RT_CAN_MODE_NORMAL              0
#define RT_CAN_MODE_LISTEN              1
#define RT_CAN_MODE_LOOPBACK            2
#define RT_CAN_MODE_LOOPBACKANLISTEN    3

#define RT_CAN_MODE_PRIV                0x01
#define RT_CAN_MODE_NOPRIV              0x00

struct rt_can_filter_item
{
    rt_uint32_t id  : 29;
    rt_uint32_t ide : 1;
    rt_uint32_t rtr : 1;
    rt_uint32_t mode : 1;
    rt_uint32_t mask;
    rt_int32_t hdr;
#ifdef RT_CAN_USING_HDR
    rt_err_t (*ind)(rt_device_t dev, void *args , rt_int32_t hdr, rt_size_t size);
    void *args;
#endif /*RT_CAN_USING_HDR*/
};

#ifdef RT_CAN_USING_HDR
#define RT_CAN_FILTER_ITEM_INIT(id,ide,rtr,mode,mask,ind,args) \
     {(id), (ide), (rtr), (mode), (mask), -1, (ind), (args)}
#define RT_CAN_FILTER_STD_INIT(id,ind,args) \
     RT_CAN_FILTER_ITEM_INIT(id,0,0,0,0xFFFFFFFF,ind,args)
#define RT_CAN_FILTER_EXT_INIT(id,ind,args) \
     RT_CAN_FILTER_ITEM_INIT(id,1,0,0,0xFFFFFFFF,ind,args)
#define RT_CAN_STD_RMT_FILTER_INIT(id,ind,args) \
     RT_CAN_FILTER_ITEM_INIT(id,0,1,0,0xFFFFFFFF,ind,args)
#define RT_CAN_EXT_RMT_FILTER_INIT(id,ind,args) \
     RT_CAN_FILTER_ITEM_INIT(id,1,1,0,0xFFFFFFFF,ind,args)
#define RT_CAN_STD_RMT_DATA_FILTER_INIT(id,ind,args) \
     RT_CAN_FILTER_ITEM_INIT(id,0,0,1,0xFFFFFFFF,ind,args)
#define RT_CAN_EXT_RMT_DATA_FILTER_INIT(id,ind,args) \
     RT_CAN_FILTER_ITEM_INIT(id,1,0,1,0xFFFFFFFF,ind,args)
#else

#define RT_CAN_FILTER_ITEM_INIT(id,ide,rtr,mode,mask) \
     {(id), (ide), (rtr), (mode), (mask), -1, }
#define RT_CAN_FILTER_STD_INIT(id) \
     RT_CAN_FILTER_ITEM_INIT(id,0,0,0,0xFFFFFFFF)
#define RT_CAN_FILTER_EXT_INIT(id) \
     RT_CAN_FILTER_ITEM_INIT(id,1,0,0,0xFFFFFFFF)
#define RT_CAN_STD_RMT_FILTER_INIT(id) \
     RT_CAN_FILTER_ITEM_INIT(id,0,1,0,0xFFFFFFFF)
#define RT_CAN_EXT_RMT_FILTER_INIT(id) \
     RT_CAN_FILTER_ITEM_INIT(id,1,1,0,0xFFFFFFFF)
#define RT_CAN_STD_RMT_DATA_FILTER_INIT(id) \
     RT_CAN_FILTER_ITEM_INIT(id,0,0,1,0xFFFFFFFF)
#define RT_CAN_EXT_RMT_DATA_FILTER_INIT(id) \
     RT_CAN_FILTER_ITEM_INIT(id,1,0,1,0xFFFFFFFF)
#endif

struct rt_can_filter_config
{
    rt_uint32_t count;
    rt_uint32_t actived;
    struct rt_can_filter_item *items;
};

struct rt_can_bit_timing
{
    rt_uint16_t prescaler;  /**< Baud rate prescaler. */
    rt_uint16_t num_seg1;   /**< Bit Timing Segment 1, in terms of Time Quanta (Tq). */
    rt_uint16_t num_seg2;   /**< Bit Timing Segment 2, in terms of Time Quanta (Tq). */
    rt_uint8_t num_sjw;     /**< Synchronization Jump Width, in terms of Time Quanta (Tq). */
    rt_uint8_t num_sspoff;  /**< Secondary Sample Point Offset, in terms of Time Quanta (Tq) for CAN-FD. */
};
struct rt_can_bit_timing_config
{
    rt_uint32_t count;                  /**< The number of bit-timing configurations (typically 1 for CAN, 2 for CAN-FD). */
    struct rt_can_bit_timing *items;    /**< A pointer to an array of bit-timing structures. */
};
struct can_configure
{
    rt_uint32_t baud_rate;
    rt_uint32_t msgboxsz;
    rt_uint32_t sndboxnumber;
    rt_uint32_t mode      : 8;
    rt_uint32_t privmode  : 8;
    rt_uint32_t reserved  : 16;
    rt_uint32_t ticks;
#ifdef RT_CAN_USING_HDR
    rt_uint32_t maxhdr;
#endif

#ifdef RT_CAN_USING_CANFD
    rt_uint32_t baud_rate_fd;       /**< The baud rate for the CAN-FD data phase. */
    rt_uint32_t use_bit_timing: 8;  /**< A flag to indicate that `can_timing` and `canfd_timing` should be used instead of `baud_rate`. */
    rt_uint32_t enable_canfd : 8;   /**< A flag to enable CAN-FD functionality. */
    rt_uint32_t reserved1 : 16;     /**< Reserved for future use. */

    /* The below fields take effect only if use_bit_timing is non-zero */
    struct rt_can_bit_timing can_timing;    /**< Custom bit-timing for the arbitration phase. */
    struct rt_can_bit_timing canfd_timing;  /**< Custom bit-timing for the data phase. */
#endif
};

#define CANDEFAULTCONFIG \
{\
        CAN1MBaud,\
        RT_CANMSG_BOX_SZ,\
        RT_CANSND_BOX_NUM,\
        RT_CAN_MODE_NORMAL,\
};

struct rt_can_ops;
#define RT_CAN_CMD_SET_FILTER       0x13
#define RT_CAN_CMD_SET_BAUD         0x14
#define RT_CAN_CMD_SET_MODE         0x15
#define RT_CAN_CMD_SET_PRIV         0x16
#define RT_CAN_CMD_GET_STATUS       0x17
#define RT_CAN_CMD_SET_STATUS_IND   0x18
#define RT_CAN_CMD_SET_BUS_HOOK     0x19

#define RT_DEVICE_CAN_INT_ERR       0x1000

enum RT_CAN_STATUS_MODE
{
    NORMAL = 0,
    ERRWARNING = 1,
    ERRPASSIVE = 2,
    BUSOFF = 4,
};
enum RT_CAN_BUS_ERR
{
    RT_CAN_BUS_NO_ERR = 0,
    RT_CAN_BUS_BIT_PAD_ERR = 1,
    RT_CAN_BUS_FORMAT_ERR = 2,
    RT_CAN_BUS_ACK_ERR = 3,
    RT_CAN_BUS_IMPLICIT_BIT_ERR = 4,
    RT_CAN_BUS_EXPLICIT_BIT_ERR = 5,
    RT_CAN_BUS_CRC_ERR = 6,
};

struct rt_can_status
{
    rt_uint32_t rcverrcnt;
    rt_uint32_t snderrcnt;
    rt_uint32_t errcode;
    rt_uint32_t rcvpkg;
    rt_uint32_t dropedrcvpkg;
    rt_uint32_t sndpkg;
    rt_uint32_t dropedsndpkg;
    rt_uint32_t bitpaderrcnt;
    rt_uint32_t formaterrcnt;
    rt_uint32_t ackerrcnt;
    rt_uint32_t biterrcnt;
    rt_uint32_t crcerrcnt;
    rt_uint32_t rcvchange;
    rt_uint32_t sndchange;
    rt_uint32_t lasterrtype;
};

#ifdef RT_CAN_USING_HDR
struct rt_can_hdr
{
    rt_uint32_t connected;
    rt_uint32_t msgs;
    struct rt_can_filter_item filter;
    struct rt_list_node list;
};
#endif
struct rt_can_device;
typedef rt_err_t (*rt_canstatus_ind)(struct rt_can_device *, void *);
typedef struct rt_can_status_ind_type
{
    rt_canstatus_ind ind;
    void *args;
} *rt_can_status_ind_type_t;
typedef void (*rt_can_bus_hook)(struct rt_can_device *);
struct rt_can_msg
{
    rt_uint32_t id  : 29;           /**< CAN ID (Standard or Extended). */
    rt_uint32_t ide : 1;            /**< Identifier type: 0=Standard ID, 1=Extended ID. */
    rt_uint32_t rtr : 1;            /**< Frame type: 0=Data Frame, 1=Remote Frame. */
    rt_uint32_t rsv : 1;            /**< Reserved bit. */
    rt_uint32_t len : 8;            /**< Data Length Code (DLC) from 0 to 8. */
    rt_uint32_t priv : 8;           /**< Private data, used to specify the hardware mailbox in private mode. */
    rt_int32_t hdr_index : 8;       /**< For received messages, the index of the hardware filter that matched the message. */
#ifdef RT_CAN_USING_CANFD
    rt_uint32_t fd_frame : 1;       /**< CAN-FD frame indicator. */
    rt_uint32_t brs : 1;            /**< Bit-rate switching indicator for CAN-FD. */
#endif
#ifdef RT_CAN_USING_CANFD
    rt_uint8_t data[64];            /**< CAN-FD message payload (up to 64 bytes). */
#else
    rt_uint8_t data[8];             /**< CAN message payload (up to 8 bytes). */
#endif
};
typedef struct rt_can_msg *rt_can_msg_t;
struct rt_can_device
{
    struct rt_device parent;

    const struct rt_can_ops *ops;
    struct can_configure config;
    struct rt_can_status status;

    rt_uint32_t timerinitflag;
    struct rt_timer timer;

    struct rt_can_status_ind_type status_indicate;
#ifdef RT_CAN_USING_HDR
    struct rt_can_hdr *hdr;
#endif
#ifdef RT_CAN_USING_BUS_HOOK
    rt_can_bus_hook bus_hook;
#endif /*RT_CAN_USING_BUS_HOOK*/
    struct rt_mutex lock;
    void *can_rx;
    void *can_tx;
};
typedef struct rt_can_device *rt_can_t;

#define RT_CAN_STDID 0
#define RT_CAN_EXTID 1
#define RT_CAN_DTR   0
#define RT_CAN_RTR   1

typedef struct rt_can_status *rt_can_status_t;

struct rt_can_msg_list
{
    struct rt_list_node list;
#ifdef RT_CAN_USING_HDR
    struct rt_list_node hdrlist;
    struct rt_can_hdr *owner;
#endif
    struct rt_can_msg data;
};

struct rt_can_rx_fifo
{
    /* software fifo */
    struct rt_can_msg_list *buffer;
    rt_uint32_t freenumbers;
    struct rt_list_node freelist;
    struct rt_list_node uselist;
};

#define RT_CAN_SND_RESULT_OK        0
#define RT_CAN_SND_RESULT_ERR       1
#define RT_CAN_SND_RESULT_WAIT      2

#define RT_CAN_EVENT_RX_IND         0x01    /* Rx indication */
#define RT_CAN_EVENT_TX_DONE        0x02    /* Tx complete   */
#define RT_CAN_EVENT_TX_FAIL        0x03    /* Tx fail   */
#define RT_CAN_EVENT_RX_TIMEOUT     0x05    /* Rx timeout    */
#define RT_CAN_EVENT_RXOF_IND       0x06    /* Rx overflow */

struct rt_can_sndbxinx_list
{
    struct rt_list_node list;
    struct rt_completion completion;
    rt_uint32_t result;
};

struct rt_can_tx_fifo
{
    struct rt_can_sndbxinx_list *buffer;
    struct rt_semaphore sem;
    struct rt_list_node freelist;
};

struct rt_can_ops
{
    rt_err_t (*configure)(struct rt_can_device *can, struct can_configure *cfg);
    rt_err_t (*control)(struct rt_can_device *can, int cmd, void *arg);
    int (*sendmsg)(struct rt_can_device *can, const void *buf, rt_uint32_t boxno);
    int (*recvmsg)(struct rt_can_device *can, void *buf, rt_uint32_t boxno);
};

rt_err_t rt_hw_can_register(struct rt_can_device    *can,
                            const char              *name,
                            const struct rt_can_ops *ops,
                            void                    *data);
void rt_hw_can_isr(struct rt_can_device *can, int event);
#endif /*_CAN_H*/

