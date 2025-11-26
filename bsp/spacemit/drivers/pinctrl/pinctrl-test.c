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

#ifdef RT_USING_FINSH

static inline uint32_t read_reg(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static int check_pull_type(uint32_t val, pull_type_t pull_type)
{
	uint32_t mask;
	if (!(val & PMUX_PULL_SEL))
		return 0;

	mask = PMUX_PULLDWN << (pull_type - PMUX_PULL_DOWN);
	if (val & mask)
		return 1;

	return 0;
}

static int check_mode(uint32_t val, uint32_t mode)
{
	val &= PMUX_AF_SEL_MASK;

	return val == mode;
}

static int check_drive_strength(uint32_t val, uint32_t strength)
{
	val &= PMUX_DRIVE_MASK;
	val >>= PMUX_DRIVE_SHIFT;

	return val == strength;
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

	tx_val = read_reg(0xd401e0bc);
	rx_val = read_reg(0xd401e0c0);

	return check_all(tx_val, PMUX_PULL_UP, 1, 4) &&
	       check_all(rx_val, PMUX_PULL_UP, 1, 4);
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
