#include <rtdef.h>
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <rtbitops.h>

#undef ENOSPC
#define ENOSPC		28	/* No space left on device */
#undef EBUSY
#define EBUSY		17
#undef EPROBE_DEFER
#define EPROBE_DEFER    517     /* Driver requests probe retry */

extern struct rt_spinlock gpio_lock;

static rt_list_t gpio_chips = RT_LIST_OBJECT_INIT(gpio_chips);

struct gpio_desc {
        struct gpio_chip        *chip;
        unsigned long           flags;
/* flag symbols are bit numbers */
#define FLAG_REQUESTED  0
#define FLAG_IS_OUT     1
#define FLAG_EXPORT     2       /* protected by sysfs_lock */
#define FLAG_SYSFS      3       /* exported via /sys/class/gpio/control */
#define FLAG_TRIG_FALL  4       /* trigger on falling edge */
#define FLAG_TRIG_RISE  5       /* trigger on rising edge */
#define FLAG_ACTIVE_LOW 6       /* sysfs value has active low */
#define FLAG_OPEN_DRAIN 7       /* Gpio is open drain type */
#define FLAG_OPEN_SOURCE 8      /* Gpio is open source type */

#define ID_SHIFT        16      /* add new flags before this one */

#define GPIO_FLAGS_MASK         ((1 << ID_SHIFT) - 1)
#define GPIO_TRIGGER_MASK       (BIT(FLAG_TRIG_FALL) | BIT(FLAG_TRIG_RISE))

        const char              *label;
};

static struct gpio_desc gpio_desc[ARCH_NR_GPIOS];

static inline void desc_set_label(struct gpio_desc *d, const char *label)
{
	d->label = label;
}

/*
 * Return the GPIO number of the passed descriptor relative to its chip
 */
static int gpio_chip_hwgpio(const struct gpio_desc *desc)
{
	return desc - &desc->chip->desc[0];
}

/**
 * Convert a GPIO descriptor to the integer namespace.
 * This should disappear in the future but is needed since we still
 * use GPIO numbers for error messages and sysfs nodes
 */
static int desc_to_gpio(const struct gpio_desc *desc)
{
	return desc->chip->base + gpio_chip_hwgpio(desc);
}

/* dynamic allocation of GPIOs, e.g. on a hotplugged device */
static int gpiochip_find_base(int ngpio)
{
	struct gpio_chip *chip;
	int base = ARCH_NR_GPIOS - ngpio;

	rt_list_for_each_entry_reverse(chip, &gpio_chips, list) {
		/* found a free space? */
		if (chip->base + chip->ngpio <= base)
			break;
		else
			/* nope, check the space right before the chip */
			base = chip->base - ngpio;
	}

	if (gpio_is_valid(base)) {
		rt_kprintf("%s: found new base at %d\n", __func__, base);
		return base;
	} else {
		rt_kprintf("%s: cannot find free range\n", __func__);
		return -ENOSPC;
	}
}

/*
 * Add a new chip to the global chips list, keeping the list of chips sorted
 * by base order.
 *
 * Return -EBUSY if the new chip overlaps with some other chip's integer
 * space.
 */
static int gpiochip_add_to_list(struct gpio_chip *chip)
{
	rt_list_t *pos = &gpio_chips;
	struct gpio_chip *_chip;
	int err = 0;

	/* find where to insert our chip */
	rt_list_for_each(pos, &gpio_chips) {
		_chip = rt_list_entry(pos, struct gpio_chip, list);
		/* shall we insert before _chip? */
		if (_chip->base >= chip->base + chip->ngpio)
			break;
	}

	/* are we stepping on the chip right before? */
	if (pos != &gpio_chips && pos->prev != &gpio_chips) {
		_chip = rt_list_entry(pos->prev, struct gpio_chip, list);
		if (_chip->base + _chip->ngpio > chip->base) {
			rt_kprintf("GPIO integer space overlap, cannot add chip\n");
			err = -EBUSY;
		}
	}

	if (!err)
		rt_list_insert_after(pos, &chip->list);

	return err;
}

