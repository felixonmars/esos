/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_uart.h"
#include "pxa_uart.h"

/*
 * setting config may be accessed when the UART is not
 * busy(USR[0]=0) and the DLAB bit(LCR[7]) is set.
 */

static pxa_uart_priv_t *uart_instance;

/**
  \brief       set the bautrate of uart.
  \param[in]   addr  uart base to operate.
  \return      error code
*/
/* yong */
rt_int32_t pxa_uart_config_baudrate(uart_handle_t handle, rt_uint32_t baud)
{
    pxa_uart_priv_t *uart_priv = handle;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);

    /* baudrate=(seriak clock freq)/(16*divisor); algorithm :rounding*/
    rt_uint32_t divisor = ((clk_get_rate(uart_priv->clk) * 10) / baud) >> 4;

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
  \brief       config uart mode.
  \param[in]   handle  uart handle to operate.
  \param[in]   mode    \ref uart_mode_e
  \return      error code
*/
/* yong */
rt_int32_t pxa_uart_config_mode(uart_handle_t handle, uart_mode_e mode)
{
    if (mode == UART_MODE_ASYNCHRONOUS)
    {
        return 0;
    }

    return -1;
}

/**
  \brief       config uart parity.
  \param[in]   handle  uart handle to operate.
  \param[in]   parity    \ref uart_parity_e
  \return      error code
*/
/* yong */
rt_int32_t pxa_uart_config_parity(uart_handle_t handle, uart_parity_e parity)
{
    pxa_uart_priv_t *uart_priv = handle;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);

    switch (parity)
    {
        case UART_PARITY_NONE:
            /*CLear the PEN bit(LCR[3]) to disable parity.*/
            addr->LCR &= (~LCR_PARITY_ENABLE);
            break;

        case UART_PARITY_ODD:
            /* Set PEN and clear EPS(LCR[4]) to set the ODD parity. */
            addr->LCR |= LCR_PARITY_ENABLE;
            addr->LCR &= LCR_PARITY_ODD;
            break;

        case UART_PARITY_EVEN:
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
  \brief       config uart stop bit number.
  \param[in]   handle  uart handle to operate.
  \param[in]   stopbits  \ref uart_stop_bits_e
  \return      error code
*/
/* yong */
rt_int32_t pxa_uart_config_stopbits(uart_handle_t handle, uart_stop_bits_e stopbit)
{
    pxa_uart_priv_t *uart_priv = handle;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);

    switch (stopbit)
    {
        case UART_STOP_BITS_1:
            /* Clear the STOP bit to set 1 stop bit*/
            addr->LCR &= LCR_STOP_BIT1;
            break;

        case UART_STOP_BITS_2:
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
  \brief       config uart data length.
  \param[in]   handle  uart handle to operate.
  \param[in]   databits      \ref uart_data_bits_e
  \return      error code
*/
/* yong */
rt_int32_t pxa_uart_config_databits(uart_handle_t handle, uart_data_bits_e databits)
{
    pxa_uart_priv_t *uart_priv = handle;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);

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
        case UART_DATA_BITS_5:
            addr->LCR &= LCR_WORD_SIZE_5;
            break;

        case UART_DATA_BITS_6:
            addr->LCR &= 0xfd;
            addr->LCR |= LCR_WORD_SIZE_6;
            break;

        case UART_DATA_BITS_7:
            addr->LCR &= 0xfe;
            addr->LCR |= LCR_WORD_SIZE_7;
            break;

        case UART_DATA_BITS_8:
            addr->LCR |= LCR_WORD_SIZE_8;
            break;

        default:
            return -1;
    }

    return 0;
}


/* yong */
rt_int32_t pxa_uart_set_int_flag(uart_handle_t handle, rt_uint32_t flag)
{
    pxa_uart_priv_t *uart_priv = handle;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);

    addr->IER |= flag;

    return 0;
}

/* yong */
rt_int32_t pxa_uart_clr_int_flag(uart_handle_t handle, rt_uint32_t flag)
{
    pxa_uart_priv_t *uart_priv = handle;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);

    addr->IER &= ~flag;

    return 0;
}

