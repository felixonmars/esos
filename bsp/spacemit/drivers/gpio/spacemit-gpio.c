/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>

/* Register offset structure for different chip variants */
struct spacemit_gpio_reg_offsets {
	uint32_t gplr;
	uint32_t gpdr;
	uint32_t gpsr;
	uint32_t gpcr;
	uint32_t grer;
	uint32_t gfer;
	uint32_t gedr;
	uint32_t gsdr;
	uint32_t gcdr;
	uint32_t gsrer;
	uint32_t gcrer;
	uint32_t gsfer;
	uint32_t gcfer;
	uint32_t gapmask;
	uint32_t gcpmask;
};

/* K1 register offsets */
static const struct spacemit_gpio_reg_offsets k1_regs = {
	.gplr    = 0x00,
	.gpdr    = 0x0c,
	.gpsr    = 0x18,
	.gpcr    = 0x24,
	.grer    = 0x30,
	.gfer    = 0x3c,
	.gedr    = 0x48,
	.gsdr    = 0x54,
	.gcdr    = 0x60,
	.gsrer   = 0x6c,
	.gcrer   = 0x78,
	.gsfer   = 0x84,
	.gcfer   = 0x90,
	.gapmask = 0x9c,
	.gcpmask = 0xA8,
};

/* K3 register offsets */
static const struct spacemit_gpio_reg_offsets k3_regs = {
	.gplr    = 0x00,
	.gpdr    = 0x04,
	.gpsr    = 0x08,
	.gpcr    = 0x0c,
	.grer    = 0x10,
	.gfer    = 0x14,
	.gedr    = 0x18,
	.gsdr    = 0x1c,
	.gcdr    = 0x20,
	.gsrer   = 0x24,
	.gcrer   = 0x28,
	.gsfer   = 0x2c,
	.gcfer   = 0x30,
	.gapmask = 0x34,
	.gcpmask = 0x38,
};

/* Chip-specific configuration */
struct spacemit_gpio_chip_data {
	const struct spacemit_gpio_reg_offsets *regs;
	uint32_t bank_offsets[4];  /* Support up to 4 banks */
	int gpio_base;             /* GPIO base number for this controller */
	int total_gpios;           /* Total number of GPIOs for this controller */
};

/* K1 chip data */
static const struct spacemit_gpio_chip_data k1_chip_data = {
	.regs         = &k1_regs,
	.bank_offsets = {0x0, 0x4, 0x8, 0x100},
	.gpio_base    = 0,         /* GPIO 0-127 */
	.total_gpios  = 128,       /* 4 banks * 32 GPIOs */
};

/* K3 AP-GPIO chip data (big core) - 128 GPIOs, 4 banks */
static const struct spacemit_gpio_chip_data k3_ap_gpio_chip_data = {
	.regs         = &k3_regs,
	.bank_offsets = {0x0, 0x40, 0x80, 0x100},
	.gpio_base    = 0,         /* GPIO 0-127 */
	.total_gpios  = 128,       /* 4 banks * 32 GPIOs */
};

/* K3 R-GPIO chip data (small core) - 36 GPIOs, 2 banks
 * Bank 0: R-GPIO0 with 32 GPIOs (GPIO 128-159)
 * Bank 1: R-GPIO1 with 4 GPIOs (GPIO 160-163)
 * Both banks share the same IRQ line
 */
static const struct spacemit_gpio_chip_data k3_r_gpio_chip_data = {
	.regs         = &k3_regs,
	.bank_offsets = {0x0, 0x40, 0x0, 0x0},  /* Bank 0 at 0x0, Bank 1 at 0x40 */
	.gpio_base    = 128,       /* GPIO 128-163 (R-GPIO 0-35) */
	.total_gpios  = 36,        /* 32 + 4 GPIOs */
};

#define BANK_GPIO_NUMBER	(32)
#define BANK_GPIO_MASK		(BANK_GPIO_NUMBER - 1)