/**
 * gpiochip_add_pin_range() - add a range for GPIO <-> pin mapping
 * @chip: the gpiochip to add the range for
 * @pinctrl_name: the dev_name() of the pin controller to map to
 * @gpio_offset: the start offset in the current gpio_chip number space
 * @pin_offset: the start offset in the pin controller number space
 * @npins: the number of pins from the offset of each pin space (GPIO and
 *      pin controller) to accumulate in this range
 */
int gpiochip_add_pin_range(struct gpio_chip *chip, const char *pinctl_name,
                           rt_uint32_t gpio_offset, rt_uint32_t pin_offset,
                           rt_uint32_t npins)
{
	struct gpio_pin_range *pin_range;
	int ret;

	pin_range = rt_calloc(1, sizeof(*pin_range));
	if (!pin_range) {
		rt_kprintf("%s: GPIO chip: failed to allocate pin ranges\n", chip->label);
		return -RT_ENOMEM;
        }

	/* Use local offset as range ID */
	pin_range->range.id = gpio_offset;
	pin_range->range.gc = chip;
	pin_range->range.name = chip->label;
	pin_range->range.base = chip->base + gpio_offset;
	pin_range->range.pin_base = pin_offset;
	pin_range->range.npins = npins;
	pin_range->pctldev = pinctrl_find_and_add_gpio_range(pinctl_name, &pin_range->range);
        if (IS_ERR(pin_range->pctldev)) {
		ret = PTR_ERR(pin_range->pctldev);
		rt_kprintf("%s: GPIO chip: could not create pin range\n", chip->label);
		rt_free(pin_range);
		return ret;
	}

 	rt_kprintf("GPIO chip %s: created GPIO range %d->%d ==> %s PIN %d->%d\n",
			chip->label, gpio_offset, gpio_offset + npins - 1,
			pinctl_name,
			pin_offset, pin_offset + npins - 1);

	rt_list_insert_after(&chip->pin_ranges, &pin_range->node);

	return 0;
}

/**
 * gpiochip_add() - register a gpio_chip
 * @chip: the chip to register, with chip->base initialized
 * Context: potentially before irqs or kmalloc will work
 *
 * Returns a negative errno if the chip can't be registered, such as
 * because the chip->base is invalid or already associated with a
 * different chip.  Otherwise it returns zero as a success code.
 *
 * When gpiochip_add() is called very early during boot, so that GPIOs
 * can be freely used, the chip->dev device must be registered before
 * the gpio framework's arch_initcall().  Otherwise sysfs initialization
 * for GPIOs will fail rudely.
 *
 * If chip->base is negative, this requests dynamic assignment of
 * a range of valid GPIOs.
 */
int gpiochip_add(struct gpio_chip *chip)
{
	unsigned long flags;
	int		status = 0;
	unsigned	id;
	int		base = chip->base;

	if ((!gpio_is_valid(base) || !gpio_is_valid(base + chip->ngpio - 1))
			&& base >= 0) {
		status = -RT_EINVAL;
		goto fail;
	}

	flags = rt_spin_lock_irqsave(&gpio_lock);

	if (base < 0) {
		base = gpiochip_find_base(chip->ngpio);
		if (base < 0) {
			status = base;
			goto unlock;
		}
		chip->base = base;
	}

	status = gpiochip_add_to_list(chip);

	if (status == 0) {
		chip->desc = &gpio_desc[chip->base];

		for (id = 0; id < chip->ngpio; id++) {
			struct gpio_desc *desc = &chip->desc[id];
			desc->chip = chip;

                        /* REVISIT:  most hardware initializes GPIOs as
                         * inputs (often with pullups enabled) so power
                         * usage is minimized.  Linux code should set the
                         * gpio direction first thing; but until it does,
                         * and in case chip->get_direction is not set,
                         * we may expose the wrong direction in sysfs.
                         */
			desc->flags = !chip->direction_input
				? (1 << FLAG_IS_OUT)
				: 0;
		}
	}

	rt_spin_unlock_irqrestore(&gpio_lock, flags);

	rt_list_init(&chip->pin_ranges);

 	of_gpiochip_add(chip);

	if (status)
		goto fail;
/**
 *	status = gpiochip_export(chip);
 *	if (status)
 *		goto fail;
 */
	rt_kprintf("gpiochip_add: registered GPIOs %d to %d on device: %s\n",
			chip->base, chip->base + chip->ngpio - 1,
			chip->label ? : "generic");

	return 0;

unlock:
	rt_spin_unlock_irqrestore(&gpio_lock, flags);
fail:
	/* failures here can mean systems won't boot... */
	rt_kprintf("gpiochip_add: gpios %d..%d (%s) failed to register\n",
			chip->base, chip->base + chip->ngpio - 1,
			chip->label ? : "generic");
	return status;
}

