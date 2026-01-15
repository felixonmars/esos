/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PXA UART Basic Test: Send/Receive and Baudrate Switching
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include "pxa_uart.h"

#define DEV_NAME_MAX 32
static char g_uart_dev_name[DEV_NAME_MAX] = {0};

/* UART event callback (implement as needed) */
static void uart_callback(int port, volatile uart_event_e event)
{
    /* Handle UART events */
    (void)port;
    (void)event;
}

void uart_init(const char *port_name)
{
    rt_device_t dev = rt_device_find(port_name);
    if (dev == RT_NULL) {
        rt_kprintf("Error: Device %s not found!\n", port_name);
        return;
    }

    rt_memset(g_uart_dev_name, 0, sizeof(g_uart_dev_name));
    rt_strncpy(g_uart_dev_name, port_name, sizeof(g_uart_dev_name) - 1);

    rt_kprintf("Success: UART target set to %s\n", g_uart_dev_name);
}

/**
 * Send test
 */
void uart_send(void)
{
	if (g_uart_dev_name[0] == '\0') {
        rt_kprintf("Error: Please run 'uart init <port>' first.\n");
        return;
    }

    rt_device_t dev = rt_device_find(g_uart_dev_name);
    if (!dev) {
        rt_kprintf("Device uart2 not found\n");
        return;
    }

    if (rt_device_open(dev, RT_DEVICE_FLAG_RDWR) != RT_EOK) {
        rt_kprintf("Device open failed\n");
        return;
    }

    char *msg = "Hello RT-Thread Standard Framework!\r\n";
    rt_size_t len = rt_strlen(msg);

    rt_kprintf("Sending %d bytes...\n", len);

    rt_size_t written = rt_device_write(dev, 0, msg, len);

    if (written == len) {
        rt_kprintf("Send success\n");
    } else {
        rt_kprintf("Send incomplete. Written: %d/%d\n", written, len);
    }

    rt_device_close(dev);
}