#define gpio_to_bank_idx(gpio)	((gpio) / BANK_GPIO_NUMBER)
#define gpio_to_bank_offset(gpio)	((gpio) & BANK_GPIO_MASK)
#define bank_to_gpio(idx, offset)	(((idx) * BANK_GPIO_NUMBER) \
		| ((offset) & BANK_GPIO_MASK))

struct spacemit_gpio_bank {
	void *reg_bank;
	unsigned int irq_mask;
	unsigned int irq_rising_edge;
	unsigned int irq_falling_edge;
};

struct spacemit_gpio_chip {
	struct gpio_chip chip;
	void *reg_base;
	int irq;
	uint32_t ngpio;
	uint32_t nbank;
	struct spacemit_gpio_bank *banks;
	const struct spacemit_gpio_chip_data *chip_data;
	struct rt_pin_irq_hdr *pin_irq_hdr_tab;  /* Per-chip IRQ handler table */
	struct rt_spinlock lock;                  /* Spinlock for register access protection */
	struct clk *clk;                          /* Clock for GPIO controller */
	struct clk *rst_clk;                      /* Reset clock (deassert on enable) */
};

struct spacemit_gpio_compatible {
	const char *compatible;
	const struct spacemit_gpio_chip_data *chip_data;
};

static struct spacemit_gpio_compatible __compatible[] = {
	{ .compatible = "spacemit,k1x-gpio", .chip_data = &k1_chip_data },
	{ .compatible = "spacemit,k3-gpio", .chip_data = &k3_ap_gpio_chip_data },
	{ .compatible = "spacemit,k3-rgpio", .chip_data = &k3_r_gpio_chip_data },
	{ },
};

/* Global chip list for PIN device lookup */
#define MAX_GPIO_CHIPS 16
static struct spacemit_gpio_chip *g_gpio_chips[MAX_GPIO_CHIPS];
static int g_gpio_chip_count = 0;

/* Helper function to find spacemit_gpio_chip from global GPIO number */
static struct spacemit_gpio_chip *gpio_num_to_spacemit_chip(unsigned int gpio)
{
	int i;

	for (i = 0; i < g_gpio_chip_count; i++) {
		struct spacemit_gpio_chip *chip = g_gpio_chips[i];
		if (chip && gpio >= chip->chip.base &&
		    gpio < chip->chip.base + chip->chip.ngpio) {
			return chip;
		}
	}
	return RT_NULL;
}

static int gpio_probe_dt(struct dtb_node *np, struct spacemit_gpio_chip *chip)
{
	int i;

	/* Initialize spinlock for register access protection */
	rt_spin_lock_init(&chip->lock);

	/* Allocate GPIO banks */
	chip->banks = rt_calloc(chip->nbank, sizeof(struct spacemit_gpio_bank));
	if (chip->banks == RT_NULL)
		return -RT_ENOMEM;

	/* Allocate per-chip IRQ handler table */
	chip->pin_irq_hdr_tab = rt_calloc(chip->chip_data->total_gpios, sizeof(struct rt_pin_irq_hdr));
	if (chip->pin_irq_hdr_tab == RT_NULL) {
		rt_free(chip->banks);
		return -RT_ENOMEM;
	}

	/* Initialize IRQ handler table */
	for (i = 0; i < chip->chip_data->total_gpios; i++) {
		chip->pin_irq_hdr_tab[i].pin = -1;
		chip->pin_irq_hdr_tab[i].hdr = RT_NULL;
		chip->pin_irq_hdr_tab[i].mode = 0;
		chip->pin_irq_hdr_tab[i].args = RT_NULL;
	}

	/* Use chip-specific bank offsets */
	for (i = 0; i < chip->nbank; i++) {
		struct spacemit_gpio_bank *bank = &chip->banks[i];
		const struct spacemit_gpio_reg_offsets *regs = chip->chip_data->regs;
		unsigned int test_val;

		bank->reg_bank = chip->reg_base + chip->chip_data->bank_offsets[i];
		bank->irq_mask = 0;
		bank->irq_rising_edge = 0;
		bank->irq_falling_edge = 0;

		/* Disable all interrupts initially (R-CPU only uses CPMASK) */
		writel(0, bank->reg_bank + regs->gcpmask);
		writel(0, bank->reg_bank + regs->grer);
		writel(0, bank->reg_bank + regs->gfer);
		/* Clear all pending interrupts */
		writel(0xffffffff, bank->reg_bank + regs->gedr);
	}

	chip->ngpio = chip->chip_data->total_gpios;

	return 0;
}

