/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
*/

/*
 * Simple I2C test app for small-core (RT-Thread) using spacemit i2c driver
 *
 * Features:
 * - Basic write-then-read (EEPROM-like) at address 0x50
 * - Transfer tests (interrupt mode - driver default)
 * - Optional bus and address parameters via RT-Thread shell
 * - Individual commands for each test case
 * - Single address testing for device detection
 *
 * Usage (RT-Thread shell):
 *   i2c_list                            # List available I2C bus devices
 *   i2c_addr [bus] [addr]               # Test single I2C address
 *   i2c_test_all [bus] [addr]           # Run all tests (default: ri2c0 0x50)
 *   i2c_test_basic [bus] [addr]         # Basic write-then-read test
 *   i2c_test_transfer [bus] [addr]      # Data transfer test
 *   i2c_test_std_speed [bus] [addr]     # Standard mode speed verification
 *   i2c_test_fast_speed [bus] [addr]    # Fast mode speed verification
 *   i2c_test_info [bus]                 # Display adapter information
 *
 * Examples:
 *   i2c_list                            # List all I2C buses
 *   i2c_addr ri2c0 0x50                 # Test address 0x50 on ri2c0
 *   i2c_test_all                        # Run all tests with defaults
 *   i2c_test_basic ri2c1                # Basic test on ri2c1
 *   i2c_test_transfer ri2c0 0x50        # Transfer test with specific address
 *
 * Note: Driver uses interrupt mode by default (intr_enable=true)
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/i2c.h>
#include <drivers/i2c_dev.h>
#include <string.h>
#include <stdlib.h>
#include <dtb_node.h>

#ifndef I2C_K1_DEFAULT_BUS
#define I2C_K1_DEFAULT_BUS   "ri2c0"
#endif

#ifndef I2C_K1_DEFAULT_ADDR
#define I2C_K1_DEFAULT_ADDR  0x50
#endif

/* ri2c0 base address for register access */
#ifndef I2C_K1_RI2C0_BASE
#define I2C_K1_RI2C0_BASE    0xC0886000
#endif

/* ILCR register offset */
#define SPACEMIT_ILCR_OFFSET 0x10

/* Input clock frequency for ri2c (typically 32 MHz) */
#define I2C_INPUT_CLK_HZ     32000000

/* Forward declarations */
static rt_uint32_t read_dt_property_u32(const char *compatible, const char *prop_name);

/* Helper function to get input clock from device tree */
static rt_uint32_t get_input_clk_hz(const char *bus_name)
{
	const char *compatible;

	if (rt_strcmp(bus_name, "ri2c0") == 0)
		compatible = "spacemit,k1x-ri2c0";
	else if (rt_strcmp(bus_name, "ri2c1") == 0)
		compatible = "spacemit,k1x-ri2c1";
	else if (rt_strcmp(bus_name, "ri2c2") == 0)
		compatible = "spacemit,k1x-ri2c2";
	else
		return I2C_INPUT_CLK_HZ;  /* Default */

	rt_uint32_t clk_rate = read_dt_property_u32(compatible, "spacemit,i2c-clk-rate");
	return (clk_rate > 0) ? clk_rate : I2C_INPUT_CLK_HZ;
}

typedef struct {
	const char *bus_name;
	rt_uint16_t addr7;
} i2c_k1_ctx_t;

static rt_err_t i2c_k1_write_then_read(i2c_k1_ctx_t *ctx,
                                       rt_uint8_t reg,
                                       rt_uint8_t *rx,
                                       rt_size_t rx_len)
{
	struct rt_i2c_bus_device *bus = rt_i2c_bus_device_find(ctx->bus_name);
	if (bus == RT_NULL)
	{
		rt_kprintf("I2C bus '%s' not found (use 'i2c_list' to check)\n", ctx->bus_name);
		return -RT_ENOSYS;
	}

	rt_uint8_t reg_buf = reg;
	struct rt_i2c_msg msgs[2];

	/* write 1 byte register/address pointer */
	msgs[0].addr  = ctx->addr7;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf   = &reg_buf;
	msgs[0].len   = 1;

	/* read rx_len bytes */
	msgs[1].addr  = ctx->addr7;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf   = rx;
	msgs[1].len   = rx_len;

	rt_size_t ret = rt_i2c_transfer(bus, msgs, 2);
	return (ret == 2) ? RT_EOK : -RT_ERROR;
}

