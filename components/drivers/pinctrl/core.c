#include "pinctrl-core.h"
#include "pinmux.h"
#include "pinconf.h"
#include "devicetree.h"

#define EPROBE_DEFER    517     /* Driver requests probe retry */
#define ENODEV          19      /* No such device */

static bool pinctrl_dummy_state;

extern struct rt_mutex pinctrldev_list_mutex;
extern struct rt_mutex pinctrl_list_mutex;
extern struct rt_mutex pinctrl_maps_mutex;
static rt_list_t pinctrldev_list = RT_LIST_OBJECT_INIT(pinctrldev_list);
static rt_list_t pinctrl_list = RT_LIST_OBJECT_INIT(pinctrl_list);
static rt_list_t pinctrl_maps = RT_LIST_OBJECT_INIT(pinctrl_maps);

/* Simplified asprintf. */
char *rt_kvasprintf(const char *fmt, va_list ap)
{
	unsigned int len;
	char *p;
	va_list aq;

	va_copy(aq, ap);
	len = rt_vsnprintf(NULL, 0, fmt, aq);
	va_end(aq);
	
	p = rt_malloc(len+1);
	if (!p)
		return RT_NULL;

	rt_vsnprintf(p, len+1, fmt, ap);

	return p;
}

char *rt_kasprintf(const char *fmt, ...)
{
	va_list ap;
	char *p;

	va_start(ap, fmt);
	p = rt_kvasprintf(fmt, ap);
	va_end(ap);

	return p;
}

/**
 * kmemdup - duplicate region of memory
 *
 * @src: memory region to duplicate
 * @len: memory region length
 * @gfp: GFP mask to use
 */
void *kmemdup(const void *src, size_t len)
{
	void *p;
	
	p = rt_malloc(len);
	if (p)
		rt_memcpy(p, src, len);

	return p;
}

/**
 * pinctrl_provide_dummies() - indicate if pinctrl provides dummy state support
 *
 * Usually this function is called by platforms without pinctrl driver support
 * but run with some shared drivers using pinctrl APIs.
 * After calling this function, the pinctrl core will return successfully
 * with creating a dummy state for the driver to keep going smoothly.
 */
void pinctrl_provide_dummies(void)
{
	pinctrl_dummy_state = true;
}

static struct pinctrl *find_pinctrl(struct dtb_node *dev)
{
	struct pinctrl *p;

	rt_mutex_take(&pinctrl_list_mutex, RT_WAITING_FOREVER);
	rt_list_for_each_entry(p, &pinctrl_list, node)
		if (p->dev == dev) {
			rt_mutex_release(&pinctrl_list_mutex);
			return p;
		}

	rt_mutex_release(&pinctrl_list_mutex);
	
	return NULL;
}

static struct pinctrl_state *find_state(struct pinctrl *p,
                                        const char *name)
{
	struct pinctrl_state *state;

	rt_list_for_each_entry(state, &p->states, node)
		if (!rt_strcmp(state->name, name))
			return state;

	return NULL;
}

static struct pinctrl_state *create_state(struct pinctrl *p,
                                          const char *name)
{
	struct pinctrl_state *state;

	state = rt_calloc(1, sizeof(*state));
	if (state == NULL) {
		rt_kprintf("failed to alloc struct pinctrl_state\n");
		return ERR_PTR(-RT_ENOMEM);
	}

	state->name = name;
	rt_list_init(&state->settings);

	rt_list_insert_after(&p->states, &state->node);

	return state;
}

/**
 * get_pinctrl_dev_from_devname() - look up pin controller device
 * @devname: the name of a device instance, as returned by dev_name()
 *
 * Looks up a pin control device matching a certain device name or pure device
 * pointer, the pure device pointer will take precedence.
 */
struct pinctrl_dev *get_pinctrl_dev_from_devname(const char *devname)
{
	struct pinctrl_dev *pctldev = NULL;

	if (!devname)
		return NULL;

	rt_mutex_take(&pinctrldev_list_mutex, RT_WAITING_FOREVER);

	rt_list_for_each_entry(pctldev, &pinctrldev_list, node) {
		if (!rt_strcmp(pctldev->dev->name, devname)) {
			/* Matched on device name */
			rt_mutex_release(&pinctrldev_list_mutex);
			return pctldev;
		}
	}

