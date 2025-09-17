/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>

#ifdef RT_USING_CONSOLE

#include <rthw.h>
#include <rtdevice.h>
#include <riscv-clic.h>
#include <drivers/serial.h>
#include "ck_usart.h"
#include "drv_usart.h"

static  usart_handle_t uart_handle;
static struct rt_serial_device  serial;

static void usart_irqhandler(int vector, void *param)
{
    rt_hw_serial_isr(&serial,RT_SERIAL_EVENT_RX_IND);
}

struct
{
    rt_uint32_t base;
    rt_uint32_t irq;
    void *handler;
} sg_usart_config[CONFIG_USART_NUM] =
{
    {UART_REG_BASE, UART_IRQn, usart_irqhandler},
};

rt_int32_t target_usart_init(rt_int32_t idx, rt_uint32_t *base, rt_uint32_t *irq, void **handler)
{
    if (idx >= CONFIG_USART_NUM)
    {
        return -1;
    }

    if (base != RT_NULL)
    {
        *base = sg_usart_config[idx].base;
    }

    if (irq != RT_NULL)
    {
        *irq = sg_usart_config[idx].irq;
    }

    if (handler != RT_NULL)
    {
        *handler = sg_usart_config[idx].handler;
    }

    return idx;
}

/*
 * UART interface
 */
static rt_err_t uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    int ret;
    usart_handle_t uart;
    rt_uint32_t bauds;
    usart_parity_e parity;
    usart_stop_bits_e stopbits;
    usart_data_bits_e databits;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    /* set baudrate parity...*/
    bauds = cfg->baud_rate;

    if (cfg->parity == PARITY_EVEN)
        parity = USART_PARITY_EVEN;
    else if (cfg->parity == PARITY_ODD)
        parity = USART_PARITY_ODD;
    else
        parity = USART_PARITY_NONE;

    stopbits = USART_STOP_BITS_1 ;
    databits = USART_DATA_BITS_8;

    ret = csi_usart_config(uart, bauds, USART_MODE_ASYNCHRONOUS, parity, stopbits, databits);

    if (ret < 0)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    usart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* Disable the UART Interrupt */
        ck_usart_clr_int_flag(uart, IER_RDA_INT_ENABLE);
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* Enable the UART Interrupt */
        ck_usart_set_int_flag(uart, IER_RDA_INT_ENABLE);
        break;
    }

    return (RT_EOK);
}

static int uart_putc(struct rt_serial_device *serial, char c)
{
    usart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);
    csi_usart_putchar(uart,c);

    return (1);
}

static int uart_getc(struct rt_serial_device *serial)
{
    int ch;
    usart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (usart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);


    ch = csi_uart_getchar(uart);

    return ch;
}

const struct rt_uart_ops _uart_ops =
{
    uart_configure,
    uart_control,
    uart_putc,
    uart_getc,
};

int rt_hw_usart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
#ifndef SOC_SPACEMIT_K1_X
    rcpu_clk_en_t *rcpu_ck_en = (rcpu_clk_en_t *)RCPU_CLK_EN_BASE;
    rcpu_sw_reset_t *rcpu_rstn = (rcpu_sw_reset_t *)RCPU_SW_RST_BASE;
    /* enable clk */
    rcpu_ck_en->bits.mcu_uart_clken = 1;

    /* enable reset */
    rcpu_rstn->bits.mcu_uart_rstn = 1;
#else
    uart_clk_rst_t *uart_clk_rst_en = (uart_clk_rst_t*)UART_CR_REG_BASE;
    /* enable uart clk&reset */
    uart_clk_rst_en->bits.uart_rsten = 1;
    uart_clk_rst_en->bits.uart_fclken = 1;
    uart_clk_rst_en->bits.uart_pclken = 1;
    uart_clk_rst_en->bits.uart_fclk_sel = 0;
#endif

    serial.ops                 = & _uart_ops;
    serial.config              = config;
    serial.config.bufsz        = 2048;
    serial.config.baud_rate    = 115200;

    uart_handle = csi_usart_initialize(0, RT_NULL);

    rt_hw_interrupt_install(UART_IRQn, usart_irqhandler, RT_NULL, RT_NULL);
    rt_hw_interrupt_umask(UART_IRQn);

    rt_hw_serial_register(&serial,
                          "uart",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                          uart_handle);

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_usart_init);
#endif
