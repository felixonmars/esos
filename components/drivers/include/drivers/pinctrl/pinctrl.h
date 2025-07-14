#ifndef __RT_PINCTRL_H__
#define __RT_PINCTRL_H__

struct pinctrl_dev;
struct pinctrl_map;
struct pinmux_ops;
struct pinconf_ops;
struct gpio_chip;

/**
 * struct pinctrl_pin_desc - boards/machines provide information on their
 * pins, pads or other muxable units in this struct
 * @number: unique pin number from the global pin number space
 * @name: a name for this pin
 */
struct pinctrl_pin_desc {
	rt_uint32_t number;
	const char *name;
};

/* Convenience macro to define a single named or anonymous pin descriptor */
#define PINCTRL_PIN(a, b) { .number = a, .name = b }
#define PINCTRL_PIN_ANON(a) { .number = a }

/**
 * struct pinctrl_gpio_range - each pin controller can provide subranges of
 * the GPIO number space to be handled by the controller
 * @node: list node for internal use
 * @name: a name for the chip in this range
 * @id: an ID number for the chip in this range
 * @base: base offset of the GPIO range
 * @pin_base: base pin number of the GPIO range if pins == NULL
 * @pins: enumeration of pins in GPIO range or NULL
 * @npins: number of pins in the GPIO range, including the base number
 * @gc: an optional pointer to a gpio_chip
 */
struct pinctrl_gpio_range {
	rt_list_t node;
        const char *name;
        rt_uint32_t id;
        rt_uint32_t base;
        rt_uint32_t pin_base;
        rt_uint32_t const *pins;
        rt_uint32_t npins;
        struct gpio_chip *gc;
};

/**
 * struct pinctrl_ops - global pin control operations, to be implemented by
 * pin controller drivers.
 * @get_groups_count: Returns the count of total number of groups registered.
 * @get_group_name: return the group name of the pin group
 * @get_group_pins: return an array of pins corresponding to a certain
 *      group selector @pins, and the size of the array in @num_pins
 * @pin_dbg_show: optional debugfs display hook that will provide per-device
 *      info for a certain pin in debugfs
 * @dt_node_to_map: parse a device tree "pin configuration node", and create
 *      mapping table entries for it. These are returned through the @map and
 *      @num_maps output parameters. This function is optional, and may be
 *      omitted for pinctrl drivers that do not support device tree.
 * @dt_free_map: free mapping table entries created via @dt_node_to_map. The
 *      top-level @map pointer must be freed, along with any dynamically
 *      allocated members of the mapping table entries themselves. This
 *      function is optional, and may be omitted for pinctrl drivers that do
 *      not support device tree.
 */
struct pinctrl_ops {
	int (*get_groups_count) (struct pinctrl_dev *pctldev);
	const char *(*get_group_name) (struct pinctrl_dev *pctldev,
			rt_uint32_t selector);
	int (*get_group_pins) (struct pinctrl_dev *pctldev,
			rt_uint32_t selector,
			const rt_uint32_t **pins,
			rt_uint32_t *num_pins);
	int (*dt_node_to_map) (struct pinctrl_dev *pctldev,
			struct dtb_node *np_config,
			struct pinctrl_map **map, rt_uint32_t *num_maps);
	void (*dt_free_map) (struct pinctrl_dev *pctldev,
			struct pinctrl_map *map, rt_uint32_t num_maps);
};

/**
 * struct pinctrl_desc - pin controller descriptor, register this to pin
 * control subsystem
 * @name: name for the pin controller
 * @pins: an array of pin descriptors describing all the pins handled by
 *      this pin controller
 * @npins: number of descriptors in the array, usually just ARRAY_SIZE()
 *      of the pins field above
 * @pctlops: pin control operation vtable, to support global concepts like
 *      grouping of pins, this is optional.
 * @pmxops: pinmux operations vtable, if you support pinmuxing in your driver
 * @confops: pin config operations vtable, if you support pin configuration in
 *      your driver
 * @owner: module providing the pin controller, used for refcounting
 */
struct pinctrl_desc {
	const char *name;
	struct pinctrl_pin_desc const *pins;
	rt_uint32_t npins;
	const struct pinctrl_ops *pctlops;
	const struct pinmux_ops *pmxops;
	const struct pinconf_ops *confops;
	/* struct module *owner; */
};

struct pinctrl_dev *pinctrl_register(struct pinctrl_desc *pctldesc,
		struct dtb_node *dev, void *driver_data);
const char *pinctrl_dev_get_name(struct pinctrl_dev *pctldev);
struct pinctrl *pinctrl_get(struct dtb_node *dev);
void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev);
extern struct pinctrl_dev *pinctrl_find_and_add_gpio_range(const char *devname, struct pinctrl_gpio_range *range);

#endif /* __RT_PINCTRL_H__ */