	rt_mutex_release(&pinctrldev_list_mutex);

	return NULL;
}

static int add_setting(struct pinctrl *p, struct pinctrl_map const *map)
{
	struct pinctrl_state *state;
	struct pinctrl_setting *setting;
	int ret;

	state = find_state(p, map->name);
	if (!state)
		state = create_state(p, map->name);
	if (IS_ERR(state))
		return PTR_ERR(state);

	if (map->type == PIN_MAP_TYPE_DUMMY_STATE)
		return 0;

        setting = rt_calloc(1, sizeof(*setting));
	if (setting == NULL) {
		rt_kprintf("failed to alloc struct pinctrl_setting\n");
		return -RT_ENOMEM;
	}

	setting->type = map->type;

	setting->pctldev = get_pinctrl_dev_from_devname(map->ctrl_dev_name);
	if (setting->pctldev == NULL) {
		rt_free(setting);
		/* Do not defer probing of hogs (circular loop) */
		if (!strcmp(map->ctrl_dev_name, map->dev_name))
			return -ENODEV;
		/*
		 * OK let us guess that the driver is not there yet, and
		 * let's defer obtaining this pinctrl handle to later...
		 */
                rt_kprintf("unknown pinctrl device %s in map entry, deferring probe", map->ctrl_dev_name);
		return -EPROBE_DEFER;
	}

	setting->dev_name = map->dev_name;

	switch (map->type) {
	case PIN_MAP_TYPE_MUX_GROUP:
		ret = pinmux_map_to_setting(map, setting);
		break;
	case PIN_MAP_TYPE_CONFIGS_PIN:
	case PIN_MAP_TYPE_CONFIGS_GROUP:
		ret = pinconf_map_to_setting(map, setting);
		break;
	default:
		ret = -RT_EINVAL;
		break;
	}
	if (ret < 0) {
		rt_free(setting);
		return ret;
	}

	rt_list_insert_after(&state->settings, &setting->node);

	return 0;
}

struct pinctrl_dev *get_pinctrl_dev_from_of_node(struct dtb_node *np)
{
	struct pinctrl_dev *pctldev;
	
	rt_mutex_take(&pinctrldev_list_mutex, RT_WAITING_FOREVER);

	rt_list_for_each_entry(pctldev, &pinctrldev_list, node)
		if (pctldev->dev == np) {
			rt_mutex_release(&pinctrldev_list_mutex);
			return pctldev;
	}

	rt_mutex_release(&pinctrldev_list_mutex);

	return NULL;
}

void pinctrl_unregister_map(struct pinctrl_map const *map)
{
	struct pinctrl_maps *maps_node;

	rt_mutex_take(&pinctrl_maps_mutex, RT_WAITING_FOREVER);
	rt_list_for_each_entry(maps_node, &pinctrl_maps, node) {
		if (maps_node->maps == map) {
			rt_list_remove(&maps_node->node);
			rt_free(maps_node);
			rt_mutex_release(&pinctrl_maps_mutex);
			return;
		}
	}
	rt_mutex_release(&pinctrl_maps_mutex);
}