static int spacemit_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	/* Request GPIO from pinctrl subsystem to configure GPIO function mux */
	return 0;
}

static void spacemit_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	/* Free GPIO from pinctrl subsystem */
}

static int spacemit_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct spacemit_gpio_chip *spacemit_chip =
		rt_container_of(chip, struct spacemit_gpio_chip, chip);
	const struct spacemit_gpio_reg_offsets *regs = spacemit_chip->chip_data->regs;
	struct spacemit_gpio_bank *bank = &spacemit_chip->banks[gpio_to_bank_idx(offset)];
	unsigned int bit = (1 << gpio_to_bank_offset(offset));
	rt_base_t level;

	level = rt_spin_lock_irqsave(&spacemit_chip->lock);
	writel(bit, bank->reg_bank + regs->gcdr);
	rt_spin_unlock_irqrestore(&spacemit_chip->lock, level);

	return 0;
}

static int spacemit_gpio_direction_output(struct gpio_chip *chip,
					  unsigned offset, int value)
{
	struct spacemit_gpio_chip *spacemit_chip =
		rt_container_of(chip, struct spacemit_gpio_chip, chip);
	const struct spacemit_gpio_reg_offsets *regs = spacemit_chip->chip_data->regs;
	struct spacemit_gpio_bank *bank = &spacemit_chip->banks[gpio_to_bank_idx(offset)];
	unsigned int bit = (1 << gpio_to_bank_offset(offset));
	rt_base_t level;

	level = rt_spin_lock_irqsave(&spacemit_chip->lock);

	/* Set direction to output first */
	writel(bit, bank->reg_bank + regs->gsdr);

	/* Then set value */
	if (value)
		writel(bit, bank->reg_bank + regs->gpsr);
	else
		writel(bit, bank->reg_bank + regs->gpcr);

	rt_spin_unlock_irqrestore(&spacemit_chip->lock, level);

	return 0;
}

static int spacemit_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct spacemit_gpio_chip *spacemit_chip =
		rt_container_of(chip, struct spacemit_gpio_chip, chip);
	const struct spacemit_gpio_reg_offsets *regs = spacemit_chip->chip_data->regs;
	struct spacemit_gpio_bank *bank = &spacemit_chip->banks[gpio_to_bank_idx(offset)];
	unsigned int bit = (1 << gpio_to_bank_offset(offset));
	unsigned int gplr;
	rt_base_t level;
	int ret;

	level = rt_spin_lock_irqsave(&spacemit_chip->lock);
	gplr = readl(bank->reg_bank + regs->gplr);
	ret = !!(gplr & bit);
	rt_spin_unlock_irqrestore(&spacemit_chip->lock, level);

	return ret;
}

static void spacemit_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct spacemit_gpio_chip *spacemit_chip =
		rt_container_of(chip, struct spacemit_gpio_chip, chip);
	const struct spacemit_gpio_reg_offsets *regs = spacemit_chip->chip_data->regs;
	struct spacemit_gpio_bank *bank = &spacemit_chip->banks[gpio_to_bank_idx(offset)];
	unsigned int bit = (1 << gpio_to_bank_offset(offset));
	unsigned int gpdr;
	rt_base_t level;

	level = rt_spin_lock_irqsave(&spacemit_chip->lock);

	gpdr = readl(bank->reg_bank + regs->gpdr);

	/* Is it configured as output? */
	if (gpdr & bit) {
		if (value)
			writel(bit, bank->reg_bank + regs->gpsr);
		else
			writel(bit, bank->reg_bank + regs->gpcr);
	}

	rt_spin_unlock_irqrestore(&spacemit_chip->lock, level);
}