/**
 * Convert a GPIO number to its descriptor
 */
static struct gpio_desc *gpio_to_desc(unsigned gpio)
{
	if (!gpio_is_valid(gpio)) {
		rt_kprintf("invalid GPIO %d\n", gpio);
		return NULL;
	} else
		return &gpio_desc[gpio];
}

static struct gpio_chip *gpiod_to_chip(const struct gpio_desc *desc)
{
	return desc ? desc->chip : NULL;
}

/* caller holds gpio_lock *OR* gpio is marked as requested */
struct gpio_chip *gpio_to_chip(unsigned gpio)
{
	return gpiod_to_chip(gpio_to_desc(gpio));
}

/**
 * gpiochip_find() - iterator for locating a specific gpio_chip
 * @data: data to pass to match function
 * @callback: Callback function to check gpio_chip
 *
 * Similar to bus_find_device.  It returns a reference to a gpio_chip as
 * determined by a user supplied @match callback.  The callback should return
 * 0 if the device doesn't match and non-zero if it does.  If the callback is
 * non-zero, this function will return to the caller and not iterate over any
 * more gpio_chips.
 */
struct gpio_chip *gpiochip_find(void *data,
                                int (*match)(struct gpio_chip *chip,
                                             void *data))
{
	unsigned long flags;
	struct gpio_chip *chip;

	flags = rt_spin_lock_irqsave(&gpio_lock);
	
	rt_list_for_each_entry(chip, &gpio_chips, list)
		if (match(chip, data))
			break;

	/* No match? */
	if (&chip->list == &gpio_chips)
		chip = NULL;

	rt_spin_unlock_irqrestore(&gpio_lock, flags);

	return chip;
}

/* caller ensures gpio is valid and requested, chip->get_direction may sleep  */
static int gpiod_get_direction(const struct gpio_desc *desc)
{
	struct gpio_chip        *chip;
	unsigned                offset;
	int                     status = -RT_EINVAL;

	chip = gpiod_to_chip(desc);
	offset = gpio_chip_hwgpio(desc);

	if (!chip->get_direction)
		return status;

	status = chip->get_direction(chip, offset);
	if (status > 0) {
		/* GPIOF_DIR_IN, or other positive */
		status = 1;
		/* FLAG_IS_OUT is just a cache of the result of get_direction(),
		* so it does not affect constness per se */
		clear_bit(FLAG_IS_OUT, &((struct gpio_desc *)desc)->flags);
	}

	if (status == 0) {
		/* GPIOF_DIR_OUT */
		set_bit(FLAG_IS_OUT, &((struct gpio_desc *)desc)->flags);
	}

	return status;
}

/* These "optional" allocation calls help prevent drivers from stomping
 * on each other, and help provide better diagnostics in debugfs.
 * They're called even less than the "set direction" calls.
 */
