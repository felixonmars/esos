/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>

#ifdef RT_USING_CONSOLE

#include <rthw.h>
#include <rtdevice.h>
#include <drivers/serial.h>
#include "pxa_uart.h"
#include "drv_uart.h"


static void uart_irqhandler(rt_int32_t vector, void *param)
{
    struct rt_serial_device *serial = (struct rt_serial_device *)param;

    rt_hw_serial_isr(serial,RT_SERIAL_EVENT_RX_IND);
}

struct
{
    rt_uint32_t base;
    rt_uint32_t irq;
    void *handler;
    const char *name;
    uart_handle_t uart_handle;
    struct rt_serial_device serial;
    struct dtb_compatible_array __compatible;
} sg_uart_config[] = {
    {
        .name = "uart0",
        .handler = uart_irqhandler,
        .__compatible = {
            .compatible = "spacemit,pxa-uart0",
        },
    },

    {
        .name = "uart1",
        .handler = uart_irqhandler,
        .__compatible = {
            .compatible = "spacemit,pxa-uart1",
        },
    },
};

rt_int32_t target_uart_init(rt_int32_t idx, rt_uint32_t *base, rt_uint32_t *irq, void **handler)
{
    if (base != RT_NULL)
        *base = sg_uart_config[idx].base;

    if (irq != RT_NULL)
        *irq = sg_uart_config[idx].irq;

    if (handler != RT_NULL)
        *handler = sg_uart_config[idx].handler;

    return idx;
}

/*
 * UART interface
 */
static rt_err_t uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    rt_int32_t ret;
    uart_handle_t uart;
    rt_uint32_t bauds;
    uart_parity_e parity;
    uart_stop_bits_e stopbits;
    uart_data_bits_e databits;

    RT_ASSERT(serial != RT_NULL);
    uart = (uart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    /* set baudrate parity...*/
    bauds = cfg->baud_rate;

    if (cfg->parity == PARITY_EVEN)
        parity = UART_PARITY_EVEN;
    else if (cfg->parity == PARITY_ODD)
        parity = UART_PARITY_ODD;
    else
        parity = UART_PARITY_NONE;

    stopbits = UART_STOP_BITS_1 ;
    databits = UART_DATA_BITS_8;

    ret = pxa_uart_config(uart, bauds, UART_MODE_ASYNCHRONOUS, parity, stopbits, databits);

    if (ret < 0) {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t uart_control(struct rt_serial_device *serial, rt_int32_t cmd, void *arg)
{
    uart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (uart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    switch (cmd) {
    case RT_DEVICE_CTRL_CLR_INT:
        /* Disable the UART Interrupt */
        pxa_uart_clr_int_flag(uart, IER_RDA_INT_ENABLE);
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* Enable the UART Interrupt */
        pxa_uart_set_int_flag(uart, IER_RDA_INT_ENABLE);
        break;
    }

    return (RT_EOK);
}

static rt_int32_t uart_putc(struct rt_serial_device *serial, char c)
{
    uart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (uart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);
    pxa_uart_putchar(uart,c);

    return (1);
}

static rt_int32_t uart_getc(struct rt_serial_device *serial)
{
    rt_int32_t ch;
    uart_handle_t uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (uart_handle_t)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);


    ch = pxa_uart_getchar(uart);

    return ch;
}

const struct rt_uart_ops _uart_ops =
{
    uart_configure,
    uart_control,
    uart_putc,
    uart_getc,
};

rt_int32_t rt_hw_uart_init(void)
{
	rt_int32_t i, ret;
    pxa_uart_priv_t *priv;
    struct clk *clk, *rst;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    struct dtb_node *compatible_node;
    struct dtb_node *dtb_head_node = get_dtb_node_head();

    i = alloc_uart_memory(sizeof(sg_uart_config) / sizeof(sg_uart_config[0]));
    if (i < 0)
        return i;

    for (i = 0; i < sizeof(sg_uart_config) / sizeof(sg_uart_config[0]); ++i) {
        if (sg_uart_config[i].__compatible.compatible) {
            compatible_node = dtb_node_find_compatible_node(dtb_head_node,
                    sg_uart_config[i].__compatible.compatible);

            if (compatible_node != RT_NULL) {
                if (!dtb_node_device_is_available(compatible_node))
                    continue;

                /* get the register base */
                sg_uart_config[i].base = dtb_node_get_addr_index(compatible_node, 0);
                if (sg_uart_config[i].base < 0) {
                    rt_kprintf("get reg of uart error\n");
                    return -RT_ERROR;
                }

                /* get the interrupt irq */
                sg_uart_config[i].irq = dtb_node_irq_get(compatible_node, 0);
                if (sg_uart_config[i].irq < 0) {
                    rt_kprintf("get irq of uart error\n");
                    return -RT_ERROR;
                }

                clk = of_clk_get(compatible_node, 0);
                if (IS_ERR(clk)) {
                    rt_kprintf("%s:%d, get clk failed\n", __func__, __LINE__);
                    return -RT_EINVAL;
                }

                /* get the reset */
                rst = of_clk_get(compatible_node, 1);
                if (IS_ERR(rst)) {
                    rt_kprintf("%s:%d, get clk failed\n", __func__, __LINE__);
                    return -RT_EINVAL;
                }

                /* enable clk */
                ret = clk_prepare_enable(clk);
                if (ret) {
                    rt_kprintf("%s:%d, enable clk faild\n", __func__, __LINE__);
                    return -RT_EINVAL;
                }

                /* enable reset */
                ret = clk_prepare_enable(rst);
                if (ret) {
                    rt_kprintf("%s:%d, reset faild\n", __func__, __LINE__);
                    return -RT_EINVAL;
                }

                /* register the uart */
                /* for k1x, get the uart parameters from dts */
                sg_uart_config[i].serial.ops    = & _uart_ops;
                sg_uart_config[i].serial.config    = config;
                sg_uart_config[i].serial.config.bufsz    = 2048;
                sg_uart_config[i].serial.config.baud_rate    = 115200;

                sg_uart_config[i].uart_handle = pxa_uart_initialize(i, RT_NULL);
                priv = (pxa_uart_priv_t *)sg_uart_config[i].uart_handle;
                priv->clk = clk;
                priv->rst = rst;

                /* get the clock */
                rt_hw_interrupt_install(sg_uart_config[i].irq, uart_irqhandler,
                                        (void *)&sg_uart_config[i].serial, RT_NULL);
                rt_hw_interrupt_umask(sg_uart_config[i].irq);

                rt_hw_serial_register(&sg_uart_config[i].serial,
                        sg_uart_config[i].name,
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                        sg_uart_config[i].uart_handle);
            }
        }
    }

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_uart_init);
#endif