static rt_err_t i2c_k1_write_block(i2c_k1_ctx_t *ctx,
                                   rt_uint8_t start_reg,
                                   const rt_uint8_t *tx,
                                   rt_size_t tx_len)
{
	struct rt_i2c_bus_device *bus = rt_i2c_bus_device_find(ctx->bus_name);
	if (bus == RT_NULL)
	{
		rt_kprintf("I2C bus '%s' not found\n", ctx->bus_name);
		return -RT_ENOSYS;
	}

	/* compose buffer: [reg][data...] */
	rt_uint8_t *buf = (rt_uint8_t *)rt_malloc(tx_len + 1);
	if (!buf)
		return -RT_ENOMEM;

	buf[0] = start_reg;
	rt_memcpy(buf + 1, tx, tx_len);

	struct rt_i2c_msg msg;
	msg.addr  = ctx->addr7;
	msg.flags = RT_I2C_WR;
	msg.buf   = buf;
	msg.len   = tx_len + 1;

	rt_size_t ret = rt_i2c_transfer(bus, &msg, 1);
	rt_free(buf);
	return (ret == 1) ? RT_EOK : -RT_ERROR;
}

static void dump_hex_line(const char *prefix, const rt_uint8_t *buf, rt_size_t len)
{
	rt_kprintf("%s", prefix ? prefix : "");
	for (rt_size_t i = 0; i < len; i++)
	{
		rt_kprintf("%02X ", buf[i]);
	}
	rt_kprintf("\n");
}

static rt_err_t test_basic(i2c_k1_ctx_t *ctx)
{
	rt_kprintf("\n[TEST] Basic write-then-read (non-destructive)\n");

	/* Step 1: Read and save original values */
	rt_uint8_t original[8] = {0};
	rt_uint8_t test_offset = 0x10;  /* Use offset 0x10 to avoid critical data */

	rt_kprintf("Step 1: Reading original values at offset 0x%02X...\n", test_offset);
	rt_err_t err = i2c_k1_write_then_read(ctx, test_offset, original, sizeof(original));
	if (err)
	{
		rt_kprintf("Read original values failed: %d\n", err);
		return err;
	}
	dump_hex_line("Original: ", original, sizeof(original));

	/* Step 2: Write test values and verify */
	rt_uint8_t tx[8];
	for (int i = 0; i < (int)sizeof(tx); i++) tx[i] = (rt_uint8_t)(0xA0 + i);
	rt_uint8_t rx[8] = {0};

	rt_kprintf("Step 2: Writing test values...\n");
	err = i2c_k1_write_block(ctx, test_offset, tx, sizeof(tx));
	if (err)
	{
		rt_kprintf("Write test values failed: %d\n", err);
		return err;
	}

	/* Wait for EEPROM write cycle */
	rt_thread_mdelay(10);

	rt_kprintf("Verifying written values...\n");
	err = i2c_k1_write_then_read(ctx, test_offset, rx, sizeof(rx));
	if (err)
	{
		rt_kprintf("Read back test values failed: %d\n", err);
		return err;
	}

	dump_hex_line("TX: ", tx, sizeof(tx));
	dump_hex_line("RX: ", rx, sizeof(rx));

	int mism = 0;
	for (int i = 0; i < (int)sizeof(tx); i++) if (tx[i] != rx[i]) mism++;
	if (mism)
	{
		rt_kprintf("Write verification FAILED, mismatches=%d\n", mism);
		return -RT_ERROR;
	}
	rt_kprintf("Write verification PASSED\n");

	/* Step 3: Restore original values */
	rt_kprintf("Step 3: Restoring original values...\n");
	err = i2c_k1_write_block(ctx, test_offset, original, sizeof(original));
	if (err)
	{
		rt_kprintf("Restore original values failed: %d\n", err);
		return err;
	}

	/* Wait for EEPROM write cycle */
	rt_thread_mdelay(10);

	/* Verify restoration */
	rt_uint8_t verify[8] = {0};
	rt_kprintf("Verifying restored values...\n");
	err = i2c_k1_write_then_read(ctx, test_offset, verify, sizeof(verify));
	if (err)
	{
		rt_kprintf("Read restored values failed: %d\n", err);
		return err;
	}

	mism = 0;
	for (int i = 0; i < (int)sizeof(original); i++) if (original[i] != verify[i]) mism++;
	if (mism)
	{
		rt_kprintf("Restore verification FAILED, mismatches=%d\n", mism);
		dump_hex_line("Expected: ", original, sizeof(original));
		dump_hex_line("Got:      ", verify, sizeof(verify));
		return -RT_ERROR;
	}
	rt_kprintf("Restore verification PASSED\n");

	rt_kprintf("Basic test PASSED (non-destructive)\n");
	return RT_EOK;
}