static int gpiod_request(struct gpio_desc *desc, const char *label)
{
	unsigned long flags;
	struct gpio_chip        *chip;
	int status = -EPROBE_DEFER;

	if (!desc) {
		rt_kprintf("%s: invalid GPIO\n", __func__);
		return -RT_EINVAL;
	}
	
	flags = rt_spin_lock_irqsave(&gpio_lock);

	chip = desc->chip;
	if (chip == NULL)
		goto done;

#if 0
        if (!try_module_get(chip->owner))
                goto done;
#endif

	/* NOTE:  gpio_request() can be called in early boot,
	 * before IRQs are enabled, for non-sleeping (SOC) GPIOs.
	 */

	if (test_and_set_bit(FLAG_REQUESTED, &desc->flags) == 0) {
		desc_set_label(desc, label ? : "?");
		status = 0;
	} else {
		status = -EBUSY;
#if 0
                module_put(chip->owner);
#endif
		goto done;
	}

	if (chip->request) {
		/* chip->request may sleep */
		rt_spin_unlock_irqrestore(&gpio_lock, flags);
		status = chip->request(chip, gpio_chip_hwgpio(desc));
		flags = rt_spin_lock_irqsave(&gpio_lock);

		if (status < 0) {
			desc_set_label(desc, NULL);
#if 0
                        module_put(chip->owner);
#endif
			clear_bit(FLAG_REQUESTED, &desc->flags);
			goto done;
		}
	}

	if (chip->get_direction) {
		/* chip->get_direction may sleep */
		rt_spin_unlock_irqrestore(&gpio_lock, flags);
		gpiod_get_direction(desc);
		flags = rt_spin_lock_irqsave(&gpio_lock);
	}
done:
	if (status)
		rt_kprintf("_gpio_request: gpio-%d (%s) status %d\n", desc_to_gpio(desc), label ? : "?", status);

	rt_spin_unlock_irqrestore(&gpio_lock, flags);

	return status;
}

int gpio_request(unsigned gpio, const char *label)
{
	return gpiod_request(gpio_to_desc(gpio), label);
}

/* Warn when drivers omit gpio_request() calls -- legal but ill-advised
 * when setting direction, and otherwise illegal.  Until board setup code
 * and drivers use explicit requests everywhere (which won't happen when
 * those calls have no teeth) we can't avoid autorequesting.  This nag
 * message should motivate switching to explicit requests... so should
 * the weaker cleanup after faults, compared to gpio_request().
 *
 * NOTE: the autorequest mechanism is going away; at this point it's
 * only "legal" in the sense that (old) code using it won't break yet,
 * but instead only triggers a WARN() stack dump.
 */
static int gpio_ensure_requested(struct gpio_desc *desc)
{
	const struct gpio_chip *chip = desc->chip;
	const int gpio = desc_to_gpio(desc);

	if (test_and_set_bit(FLAG_REQUESTED, &desc->flags) == 0) {
		rt_kprintf("autorequest GPIO-%d\n", gpio);
#if 0
                if (!try_module_get(chip->owner)) {
                        pr_err("GPIO-%d: module can't be gotten \n", gpio);
                        clear_bit(FLAG_REQUESTED, &desc->flags);
                        /* lose */
                        return -EIO;
                }
#endif
		desc_set_label(desc, "[auto]");
		/* caller must chip->request() w/o spinlock */
		if (chip->request)
			return 1;
	}

	return 0;
}

/* Drivers MUST set GPIO direction before making get/set calls.  In
 * some cases this is done in early boot, before IRQs are enabled.
 *
 * As a rule these aren't called more than once (except for drivers
 * using the open-drain emulation idiom) so these are natural places
 * to accumulate extra debugging checks.  Note that we can't (yet)
 * rely on gpio_request() having been called beforehand.
 */

