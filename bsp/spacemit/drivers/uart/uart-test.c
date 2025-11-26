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

/* UART handle (global) */
static uart_handle_t g_uart_handle = RT_NULL;
static int g_uart_port = 0; /* Current UART port number */

/* UART event callback (implement as needed) */
static void uart_callback(int port, volatile uart_event_e event)
{
    /* Handle UART events */
    (void)port;
    (void)event;
}

/**
 * Internal UART initialization function
 */
static void uart_init_internal(int port)
{
    pxa_uart_priv_t *priv;

    /* Store the port number */
    g_uart_port = port;

    rt_kprintf("[DEBUG] Requesting UART port: %d\n", port);

    /* Get the already initialized UART handle */
    g_uart_handle = pxa_uart_initialize(port, uart_callback);
    if (g_uart_handle == RT_NULL) {
        rt_kprintf("UART%d initialization failed (check if enabled in device tree)\n", port);
        return;
    }

    /* Get the private data to check actual port info */
    priv = (pxa_uart_priv_t *)g_uart_handle;
    rt_kprintf("[DEBUG] Handle idx: %d, base: 0x%08x, irq: %d\n",
               priv->idx, priv->base, priv->irq);

    /* Configure UART: 115200 8N1 */
    pxa_uart_config(g_uart_handle, 115200,
                   UART_MODE_ASYNCHRONOUS,
                   UART_PARITY_NONE,
                   UART_STOP_BITS_1,
                   UART_DATA_BITS_8);

    rt_kprintf("UART%d initialized successfully (115200 8N1)\n", port);
}

/**
 * Initialize UART with specified port (MSH command)
 * Note: Do not allocate memory here as it's already done during system init
 */
void uart_init_port(int argc, char **argv)
{
    int port;

    if (argc < 2) {
        rt_kprintf("Usage: uart_init_port <port>\n");
        return;
    }

    port = atoi(argv[1]);
    uart_init_internal(port);
}

/**
 * Send test
 */
void uart_send(void)
{
    char *msg = "Hello PXA UART!\n";

    if (g_uart_handle == RT_NULL) {
        rt_kprintf("Please run uart_init_port first\n");
        return;
    }

    rt_kprintf("Sending: %s", msg);

    for (int i = 0; msg[i] != '\0'; i++) {
        if (pxa_uart_putchar(g_uart_handle, msg[i]) != 0) {
            rt_kprintf("Send failed\n");
            return;
        }
    }

    rt_kprintf("Send completed\n");
}

/**
 * Receive test
 */
void uart_recv(int timeout_ms)
{
    int recv_count = 0;

    if (g_uart_handle == RT_NULL) {
        rt_kprintf("Please run uart_init_port first\n");
        return;
    }

    rt_kprintf("Waiting for data (timeout %d ms)...\n", timeout_ms);
    rt_kprintf("Tip: Please send data or connect TX-RX\n");

    for (int i = 0; i < timeout_ms; i++) {
        int ch = pxa_uart_getchar(g_uart_handle);
        if (ch >= 0) {
            rt_kprintf("%c", (char)ch);
            recv_count++;

            /* Exit if newline or max count reached */
            if (ch == '\n' || recv_count >= 128) {
                break;
            }
        }
        rt_thread_mdelay(1);
    }

    if (recv_count > 0) {
        rt_kprintf("\nReceived %d characters\n", recv_count);
    } else {
        rt_kprintf("No data received\n");
    }
}

/**
 * Baudrate switching test
 */
void uart_baud(rt_uint32_t baudrate)
{
    if (g_uart_handle == RT_NULL) {
        rt_kprintf("Please run uart_init_port first\n");
        return;
    }

    rt_kprintf("UART%d switching to baudrate %d...\n", g_uart_port, baudrate);

    if (pxa_uart_config_baudrate(g_uart_handle, baudrate) != 0) {
        rt_kprintf("Baudrate switch failed\n");
        return;
    }

    rt_kprintf("Baudrate switched successfully\n");

    /* Send test message for verification */
    char test_msg[64];
    rt_snprintf(test_msg, sizeof(test_msg), "Test@%d\n", baudrate);

    for (int i = 0; test_msg[i] != '\0'; i++) {
        pxa_uart_putchar(g_uart_handle, test_msg[i]);
    }
}

/**
 * Baudrate loop switching test
 */