/**
  \brief       get character in query mode.
  \param[in]   instance  uart instance to operate.
  \param[in]   the pointer to the recieve charater.
  \return      error code
*/
/* yong */
rt_int32_t pxa_uart_getchar(uart_handle_t handle)
{
    volatile rt_int32_t ch;

    pxa_uart_priv_t *uart_priv = handle;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);

    ch = -1;

    if (addr->LSR & LSR_DATA_READY)
    {
        ch = addr->RBR & 0xff;
    }

    return ch;
}


/**
  \brief       transmit character in query mode.
  \param[in]   instance  uart instance to operate.
  \param[in]   ch  the input charater
  \return      error code
*/
rt_int32_t pxa_uart_putchar(uart_handle_t handle, rt_uint8_t ch)
{
    pxa_uart_priv_t *uart_priv = handle;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);
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
  \brief       config uart mode.
  \param[in]   handle  uart handle to operate.
  \param[in]   baud      baud rate
  \param[in]   mode      \ref uart_mode_e
  \param[in]   parity    \ref uart_parity_e
  \param[in]   stopbits  \ref uart_stop_bits_e
  \param[in]   bits      \ref uart_data_bits_e
  \return      error code
*/
/* yong */
rt_int32_t pxa_uart_config(uart_handle_t handle,
                         rt_uint32_t baud,
                         uart_mode_e mode,
                         uart_parity_e parity,
                         uart_stop_bits_e stopbits,
                         uart_data_bits_e bits)
{
    rt_int32_t ret;

#ifndef SOC_SPACEMIT_K3
    /* control the data_bit of the uart*/
    ret = pxa_uart_config_baudrate(handle, baud);

    if (ret < 0)
    {
        return ret;
    }
#endif
    /* control mode of the uart*/
    ret = pxa_uart_config_mode(handle, mode);

    if (ret < 0)
    {
        return ret;
    }

    /* control the parity of the uart*/
    ret = pxa_uart_config_parity(handle, parity);

    if (ret < 0)
    {
        return ret;
    }

    /* control the stopbit of the uart*/
    ret = pxa_uart_config_stopbits(handle, stopbits);

    if (ret < 0)
    {
        return ret;
    }

    ret = pxa_uart_config_databits(handle, bits);

    if (ret < 0)
    {
        return ret;
    }

    return 0;
}

extern rt_int32_t target_uart_init(rt_int32_t idx, rt_uint32_t *base, rt_uint32_t *irq, void **handler);

/**
  \brief       Initialize UART Interface. 1. Initializes the resources needed for the UART interface 2.registers event callback function
  \param[in]   idx uart index
  \param[in]   cb_event  Pointer to \ref uart_event_cb_t
  \return      return uart handle if success
*/
uart_handle_t pxa_uart_initialize(rt_int32_t idx, uart_event_cb_t cb_event)
{
    rt_uint32_t base = 0u;
    rt_uint32_t irq = 0u;
    void *handler;

    rt_int32_t ret = target_uart_init(idx, &base, &irq, &handler);

    if (ret < 0)
    {
        return RT_NULL;
    }

    pxa_uart_priv_t *uart_priv = &uart_instance[idx];
    uart_priv->base = base;
    uart_priv->irq = irq;
    uart_priv->cb_event = cb_event;
    uart_priv->idx = idx;
    pxa_uart_reg_t *addr = (pxa_uart_reg_t *)(uart_priv->base);

    /* enable received data available */
    addr->IER = IER_RDA_INT_ENABLE | IIR_RECV_LINE_ENABLE;
    addr->IER |= UART_IER_UUE;

    return uart_priv;
}

rt_int32_t alloc_uart_memory(rt_uint32_t num)
{
	uart_instance = rt_calloc(num, sizeof(pxa_uart_priv_t));
	if (!uart_instance) {
		rt_kprintf("%s:%d, failed\n", __func__, __LINE__);
		return -RT_ENOMEM;
	}

	return 0;
}