static rt_err_t test_transfer(i2c_k1_ctx_t *ctx)
{
	rt_kprintf("\n[TEST] Data Transfer Test (Interrupt Mode, non-destructive)\n");

	/* Step 1: Read and save original values */
	rt_uint8_t original[16] = {0};
	rt_uint8_t test_offset = 0x20;  /* Use offset 0x20 to avoid conflict with test_basic */

	rt_kprintf("Step 1: Reading original values at offset 0x%02X...\n", test_offset);
	rt_err_t err = i2c_k1_write_then_read(ctx, test_offset, original, sizeof(original));
	if (err)
	{
		rt_kprintf("Read original values failed: %d\n", err);
		return err;
	}
	dump_hex_line("Original: ", original, sizeof(original));

	/* Step 2: Write test values and verify */
	rt_uint8_t tx[16];
	for (int i = 0; i < (int)sizeof(tx); i++) tx[i] = (rt_uint8_t)(0x10 + i);
	rt_uint8_t rx[16] = {0};

	rt_kprintf("Step 2: Writing test values...\n");
	err = i2c_k1_write_block(ctx, test_offset, tx, sizeof(tx));
	if (err)
	{
		rt_kprintf("Transfer write failed: %d\n", err);
		return err;
	}

	/* Wait for EEPROM write cycle */
	rt_thread_mdelay(10);

	rt_kprintf("Verifying written values...\n");
	err = i2c_k1_write_then_read(ctx, test_offset, rx, sizeof(rx));
	if (err)
	{
		rt_kprintf("Transfer read failed: %d\n", err);
		return err;
	}
	dump_hex_line("TX: ", tx, sizeof(tx));
	dump_hex_line("RX: ", rx, sizeof(rx));

	int mism = 0;
	for (int i = 0; i < (int)sizeof(tx); i++) if (tx[i] != rx[i]) mism++;
	if (mism)
	{
		rt_kprintf("Transfer test FAILED, mismatches=%d\n", mism);
		return -RT_ERROR;
	}
	rt_kprintf("Transfer verification PASSED\n");

	/* Step 3: Restore original values */
	rt_kprintf("Step 3: Restoring original values...\n");
	err = i2c_k1_write_block(ctx, test_offset, original, sizeof(original));
	if (err)
	{
		rt_kprintf("Restore original values failed: %d\n", err);
		return err;
	}

	/* Wait for EEPROM write cycle */
	rt_thread_mdelay(10);

	/* Verify restoration */
	rt_uint8_t verify[16] = {0};
	rt_kprintf("Verifying restored values...\n");
	err = i2c_k1_write_then_read(ctx, test_offset, verify, sizeof(verify));
	if (err)
	{
		rt_kprintf("Read restored values failed: %d\n", err);
		return err;
	}

	mism = 0;
	for (int i = 0; i < (int)sizeof(original); i++) if (original[i] != verify[i]) mism++;
	if (mism)
	{
		rt_kprintf("Restore verification FAILED, mismatches=%d\n", mism);
		dump_hex_line("Expected: ", original, sizeof(original));
		dump_hex_line("Got:      ", verify, sizeof(verify));
		return -RT_ERROR;
	}
	rt_kprintf("Restore verification PASSED\n");

	rt_kprintf("Transfer test PASSED (non-destructive)\n");
	return RT_EOK;
}


static rt_uint32_t read_ilcr_register(rt_uint32_t base_addr)
{
	/* Read ILCR register directly from memory-mapped I/O */
	volatile rt_uint32_t *ilcr_reg = (volatile rt_uint32_t *)(uintptr_t)(base_addr + SPACEMIT_ILCR_OFFSET);
	return *ilcr_reg;
}

static rt_uint32_t calculate_scl_frequency(rt_uint32_t input_clk_hz, rt_uint32_t load_value)
{
	/* Formula: SCL_freq = input_clk / (2 * (load_value + 1)) */
	if (load_value == 0)
		return 0;
	return input_clk_hz / (2 * (load_value + 1));
}

