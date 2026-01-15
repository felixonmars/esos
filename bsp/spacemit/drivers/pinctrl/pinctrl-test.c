#include <rtthread.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
	PMUX_PULL_DIS,
	PMUX_PULL_DOWN,
	PMUX_PULL_UP,
}pull_type_t;

#define PMUX_BASE_ADDR			0xd401e000

#define PMUX_BIT(x)			(1ul << x)

#define PMUX_PULL_SEL			PMUX_BIT(15)
#define PMUX_PULLUP			PMUX_BIT(14)
#define PMUX_PULLDWN			PMUX_BIT(13)

#define PMUX_DRIVE_SHIFT		(9)
#define PMUX_DRIVE_MASK			(0xf << PMUX_DRIVE_SHIFT)

#define PMUX_AF_SEL_MASK		(0x07)

#define ERR(fmt, ...) \
	rt_kprintf("[%s:%d %s] " fmt"\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifdef RT_USING_FINSH

static inline uint32_t _pin2offset(uint16_t pin)
{
	if (pin > 130)
		pin += 2;
	return pin << 2;
}

static inline uint32_t read_reg(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static int check_pull_type(uint32_t val, pull_type_t pull_type)
{
	uint32_t mask;

	if (pull_type == PMUX_PULL_DIS) {
		if (!(val & PMUX_PULLUP) && !(val & PMUX_PULLDWN))
			return 1;

		ERR("check PMUX_PULL_DIS but pull-up/down is set");

		return 0;
	}

	if (!(val & PMUX_PULL_SEL)) {
		ERR("PMUX_PULL_SEL unmask: %x", val);
		return 0;
	}

	mask = PMUX_PULLDWN << (pull_type - PMUX_PULL_DOWN);
	if (val & mask)
		return 1;

	ERR("%s unmask: %x", pull_type == PMUX_PULL_DOWN ? \
	    "PMUX_PULL_DOWN" : "PMUX_PULL_UP", val);
	return 0;
}

static int check_mode(uint32_t val, uint32_t mode)
{
	int ret;

	val &= PMUX_AF_SEL_MASK;

	ret = val == mode;

	if (!ret)
		ERR("actual mode: %d, expected: %d", val, mode);

	return ret;
}

static int check_drive_strength(uint32_t val, uint32_t strength)
{
	int ret;

	val &= PMUX_DRIVE_MASK;
	val >>= PMUX_DRIVE_SHIFT;

	ret = val == strength;

	if (!ret)
		ERR("actual strength: %d, expected: %d", val, strength);

	return ret;
}

static inline int check_all(uint32_t val, pull_type_t pull_type,
			uint32_t mode, uint32_t strength)
{
	return check_pull_type(val, pull_type) &&
	       check_mode(val, mode) &&
	       check_drive_strength(val, strength);
}

static int pmux_check_uart0(void)
{
	uint32_t tx_val, rx_val;

	tx_val = read_reg(0xd401e1e8);
	rx_val = read_reg(0xd401e1ec);

	return check_all(tx_val, PMUX_PULL_UP, 4, 8) &&
	       check_all(rx_val, PMUX_PULL_UP, 4, 8);
}

static void pmux_print_usage(void)
{
	rt_kprintf("Usage: pmux_check_pin <pin> <pull> <mode> <strength>\n");
	rt_kprintf("  pull: 0=DIS, 1=DOWN, 2=UP\n");
	rt_kprintf("  mode: 0-7\n");
	rt_kprintf("  strength: 0-15\n");
}

static void pmux_test(int argc, char **argv)
{
	int pin, ret, pull_in, mode_in, strength_in;
	uint32_t val;
	uint32_t reg_addr;

	if (argc == 1) {
		rt_kprintf("No arguments, fall back to check uart0\n");

		ret = pmux_check_uart0();
		if (!ret)
			return;

		rt_kprintf("ok\n");
		return;
	}

	if (argv[1][0] >= '0' && argv[1][0] <= '9') {
		if (argc < 5) {
			pmux_print_usage();
			return;
		}

		pin = atoi(argv[1]);
		pull_in = atoi(argv[2]);
		mode_in = atoi(argv[3]);
		strength_in = atoi(argv[4]);

		reg_addr = PMUX_BASE_ADDR + _pin2offset((uint16_t)pin);
		val = read_reg(reg_addr);

		if (!check_all(val, (pull_type_t)pull_in, (uint32_t)mode_in, (uint32_t)strength_in))
			return;

		rt_kprintf("ok\n");
	} else {
		pmux_print_usage();
	}
}

#include <finsh.h>
MSH_CMD_EXPORT(pmux_test, pinctrl driver test. e.g: pmux_test help);
#endif
