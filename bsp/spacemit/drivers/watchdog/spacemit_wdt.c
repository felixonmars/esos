/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include <drivers/watchdog.h>

#define WDT_WMER	(0x00b8)
#define WDT_WMR		(0x00bc)
#define WDT_WVR		(0x00cc)
#define WDT_WCR		(0x00c8)
#define WDT_WSR		(0x00c0)
#define WDT_WFAR	(0x00b0)
#define WDT_WSAR	(0x00b4)
#define WDT_WICR	(0x00c4)

/* watchdog timer 16bit, 256Hz */
#define DEFAULT_SHIFT				8
#define SPACEMIT_WATCHDOG_MAX_TIMEOUT		255
#define SPACEMIT_DEFAULT_TIMEOUT		100

#define MPMU_APRR		(0x1020)
#define MPMU_APRR_WDTR		(1 << 4)
#define MPMU_ARSR		(0x1028)
#define MPMU_ARSR_SWR_MASK	(0x3f << 8)

struct spacemit_wdt {
	rt_watchdog_t dev;
	char *name;
	struct clk *clk;
	struct clk *rst;
	void *mmio_base;
	void *mpmu_base;
	bool wdt_clk_enable;
	rt_spinlock_t wdt_lock;
	rt_uint32_t timeout;
};

static inline rt_uint32_t spacemit_wdt_read(struct spacemit_wdt *chip, rt_uint32_t reg)
{
	return readl(chip->mmio_base + reg);
}

static inline void spacemit_wdt_write_access(struct spacemit_wdt *chip)
{
	writel(0xbaba, chip->mmio_base + WDT_WFAR);
	writel(0xeb10, chip->mmio_base + WDT_WSAR);
}

static inline void spacemit_wdt_write(struct spacemit_wdt *chip, rt_uint32_t reg, rt_uint32_t val)
{
	spacemit_wdt_write_access(chip);
	writel(val, chip->mmio_base + reg);
}

static inline struct spacemit_wdt *to_spacemit_wdt(rt_watchdog_t *dev)
{
	return rt_container_of(dev, struct spacemit_wdt, dev);
}

rt_err_t spacemit_wdt_init(rt_watchdog_t *wdt)
{
	struct spacemit_wdt *chip = to_spacemit_wdt(wdt);
	rt_int32_t reg;

	chip->timeout = 0;
	reg = readl(chip->mpmu_base + MPMU_ARSR);
	reg &= ~MPMU_ARSR_SWR_MASK;
	writel(reg, chip->mpmu_base + MPMU_ARSR);

	return 0;
}

static void spacemit_wdt_set_timeout(struct spacemit_wdt *chip, rt_uint32_t timeout)
{
	rt_uint32_t tick = timeout << DEFAULT_SHIFT;
	if (tick > 0xffff) {
		rt_kprintf("watchdog timeout out of range, use max value\n");
		timeout = SPACEMIT_WATCHDOG_MAX_TIMEOUT;
		tick = timeout << DEFAULT_SHIFT;
	}
	chip->timeout = timeout;
	spacemit_wdt_write(chip, WDT_WMR, tick);
}

static rt_uint32_t spacemit_wdt_get_timeout(struct spacemit_wdt *chip)
{
	return chip->timeout;
}

static void spacemit_enable_wdt_clk(struct spacemit_wdt *chip)
{
	if (chip->wdt_clk_enable == false) {
		clk_prepare_enable(chip->clk);
		clk_prepare_enable(chip->rst);
		chip->wdt_clk_enable = true;
	}
}

static void spacemit_disable_wdt_clk(struct spacemit_wdt *chip)
{
	clk_disable_unprepare(chip->rst);
	clk_disable_unprepare(chip->clk);
}

static void spacemit_wdt_stop(struct spacemit_wdt *chip)
{
	rt_spin_lock(&chip->wdt_lock);

	spacemit_wdt_write(chip, WDT_WCR, 0x1);

	spacemit_wdt_write(chip, WDT_WMER, 0x0);

	rt_spin_unlock(&chip->wdt_lock);

	spacemit_disable_wdt_clk(chip);
}

static void spacemit_wdt_start(struct spacemit_wdt *chip)
{
	rt_uint32_t timeout, reg;
	void *mpmu_aprr;

	spacemit_enable_wdt_clk(chip);

	rt_spin_lock(&chip->wdt_lock);

	timeout = spacemit_wdt_get_timeout(chip);
	if ((timeout == 0) && (chip->timeout != 0))
		spacemit_wdt_set_timeout(chip, chip->timeout);
	else if ((timeout == 0) && (chip->timeout == 0)) {
		rt_kprintf("wdt has no timeout, use default timeout\n");
		spacemit_wdt_set_timeout(chip, SPACEMIT_DEFAULT_TIMEOUT);
	}

	spacemit_wdt_write(chip, WDT_WMER, 0x3);

	reg = readl(chip->mpmu_base + MPMU_APRR);
	reg |= MPMU_APRR_WDTR;
	writel(reg, chip->mpmu_base + MPMU_APRR);

	spacemit_wdt_write(chip, WDT_WSR, 0x0);

	rt_spin_unlock(&chip->wdt_lock);
}