static int spacemit_gpio_of_xlate(struct gpio_chip *chip,
				  const struct fdt_phandle_args *gpiospec,
				  unsigned int *flags)
{
	struct spacemit_gpio_chip *spacemit_chip =
		rt_container_of(chip, struct spacemit_gpio_chip, chip);

	/* GPIO index start from 0. */
	if (gpiospec->args[0] >= spacemit_chip->ngpio)
		return -RT_EINVAL;

	if (flags)
		*flags = gpiospec->args[1];

	return gpiospec->args[0];
}

/* GPIO interrupt handler - param is the chip pointer */
static void spacemit_gpio_irq_handler(int irq, void *param)
{
	struct spacemit_gpio_chip *spacemit_chip = (struct spacemit_gpio_chip *)param;
	const struct spacemit_gpio_reg_offsets *regs;
	int i;

	if (!spacemit_chip)
		return;

	regs = spacemit_chip->chip_data->regs;

	for (i = 0; i < spacemit_chip->nbank; i++) {
		struct spacemit_gpio_bank *bank = &spacemit_chip->banks[i];
		unsigned int gedr, pending;
		int bit;

		gedr = readl(bank->reg_bank + regs->gedr);
		if (!gedr)
			continue;

		/* Clear edge detection status */
		writel(gedr, bank->reg_bank + regs->gedr);

		pending = gedr & bank->irq_mask;
		if (!pending)
			continue;

		/* Handle each pending interrupt */
		for (bit = 0; bit < 32; bit++) {
			if (pending & (1 << bit)) {
				int gpio_offset = i * 32 + bit;  /* Offset within this chip */

				/* Call registered callback if exists */
				if (gpio_offset < spacemit_chip->ngpio &&
				    spacemit_chip->pin_irq_hdr_tab[gpio_offset].hdr) {
					spacemit_chip->pin_irq_hdr_tab[gpio_offset].hdr(
						spacemit_chip->pin_irq_hdr_tab[gpio_offset].args);
				}
			}
		}
	}
}

static int spacemit_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct spacemit_gpio_chip *spacemit_chip =
		rt_container_of(chip, struct spacemit_gpio_chip, chip);

	return spacemit_chip->irq;
}

/* Helper function for interrupt control - takes chip pointer directly */
static rt_err_t spacemit_gpio_irq_enable(struct spacemit_gpio_chip *spacemit_chip,
					 unsigned int gpio, rt_uint32_t enabled)
{
	const struct spacemit_gpio_reg_offsets *regs;
	struct spacemit_gpio_bank *bank;
	unsigned int offset, bit;
	rt_base_t level;

	if (!spacemit_chip)
		return -RT_ERROR;

	offset = gpio - spacemit_chip->chip.base;
	regs = spacemit_chip->chip_data->regs;
	bank = &spacemit_chip->banks[gpio_to_bank_idx(offset)];
	bit = (1 << gpio_to_bank_offset(offset));

	level = rt_spin_lock_irqsave(&spacemit_chip->lock);

	if (enabled) {
		bank->irq_mask |= bit;

		/* Enable edge detection */
		if (bank->irq_rising_edge & bit)
			writel(bit, bank->reg_bank + regs->gsrer);
		if (bank->irq_falling_edge & bit)
			writel(bit, bank->reg_bank + regs->gsfer);

		/* Unmask interrupt (1=not masked, 0=masked) */
		writel(readl(bank->reg_bank + regs->gcpmask) | bit,
		       bank->reg_bank + regs->gcpmask);
		writel(readl(bank->reg_bank + regs->gapmask) | bit,
		       bank->reg_bank + regs->gapmask);

		/* Clear any pending interrupt */
		writel(bit, bank->reg_bank + regs->gedr);
	} else {
		bank->irq_mask &= ~bit;

		/* Disable edge detection */
		if (bank->irq_rising_edge & bit)
			writel(bit, bank->reg_bank + regs->gcrer);
		if (bank->irq_falling_edge & bit)
			writel(bit, bank->reg_bank + regs->gcfer);

		/* Mask interrupt (0=masked, 1=not masked) */
		writel(readl(bank->reg_bank + regs->gcpmask) & ~bit,
		       bank->reg_bank + regs->gcpmask);
		writel(readl(bank->reg_bank + regs->gapmask) & ~bit,
		       bank->reg_bank + regs->gapmask);
	}

	rt_spin_unlock_irqrestore(&spacemit_chip->lock, level);

	return RT_EOK;
}