static rt_uint32_t read_dt_property_u32(const char *compatible, const char *prop_name)
{
	struct dtb_node *dtb_head = get_dtb_node_head();
	struct dtb_node *node = dtb_node_find_compatible_node(dtb_head, compatible);

	if (!node)
	{
		rt_kprintf("Device tree node '%s' not found\n", compatible);
		return 0;
	}

	int property_size;
	rt_uint32_t value = 0;
	rt_uint32_t *property_ptr;

	for_each_property_cell(node, prop_name, value, property_ptr, property_size)
	{
		return value;  /* Return first cell */
	}

	return 0;
}

static rt_bool_t read_dt_property_exists(const char *compatible, const char *prop_name)
{
	struct dtb_node *dtb_head = get_dtb_node_head();
	struct dtb_node *node = dtb_node_find_compatible_node(dtb_head, compatible);

	if (!node)
		return RT_FALSE;

	struct dtb_property *prop = dtb_node_get_dtb_node_property(node, prop_name, RT_NULL);
	return (prop != RT_NULL);
}

static void print_dt_config(const char *bus_name)
{
	const char *compatible;

	/* Map bus name to compatible string */
	if (rt_strcmp(bus_name, "ri2c0") == 0)
		compatible = "spacemit,k1x-ri2c0";
	else if (rt_strcmp(bus_name, "ri2c1") == 0)
		compatible = "spacemit,k1x-ri2c1";
	else if (rt_strcmp(bus_name, "ri2c2") == 0)
		compatible = "spacemit,k1x-ri2c2";
	else
		return;

	rt_kprintf("\n=== Device Tree Configuration ===\n");

	/* Show input clock rate */
	rt_uint32_t input_clk = get_input_clk_hz(bus_name);
	rt_kprintf("Input Clock: %d Hz (%.1f MHz)\n", input_clk, input_clk / 1000000.0);

	/* Check speed mode */
	rt_bool_t fast_mode = read_dt_property_exists(compatible, "spacemit,i2c-fast-mode");
	if (fast_mode)
		rt_kprintf("Mode: Fast Mode (400 kHz)\n");
	else
		rt_kprintf("Mode: Standard Mode (100 kHz)\n");
}

static void verify_scl_frequency(rt_uint32_t base_addr, const char *bus_name)
{
	rt_uint32_t ilcr = read_ilcr_register(base_addr);
	rt_uint32_t slv = ilcr & 0x1FF;           /* bits 8:0 - Standard Mode Load Value */
	rt_uint32_t flv = (ilcr >> 9) & 0x1FF;    /* bits 17:9 - Fast Mode Load Value */

	/* Get actual input clock from device tree */
	rt_uint32_t input_clk = get_input_clk_hz(bus_name);

	rt_kprintf("\n=== Hardware Register Status ===\n");
	rt_kprintf("ILCR Register: 0x%08X\n", ilcr);

	/* Calculate and display Standard Mode frequency */
	rt_uint32_t std_freq = calculate_scl_frequency(input_clk, slv);
	if (std_freq > 0)
	{
		rt_kprintf("Standard Mode (SLV=%d): %d Hz (%.1f kHz)\n",
		           slv, std_freq, std_freq / 1000.0);
	}

	/* Calculate and display Fast Mode frequency */
	rt_uint32_t fast_freq = calculate_scl_frequency(input_clk, flv);
	if (fast_freq > 0)
	{
		rt_kprintf("Fast Mode (FLV=%d): %d Hz (%.1f kHz)\n",
		           flv, fast_freq, fast_freq / 1000.0);
	}
}