static rt_uint32_t spacemit_wdt_get_timeleft(struct spacemit_wdt *chip)
{
	rt_uint32_t ret = spacemit_wdt_read(chip, WDT_WVR);
	ret = (chip->timeout << DEFAULT_SHIFT - ret) >> 8;
	return ret;
}

static int spacemit_wdt_ping(struct spacemit_wdt *chip)
{
	rt_int32_t ret;

	rt_spin_lock(&chip->wdt_lock);
	if (chip->timeout != 0) {
		spacemit_wdt_write(chip, WDT_WCR, 0x1);
	} else
		ret = -RT_EINVAL;

	rt_spin_unlock(&chip->wdt_lock);

	return 0;
}

static rt_err_t spacemit_wdt_ctrl(rt_watchdog_t *wdt, int cmd, void *arg)
{
	struct spacemit_wdt *chip = to_spacemit_wdt(wdt);
	rt_err_t err;
	rt_uint32_t ret;

	switch (cmd) {
	case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:
		ret = spacemit_wdt_get_timeout(chip);
		*(rt_uint32_t *)arg = ret;
		err = 0;
		break;
	case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:
		ret = *(rt_uint32_t *)arg;
		spacemit_wdt_set_timeout(chip, ret);
		err = 0;
		break;
	case RT_DEVICE_CTRL_WDT_GET_TIMELEFT:
		ret = spacemit_wdt_get_timeleft(chip);
		*(rt_uint32_t *)arg = ret;
		err = 0;
		break;
	case RT_DEVICE_CTRL_WDT_KEEPALIVE:
		err = spacemit_wdt_ping(chip);
		break;
	case RT_DEVICE_CTRL_WDT_START:
		spacemit_wdt_start(chip);
		err = 0;
		break;
	case RT_DEVICE_CTRL_WDT_STOP:
		spacemit_wdt_stop(chip);
		err = 0;
		break;
	default:
		err = -RT_EINVAL;
		break;
	}

	return err;
}

static const struct rt_watchdog_ops spacemit_wdt_ops = {
	.init = spacemit_wdt_init,
	.control = spacemit_wdt_ctrl,
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k3-wdt", .data = "k3-wdt" },
	{},
};

int spacemit_wdt_probe(void)
{
	int i;
	struct spacemit_wdt *chip;
	rt_watchdog_t *dev;
	struct dtb_node *compatible_node;
	struct dtb_node *dtb_head_node = get_dtb_node_head();

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); i++) {
		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);
		if (compatible_node != RT_NULL) {
			/* check the status */
			if (!dtb_node_device_is_available(compatible_node))
				continue;
			chip = rt_calloc(1, sizeof(struct spacemit_wdt));
			if (!chip) {
				rt_kprintf("%s:%d, calloc failed\n", __func__, __LINE__);
				return -RT_ENOMEM;
			}

			dev = &chip->dev;
			chip->wdt_clk_enable = false;
			chip->name = (char *)__compatible[i].data;
			chip->mmio_base = (void *)dtb_node_get_addr_index(compatible_node, 0);
			if (chip->mmio_base < 0) {
				rt_kprintf("get watchdog mmio_base failed\n");
				return -RT_ERROR;
			}
			chip->mpmu_base = (void *)dtb_node_get_addr_index(compatible_node, 1);
			if (chip->mpmu_base < 0) {
				rt_kprintf("get watchdog mpmu_base failed\n");
				return -RT_ERROR;
			}
			chip->clk = of_clk_get(compatible_node, 0);
			if (IS_ERR(chip->clk)) {
				rt_kprintf("get watchdog clk failed\n");
				return -RT_ERROR;
			}
			chip->rst = of_clk_get(compatible_node, 1);
			if (IS_ERR(chip->rst)) {
				rt_kprintf("get watchdog reset failed\n");
				return -RT_ERROR;
			}

			spacemit_enable_wdt_clk(chip);
			rt_spin_lock_init(&chip->wdt_lock);
			dev->ops = &spacemit_wdt_ops;
			rt_hw_watchdog_register(dev, chip->name, RT_DEVICE_FLAG_RDWR, (void *)chip);
		}
	}
	return 0;
}
INIT_DEVICE_EXPORT(spacemit_wdt_probe);