/* Helper function for interrupt mode configuration - takes chip pointer directly */
static rt_err_t spacemit_gpio_irq_mode(struct spacemit_gpio_chip *spacemit_chip,
				       unsigned int gpio, rt_uint32_t mode)
{
	const struct spacemit_gpio_reg_offsets *regs;
	struct spacemit_gpio_bank *bank;
	unsigned int offset, bit;
	rt_base_t level;
	rt_err_t ret = RT_EOK;

	if (!spacemit_chip)
		return -RT_ERROR;

	offset = gpio - spacemit_chip->chip.base;
	regs = spacemit_chip->chip_data->regs;
	bank = &spacemit_chip->banks[gpio_to_bank_idx(offset)];
	bit = (1 << gpio_to_bank_offset(offset));

	level = rt_spin_lock_irqsave(&spacemit_chip->lock);

	/* Clear previous settings */
	bank->irq_rising_edge &= ~bit;
	bank->irq_falling_edge &= ~bit;
	writel(bit, bank->reg_bank + regs->gcrer);
	writel(bit, bank->reg_bank + regs->gcfer);

	/* Configure new trigger mode */
	switch (mode) {
	case PIN_IRQ_MODE_RISING:
		bank->irq_rising_edge |= bit;
		writel(bit, bank->reg_bank + regs->gsrer);
		break;
	case PIN_IRQ_MODE_FALLING:
		bank->irq_falling_edge |= bit;
		writel(bit, bank->reg_bank + regs->gsfer);
		break;
	case PIN_IRQ_MODE_RISING_FALLING:
		bank->irq_rising_edge |= bit;
		bank->irq_falling_edge |= bit;
		writel(bit, bank->reg_bank + regs->gsrer);
		writel(bit, bank->reg_bank + regs->gsfer);
		break;
	default:
		ret = -RT_EINVAL;
		break;
	}

	rt_spin_unlock_irqrestore(&spacemit_chip->lock, level);

	return ret;
}

/* PIN device operations
 * Note: Uses global GPIO number space, finds the appropriate chip dynamically
 */
static void spacemit_pin_mode(struct rt_device *device, rt_base_t pin, rt_base_t mode)
{
	struct spacemit_gpio_chip *chip = gpio_num_to_spacemit_chip(pin);

	if (!chip) {
		rt_kprintf("Error: GPIO %d not found in any controller\n", pin);
		return;
	}

	switch (mode) {
	case PIN_MODE_OUTPUT:
		gpio_direction_output(pin, 0);
		break;
	case PIN_MODE_INPUT:
		gpio_direction_input(pin);
		break;
	case PIN_MODE_INPUT_PULLUP:
	case PIN_MODE_INPUT_PULLDOWN:
		gpio_direction_input(pin);
		/* Note: Pull-up/down should be configured via pinctrl */
		break;
	case PIN_MODE_OUTPUT_OD:
		/* Note: Open-drain should be configured via pinctrl */
		gpio_direction_output(pin, 0);
		break;
	}
}

static void spacemit_pin_write(struct rt_device *device, rt_base_t pin, rt_base_t value)
{
	struct spacemit_gpio_chip *chip = gpio_num_to_spacemit_chip(pin);

	if (!chip) {
		rt_kprintf("Error: GPIO %d not found in any controller\n", pin);
		return;
	}

	gpio_set_value(pin, value);
}

static int spacemit_pin_read(struct rt_device *device, rt_base_t pin)
{
	struct spacemit_gpio_chip *chip = gpio_num_to_spacemit_chip(pin);

	if (!chip) {
		rt_kprintf("Error: GPIO %d not found in any controller\n", pin);
		return -1;
	}

	return gpio_get_value(pin);
}

