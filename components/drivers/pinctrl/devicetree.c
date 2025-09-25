#include <rtthread.h>
#include <rtdef.h>
#include <rtdevice.h>
#include "pinctrl-core.h"

#define EPROBE_DEFER    517     /* Driver requests probe retry */
#define ESHUTDOWN       58      /* Cannot send after transport endpoint shutdown */
#define EBUSY           16      /* Device or resource busy */
#define ENODEV		19

/**
 * struct pinctrl_dt_map - mapping table chunk parsed from device tree
 * @node: list node for struct pinctrl's @dt_maps field
 * @pctldev: the pin controller that allocated this struct, and will free it
 * @maps: the mapping table entries
 */
struct pinctrl_dt_map {
	rt_list_t node;
	struct pinctrl_dev *pctldev;
	struct pinctrl_map *map;
	rt_uint32_t num_maps;
};


static void dt_free_map(struct pinctrl_dev *pctldev,
                     struct pinctrl_map *map, rt_uint32_t num_maps)
{
	if (pctldev) {
		const struct pinctrl_ops *ops = pctldev->desc->pctlops;
		ops->dt_free_map(pctldev, map, num_maps);
	} else {
		/* There is no pctldev for PIN_MAP_TYPE_DUMMY_STATE */
		rt_free(map);
	}
}

void pinctrl_dt_free_maps(struct pinctrl *p)
{
	struct pinctrl_dt_map *dt_map, *n1;

	rt_list_for_each_entry_safe(dt_map, n1, &p->dt_maps, node) {
		pinctrl_unregister_map(dt_map->map);
		rt_list_remove(&dt_map->node);
		dt_free_map(dt_map->pctldev, dt_map->map,
				dt_map->num_maps);
		rt_free(dt_map);
	}

	dtb_node_put(p->dev);
}

static int dt_remember_or_free_map(struct pinctrl *p, const char *statename,
                                   struct pinctrl_dev *pctldev,
                                   struct pinctrl_map *map, rt_uint32_t num_maps)
{
	int i;
	struct pinctrl_dt_map *dt_map;

	/* Initialize common mapping table entry fields */
	for (i = 0; i < num_maps; i++) {
		map[i].dev_name = p->dev->name;
		map[i].name = statename;
		if (pctldev)
			map[i].ctrl_dev_name = pctldev->dev->name;
	}

	/* Remember the converted mapping table entries */
	dt_map = rt_calloc(1, sizeof(*dt_map));
	if (!dt_map) {
		rt_kprintf("failed to alloc struct pinctrl_dt_map\n");
		dt_free_map(pctldev, map, num_maps);
		return -RT_ENOMEM;
	}

	dt_map->pctldev = pctldev;
	dt_map->map = map;
	dt_map->num_maps = num_maps;
	rt_list_insert_after(&p->dt_maps, &dt_map->node);

	return pinctrl_register_map(map, num_maps, false, true);
}

static int dt_to_map_one_config(struct pinctrl *p, const char *statename,
                                struct dtb_node *np_config)
{
	struct dtb_node *np_pctldev;
	struct pinctrl_dev *pctldev;
	const struct pinctrl_ops *ops;
	int ret;
	struct pinctrl_map *map;
	rt_uint32_t num_maps;
	struct dtb_node *head;

	head = get_dtb_node_head();

	/* Find the pin controller containing np_config */
	np_pctldev = dtb_node_get(np_config);
	for (;;) {
		np_pctldev = dtb_node_get_parent(np_pctldev);
		if (!np_pctldev || head == np_pctldev) {
			rt_kprintf("could not find pctldev for node %s, deferring probe\n", np_config->name);
			dtb_node_put(np_pctldev);
			/* OK let's just assume this will appear later then */
			return -EPROBE_DEFER;
		}
		pctldev = get_pinctrl_dev_from_of_node(np_pctldev);
		if (pctldev)
			break;

		/* Do not defer probing of hogs (circular loop) */
		if (np_pctldev == p->dev) {
			dtb_node_put(np_pctldev);
			return -ENODEV;
		}
	}

	dtb_node_put(np_pctldev);

	/*
	 * Call pinctrl driver to parse device tree node, and
	 * generate mapping table entries
	*/
	ops = pctldev->desc->pctlops;
	if (!ops->dt_node_to_map) {
		rt_kprintf("pctldev %s doesn't support DT\n", pctldev->dev->name);
		return -ENODEV;
	}

	ret = ops->dt_node_to_map(pctldev, np_config, &map, &num_maps);
	if (ret < 0)
		return ret;

	/* Stash the mapping table chunk away for later use */
	return dt_remember_or_free_map(p, statename, pctldev, map, num_maps);
}

static int dt_remember_dummy_state(struct pinctrl *p, const char *statename)
{
	struct pinctrl_map *map;

	map = rt_calloc(1, sizeof(*map));
	if (!map) {
		rt_kprintf("failed to alloc struct pinctrl_map\n");
		return -RT_ENOMEM;
	}

	/* There is no pctldev for PIN_MAP_TYPE_DUMMY_STATE */
	map->type = PIN_MAP_TYPE_DUMMY_STATE;

	return dt_remember_or_free_map(p, statename, NULL, map, 1);
}

extern char *rt_kasprintf(const char *fmt, ...);

int pinctrl_dt_to_map(struct pinctrl *p)
{
	struct dtb_node *np = p->dev;
	int state, ret;
	char *propname;
	struct dtb_property *prop;
	const char *statename;
	const uint32_t *list;
	int size, config;
	phandle phandle;
	struct dtb_node *np_config;

	/* CONFIG_OF enabled, p->dev not instantiated from DT */
	if (!np) {
		rt_kprintf("no of_node; not parsing pinctrl DT\n");
		return 0;
	}

	/* For each defined state ID */
	for (state = 0; ; state++) {
		/* Retrieve the pinctrl-* property */
		propname = rt_kasprintf("pinctrl-%d", state);
		prop = dtb_node_get_dtb_node_property(np, propname, &size);
		rt_free(propname);
		if (!prop)
			break;
		list = prop->value;
		size /= sizeof(*list);

		/* Determine whether pinctrl-names property names the state */
		ret = dtb_node_read_string_index(np, "pinctrl-names", state, &statename);

		/*
		 * If not, statename is just the integer state ID. But rather
		 * than dynamically allocate it and have to free it later,
		 * just point part way into the property name for the string.
		 */
		if (ret < 0) {
			/* strlen("pinctrl-") == 8 */
			statename = prop->name + 8;
		}

		/* For every referenced pin configuration node in it */
		for (config = 0; config < size; config++) {
			phandle = fdt32_to_cpu(*(list++));

			/* Look up the pin configuration node */
			np_config = dtb_node_find_node_by_phandle(phandle);
			if (!np_config) {
				rt_kprintf("prop %s index %i invalid phandle\n", prop->name, config);
				ret = -RT_EINVAL;
				goto err;
			}

			/* Parse the node */
			ret = dt_to_map_one_config(p, statename, np_config);
			dtb_node_put(np_config);
			if (ret < 0)
				goto err;
		}

		/* No entries in DT? Generate a dummy state table entry */
		if (!size) {
			ret = dt_remember_dummy_state(p, statename);
			if (ret < 0)
				goto err;
		}
	}

	return 0;

err:
	pinctrl_dt_free_maps(p);
	return ret;
}
