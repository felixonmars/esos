/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Usage (RT-Thread Shell):
 *   i2c_list                            # List available I2C bus devices
 *   i2c_test [bus]                      # Full range read/write test and restore (default address: 0x50)
 *   i2c_read <bus> <addr> <len>         # Read specified length of data starting from address 0x00
 *
 * Examples:
 *   i2c_list                            # List all I2C buses
 *   i2c_test ri2c1                      # Run test on ri2c1 bus
 *   i2c_read ri2c0 0x50 64              # Read 64 bytes from address 0x00
 *
 * Note: Driver uses interrupt mode by default (intr_enable=true)
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/i2c.h>
#include <drivers/i2c_dev.h>
#include <string.h>
#include <stdlib.h>


/* Default I2C slave device address (usually EEPROM address) */
#ifndef I2C_K1_DEFAULT_ADDR
#define I2C_K1_DEFAULT_ADDR 0x50
#endif

/* Test context structure, used to pass bus and address info between test functions */
typedef struct {
	const char *bus_name;   /* I2C bus device name */
	rt_uint16_t addr7;      /* 7-bit slave device address */
} i2c_k1_ctx_t;


static struct rt_i2c_bus_device *i2c_k1_get_bus(const char *bus_name)
{
	struct rt_i2c_bus_device *bus = rt_i2c_bus_device_find(bus_name);
	if (bus == RT_NULL) {
		rt_kprintf("I2C bus '%s' not found (use 'i2c_list' to check)\n", bus_name);
		return RT_NULL;
	}
	return bus;
}

static rt_err_t i2c_k1_read_byte(struct rt_i2c_bus_device *bus,
                                 rt_uint16_t addr7,
                                 rt_uint8_t reg,
                                 rt_uint8_t *val)
{
	rt_uint8_t reg_buf = reg;
	struct rt_i2c_msg msgs[2];

	msgs[0].addr = addr7;
	msgs[0].flags = RT_I2C_WR;
	msgs[0].buf = &reg_buf;
	msgs[0].len = 1;

	msgs[1].addr = addr7;
	msgs[1].flags = RT_I2C_RD;
	msgs[1].buf = val;
	msgs[1].len = 1;

	return (rt_i2c_transfer(bus, msgs, 2) == 2) ? RT_EOK : -RT_ERROR;
}

static rt_err_t i2c_k1_write_byte(struct rt_i2c_bus_device *bus,
                                  rt_uint16_t addr7,
                                  rt_uint8_t reg,
                                  rt_uint8_t val)
{
	rt_uint8_t buf[2] = {reg, val};
	struct rt_i2c_msg msg;

	msg.addr = addr7;
	msg.flags = RT_I2C_WR;
	msg.buf = buf;
	msg.len = 2;

	return (rt_i2c_transfer(bus, &msg, 1) == 1) ? RT_EOK : -RT_ERROR;
}

static rt_err_t test_full_range(i2c_k1_ctx_t *ctx)
{
	struct rt_i2c_bus_device *bus = i2c_k1_get_bus(ctx->bus_name);
	if (bus == RT_NULL)
		return -RT_ENOSYS;

	rt_uint8_t original[256];
	rt_uint8_t verify[256];
	rt_err_t err = RT_EOK;
	rt_err_t restore_err = RT_EOK;

	rt_kprintf("\n[TEST] Full Range Read/Write Verify (0x00-0xFF)\n");

	/* Step 1: Read and save original data */
	for (int i = 0; i < 256; i++) {
		err = i2c_k1_read_byte(bus, ctx->addr7, (rt_uint8_t)i, &original[i]);
		if (err) {
			rt_kprintf("Read original failed at 0x%02X: %d\n", i, err);
			return err;
		}
	}
	rt_kprintf("Step 1: Saved original data (0x00-0xFF)\n");

	/* Step 2: Write pattern data (0x00~0xFF) */
	for (int i = 0; i < 256; i++) {
		err = i2c_k1_write_byte(bus, ctx->addr7, (rt_uint8_t)i, (rt_uint8_t)i);
		if (err) {
			rt_kprintf("Write pattern failed at 0x%02X: %d\n", i, err);
			goto restore;
		}
		/* EEPROM write cycle time */
		rt_thread_mdelay(50);
	}
	rt_kprintf("Step 2: Written pattern data (0x00-0xFF)\n");

	/* Step 3: Read back and verify */
	for (int i = 0; i < 256; i++) {
		err = i2c_k1_read_byte(bus, ctx->addr7, (rt_uint8_t)i, &verify[i]);
		if (err) {
			rt_kprintf("Read verify failed at 0x%02X: %d\n", i, err);
			goto restore;
		}
	}

	int mism = 0;
	for (int i = 0; i < 256; i++) {
		if (verify[i] != (rt_uint8_t)i) {
			if (mism < 8) {
				rt_kprintf("Mismatch at 0x%02X: exp 0x%02X got 0x%02X\n",
				           i, (rt_uint8_t)i, verify[i]);
			}
			mism++;
		}
	}

	if (mism) {
		rt_kprintf("Verification FAILED, mismatches=%d\n", mism);
		err = -RT_ERROR;
		goto restore;
	} else {
		rt_kprintf("Verification PASSED\n");
	}

restore:
	/* Step 4: Restore original data */
	rt_kprintf("Step 4: Restoring original data...\n");
	for (int i = 0; i < 256; i++) {
		rt_err_t r = i2c_k1_write_byte(bus, ctx->addr7, (rt_uint8_t)i, original[i]);
		if (r && restore_err == RT_EOK)
			restore_err = r;
		rt_thread_mdelay(50);
	}

	if (restore_err) {
		rt_kprintf("Restore FAILED (%d)\n", restore_err);
	} else {
		rt_kprintf("Restore PASSED\n");
	}

	if (err)
		return err;
	if (restore_err)
		return restore_err;

	rt_kprintf("Full range test PASSED\n");
	return RT_EOK;
}