static rt_err_t test_standard_mode_speed(i2c_k1_ctx_t *ctx)
{
	rt_kprintf("\n[TEST] Standard Mode Speed Verification\n");

	const char *compatible;
	if (rt_strcmp(ctx->bus_name, "ri2c0") == 0)
		compatible = "spacemit,k1x-ri2c0";
	else if (rt_strcmp(ctx->bus_name, "ri2c1") == 0)
		compatible = "spacemit,k1x-ri2c1";
	else if (rt_strcmp(ctx->bus_name, "ri2c2") == 0)
		compatible = "spacemit,k1x-ri2c2";
	else
	{
		rt_kprintf("Unsupported bus for speed test\n");
		return -RT_ERROR;
	}

	/* Check if fast mode is enabled */
	rt_bool_t fast_mode = read_dt_property_exists(compatible, "spacemit,i2c-fast-mode");
	if (fast_mode)
	{
		rt_kprintf("Standard Mode test SKIPPED (Fast Mode enabled in DT)\n");
		return RT_EOK;
	}

	/* Perform I2C read transfer to verify communication */
	rt_uint8_t rx[8] = {0};
	rt_err_t err = i2c_k1_write_then_read(ctx, 0x50, rx, sizeof(rx));
	if (err)
	{
		rt_kprintf("I2C read transfer failed: %d\n", err);
		return err;
	}
	dump_hex_line("Read data: ", rx, sizeof(rx));

	/* Read and verify frequency */
	rt_uint32_t base_addr = I2C_K1_RI2C0_BASE;
	rt_uint32_t ilcr = read_ilcr_register(base_addr);
	rt_uint32_t slv = ilcr & 0x1FF;
	rt_uint32_t input_clk = get_input_clk_hz(ctx->bus_name);
	rt_uint32_t std_freq = calculate_scl_frequency(input_clk, slv);

	rt_kprintf("Expected: Standard Mode (100 kHz)\n");
	rt_kprintf("Actual: %d Hz (%.1f kHz)\n", std_freq, std_freq / 1000.0);

	/* Tolerance: ±5% */
	if (std_freq >= 95000 && std_freq <= 105000)
	{
		rt_kprintf("Standard Mode test PASSED\n");
		return RT_EOK;
	}
	else
	{
		rt_kprintf("Standard Mode test FAILED (frequency out of range)\n");
		return -RT_ERROR;
	}
}

static rt_err_t test_fast_mode_speed(i2c_k1_ctx_t *ctx)
{
	rt_kprintf("\n[TEST] Fast Mode Speed Verification\n");

	const char *compatible;
	if (rt_strcmp(ctx->bus_name, "ri2c0") == 0)
		compatible = "spacemit,k1x-ri2c0";
	else if (rt_strcmp(ctx->bus_name, "ri2c1") == 0)
		compatible = "spacemit,k1x-ri2c1";
	else if (rt_strcmp(ctx->bus_name, "ri2c2") == 0)
		compatible = "spacemit,k1x-ri2c2";
	else
	{
		rt_kprintf("Unsupported bus for speed test\n");
		return -RT_ERROR;
	}

	/* Check if fast mode is enabled */
	rt_bool_t fast_mode = read_dt_property_exists(compatible, "spacemit,i2c-fast-mode");
	if (!fast_mode)
	{
		rt_kprintf("Fast Mode test SKIPPED (Fast Mode not enabled in DT)\n");
		return RT_EOK;
	}

	/* Perform I2C read transfer to verify communication */
	rt_uint8_t rx[8] = {0};
	rt_err_t err = i2c_k1_write_then_read(ctx, 0x50, rx, sizeof(rx));
	if (err)
	{
		rt_kprintf("I2C read transfer failed: %d\n", err);
		return err;
	}
	dump_hex_line("Read data: ", rx, sizeof(rx));

	/* Read and verify frequency */
	rt_uint32_t base_addr = I2C_K1_RI2C0_BASE;
	rt_uint32_t ilcr = read_ilcr_register(base_addr);
	rt_uint32_t flv = (ilcr >> 9) & 0x1FF;
	rt_uint32_t input_clk = get_input_clk_hz(ctx->bus_name);
	rt_uint32_t fast_freq = calculate_scl_frequency(input_clk, flv);

	rt_kprintf("Expected: Fast Mode (400 kHz)\n");
	rt_kprintf("Actual: %d Hz (%.1f kHz)\n", fast_freq, fast_freq / 1000.0);

	/* Tolerance: ±5% */
	if (fast_freq >= 380000 && fast_freq <= 420000)
	{
		rt_kprintf("Fast Mode test PASSED\n");
		return RT_EOK;
	}
	else
	{
		rt_kprintf("Fast Mode test FAILED (frequency out of range)\n");
		return -RT_ERROR;
	}
}

static void i2c_k1_print_adapter_info(const char *bus_name)
{
	struct rt_i2c_bus_device *bus = rt_i2c_bus_device_find(bus_name);
	if (!bus)
	{
		rt_kprintf("I2C bus '%s' not found\n", bus_name);
		return;
	}
	rt_kprintf("Using I2C bus: %s\n", bus_name);

	/* Print device tree configuration */
	print_dt_config(bus_name);

	/* Verify actual SCL frequency from ILCR register */
	if (rt_strcmp(bus_name, "ri2c0") == 0)
	{
		verify_scl_frequency(I2C_K1_RI2C0_BASE, bus_name);
	}
}