void uart_recv(int timeout_ms)
{
	if (g_uart_dev_name[0] == '\0') {
        rt_kprintf("Error: Please run 'uart init <port>' first.\n");
        return;
    }

    rt_device_t dev = rt_device_find(g_uart_dev_name);
    if (!dev) {
        rt_kprintf("Device uart2 not found\n");
        return;
    }

    if (rt_device_open(dev, RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        rt_kprintf("Device open failed\n");
        return;
    }

    rt_kprintf("Start receiving for %d ms...\n", timeout_ms);

    char ch;

    rt_tick_t start_tick = rt_tick_get();

    rt_tick_t timeout_tick = rt_tick_from_millisecond(timeout_ms);

    while ((rt_tick_get() - start_tick) < timeout_tick) {
        if (rt_device_read(dev, 0, &ch, 1) == 1)
            rt_kprintf("Recv: %02x\n", ch);
        else
            rt_thread_mdelay(10);
    }

    rt_kprintf("Timeout! Finish.\n");

    rt_device_close(dev);
}

void uart_baud(int baudrate)
{
    if (g_uart_dev_name[0] == '\0') {
        rt_kprintf("Error: Please run 'uart init <port>' first.\n");
        return;
    }

    rt_device_t dev = rt_device_find(g_uart_dev_name);
    if (!dev) {
        rt_kprintf("Error: Device %s not found\n", g_uart_dev_name);
        return;
    }

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = baudrate;

    rt_err_t result = rt_device_control(dev, RT_DEVICE_CTRL_CONFIG, &config);

    if (result == RT_EOK)
        rt_kprintf("Success: %s baudrate set to %d\n", g_uart_dev_name, baudrate);
    else if (result == -RT_EBUSY)
        rt_kprintf("Failed: Device is busy (opened by other thread?)\n");
    else
        rt_kprintf("Failed: rt_device_control returned %d\n", result);
}

static int uart_verify_byte_by_byte(rt_device_t dev, const char *tag)
{
    char send_buf[64];
    char rx_char = 0;
    int len;
    int error_cnt = 0;
    rt_size_t r_size;

    rt_snprintf(send_buf, sizeof(send_buf), "[%s]TEST-0123456789", tag);
    len = rt_strlen(send_buf);

    while (rt_device_read(dev, 0, &rx_char, 1) > 0);

    rt_kprintf("    [Sync-Check] Tag: %s (Len: %d)\n", tag, len);

    for (int i = 0; i < len; i++) {
        char tx_char = send_buf[i];

        if (rt_device_write(dev, 0, &tx_char, 1) != 1) {
            rt_kprintf("      Error: TX failed at index %d\n", i);
            return -1;
        }

        int wait_tick = 50;
        int received = 0;

        while (wait_tick--) {
            r_size = rt_device_read(dev, 0, &rx_char, 1);
            if (r_size > 0) {
                received = 1;
                break;
            }
            rt_thread_mdelay(1);
        }

        if (!received) {
            rt_kprintf("      Error: RX Timeout at index %d (Sent: '%c')\n", i, tx_char);
            return -1;
        }

        if (rx_char != tx_char) {
            rt_kprintf("      Error at index %d:\n", i);
            rt_kprintf("      send: %c\n", tx_char);
            rt_kprintf("      recv: %c (Hex: %02X)\n", rx_char, rx_char);
            error_cnt++;
        }
    }

    if (error_cnt == 0) {
        rt_kprintf("      Pass.\n");
        return 0;
    }
    return -1;
}

static int uart_test_baud_verify(rt_device_t dev, rt_uint32_t baud)
{
    char tag_buf[16];
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    int ret;

    rt_snprintf(tag_buf, sizeof(tag_buf), "%d", baud);
    rt_kprintf("  -> Switching to %d bps... ", baud);

	/* the device must be closed if comes here */
    config.baud_rate = baud;
    rt_err_t err = rt_device_control(dev, RT_DEVICE_CTRL_CONFIG, &config);
    if (err != RT_EOK) {
        rt_kprintf("Config Failed! Err: %d\n", err);
        return -1;
    }

    if (rt_device_open(dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        rt_kprintf("Open Failed!\n");
        return -1;
    }

    rt_kprintf("Done.\n");

    ret = uart_verify_byte_by_byte(dev, tag_buf);

    rt_device_close(dev);

    return ret;
}

void pxa_uart_basic_test_port(void)
{
    int errors = 0;
    char dev_name[16];
    rt_device_t dev;
	rt_uint32_t bauds[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1843200, 3686400};
	
	if (g_uart_dev_name[0] == '\0') {
        rt_kprintf("Error: Please run 'uart init <port>' first.\n");
        return;
    }

    dev = rt_device_find(g_uart_dev_name);
    if (dev == RT_NULL) {
        rt_kprintf("Error: Device %s not found\n", dev_name);
        return;
    }

    for (int i = 0; i < sizeof(bauds) / sizeof(bauds[0]); i++) {
        rt_uint32_t baud = bauds[i];

        if (uart_test_baud_verify(dev, baud) != 0) {
            errors++;
            rt_kprintf("  [X] Error at baudrate %d\n", baud);
        }

        rt_thread_mdelay(50);
    }

    rt_kprintf("\n========================================\n");
    rt_kprintf("  RESULT: %s (Total errors: %d)\n", errors == 0 ? "PASS" : "FAIL", errors);
    rt_kprintf("========================================\n\n");
}

#ifdef RT_USING_FINSH
/**
 * Unified UART test command
 * Usage:
 *   uart init <port>       - Initialize UART port
 *   uart send              - Send test message
 *   uart recv [timeout]    - Receive test (default timeout: 5000ms)
 *   uart baud <baudrate>   - Switch baudrate
 *   uart baudloop <n>      - Baudrate loop test
 *   uart loopback          - Loopback test (TX-RX connection required)
 *   uart test [port]       - Run complete test suite (default port: 0)
 */
static void uart(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("Usage: uart <cmd> [args]\n");
        rt_kprintf("Commands:\n");
        rt_kprintf("  init <port>       - Initialize UART port\n");
        rt_kprintf("  send              - Send test message\n");
        rt_kprintf("  recv [timeout]    - Receive test (default: 5000ms)\n");
        rt_kprintf("  baud <baudrate>   - Switch baudrate\n");
        rt_kprintf("  baudloop <n>      - Baudrate loop test\n");
        rt_kprintf("  loopback          - Loopback test\n");
        rt_kprintf("  test [port]       - Run complete test suite\n");
        return;
    }

    const char *cmd = argv[1];

    if (rt_strcmp(cmd, "init") == 0) {
        if (argc < 3) {
            rt_kprintf("Usage: uart init <port>\n");
            return;
        }
        uart_init(argv[2]);
    }
    else if (rt_strcmp(cmd, "send") == 0) {
        uart_send();
    }
    else if (rt_strcmp(cmd, "recv") == 0) {
        int timeout = (argc >= 3) ? atoi(argv[2]) : 5000;
        uart_recv(timeout);
    }
	else if (rt_strcmp(cmd, "baud") == 0) {
		if (argc < 3) {
			rt_kprintf("Usage: uart baud <baudrate>\n");
			return;
		}
		uart_baud(atoi(argv[2]));
	}
	else if (rt_strcmp(cmd, "test") == 0) {
		pxa_uart_basic_test_port();
	}
    else {
        rt_kprintf("Unknown command: %s\n", cmd);
        rt_kprintf("Use 'uart' without arguments for help\n");
    }
}
MSH_CMD_EXPORT(uart, UART test commands - usage: uart <cmd> [args]);
#endif /* RT_USING_FINSH */
