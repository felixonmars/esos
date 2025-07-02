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
    const char *name;
    usart_handle_t uart_handle;
    struct rt_serial_device serial;
    struct dtb_compatible_array __compatible;
} sg_usart_config[] = {
    {
        .name = "uart0",
        .handler = usart_irqhandler,
        .__compatible = {
            .compatible = "spacemit,pxa-uart0",
        },
    },

    {
        .name = "uart1",
        .handler = usart_irqhandler,
        .__compatible = {
            .compatible = "spacemit,pxa-uart1",
        },
    },
};

rt_int32_t target_usart_init(rt_int32_t idx, rt_uint32_t *base, rt_uint32_t *irq, void **handler)
{
    if (base != RT_NULL)
        *base = sg_usart_config[idx].base;

    if (irq != RT_NULL)
        *irq = sg_usart_config[idx].irq;

    if (handler != RT_NULL)
        *handler = sg_usart_config[idx].handler;

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

    if (ret < 0) {
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

    switch (cmd) {
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
    int i, ret;
    ck_usart_priv_t *priv;
    struct clk *clk, *rst;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    struct dtb_node *compatible_node;
    struct dtb_node *dtb_head_node = get_dtb_node_head();

    i = alloc_usart_memory(sizeof(sg_usart_config) / sizeof(sg_usart_config[0]));
    if (i < 0)
        return i;

    for (i = 0; i < sizeof(sg_usart_config) / sizeof(sg_usart_config[0]); ++i) {
        if (sg_usart_config[i].__compatible.compatible) {
            compatible_node = dtb_node_find_compatible_node(dtb_head_node,
                    sg_usart_config[i].__compatible.compatible);

            if (compatible_node != RT_NULL) {
                if (!dtb_node_device_is_available(compatible_node))
                    continue;

                /* get the register base */
                sg_usart_config[i].base = dtb_node_get_addr_index(compatible_node, 0);
                if (sg_usart_config[i].base < 0) {
                    rt_kprintf("get reg of uart error\n");
                    return -RT_ERROR;
                }

                /* get the interrupt irq */
                sg_usart_config[i].irq = dtb_node_irq_get(compatible_node, 0);
                if (sg_usart_config[i].irq < 0) {
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
                sg_usart_config[i].serial.ops    = & _uart_ops;
                sg_usart_config[i].serial.config    = config;
                sg_usart_config[i].serial.config.bufsz    = 2048;
                sg_usart_config[i].serial.config.baud_rate    = 115200;

                sg_usart_config[i].uart_handle = csi_usart_initialize(i, RT_NULL);
                priv = (ck_usart_priv_t *)sg_usart_config[i].uart_handle;
                priv->clk = clk;
                priv->rst = rst;

                /* get the clock */
                rt_hw_interrupt_install(sg_usart_config[i].irq, usart_irqhandler,
                                        (void *)&sg_usart_config[i].serial, RT_NULL);
                rt_hw_interrupt_umask(sg_usart_config[i].irq);

                rt_hw_serial_register(&sg_usart_config[i].serial,
                        sg_usart_config[i].name,
                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                        sg_usart_config[i].uart_handle);
            }
        }
    }

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_usart_init);
#endif