/* Helper function to parse command line arguments */
static void parse_i2c_args(int argc, char **argv, i2c_k1_ctx_t *ctx)
{
	ctx->bus_name = I2C_K1_DEFAULT_BUS;
	ctx->addr7 = I2C_K1_DEFAULT_ADDR;

	if (argc >= 2 && argv[1] && rt_strlen(argv[1]) > 0)
	{
		ctx->bus_name = argv[1];
	}
	if (argc >= 3 && argv[2] && rt_strlen(argv[2]) > 0)
	{
		/* parse 7-bit address, accept "0x.." or decimal */
		if (rt_strlen(argv[2]) > 2 && (argv[2][0] == '0') && (argv[2][1] == 'x' || argv[2][1] == 'X'))
			ctx->addr7 = (rt_uint16_t)strtoul(argv[2], RT_NULL, 16);
		else
			ctx->addr7 = (rt_uint16_t)strtoul(argv[2], RT_NULL, 10);
	}
}

/* Helper function to print test header */
static void print_test_header(i2c_k1_ctx_t *ctx, const char *test_name)
{
	rt_kprintf("\n==== %s ====\n", test_name);
	rt_kprintf("Bus   : %s\n", ctx->bus_name);
	rt_kprintf("Addr  : 0x%02X (7-bit)\n", ctx->addr7 & 0x7F);
}

/* Command: Run all tests */
static int i2c_test_all_entry(int argc, char **argv)
{
	i2c_k1_ctx_t ctx;
	parse_i2c_args(argc, argv, &ctx);

	print_test_header(&ctx, "I2C K1 All Tests");
	i2c_k1_print_adapter_info(ctx.bus_name);

	rt_err_t err = RT_EOK;

	/* Functional tests */
	err = test_basic(&ctx);
	if (err) goto out;
	err = test_transfer(&ctx);
	if (err) goto out;

	/* Speed mode tests */
	err = test_standard_mode_speed(&ctx);
	if (err) goto out;
	err = test_fast_mode_speed(&ctx);
	if (err) goto out;

out:
	if (err)
		rt_kprintf("\nI2C K1 All Tests: FAILED (%d)\n", err);
	else
		rt_kprintf("\nI2C K1 All Tests: PASSED\n");
	return err;
}

/* Command: Basic write-then-read test */
static int i2c_test_basic_entry(int argc, char **argv)
{
	i2c_k1_ctx_t ctx;
	parse_i2c_args(argc, argv, &ctx);

	print_test_header(&ctx, "I2C Basic Test");
	rt_err_t err = test_basic(&ctx);

	if (err)
		rt_kprintf("\nBasic Test: FAILED (%d)\n", err);
	else
		rt_kprintf("\nBasic Test: PASSED\n");
	return err;
}

/* Command: Data transfer test */
static int i2c_test_transfer_entry(int argc, char **argv)
{
	i2c_k1_ctx_t ctx;
	parse_i2c_args(argc, argv, &ctx);

	print_test_header(&ctx, "I2C Transfer Test");
	rt_err_t err = test_transfer(&ctx);

	if (err)
		rt_kprintf("\nTransfer Test: FAILED (%d)\n", err);
	else
		rt_kprintf("\nTransfer Test: PASSED\n");
	return err;
}


/* Command: Standard mode speed verification */
static int i2c_test_std_speed_entry(int argc, char **argv)
{
	i2c_k1_ctx_t ctx;
	parse_i2c_args(argc, argv, &ctx);

	print_test_header(&ctx, "I2C Standard Mode Speed Test");
	rt_err_t err = test_standard_mode_speed(&ctx);

	if (err)
		rt_kprintf("\nStandard Mode Speed Test: FAILED (%d)\n", err);
	else
		rt_kprintf("\nStandard Mode Speed Test: PASSED\n");
	return err;
}