static int gpiod_direction_input(struct gpio_desc *desc)
{
	unsigned long flags;
	struct gpio_chip        *chip;
	int                     status = -RT_EINVAL;
	int                     offset;

	if (!desc) {
		rt_kprintf("%s: invalid GPIO\n", __func__);
		return -RT_EINVAL;
	}

	flags = rt_spin_lock_irqsave(&gpio_lock);

	chip = desc->chip;
	if (!chip || !chip->get || !chip->direction_input)
		goto fail;

	status = gpio_ensure_requested(desc);
	if (status < 0)
		goto fail;

	/* now we know the gpio is valid and chip won't vanish */
	rt_spin_unlock_irqrestore(&gpio_lock, flags);

        /* might_sleep_if(chip->can_sleep); */

	offset = gpio_chip_hwgpio(desc);
	if (status) {
		status = chip->request(chip, offset);
		if (status < 0) {
			rt_kprintf("GPIO-%d: chip request fail, %d\n", desc_to_gpio(desc), status);
			/* and it's not available to anyone else ...
			 * gpio_request() is the fully clean solution.
			 */
			goto lose;
		}
	}

	status = chip->direction_input(chip, offset);
	if (status == 0)
		clear_bit(FLAG_IS_OUT, &desc->flags);

lose:
	return status;
fail:
	rt_spin_unlock_irqrestore(&gpio_lock, flags);
	if (status)
		rt_kprintf("%s: gpio-%d status %d\n", __func__, desc_to_gpio(desc), status);

	return status;
}

int gpio_direction_input(unsigned gpio)
{
	return gpiod_direction_input(gpio_to_desc(gpio));
}

static int gpiod_direction_output(struct gpio_desc *desc, int value)
{
	unsigned long flags;
	struct gpio_chip        *chip;
	int                     status = -RT_EINVAL;
	int offset;

	if (!desc) {
		rt_kprintf("%s: invalid GPIO\n", __func__);
		return -RT_EINVAL;
	}

	/* Open drain pin should not be driven to 1 */
	if (value && test_bit(FLAG_OPEN_DRAIN,  &desc->flags))
		return gpiod_direction_input(desc);

	/* Open source pin should not be driven to 0 */
	if (!value && test_bit(FLAG_OPEN_SOURCE,  &desc->flags))
		return gpiod_direction_input(desc);

	flags = rt_spin_lock_irqsave(&gpio_lock);

	chip = desc->chip;
	if (!chip || !chip->set || !chip->direction_output)
		goto fail;

	status = gpio_ensure_requested(desc);
	if (status < 0)
		goto fail;

	/* now we know the gpio is valid and chip won't vanish */
	
	rt_spin_unlock_irqrestore(&gpio_lock, flags);

	/* might_sleep_if(chip->can_sleep); */

	offset = gpio_chip_hwgpio(desc);
	if (status) {
		status = chip->request(chip, offset);
		if (status < 0) {
			rt_kprintf("GPIO-%d: chip request fail, %d\n", desc_to_gpio(desc), status);
			/* and it's not available to anyone else ...
			 * gpio_request() is the fully clean solution.
			 */
			goto lose;
		}
	}

	status = chip->direction_output(chip, offset, value);
	if (status == 0)
		set_bit(FLAG_IS_OUT, &desc->flags);
lose:
	return status;
fail:
	rt_spin_unlock_irqrestore(&gpio_lock, flags);
        
	if (status)
		rt_kprintf("%s: gpio-%d status %d\n", __func__,
				desc_to_gpio(desc), status);
	return status;
}

int gpio_direction_output(unsigned gpio, int value)
{
	return gpiod_direction_output(gpio_to_desc(gpio), value);
}

/* I/O calls are only valid after configuration completed; the relevant
 * "is this a valid GPIO" error checks should already have been done.
 *
 * "Get" operations are often inlinable as reading a pin value register,
 * and masking the relevant bit in that register.
 *
 * When "set" operations are inlinable, they involve writing that mask to
 * one register to set a low value, or a different register to set it high.
 * Otherwise locking is needed, so there may be little value to inlining.
 *
 *------------------------------------------------------------------------
 *
 * IMPORTANT!!!  The hot paths -- get/set value -- assume that callers
 * have requested the GPIO.  That can include implicit requesting by
 * a direction setting call.  Marking a gpio as requested locks its chip
 * in memory, guaranteeing that these table lookups need no more locking
 * and that gpiochip_remove() will fail.
 *
 * REVISIT when debugging, consider adding some instrumentation to ensure
 * that the GPIO was actually requested.
 */