int pinctrl_register_map(struct pinctrl_map const *maps, unsigned num_maps,
                         bool dup, bool locked)
{
	int i, ret;
	struct pinctrl_maps *maps_node;

	/* rt_kprintf("add %d pinmux maps\n", num_maps); */

	/* First sanity check the new mapping */
	for (i = 0; i < num_maps; i++) {
		if (!maps[i].dev_name) {
			rt_kprintf("failed to register map %s (%d): no device given\n", maps[i].name, i);
                        return -RT_EINVAL;
		}

		if (!maps[i].name) {
			rt_kprintf("failed to register map %d: no map name given\n", i);
			return -RT_EINVAL;
		}

		if (maps[i].type != PIN_MAP_TYPE_DUMMY_STATE &&
				!maps[i].ctrl_dev_name) {
			rt_kprintf("failed to register map %s (%d): no pin control device given\n",
					maps[i].name, i);
			return -RT_EINVAL;
		}

		switch (maps[i].type) {
		case PIN_MAP_TYPE_DUMMY_STATE:
			break;
		case PIN_MAP_TYPE_MUX_GROUP:
			ret = pinmux_validate_map(&maps[i], i);
			if (ret < 0)
				return ret;
			break;
		case PIN_MAP_TYPE_CONFIGS_PIN:
		case PIN_MAP_TYPE_CONFIGS_GROUP:
			ret = pinconf_validate_map(&maps[i], i);
			if (ret < 0)
				return ret;
			break;
		default:
			rt_kprintf("failed to register map %s (%d): invalid type given\n", maps[i].name, i);
			return -RT_EINVAL;
		}
	}

	maps_node = rt_calloc(1, sizeof(*maps_node));
	if (!maps_node) {
		rt_kprintf("failed to alloc struct pinctrl_maps\n");
		return -RT_ENOMEM;
	}

	maps_node->num_maps = num_maps;
	if (dup) {
		maps_node->maps = kmemdup(maps, sizeof(*maps) * num_maps);
		if (!maps_node->maps) {
			rt_kprintf("failed to duplicate mapping table\n");
			rt_free(maps_node);
			return -RT_ENOMEM;
		}
	} else {
		maps_node->maps = maps;
	}

	if (!locked)
		rt_mutex_take(&pinctrl_maps_mutex, RT_WAITING_FOREVER);
	rt_list_insert_after(&pinctrl_maps, &maps_node->node);
	if (!locked)
		rt_mutex_release(&pinctrl_maps_mutex);

	return 0;
}

static void pinctrl_free_setting(bool disable_setting,
                                 struct pinctrl_setting *setting)
{
	switch (setting->type) {
	case PIN_MAP_TYPE_MUX_GROUP:
		if (disable_setting)
			pinmux_disable_setting(setting);
		pinmux_free_setting(setting);
		break;
	case PIN_MAP_TYPE_CONFIGS_PIN:
	case PIN_MAP_TYPE_CONFIGS_GROUP:
		pinconf_free_setting(setting);
		break;
	default:
		break;
	}
}

static void pinctrl_free(struct pinctrl *p, bool inlist)
{
	struct pinctrl_state *state, *n1;
	struct pinctrl_setting *setting, *n2;

	rt_mutex_take(&pinctrl_list_mutex, RT_WAITING_FOREVER);
	rt_list_for_each_entry_safe(state, n1, &p->states, node) {
		rt_list_for_each_entry_safe(setting, n2, &state->settings, node) {
			pinctrl_free_setting(state == p->state, setting);
			rt_list_remove(&setting->node);
			rt_free(setting);
		}
		rt_list_remove(&state->node);
		rt_free(state);
	}

	pinctrl_dt_free_maps(p);

	if (inlist)
		rt_list_remove(&p->node);
	rt_free(p);
	rt_mutex_release(&pinctrl_list_mutex);
}

static struct pinctrl *create_pinctrl(struct dtb_node *dev)
{
	struct pinctrl *p;
	const char *devname;
	struct pinctrl_maps *maps_node;
	int i;
	struct pinctrl_map const *map;
	int ret;

	/*
	 * create the state cookie holder struct pinctrl for each
	 * mapping, this is what consumers will get when requesting
	 * a pin control handle with pinctrl_get()
	 */
	p = rt_calloc(1, sizeof(*p));
	if (p == NULL) {
		rt_kprintf("failed to alloc struct pinctrl\n");
		return ERR_PTR(-RT_ENOMEM);
	}

	p->dev = dev;
	rt_list_init(&p->states);
	rt_list_init(&p->dt_maps);

	ret = pinctrl_dt_to_map(p);
	if (ret < 0) {
		rt_free(p);
		return ERR_PTR(ret);
	}

	devname = dev->name;

	rt_mutex_take(&pinctrl_maps_mutex, RT_WAITING_FOREVER);
	/* Iterate over the pin control maps to locate the right ones */
	for_each_maps(maps_node, i, map) {
		/* Map must be for this device */
		if (rt_strcmp(map->dev_name, devname))
			continue;

		ret = add_setting(p, map);
		/*
		 * At this point the adding of a setting may:
		 *
		 * - Defer, if the pinctrl device is not yet available
		 * - Fail, if the pinctrl device is not yet available,
		 *   AND the setting is a hog. We cannot defer that, since
		 *   the hog will kick in immediately after the device
		 *   is registered.
		 *
		 * If the error returned was not -EPROBE_DEFER then we
		 * accumulate the errors to see if we end up with
		 * an -EPROBE_DEFER later, as that is the worst case.
		 */
		if (ret == -EPROBE_DEFER) {
			pinctrl_free(p, false);
			rt_mutex_release(&pinctrl_maps_mutex);
			return ERR_PTR(ret);
		}
	}

