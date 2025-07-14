/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USART_H
#define __USART_H


#include <rtdef.h>
#include "drv_usart.h"
#include <register_defination.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BAUDRATE_DEFAULT        115200
#define UART_BUSY_TIMEOUT       1000000
#define UART_RECEIVE_TIMEOUT    1000
#define UART_TRANSMIT_TIMEOUT   1000
#define UART_MAX_FIFO           0x10
/* UART register bit definitions */

#define USR_UART_BUSY           0x01
#define USR_UART_TFE            0x04
#define USR_UART_RFNE           0x08
#define LSR_DATA_READY          0x01
#define LSR_THR_EMPTY           0x20
#define IER_RDA_INT_ENABLE      0x01
#define IER_THRE_INT_ENABLE     0x02
#define IIR_RECV_LINE_ENABLE    0x04
#define IIR_NO_ISQ_PEND         0x01
#define UART_IER_UUE		0x40   /* UART Unit Enable */

#define LCR_SET_DLAB            0x80   /* enable r/w DLR to set the baud rate */
#define LCR_PARITY_ENABLE       0x08   /* parity enabled */
#define LCR_PARITY_EVEN         0x10   /* Even parity enabled */
#define LCR_PARITY_ODD          0xef   /* Odd parity enabled */
#define LCR_WORD_SIZE_5         0xfc   /* the data length is 5 bits */
#define LCR_WORD_SIZE_6         0x01   /* the data length is 6 bits */
#define LCR_WORD_SIZE_7         0x02   /* the data length is 7 bits */
#define LCR_WORD_SIZE_8         0x03   /* the data length is 8 bits */
#define LCR_STOP_BIT1           0xfb   /* 1 stop bit */
#define LCR_STOP_BIT2           0x04   /* 1.5 stop bit */

#define DW_LSR_PFE              0x80
#define DW_LSR_TEMT             0x40
#define DW_LSR_THRE             0x40
#define DW_LSR_BI               0x10
#define DW_LSR_FE               0x08
#define DW_LSR_PE               0x04
#define DW_LSR_OE               0x02
#define DW_LSR_DR               0x01
#define DW_LSR_TRANS_EMPTY      0x20

#define DW_IIR_THR_EMPTY        0x02    /* threshold empty */
#define DW_IIR_RECV_DATA        0x04    /* received data available */
#define DW_IIR_RECV_LINE        0x06    /* receiver line status */
#define DW_IIR_CHAR_TIMEOUT     0x0c    /* character timeout */

typedef struct
{
    uintptr_t base;
    rt_uint32_t irq;
    struct clk *clk, *rst;
    usart_event_cb_t cb_event;           ///< Event callback
    rt_uint32_t rx_total_num;
    rt_uint32_t tx_total_num;
    rt_uint8_t *rx_buf;
    rt_uint8_t *tx_buf;
    volatile rt_uint32_t rx_cnt;
    volatile rt_uint32_t tx_cnt;
    volatile rt_uint32_t tx_busy;
    volatile rt_uint32_t rx_busy;
    rt_uint32_t last_tx_num;
    rt_uint32_t last_rx_num;
    rt_int32_t idx;
} ck_usart_priv_t;

#ifdef __cplusplus
}
#endif

#endif /* __USART_H */

