/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include "drv_usart.h"
#include "ck_usart.h"

/*
 * setting config may be accessed when the USART is not
 * busy(USR[0]=0) and the DLAB bit(LCR[7]) is set.
 */

#ifndef SOC_SPACEMIT_K1_X
#define WAIT_USART_IDLE(addr)\
    do {                       \
        rt_int32_t timecount = 0;  \
        while ((addr->USR & USR_UART_BUSY) && (timecount < UART_BUSY_TIMEOUT)) {\
            timecount++;\
        }\
        if (timecount >= UART_BUSY_TIMEOUT) {\
            return -1;\
        }                                   \
    } while(0)
#else
#define WAIT_USART_IDLE(addr)\
    do {                       \
    } while(0)
#endif

typedef struct
{
    rt_uint32_t base;
    rt_uint32_t irq;
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

static ck_usart_priv_t usart_instance[CONFIG_USART_NUM];

/**
  \brief       set the bautrate of usart.
  \param[in]   addr  usart base to operate.
  \return      error code
*/
/* yong */
rt_int32_t csi_usart_config_baudrate(usart_handle_t handle, rt_uint32_t baud)
{
    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);


    WAIT_USART_IDLE(addr);

    /* baudrate=(seriak clock freq)/(16*divisor); algorithm :rounding*/
    rt_uint32_t divisor = ((/* (drv_get_usart_freq(usart_priv->idx) */ UART_FUNC_CLK_FREQ  * 10) / baud) >> 4;

    if ((divisor % 10) >= 5)
    {
        divisor = (divisor / 10) + 1;
    } else
    {
        divisor = divisor / 10;
    }

    addr->LCR |= LCR_SET_DLAB;

    /* DLL and DLH is lower 8-bits and higher 8-bits of divisor.*/
    addr->DLL = divisor & 0xff;
    addr->DLH = (divisor >> 8) & 0xff;
    /*
     * The DLAB must be cleared after the baudrate is setted
     * to access other registers.
     */
    addr->LCR &= (~LCR_SET_DLAB);

    return 0;
}

/**
  \brief       config usart mode.
  \param[in]   handle  usart handle to operate.
  \param[in]   mode    \ref usart_mode_e
  \return      error code
*/
/* yong */
rt_int32_t csi_usart_config_mode(usart_handle_t handle, usart_mode_e mode)
{
    if (mode == USART_MODE_ASYNCHRONOUS)
    {
        return 0;
    }

    return -1;
}

/**
  \brief       config usart parity.
  \param[in]   handle  usart handle to operate.
  \param[in]   parity    \ref usart_parity_e
  \return      error code
*/
/* yong */
rt_int32_t csi_usart_config_parity(usart_handle_t handle, usart_parity_e parity)
{
    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);

    WAIT_USART_IDLE(addr);

    switch (parity)
    {
        case USART_PARITY_NONE:
            /*CLear the PEN bit(LCR[3]) to disable parity.*/
            addr->LCR &= (~LCR_PARITY_ENABLE);
            break;

        case USART_PARITY_ODD:
            /* Set PEN and clear EPS(LCR[4]) to set the ODD parity. */
            addr->LCR |= LCR_PARITY_ENABLE;
            addr->LCR &= LCR_PARITY_ODD;
            break;

        case USART_PARITY_EVEN:
            /* Set PEN and EPS(LCR[4]) to set the EVEN parity.*/
            addr->LCR |= LCR_PARITY_ENABLE;
            addr->LCR |= LCR_PARITY_EVEN;
            break;

        default:
            return -1;
    }

    return 0;
}

/**
  \brief       config usart stop bit number.
  \param[in]   handle  usart handle to operate.
  \param[in]   stopbits  \ref usart_stop_bits_e
  \return      error code
*/
/* yong */
rt_int32_t csi_usart_config_stopbits(usart_handle_t handle, usart_stop_bits_e stopbit)
{
    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);

    WAIT_USART_IDLE(addr);

    switch (stopbit)
    {
        case USART_STOP_BITS_1:
            /* Clear the STOP bit to set 1 stop bit*/
            addr->LCR &= LCR_STOP_BIT1;
            break;

        case USART_STOP_BITS_2:
            /*
            * If the STOP bit is set "1",we'd gotten 1.5 stop
            * bits when DLS(LCR[1:0]) is zero, else 2 stop bits.
            */
            addr->LCR |= LCR_STOP_BIT2;
            break;

        default:
            return -1;
    }

    return 0;
}

/**
  \brief       config usart data length.
  \param[in]   handle  usart handle to operate.
  \param[in]   databits      \ref usart_data_bits_e
  \return      error code
*/
/* yong */
rt_int32_t csi_usart_config_databits(usart_handle_t handle, usart_data_bits_e databits)
{
    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);

    WAIT_USART_IDLE(addr);
    /* The word size decides by the DLS bits(LCR[1:0]), and the
     * corresponding relationship between them is:
     *   DLS   word size
     *       00 -- 5 bits
     *       01 -- 6 bits
     *       10 -- 7 bits
     *       11 -- 8 bits
     */

    switch (databits)
    {
        case USART_DATA_BITS_5:
            addr->LCR &= LCR_WORD_SIZE_5;
            break;

        case USART_DATA_BITS_6:
            addr->LCR &= 0xfd;
            addr->LCR |= LCR_WORD_SIZE_6;
            break;

        case USART_DATA_BITS_7:
            addr->LCR &= 0xfe;
            addr->LCR |= LCR_WORD_SIZE_7;
            break;

        case USART_DATA_BITS_8:
            addr->LCR |= LCR_WORD_SIZE_8;
            break;

        default:
            return -1;
    }

    return 0;
}