	rt_mutex_release(&pinctrl_maps_mutex);

	if (ret < 0) {
		/* If some other error than deferral occured, return here */
		pinctrl_free(p, false);
		return ERR_PTR(ret);
	}

	/* Add the pinctrl handle to the global list */
	rt_list_insert_after(&pinctrl_list, &p->node);

	return p;
}

/**
 * pinctrl_get() - retrieves the pinctrl handle for a device
 * @dev: the device to obtain the handle for
 */
struct pinctrl *pinctrl_get(struct dtb_node *dev)
{
	struct pinctrl *p;

	if (!dev)
		return ERR_PTR(-RT_EINVAL);

	/*
	 * See if somebody else (such as the device core) has already
	 * obtained a handle to the pinctrl for this device. In that case,
	 * return another pointer to it.
	 */
	p = find_pinctrl(dev);
	if (p != NULL) {
		rt_kprintf("obtain a copy of previously claimed pinctrl\n");
		return p;
	}

	return create_pinctrl(dev);
}

/* Deletes a range of pin descriptors */
static void pinctrl_free_pindescs(struct pinctrl_dev *pctldev,
                                  const struct pinctrl_pin_desc *pins,
                                  unsigned num_pins)
{
	int i;

	for (i = 0; i < num_pins; i++) {
		struct pin_desc *pindesc;
		
		pindesc = radix_tree_lookup(&pctldev->pin_desc_tree,
				pins[i].number);
		if (pindesc != NULL) {
			radix_tree_delete(&pctldev->pin_desc_tree,
					pins[i].number);
			if (pindesc->dynamic_name)
				rt_free((void *)pindesc->name);
		}
		rt_free(pindesc);
	}
}

static int pinctrl_register_one_pin(struct pinctrl_dev *pctldev,
                                    unsigned number, const char *name)
{
	struct pin_desc *pindesc;

	pindesc = pin_desc_get(pctldev, number);
	if (pindesc != RT_NULL) {
		rt_kprintf("pin %d already registered on %s\n", number,
				pctldev->desc->name);
		return -RT_EINVAL;
	}

	pindesc = rt_calloc(1, sizeof(struct pin_desc));
	if (pindesc == RT_NULL) {
		rt_kprintf("failed to alloc struct pin_desc\n");
		return -RT_ENOMEM;
	}

	/* Set owner */
	pindesc->pctldev = pctldev;

	/* Copy basic pin info */
	if (name) {
		pindesc->name = name;
	} else {
		pindesc->name = rt_kasprintf("PIN%u", number);
		if (pindesc->name == RT_NULL) {
			rt_free(pindesc);
			return -RT_ENOMEM;
		}
		pindesc->dynamic_name = true;
	}

	radix_tree_insert(&pctldev->pin_desc_tree, number, pindesc);
	/* rt_kprintf("registered pin %d (%s) on %s\n",
			number, pindesc->name, pctldev->desc->name); */
	
	return 0;
}

static int pinctrl_register_pins(struct pinctrl_dev *pctldev,
                                 struct pinctrl_pin_desc const *pins,
                                 unsigned num_descs)
{
	unsigned i;
	int ret = 0;

	for (i = 0; i < num_descs; i++) {
		ret = pinctrl_register_one_pin(pctldev, pins[i].number, pins[i].name);
		if (ret)
			return ret;
	}

	return 0;
}

static int pinctrl_check_ops(struct pinctrl_dev *pctldev)
{
	const struct pinctrl_ops *ops = pctldev->desc->pctlops;

	if (!ops ||
		!ops->get_groups_count ||
		!ops->get_group_name ||
		!ops->get_group_pins)
		return -RT_EINVAL;

	if (ops->dt_node_to_map && !ops->dt_free_map)
		return -RT_EINVAL;

	return 0;
}