static rt_err_t spacemit_pin_attach_irq(struct rt_device *device, rt_int32_t pin,
					rt_uint32_t mode, void (*hdr)(void *args), void *args)
{
	struct spacemit_gpio_chip *chip = gpio_num_to_spacemit_chip(pin);
	int offset;

	if (!chip) {
		rt_kprintf("Error: GPIO %d not found in any controller\n", pin);
		return -RT_EINVAL;
	}

	/* Calculate offset within this chip */
	offset = pin - chip->chip.base;

	if (chip->pin_irq_hdr_tab[offset].pin != -1 && chip->pin_irq_hdr_tab[offset].hdr != RT_NULL) {
		rt_kprintf("Pin %d already attached\n", pin);
		return -RT_EBUSY;
	}

	chip->pin_irq_hdr_tab[offset].pin = pin;
	chip->pin_irq_hdr_tab[offset].mode = mode;
	chip->pin_irq_hdr_tab[offset].hdr = hdr;
	chip->pin_irq_hdr_tab[offset].args = args;

	/* Configure interrupt mode */
	return spacemit_gpio_irq_mode(chip, pin, mode);
}

static rt_err_t spacemit_pin_detach_irq(struct rt_device *device, rt_int32_t pin)
{
	struct spacemit_gpio_chip *chip = gpio_num_to_spacemit_chip(pin);
	int offset;

	if (!chip) {
		rt_kprintf("Error: GPIO %d not found in any controller\n", pin);
		return -RT_EINVAL;
	}

	/* Calculate offset within this chip */
	offset = pin - chip->chip.base;

	/* Disable interrupt first */
	spacemit_gpio_irq_enable(chip, pin, 0);

	chip->pin_irq_hdr_tab[offset].pin = -1;
	chip->pin_irq_hdr_tab[offset].mode = 0;
	chip->pin_irq_hdr_tab[offset].hdr = RT_NULL;
	chip->pin_irq_hdr_tab[offset].args = RT_NULL;

	return RT_EOK;
}

static rt_err_t spacemit_pin_irq_enable_ops(struct rt_device *device, rt_base_t pin, rt_uint32_t enabled)
{
	struct spacemit_gpio_chip *chip = gpio_num_to_spacemit_chip(pin);

	if (!chip) {
		rt_kprintf("Error: GPIO %d not found in any controller\n", pin);
		return -RT_EINVAL;
	}

	if (enabled == PIN_IRQ_ENABLE)
		return spacemit_gpio_irq_enable(chip, pin, 1);
	else if (enabled == PIN_IRQ_DISABLE)
		return spacemit_gpio_irq_enable(chip, pin, 0);

	return -RT_EINVAL;
}

static const struct rt_pin_ops spacemit_pin_ops = {
	.pin_mode = spacemit_pin_mode,
	.pin_write = spacemit_pin_write,
	.pin_read = spacemit_pin_read,
	.pin_attach_irq = spacemit_pin_attach_irq,
	.pin_detach_irq = spacemit_pin_detach_irq,
	.pin_irq_enable = spacemit_pin_irq_enable_ops,
};