/* yong */
rt_int32_t ck_usart_set_int_flag(usart_handle_t handle, rt_uint32_t flag)
{
    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);

    addr->IER |= flag;

    return 0;
}

/* yong */
rt_int32_t ck_usart_clr_int_flag(usart_handle_t handle, rt_uint32_t flag)
{
    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);

    addr->IER &= ~flag;

    return 0;
}


/**
  \brief       get character in query mode.
  \param[in]   instance  usart instance to operate.
  \param[in]   the pointer to the recieve charater.
  \return      error code
*/
/* yong */
rt_int32_t csi_usart_getchar(usart_handle_t handle, rt_uint8_t *ch)
{
    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);

    while (!(addr->LSR & LSR_DATA_READY));

    *ch = addr->RBR;

    return 0;
}


/**
  \brief       get character in query mode.
  \param[in]   instance  usart instance to operate.
  \param[in]   the pointer to the recieve charater.
  \return      error code
*/
/* yong */
int csi_uart_getchar(usart_handle_t handle)
{
    volatile int ch;

    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);

    ch = -1;

    if (addr->LSR & LSR_DATA_READY)
    {
        ch = addr->RBR & 0xff;
    }

    return ch;
}


/**
  \brief       transmit character in query mode.
  \param[in]   instance  usart instance to operate.
  \param[in]   ch  the input charater
  \return      error code
*/
rt_int32_t csi_usart_putchar(usart_handle_t handle, rt_uint8_t ch)
{
    ck_usart_priv_t *usart_priv = handle;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);
    rt_uint32_t timecount = 0;

    //asm volatile("j .");
    while ((!(addr->LSR & DW_LSR_TRANS_EMPTY)))
    {
        timecount++;

        if (timecount >= UART_BUSY_TIMEOUT)
        {
            return -1;
        }
    }

    addr->THR = ch;

    return 0;

}

/**
  \brief       config usart mode.
  \param[in]   handle  usart handle to operate.
  \param[in]   baud      baud rate
  \param[in]   mode      \ref usart_mode_e
  \param[in]   parity    \ref usart_parity_e
  \param[in]   stopbits  \ref usart_stop_bits_e
  \param[in]   bits      \ref usart_data_bits_e
  \return      error code
*/
/* yong */
rt_int32_t csi_usart_config(usart_handle_t handle,
                         rt_uint32_t baud,
                         usart_mode_e mode,
                         usart_parity_e parity,
                         usart_stop_bits_e stopbits,
                         usart_data_bits_e bits)
{
    rt_int32_t ret;

    /* control the data_bit of the usart*/
    ret = csi_usart_config_baudrate(handle, baud);

    if (ret < 0)
    {
        return ret;
    }

    /* control mode of the usart*/
    ret = csi_usart_config_mode(handle, mode);

    if (ret < 0)
    {
        return ret;
    }

    /* control the parity of the usart*/
    ret = csi_usart_config_parity(handle, parity);

    if (ret < 0)
    {
        return ret;
    }

    /* control the stopbit of the usart*/
    ret = csi_usart_config_stopbits(handle, stopbits);

    if (ret < 0)
    {
        return ret;
    }

    ret = csi_usart_config_databits(handle, bits);

    if (ret < 0)
    {
        return ret;
    }

    return 0;
}

extern rt_int32_t target_usart_init(rt_int32_t idx, rt_uint32_t *base, rt_uint32_t *irq, void **handler);

/**
  \brief       Initialize USART Interface. 1. Initializes the resources needed for the USART interface 2.registers event callback function
  \param[in]   idx usart index
  \param[in]   cb_event  Pointer to \ref usart_event_cb_t
  \return      return usart handle if success
*/
usart_handle_t csi_usart_initialize(rt_int32_t idx, usart_event_cb_t cb_event)
{
    rt_uint32_t base = 0u;
    rt_uint32_t irq = 0u;
    void *handler;

    rt_int32_t ret = target_usart_init(idx, &base, &irq, &handler);

    if (ret < 0 || ret >= CONFIG_USART_NUM)
    {
        return RT_NULL;
    }

    ck_usart_priv_t *usart_priv = &usart_instance[idx];
    usart_priv->base = base;
    usart_priv->irq = irq;
    usart_priv->cb_event = cb_event;
    usart_priv->idx = idx;
    ck_usart_reg_t *addr = (ck_usart_reg_t *)(usart_priv->base);

    /* enable received data available */
    addr->IER = IER_RDA_INT_ENABLE | IIR_RECV_LINE_ENABLE;
#ifdef SOC_SPACEMIT_K1_X
    addr->IER |= UART_IER_UUE;
#endif

    return usart_priv;
}