void uart_baud_loop(int iterations)
{
    rt_uint32_t bauds[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1843200, 3686400};
    int num_bauds = sizeof(bauds) / sizeof(bauds[0]);
    int success = 0;

    if (g_uart_handle == RT_NULL) {
        rt_kprintf("Please run uart_init_port first\n");
        return;
    }

    rt_kprintf("UART%d starting baudrate loop test (%d iterations)...\n", g_uart_port, iterations);

    for (int i = 0; i < iterations; i++) {
        for (int j = 0; j < num_bauds; j++) {
            rt_uint32_t baud = bauds[j];

            /* Switch baudrate */
            if (pxa_uart_config_baudrate(g_uart_handle, baud) != 0) {
                rt_kprintf("Switch #%d failed (baudrate %d)\n", i * num_bauds + j + 1, baud);
                continue;
            }

            /* Send test data */
            char msg[32];
            rt_snprintf(msg, sizeof(msg), "T%d@%d\n", i, baud);
            for (int k = 0; msg[k] != '\0'; k++) {
                pxa_uart_putchar(g_uart_handle, msg[k]);
            }

            success++;
            rt_thread_mdelay(50);
        }

        rt_kprintf("Completed round %d/%d\n", i + 1, iterations);
    }

    /* Restore to 115200 */
    pxa_uart_config_baudrate(g_uart_handle, 115200);

    rt_kprintf("Test completed: %d/%d switches successful\n", success, iterations * num_bauds);
}

/**
 * Loopback test
 */
void uart_loopback(void)
{
    char send_msg[] = "Loopback-Test-123";
    char recv_buf[64] = {0};
    int recv_cnt = 0;

    if (g_uart_handle == RT_NULL) {
        rt_kprintf("Please run uart_init_port first\n");
        return;
    }

    rt_kprintf("UART%d loopback test (TX-RX connection required)\n", g_uart_port);
    rt_kprintf("Sending: %s\n", send_msg);

    /* Send data */
    for (int i = 0; send_msg[i] != '\0'; i++) {
        pxa_uart_putchar(g_uart_handle, send_msg[i]);
    }

    /* Wait for data to return */
    rt_thread_mdelay(100);

    /* Receive data */
    rt_kprintf("Receiving: ");
    for (int i = 0; i < 2000; i++) {
        int ch = pxa_uart_getchar(g_uart_handle);
        if (ch >= 0 && recv_cnt < sizeof(recv_buf) - 1) {
            recv_buf[recv_cnt++] = (char)ch;
            rt_kprintf("%c", (char)ch);
        }

        if (recv_cnt >= rt_strlen(send_msg)) {
            break;
        }

        rt_thread_mdelay(1);
    }
    rt_kprintf("\n");

    recv_buf[recv_cnt] = '\0';

    /* Compare results */
    if (recv_cnt > 0) {
        if (rt_strcmp(send_msg, recv_buf) == 0) {
            rt_kprintf("Loopback test PASSED! (matched)\n");
        } else {
            rt_kprintf("Loopback test partially passed (received %d bytes)\n", recv_cnt);
        }
    } else {
        rt_kprintf("No loopback data received (check if TX-RX connected)\n");
    }
}

/**
 * Complete automatic test with specified port
 */
void pxa_uart_basic_test_port(int port)
{
    rt_kprintf("\n========================================\n");
    rt_kprintf("  PXA UART%d Basic Test\n", port);
    rt_kprintf("========================================\n\n");

    /* 1. Initialization */
    rt_kprintf("[1/4] UART%d initialization test...\n", port);
    uart_init_internal(port);
    rt_thread_mdelay(500);

    /* 2. Send test */
    rt_kprintf("\n[2/4] UART%d send test...\n", port);
    uart_send();
    rt_thread_mdelay(500);

    /* 3. Baudrate switching test */
    rt_kprintf("\n[3/4] UART%d baudrate switching test...\n", port);
    rt_uint32_t test_bauds[] = {9600, 115200, 460800, 115200};
    for (int i = 0; i < sizeof(test_bauds) / sizeof(test_bauds[0]); i++) {
        uart_baud(test_bauds[i]);
        rt_thread_mdelay(300);
    }

    /* 4. Loopback test */
    rt_kprintf("\n[4/4] UART%d loopback test...\n", port);
    uart_loopback();

    rt_kprintf("\n========================================\n");
    rt_kprintf("  UART%d Test Completed\n", port);
    rt_kprintf("========================================\n\n");
}

/**
 * Complete automatic test - main entry point (default to UART0)
 */
void pxa_uart_basic_test(void)
{
    pxa_uart_basic_test_port(0);
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
        uart_init_internal(atoi(argv[2]));
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
    else if (rt_strcmp(cmd, "baudloop") == 0) {
        if (argc < 3) {
            rt_kprintf("Usage: uart baudloop <n>\n");
            return;
        }
        uart_baud_loop(atoi(argv[2]));
    }
    else if (rt_strcmp(cmd, "loopback") == 0) {
        uart_loopback();
    }
    else if (rt_strcmp(cmd, "test") == 0) {
        int port = (argc >= 3) ? atoi(argv[2]) : 0;
        pxa_uart_basic_test_port(port);
    }
    else {
        rt_kprintf("Unknown command: %s\n", cmd);
        rt_kprintf("Use 'uart' without arguments for help\n");
    }
}
MSH_CMD_EXPORT(uart, UART test commands - usage: uart <cmd> [args]);
#endif /* RT_USING_FINSH */