/**
 * pinctrl_lookup_state() - retrieves a state handle from a pinctrl handle
 * @p: the pinctrl handle to retrieve the state from
 * @name: the state name to retrieve
 */
struct pinctrl_state *pinctrl_lookup_state(struct pinctrl *p,
                                                 const char *name)
{
	struct pinctrl_state *state;

	state = find_state(p, name);
	if (!state) {
		if (pinctrl_dummy_state) {
			/* create dummy state */
			rt_kprintf("using pinctrl dummy state (%s)\n", name);
			state = create_state(p, name);
		} else
			state = ERR_PTR(-ENODEV);
	}

	return state;
}

/**
 * pinctrl_select_state() - select/activate/program a pinctrl state to HW
 * @p: the pinctrl handle for the device that requests configuration
 * @state: the state handle to select/activate/program
 */
int pinctrl_select_state(struct pinctrl *p, struct pinctrl_state *state)
{
	struct pinctrl_setting *setting, *setting2;
	struct pinctrl_state *old_state = p->state;
	int ret;

	if (p->state == state)
		return 0;

	if (p->state) {
		/*
		 * The set of groups with a mux configuration in the old state
		 * may not be identical to the set of groups with a mux setting
		 * in the new state. While this might be unusual, it's entirely
		 * possible for the "user"-supplied mapping table to be written
	 	 * that way. For each group that was configured in the old state
                 * but not in the new state, this code puts that group into a
		 * safe/disabled state.
		 */
		rt_list_for_each_entry(setting, &p->state->settings, node) {
			bool found = false;
			if (setting->type != PIN_MAP_TYPE_MUX_GROUP)
				continue;
			rt_list_for_each_entry(setting2, &state->settings, node) {
				if (setting2->type != PIN_MAP_TYPE_MUX_GROUP)
					continue;
				if (setting2->data.mux.group == setting->data.mux.group) {
					found = true;
					break;
				}
			}
			if (!found)
				pinmux_disable_setting(setting);
		}
	}

	p->state = NULL;

	/* Apply all the settings for the new state */
	rt_list_for_each_entry(setting, &state->settings, node) {
		switch (setting->type) {
		case PIN_MAP_TYPE_MUX_GROUP:
			ret = pinmux_enable_setting(setting);
			break;
		case PIN_MAP_TYPE_CONFIGS_PIN:
		case PIN_MAP_TYPE_CONFIGS_GROUP:
			ret = pinconf_apply_setting(setting);
			break;
		default:
			ret = -RT_EINVAL;
			break;
		}

		if (ret < 0) {
			goto unapply_new_state;
		}
	}

	p->state = state;

	return 0;

unapply_new_state:
	rt_kprintf("Error applying setting, reverse things back\n");

	rt_list_for_each_entry(setting2, &state->settings, node) {
		if (&setting2->node == &setting->node)
			break;
		/*
		 * All we can do here is pinmux_disable_setting.
		 * That means that some pins are muxed differently now
		 * than they were before applying the setting (We can't
		 * "unmux a pin"!), but it's not a big deal since the pins
		 * are free to be muxed by another apply_setting.
		 */
		if (setting2->type == PIN_MAP_TYPE_MUX_GROUP)
			pinmux_disable_setting(setting2);
	}

	/* There's no infinite recursive loop here because p->state is NULL */
	if (old_state)
		pinctrl_select_state(p, old_state);

	return ret;
}

/**
 * pinctrl_register() - register a pin controller device
 * @pctldesc: descriptor for this pin controller
 * @dev: parent device for this pin controller
 * @driver_data: private pin controller data for this pin controller
 */
struct pinctrl_dev *pinctrl_register(struct pinctrl_desc *pctldesc,
		struct dtb_node *dev, void *driver_data)
{
	struct pinctrl_dev *pctldev;
	int ret;

 	if (!pctldesc)
		return RT_NULL;
	if (!pctldesc->name)
		return RT_NULL;

	pctldev = rt_calloc(1, sizeof(*pctldev));
	if (pctldev == RT_NULL) {
		rt_kprintf("failed to alloc struct pinctrl_dev\n");
		return RT_NULL;
	}