int spacemit_gpio_init(void)
{
	int i, ret;
	struct spacemit_gpio_chip *chip;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	struct dtb_node *compatible_node;

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		if (!__compatible[i].compatible)
			continue;

		compatible_node = dtb_node_find_compatible_node(dtb_head_node,
			__compatible[i].compatible);

		if (compatible_node == RT_NULL)
			continue;

		if (!dtb_node_device_is_available(compatible_node))
			continue;

		/* Allocate gpio chip */
		chip = rt_calloc(1, sizeof(struct spacemit_gpio_chip));
		if (!chip) {
			rt_kprintf("%s:%d, alloc gpio chip failed\n", __func__, __LINE__);
			return -RT_ENOMEM;
		}

		chip->reg_base = (void *)dtb_node_get_addr_index(compatible_node, 0);
		if (chip->reg_base < 0) {
			rt_kprintf("get gpio base error\n");
			return -RT_ERROR;
		}

		/* Get and enable clocks from device tree */
		chip->clk = of_clk_get(compatible_node, 0);
		if (chip->clk && !IS_ERR(chip->clk)) {
			ret = clk_prepare_enable(chip->clk);
			if (ret) {
				rt_kprintf("%s: failed to enable clock\n", __func__);
				rt_free(chip);
				continue;
			}
		} else {
			chip->clk = RT_NULL;
		}

		/* Get and enable reset clock (this will deassert reset) */
		chip->rst_clk = of_clk_get(compatible_node, 1);
		if (chip->rst_clk && !IS_ERR(chip->rst_clk)) {
			ret = clk_prepare_enable(chip->rst_clk);  // deassert reset
			if (ret) {
				rt_kprintf("%s: failed to enable reset clock\n", __func__);
				if (chip->clk) {
					clk_disable_unprepare(chip->clk);
				}
				rt_free(chip);
				continue;
			}
		} else {
			chip->rst_clk = RT_NULL;
		}

		/* Set chip-specific data */
		chip->chip_data = __compatible[i].chip_data;

		dtb_node_read_u32(compatible_node, "banks", &chip->nbank);

		/* Get interrupt number from device tree */
		ret = dtb_node_read_u32_index(compatible_node, "interrupts", 1, (uint32_t *)&chip->irq);
		if (ret || !strcmp(__compatible[i].compatible, "spacemit,k3-gpio")) {
			rt_kprintf("%s: no interrupt specified, IRQ support disabled\n",
				   __func__);
			chip->irq = 0;
		}

		gpio_probe_dt(compatible_node, chip);

		/* Initialize the gpio chip */
		chip->chip.label               = "spacemit-gpio";
		chip->chip.request             = spacemit_gpio_request;
		chip->chip.free                = spacemit_gpio_free;
		chip->chip.direction_input     = spacemit_gpio_direction_input;
		chip->chip.direction_output    = spacemit_gpio_direction_output;
		chip->chip.get                 = spacemit_gpio_get;
		chip->chip.set                 = spacemit_gpio_set;
		chip->chip.to_irq              = spacemit_gpio_to_irq;
		chip->chip.of_node             = compatible_node;
		chip->chip.of_xlate            = spacemit_gpio_of_xlate;
		chip->chip.of_gpio_n_cells     = 2;
		chip->chip.base                = chip->chip_data->gpio_base;
		chip->chip.ngpio               = chip->ngpio;

		ret = gpiochip_add(&chip->chip);
		if (ret) {
			rt_kprintf("%s: failed to add gpio chip\n", __func__);
			/* Cleanup clocks */
			if (chip->rst_clk) {
				clk_disable_unprepare(chip->rst_clk);
			}
			if (chip->clk) {
				clk_disable_unprepare(chip->clk);
			}
			rt_free(chip->pin_irq_hdr_tab);
			rt_free(chip->banks);
			rt_free(chip);
			continue;
		}

		/* Setup interrupt if available, pass chip pointer as param */
		if (chip->irq > 0) {
			rt_hw_interrupt_install(chip->irq, spacemit_gpio_irq_handler,
						chip, "gpio-irq");
			rt_hw_interrupt_umask(chip->irq);
		}

		/* Add chip to global list */
		if (g_gpio_chip_count < MAX_GPIO_CHIPS) {
			g_gpio_chips[g_gpio_chip_count++] = chip;
			rt_kprintf("GPIO: registered chip %s (GPIO %d-%d)\n",
				   chip->chip.label, chip->chip.base, chip->chip.base + chip->ngpio - 1);
		} else {
			rt_kprintf("GPIO: warning - max chip count reached\n");
		}
	}

	/* Register single global PIN device after all chips are initialized */
	if (g_gpio_chip_count > 0) {
		ret = rt_device_pin_register("pin", &spacemit_pin_ops, RT_NULL);
		if (ret != RT_EOK)
			rt_kprintf("GPIO: failed to register PIN device (ret=%d)\n", ret);
		else
			rt_kprintf("GPIO: registered global PIN device for %d controllers\n", g_gpio_chip_count);
	}

	return 0;
}
//INIT_DEVICE_EXPORT(spacemit_gpio_init);