/**
 * __gpio_get_value() - return a gpio's value
 * @gpio: gpio whose value will be returned
 * Context: any
 *
 * This is used directly or indirectly to implement gpio_get_value().
 * It returns the zero or nonzero value provided by the associated
 * gpio_chip.get() method; or zero if no such method is provided.
 */
static int gpiod_get_value(const struct gpio_desc *desc)
{
	struct gpio_chip        *chip;
	int value;
	int offset;

	if (!desc)
		return 0;

	chip = desc->chip;
	offset = gpio_chip_hwgpio(desc);

	/* Should be using gpio_get_value_cansleep() */
        /* WARN_ON(chip->can_sleep); */

	value = chip->get ? chip->get(chip, offset) : 0;

	return value;
}

int __gpio_get_value(unsigned gpio)
{
	return gpiod_get_value(gpio_to_desc(gpio));
}

/*
 *  _gpio_set_open_drain_value() - Set the open drain gpio's value.
 * @gpio: Gpio whose state need to be set.
 * @chip: Gpio chip.
 * @value: Non-zero for setting it HIGH otherise it will set to LOW.
 */
static void _gpio_set_open_drain_value(struct gpio_desc *desc, int value)
{
	int err = 0;
	struct gpio_chip *chip = desc->chip;
	int offset = gpio_chip_hwgpio(desc);

	if (value) {
		err = chip->direction_input(chip, offset);
		if (!err)
			clear_bit(FLAG_IS_OUT, &desc->flags);
	} else {
		err = chip->direction_output(chip, offset, 0);
		if (!err)
			set_bit(FLAG_IS_OUT, &desc->flags);
	}

	if (err < 0)
                rt_kprintf("%s: Error in set_value for open drain gpio%d err %d\n",
                                        __func__, desc_to_gpio(desc), err);
}

/*
 *  _gpio_set_open_source() - Set the open source gpio's value.
 * @gpio: Gpio whose state need to be set.
 * @chip: Gpio chip.
 * @value: Non-zero for setting it HIGH otherise it will set to LOW.
 */
static void _gpio_set_open_source_value(struct gpio_desc *desc, int value)
{
	int err = 0;
	struct gpio_chip *chip = desc->chip;
	int offset = gpio_chip_hwgpio(desc);

	if (value) {
		err = chip->direction_output(chip, offset, 1);
		if (!err)
			set_bit(FLAG_IS_OUT, &desc->flags);
	} else {
		err = chip->direction_input(chip, offset);
		if (!err)
			clear_bit(FLAG_IS_OUT, &desc->flags);
	}

	if (err < 0)
		rt_kprintf("%s: Error in set_value for open source gpio%d err %d\n",
                                __func__, desc_to_gpio(desc), err);
}

/**
 * __gpio_set_value() - assign a gpio's value
 * @gpio: gpio whose value will be assigned
 * @value: value to assign
 * Context: any
 *
 * This is used directly or indirectly to implement gpio_set_value().
 * It invokes the associated gpio_chip.set() method.
 */
static void gpiod_set_value(struct gpio_desc *desc, int value)
{
	struct gpio_chip        *chip;

	if (!desc)
		return;

	chip = desc->chip;
	/* Should be using gpio_set_value_cansleep() */
        /* WARN_ON(chip->can_sleep); */

	if (test_bit(FLAG_OPEN_DRAIN, &desc->flags))
		_gpio_set_open_drain_value(desc, value);
	else if (test_bit(FLAG_OPEN_SOURCE, &desc->flags))
		_gpio_set_open_source_value(desc, value);
	else
		chip->set(chip, gpio_chip_hwgpio(desc), value);
}

void __gpio_set_value(unsigned gpio, int value)
{
	return gpiod_set_value(gpio_to_desc(gpio), value);
}