	/* Initialize pin control device struct */
	/* pctldev->owner = pctldesc->owner; */
	pctldev->desc = pctldesc;
	pctldev->driver_data = driver_data;
	INIT_RADIX_TREE(&pctldev->pin_desc_tree, RT_NULL);
	rt_list_init(&pctldev->gpio_ranges);
	pctldev->dev = dev;
	rt_mutex_init(&pctldev->mutex, "pin_mutex", RT_IPC_FLAG_PRIO);

	/* check core ops for sanity */
	if (pinctrl_check_ops(pctldev)) {
		rt_kprintf("pinctrl ops lacks necessary functions\n");
		goto out_err;
	}

	/* If we're implementing pinmuxing, check the ops for sanity */
	if (pctldesc->pmxops) {
		if (pinmux_check_ops(pctldev))
			goto out_err;
	}

	/* If we're implementing pinconfig, check the ops for sanity */
	if (pctldesc->confops) {
		if (pinconf_check_ops(pctldev))
			goto out_err;
	}

	/* Register all the pins */
	/* rt_kprintf("try to register %d pins ...\n",  pctldesc->npins); */
	ret = pinctrl_register_pins(pctldev, pctldesc->pins, pctldesc->npins);
	if (ret) {
		rt_kprintf("error during pin registration\n");
		pinctrl_free_pindescs(pctldev, pctldesc->pins, pctldesc->npins);
		goto out_err;
	}

	rt_mutex_take(&pinctrldev_list_mutex, RT_WAITING_FOREVER);
	rt_list_insert_after(&pinctrldev_list, &pctldev->node);
	rt_mutex_release(&pinctrldev_list_mutex);

	pctldev->p = pinctrl_get(pctldev->dev);

	if (!IS_ERR(pctldev->p)) {
		pctldev->hog_default =
			pinctrl_lookup_state(pctldev->p, PINCTRL_STATE_DEFAULT);
		if (IS_ERR(pctldev->hog_default)) {
			rt_kprintf("failed to lookup the default state\n");
		} else {
			if (pinctrl_select_state(pctldev->p, pctldev->hog_default))
				rt_kprintf("failed to select default state\n");
		}
	}

	return pctldev;

out_err:
	rt_mutex_detach(&pctldev->mutex);
	rt_free(pctldev);
	return RT_NULL;
}

/**
 * pinctrl_get_group_selector() - returns the group selector for a group
 * @pctldev: the pin controller handling the group
 * @pin_group: the pin group to look up
 */
int pinctrl_get_group_selector(struct pinctrl_dev *pctldev,
                               const char *pin_group)
{
	const struct pinctrl_ops *pctlops = pctldev->desc->pctlops;
	unsigned ngroups = pctlops->get_groups_count(pctldev);
	unsigned group_selector = 0;

	while (group_selector < ngroups) {
		const char *gname = pctlops->get_group_name(pctldev, group_selector);
		if (!rt_strcmp(gname, pin_group)) {
			rt_kprintf("found group selector %u for %s\n",
					group_selector,
					pin_group);
			return group_selector;
		}

		group_selector++;
	}

	rt_kprintf("does not have pin group %s\n", pin_group);

	return -RT_EINVAL;
}

/**
 * pin_get_from_name() - look up a pin number from a name
 * @pctldev: the pin control device to lookup the pin on
 * @name: the name of the pin to look up
 */
int pin_get_from_name(struct pinctrl_dev *pctldev, const char *name)
{
	unsigned i, pin;

	/* The pin number can be retrived from the pin controller descriptor */
	for (i = 0; i < pctldev->desc->npins; i++) {
		struct pin_desc *desc;

		pin = pctldev->desc->pins[i].number;
		desc = pin_desc_get(pctldev, pin);
		/* Pin space may be sparse */
		if (desc == NULL)
			continue;
		if (desc->name && !rt_strcmp(name, desc->name))
			return pin;
	}

	return -RT_EINVAL;
}

const char *pinctrl_dev_get_name(struct pinctrl_dev *pctldev)
{
	/* We're not allowed to register devices without name */
	return pctldev->desc->name;
}

void *pinctrl_dev_get_drvdata(struct pinctrl_dev *pctldev)
{
	return pctldev->driver_data;
}

