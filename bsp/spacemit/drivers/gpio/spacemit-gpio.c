#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>

#define GPLR		0x0
#define GPDR		0xc
#define GPSR		0x18
#define GPCR		0x24
#define GRER		0x30
#define GFER		0x3c
#define GEDR		0x48
#define GSDR		0x54
#define GCDR		0x60
#define GSRER		0x6c
#define GCRER		0x78
#define GSFER		0x84
#define GCFER		0x90
#define GAPMASK		0x9c
#define GCPMASK		0xa8

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
};

static struct dtb_compatible_array __compatible[] = {
	{ .compatible = "spacemit,k1x-gpio" },
	{ .compatible = "spacemit,k2-gpio" },
	{ },
};

static int gpio_probe_dt(struct dtb_node *np, struct spacemit_gpio_chip *chip)
{
	struct dtb_node *child;
	rt_uint32_t offset;
	int i, ret;

        chip->banks = rt_calloc(chip->nbank, sizeof(struct spacemit_gpio_bank));
	if (chip->banks == RT_NULL)
		return -RT_ENOMEM;

	i = 0;
	dtb_node_for_each_subnode(child, np) {
		ret = dtb_node_read_u32(child, "reg-offset", &offset);
		if (ret) {
			dtb_node_put(child);
			return ret;
		}
		chip->banks[i].reg_bank = chip->reg_base + offset;
		i++;
	}

	chip->ngpio = chip->nbank * BANK_GPIO_NUMBER;

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
	struct spacemit_gpio_bank *bank =
		&spacemit_chip->banks[gpio_to_bank_idx(offset)];
	unsigned int bit = (1 << gpio_to_bank_offset(offset));

	writel(bit, bank->reg_bank + GCDR);

        return 0;
}

static int spacemit_gpio_direction_output(struct gpio_chip *chip,
		unsigned offset, int value)
{
        struct spacemit_gpio_chip *spacemit_chip =
                        rt_container_of(chip, struct spacemit_gpio_chip, chip);
        struct spacemit_gpio_bank *bank =
                        &spacemit_chip->banks[gpio_to_bank_idx(offset)];
        unsigned int bit = (1 << gpio_to_bank_offset(offset));

        /* Set value first. */
        writel(bit, bank->reg_bank + (value ? GPSR : GPCR));

        writel(bit, bank->reg_bank + GSDR);

        return 0;
}

static int spacemit_gpio_get(struct gpio_chip *chip, unsigned offset)
{
        struct spacemit_gpio_chip *spacemit_chip =
                        rt_container_of(chip, struct spacemit_gpio_chip, chip);
        struct spacemit_gpio_bank *bank =
                        &spacemit_chip->banks[gpio_to_bank_idx(offset)];
        unsigned int bit = (1 << gpio_to_bank_offset(offset));
        unsigned int gplr;

        gplr  = readl(bank->reg_bank + GPLR);

        return !!(gplr & bit);
}

static void spacemit_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
        struct spacemit_gpio_chip *spacemit_chip =
                        rt_container_of(chip, struct spacemit_gpio_chip, chip);
        struct spacemit_gpio_bank *bank =
                        &spacemit_chip->banks[gpio_to_bank_idx(offset)];
        unsigned int bit = (1 << gpio_to_bank_offset(offset));
        unsigned int gpdr;

        gpdr = readl(bank->reg_bank + GPDR);
        /* Is it configured as output? */
        if (gpdr & bit)
                writel(bit, bank->reg_bank + (value ? GPSR : GPCR));
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
			
			compatible_node = dtb_node_find_compatible_node(dtb_head_node, __compatible[i].compatible);

			if (compatible_node != RT_NULL) {
				
				if (!dtb_node_device_is_available(compatible_node))
					continue;

				/* do someting */
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

				dtb_node_read_u32(compatible_node, "banks", &chip->nbank);

				gpio_probe_dt(compatible_node, chip);

				/* Initialize the gpio chip */
				chip->chip.label = "spacemit-gpio";
				chip->chip.request = spacemit_gpio_request;
				chip->chip.free = spacemit_gpio_free;
				chip->chip.direction_input  = spacemit_gpio_direction_input;
				chip->chip.direction_output = spacemit_gpio_direction_output;
				chip->chip.get = spacemit_gpio_get;
				chip->chip.set = spacemit_gpio_set;
				/* chip->chip.to_irq = k1x_gpio_to_irq; */
				chip->chip.of_node = compatible_node;
				chip->chip.of_xlate = spacemit_gpio_of_xlate;
				chip->chip.of_gpio_n_cells = 2;
				chip->chip.ngpio = chip->ngpio;

				gpiochip_add(&chip->chip);
			}
		}
	}

	return 0;
}
// INIT_DEVICE_EXPORT(spacemit_gpio_init);

