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

/* K3 R-GPIO chip data (small core) - 32 GPIOs, 1 bank */
static const struct spacemit_gpio_chip_data k3_r_gpio_chip_data = {
	.regs         = &k3_regs,
	.bank_offsets = {0x0, 0x0, 0x0, 0x0},  /* Only bank 0 is used */
	.gpio_base    = 128,       /* GPIO 128-159 (R-GPIO 0-31) */
	.total_gpios  = 32,        /* 1 bank * 32 GPIOs */
};

/* K3 R-GPIO1 chip data (small core) - 4 GPIOs, 1 bank */
static const struct spacemit_gpio_chip_data k3_r_gpio1_chip_data = {
	.regs         = &k3_regs,
	.bank_offsets = {0x0, 0x0, 0x0, 0x0},  /* Only bank 0 is used */
	.gpio_base    = 160,       /* GPIO 160-163 (R-GPIO1 0-3) */
	.total_gpios  = 4,         /* Only 4 GPIOs */
};

#define BANK_GPIO_NUMBER	(32)
#define BANK_GPIO_MASK		(BANK_GPIO_NUMBER - 1)

#define gpio_to_bank_idx(gpio)	((gpio) / BANK_GPIO_NUMBER)
#define gpio_to_bank_offset(gpio)	((gpio) & BANK_GPIO_MASK)
#define bank_to_gpio(idx, offset)	(((idx) * BANK_GPIO_NUMBER) \
		| ((offset) & BANK_GPIO_MASK))

struct spacemit_gpio_bank {
	void *reg_bank;
/**
 *	unsigned int irq_mask;
 *	unsigned int irq_rising_edge;
 *	unsigned int irq_falling_edge;
 */
};

struct spacemit_gpio_chip {
	struct gpio_chip chip;
	void *reg_base;
/**
 *	int irq;
 *	struct irq_domain *domain;
 */
	uint32_t ngpio;
	uint32_t nbank;
	struct spacemit_gpio_bank *banks;
	const struct spacemit_gpio_chip_data *chip_data;
};

struct spacemit_gpio_compatible {
	const char *compatible;
	const struct spacemit_gpio_chip_data *chip_data;
};

static struct spacemit_gpio_compatible __compatible[] = {
	{ .compatible = "spacemit,k1x-gpio", .chip_data = &k1_chip_data },
	{ .compatible = "spacemit,k3-gpio", .chip_data = &k3_ap_gpio_chip_data },
	{ .compatible = "spacemit,k3-rgpio0", .chip_data = &k3_r_gpio_chip_data },
	{ .compatible = "spacemit,k3-rgpio1", .chip_data = &k3_r_gpio1_chip_data },
	{ },
};

static int gpio_probe_dt(struct dtb_node *np, struct spacemit_gpio_chip *chip)
{
	int i;

	chip->banks = rt_calloc(chip->nbank, sizeof(struct spacemit_gpio_bank));
	if (chip->banks == RT_NULL)
		return -RT_ENOMEM;

	/* Use chip-specific bank offsets */
	for (i = 0; i < chip->nbank; i++) {
		chip->banks[i].reg_bank = chip->reg_base + chip->chip_data->bank_offsets[i];
	}

	chip->ngpio = chip->chip_data->total_gpios;

	return 0;
}

static int spacemit_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return pinctrl_request_gpio(chip->base + offset);
}

static void spacemit_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	return pinctrl_free_gpio(chip->base + offset);
}

static int spacemit_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct spacemit_gpio_chip *spacemit_chip =
		rt_container_of(chip, struct spacemit_gpio_chip, chip);
	const struct spacemit_gpio_reg_offsets *regs = spacemit_chip->chip_data->regs;
	struct spacemit_gpio_bank *bank = &spacemit_chip->banks[gpio_to_bank_idx(offset)];
	unsigned int bit = (1 << gpio_to_bank_offset(offset));

	writel(bit, bank->reg_bank + regs->gcdr);

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

	/* Set value first. */
	writel(bit, bank->reg_bank + (value ? regs->gpsr : regs->gpcr));

	writel(bit, bank->reg_bank + regs->gsdr);

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

	gplr = readl(bank->reg_bank + regs->gplr);

	return !!(gplr & bit);
}

static void spacemit_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct spacemit_gpio_chip *spacemit_chip =
		rt_container_of(chip, struct spacemit_gpio_chip, chip);
	const struct spacemit_gpio_reg_offsets *regs = spacemit_chip->chip_data->regs;
	struct spacemit_gpio_bank *bank = &spacemit_chip->banks[gpio_to_bank_idx(offset)];
	unsigned int bit = (1 << gpio_to_bank_offset(offset));
	unsigned int gpdr;

	gpdr = readl(bank->reg_bank + regs->gpdr);
	/* Is it configured as output? */
	if (gpdr & bit)
		writel(bit, bank->reg_bank + (value ? regs->gpsr : regs->gpcr));
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

int spacemit_gpio_init(void)
{
	int i;
	struct spacemit_gpio_chip *chip;
	struct dtb_node *dtb_head_node = get_dtb_node_head();
	struct dtb_node *compatible_node;

	for (i = 0; i < sizeof(__compatible) / sizeof(__compatible[0]); ++i) {
		if (__compatible[i].compatible) {
			compatible_node = dtb_node_find_compatible_node(dtb_head_node,
				__compatible[i].compatible);

			if (compatible_node != RT_NULL) {
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

				/* Set chip-specific data */
				chip->chip_data = __compatible[i].chip_data;

				dtb_node_read_u32(compatible_node, "banks", &chip->nbank);

				gpio_probe_dt(compatible_node, chip);

				/* Initialize the gpio chip */
				chip->chip.label               = "spacemit-gpio";
				chip->chip.request             = spacemit_gpio_request;
				chip->chip.free                = spacemit_gpio_free;
				chip->chip.direction_input     = spacemit_gpio_direction_input;
				chip->chip.direction_output    = spacemit_gpio_direction_output;
				chip->chip.get                 = spacemit_gpio_get;
				chip->chip.set                 = spacemit_gpio_set;
				/* chip->chip.to_irq = k1x_gpio_to_irq; */
				chip->chip.of_node             = compatible_node;
				chip->chip.of_xlate            = spacemit_gpio_of_xlate;
				chip->chip.of_gpio_n_cells     = 2;
				chip->chip.base                = chip->chip_data->gpio_base;
				chip->chip.ngpio               = chip->ngpio;

				gpiochip_add(&chip->chip);
			}
		}
	}

	return 0;
}
// INIT_DEVICE_EXPORT(spacemit_gpio_init);