/* Command: Fast mode speed verification */
static int i2c_test_fast_speed_entry(int argc, char **argv)
{
	i2c_k1_ctx_t ctx;
	parse_i2c_args(argc, argv, &ctx);

	print_test_header(&ctx, "I2C Fast Mode Speed Test");
	rt_err_t err = test_fast_mode_speed(&ctx);

	if (err)
		rt_kprintf("\nFast Mode Speed Test: FAILED (%d)\n", err);
	else
		rt_kprintf("\nFast Mode Speed Test: PASSED\n");
	return err;
}

/* Command: Display adapter information */
static int i2c_test_info_entry(int argc, char **argv)
{
	const char *bus_name = I2C_K1_DEFAULT_BUS;

	if (argc >= 2 && argv[1] && rt_strlen(argv[1]) > 0)
	{
		bus_name = argv[1];
	}

	rt_kprintf("\n==== I2C Adapter Information ====\n");
	rt_kprintf("Bus   : %s\n", bus_name);
	i2c_k1_print_adapter_info(bus_name);

	return RT_EOK;
}

/* Command: List all I2C devices */
static int i2c_list_devices_entry(int argc, char **argv)
{
	const char *bus_names[] = {"ri2c0", "ri2c1", "ri2c2"};
	int found_count = 0;

	rt_kprintf("Available I2C buses:\n");

	for (int i = 0; i < sizeof(bus_names) / sizeof(bus_names[0]); i++)
	{
		struct rt_i2c_bus_device *bus = rt_i2c_bus_device_find(bus_names[i]);
		if (bus != RT_NULL)
		{
			rt_kprintf("  %s\n", bus_names[i]);
			found_count++;
		}
	}

	if (found_count == 0)
	{
		rt_kprintf("  None\n");
	}

	return RT_EOK;
}

/* Command: Test single I2C address */
static int i2c_test_addr_entry(int argc, char **argv)
{
	const char *bus_name = I2C_K1_DEFAULT_BUS;
	rt_uint8_t addr = I2C_K1_DEFAULT_ADDR;
	struct rt_i2c_bus_device *bus;

	if (argc >= 2 && argv[1] && rt_strlen(argv[1]) > 0)
	{
		bus_name = argv[1];
	}
	if (argc >= 3 && argv[2] && rt_strlen(argv[2]) > 0)
	{
		if (rt_strlen(argv[2]) > 2 && (argv[2][0] == '0') && (argv[2][1] == 'x' || argv[2][1] == 'X'))
			addr = (rt_uint8_t)strtoul(argv[2], RT_NULL, 16);
		else
			addr = (rt_uint8_t)strtoul(argv[2], RT_NULL, 10);
	}

	bus = rt_i2c_bus_device_find(bus_name);
	if (bus == RT_NULL)
	{
		rt_kprintf("I2C bus '%s' not found\n", bus_name);
		return -RT_ERROR;
	}

	rt_kprintf("Testing %s address 0x%02x...\n", bus_name, addr);

	struct rt_i2c_msg msg;
	rt_uint8_t dummy = 0;

	msg.addr = addr;
	msg.flags = RT_I2C_RD;
	msg.buf = &dummy;
	msg.len = 0;

	if (rt_i2c_transfer(bus, &msg, 1) == 1)
	{
		rt_kprintf("Device found at 0x%02x\n", addr);
	}
	else
	{
		rt_kprintf("No device at 0x%02x\n", addr);
	}

	return RT_EOK;
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT_ALIAS(i2c_test_all_entry, i2c_test_all, Run all I2C tests [bus] [addr]);
MSH_CMD_EXPORT_ALIAS(i2c_test_basic_entry, i2c_test_basic, I2C basic write-read test [bus] [addr]);
MSH_CMD_EXPORT_ALIAS(i2c_test_transfer_entry, i2c_test_transfer, I2C data transfer test [bus] [addr]);
MSH_CMD_EXPORT_ALIAS(i2c_test_std_speed_entry, i2c_test_std_speed, I2C standard mode speed test [bus] [addr]);
MSH_CMD_EXPORT_ALIAS(i2c_test_fast_speed_entry, i2c_test_fast_speed, I2C fast mode speed test [bus] [addr]);
MSH_CMD_EXPORT_ALIAS(i2c_test_info_entry, i2c_test_info, Display I2C adapter info [bus]);
MSH_CMD_EXPORT_ALIAS(i2c_list_devices_entry, i2c_list, List all available I2C devices);
MSH_CMD_EXPORT_ALIAS(i2c_test_addr_entry, i2c_addr, Test single I2C address [bus] [addr]);
#endif
