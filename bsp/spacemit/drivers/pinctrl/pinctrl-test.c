#include <rtthread.h>
#include <stdint.h>

typedef enum {
	PMUX_PULL_DIS,
	PMUX_PULL_DOWN,
	PMUX_PULL_UP,
}pull_type_t;

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

static int pmux_test(void)
{
	int ret;

	ret = pmux_check_uart0();
	if (ret) {
		rt_kprintf("%s:%d, check_uart0 failed\n", __func__, __LINE__);
		return -RT_EINVAL;
	}

	rt_kprintf("ok\n");
	return 0;
}

#include <finsh.h>
MSH_CMD_EXPORT(pmux_test, pinctrl driver test. e.g: pmux_test());
#endif