static rt_err_t test_read_range(const char *bus_name, rt_uint16_t addr7, rt_size_t len)
{
	struct rt_i2c_bus_device *bus = i2c_k1_get_bus(bus_name);
	if (bus == RT_NULL)
		return -RT_ENOSYS;

	if (len == 0 || len > 256) {
		rt_kprintf("Invalid length: %u (valid range: 1-256)\n", (unsigned)len);
		return -RT_ERROR;
	}

	rt_kprintf("\n[TEST] Read Range (0x%02X-0x%02X), len=%u\n", (rt_uint8_t)0, (rt_uint8_t)(len - 1), (unsigned)len);

	for (rt_size_t i = 0; i < len; i++) {
		rt_uint8_t val = 0;
		rt_err_t err = i2c_k1_read_byte(bus, addr7, (rt_uint8_t)i, &val);
		if (err) {
			rt_kprintf("Read failed at 0x%02X: %d\n", (rt_uint8_t)i, err);
			return err;
		}

		if ((i % 16) == 0) {
			if (i != 0)
				rt_kprintf("\n");
			rt_kprintf("0x%02X: ", (rt_uint8_t)i);
		}
		rt_kprintf("%02X ", val);
	}
	rt_kprintf("\n");
	return RT_EOK;
}

static int parse_i2c_args(int argc, char **argv, i2c_k1_ctx_t *ctx)
{
	ctx->addr7 = I2C_K1_DEFAULT_ADDR;

	if (argc < 2 || !argv[1] || rt_strlen(argv[1]) == 0) {
		return -RT_ERROR;
	}
	ctx->bus_name = argv[1];
	return RT_EOK;
}

static void print_test_header(i2c_k1_ctx_t *ctx, const char *test_name)
{
	rt_kprintf("\n==== %s ====\n", test_name);
	rt_kprintf("Bus   : %s\n", ctx->bus_name);
	rt_kprintf("Addr  : 0x%02X (7-bit)\n", ctx->addr7 & 0x7F);
}

/* 全量读写测试 */
static int i2c_test_basic_entry(int argc, char **argv)
{
	i2c_k1_ctx_t ctx;

	if (parse_i2c_args(argc, argv, &ctx) != RT_EOK) {
		rt_kprintf("Usage: i2c_test_basic <bus>\n");
		return -RT_ERROR;
	}

	print_test_header(&ctx, "I2C Full Range Read/Write Test");
	rt_err_t err = test_full_range(&ctx);

	if (err)
		rt_kprintf("\nFull Range Test: FAILED (%d)\n", err);
	else
		rt_kprintf("\nFull Range Test: PASSED\n");
	return err;
}

static int i2c_test_read_entry(int argc, char **argv)
{

	if (argc < 4) {
		rt_kprintf("Usage: i2c_test_read <bus> <addr> <len>\n");
		return -RT_ERROR;
	}

	const char *bus_name = argv[1];
	rt_uint16_t addr7;
	rt_size_t len;

	if (rt_strlen(argv[2]) > 2 && (argv[2][0] == '0') && (argv[2][1] == 'x' || argv[2][1] == 'X'))
		addr7 = (rt_uint16_t)strtoul(argv[2], RT_NULL, 16);
	else
		addr7 = (rt_uint16_t)strtoul(argv[2], RT_NULL, 10);

	if (rt_strlen(argv[3]) > 2 && (argv[3][0] == '0') && (argv[3][1] == 'x' || argv[3][1] == 'X'))
		len = (rt_size_t)strtoul(argv[3], RT_NULL, 16);
	else
		len = (rt_size_t)strtoul(argv[3], RT_NULL, 10);

	rt_kprintf("\n==== I2C Read Test ====\n");
	rt_kprintf("Bus   : %s\n", bus_name);
	rt_kprintf("Addr  : 0x%02X (7-bit)\n", addr7 & 0x7F);
	rt_kprintf("Len   : %u\n", (unsigned)len);

	return test_read_range(bus_name, addr7, len);
}

static int i2c_list_devices_entry(int argc, char **argv)
{
	const char *bus_names[] = {"ri2c0", "ri2c1", "ri2c2"};
	int found_count = 0;

	(void)argc;
	(void)argv;

	rt_kprintf("Available I2C buses:\n");

	for (int i = 0; i < (int)(sizeof(bus_names) / sizeof(bus_names[0])); i++) {
		struct rt_i2c_bus_device *bus = rt_i2c_bus_device_find(bus_names[i]);
		if (bus != RT_NULL) {
			rt_kprintf("  %s\n", bus_names[i]);
			found_count++;
		}
	}

	if (found_count == 0) {
		rt_kprintf("  None\n");
	}

	return RT_EOK;
}

#ifdef RT_USING_FINSH
MSH_CMD_EXPORT_ALIAS(i2c_test_basic_entry, i2c_test, I2C full range read/write test <bus>);
MSH_CMD_EXPORT_ALIAS(i2c_test_read_entry, i2c_read, I2C read test [bus] [addr] [len]);
MSH_CMD_EXPORT_ALIAS(i2c_list_devices_entry, i2c_list, List all available I2C devices);
#endif