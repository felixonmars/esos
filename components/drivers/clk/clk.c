#include <rtdevice.h>
#include <rthw.h>
#include <limits.h>

#define EPROBE_DEFER    517     /* Driver requests probe retry */
#define ESHUTDOWN       58      /* Cannot send after transport endpoint shutdown */
#define EBUSY           16      /* Device or resource busy */

extern struct rt_spinlock enable_lock;

extern struct rt_mutex clk_prepare_mutex;
extern struct rt_mutex of_clk_mutex;

static HLIST_HEAD(clk_root_list);
static HLIST_HEAD(clk_orphan_list);
static rt_list_t of_clk_providers = RT_LIST_OBJECT_INIT(of_clk_providers);

struct clk {
	struct clk_core *core;
	/* struct device *dev; */
	/* const char *dev_id; */
	const char *con_id;
	rt_uint64_t min_rate;
	rt_uint64_t max_rate;
	rt_uint32_t exclusive_count;
	struct hlist_node clks_node;
};


/***    private data structures    ***/

struct clk_parent_map {
	const struct clk_hw     *hw;
	struct clk_core         *core;
	const char              *fw_name;
	const char              *name;
	int                     index;
};

struct clk_core {
	const char              *name;
	const struct clk_ops    *ops;
	struct clk_hw           *hw;
	/* struct device           *dev; */
	struct dtb_node      *of_node;
	struct clk_core         *parent;
	struct clk_parent_map   *parents;
	unsigned char           num_parents;
	unsigned char           new_parent_index;
	rt_uint64_t           rate;
	rt_uint64_t           req_rate;
	rt_uint64_t           new_rate;
	struct clk_core         *new_parent;
	struct clk_core         *new_child;
	rt_uint64_t           flags;
	unsigned char           orphan;
	rt_uint32_t            enable_count;
	rt_uint32_t            prepare_count;
	rt_uint32_t            protect_count;
	rt_uint64_t           min_rate;
	rt_uint64_t           max_rate;
	struct hlist_head       children;
	struct hlist_node       child_node;
	struct hlist_head       clks;
	rt_uint32_t            notifier_count;
};

/**
 * struct of_clk_provider - Clock provider registration structure
 * @link: Entry in global list of clock providers
 * @node: Pointer to device tree node of clock provider
 * @get: Get clock callback.  Returns NULL or a struct clk for the
 *       given clock specifier
 * @get_hw: Get clk_hw callback.  Returns NULL, ERR_PTR or a
 *       struct clk_hw for the given clock specifier
 * @data: context pointer to be passed into @get callback
 */
struct of_clk_provider {
        rt_list_t link;

        struct dtb_node *node;
        struct clk *(*get)(struct fdt_phandle_args *clkspec, void *data);
        struct clk_hw *(*get_hw)(struct fdt_phandle_args *clkspec, void *data);
        void *data;
};

static void clk_prepare_lock(void)
{
	rt_mutex_take(&clk_prepare_mutex, RT_WAITING_FOREVER);
}

static void clk_prepare_unlock(void)
{
	rt_mutex_release(&clk_prepare_mutex);
}

static rt_uint64_t clk_enable_lock(void)
{
	rt_base_t flags;

	flags = rt_spin_lock_irqsave(&enable_lock);

	return flags;
}

static void clk_enable_unlock(rt_uint64_t flags)
{
        rt_spin_unlock_irqrestore(&enable_lock, flags);
}

/**
 * clk_core_link_consumer - Add a clk consumer to the list of consumers in a clk_core
 * @core: clk to add consumer to
 * @clk: consumer to link to a clk
 */
static void clk_core_link_consumer(struct clk_core *core, struct clk *clk)
{
	clk_prepare_lock();
	hlist_add_head(&clk->clks_node, &core->clks);
	clk_prepare_unlock();
}

/**
 * clk_core_unlink_consumer - Remove a clk consumer from the list of consumers in a clk_core
 * @clk: consumer to unlink
 */
static void clk_core_unlink_consumer(struct clk *clk)
{
	hlist_del(&clk->clks_node);
}

/**
 * alloc_clk - Allocate a clk consumer, but leave it unlinked to the clk_core
 * @core: clk to allocate a consumer for
 * @dev_id: string describing device name
 * @con_id: connection ID string on device
 *
 * Returns: clk consumer left unlinked from the consumer list
 */
static struct clk *alloc_clk(struct clk_core *core, const char *dev_id,
                             const char *con_id)
{
	struct clk *clk;

	clk = rt_calloc(1, sizeof(*clk));
	if (!clk)
		return ERR_PTR(-RT_ENOMEM);

	clk->core = core;
	/* clk->dev_id = dev_id; */
	clk->con_id = con_id;
	clk->max_rate = ULONG_MAX;

	return clk;
}

static int clk_cpy_name(const char **dst_p, const char *src, bool must_exist)
{
	const char *dst;

	if (!src) {
		if (must_exist)
			return -RT_EINVAL;
		return 0;
	}

	*dst_p = dst = src;
	if (!dst)
		return -RT_ENOMEM;

	return 0;
}

static int clk_core_populate_parent_map(struct clk_core *core,
                                        const struct clk_init_data *init)
{
	unsigned char num_parents = init->num_parents;
	const char * const *parent_names = init->parent_names;
	const struct clk_hw **parent_hws = init->parent_hws;
	const struct clk_parent_data *parent_data = init->parent_data;
	int i, ret = 0;
	struct clk_parent_map *parents, *parent;

	if (!num_parents)
		return 0;

	/*
	 * Avoid unnecessary string look-ups of clk_core's possible parents by
	 * having a cache of names/clk_hw pointers to clk_core pointers.
	 */
	parents = rt_calloc(num_parents, sizeof(*parents));
	core->parents = parents;
	if (!parents)
		return -RT_ENOMEM;

	/* Copy everything over because it might be __initdata */
	for (i = 0, parent = parents; i < num_parents; i++, parent++) {
		parent->index = -1;
		if (parent_names) {
			/* throw a WARN if any entries are NULL */
			if(!parent_names[i]) {
				rt_kprintf("%s: invalid NULL in %s's .parent_names\n", __func__, core->name);
			}

			ret = clk_cpy_name(&parent->name, parent_names[i], true);
		} else if (parent_data) {

			parent->hw = parent_data[i].hw;
			parent->index = parent_data[i].index;
			ret = clk_cpy_name(&parent->fw_name, parent_data[i].fw_name, false);
			if (!ret)
				ret = clk_cpy_name(&parent->name, parent_data[i].name, false);
		} else if (parent_hws) {
			parent->hw = parent_hws[i];
		} else {
			ret = -RT_EINVAL;
			rt_kprintf("Must specify parents if num_parents > 0\n");
		}

		if (ret) {
			rt_free(parents);
			return ret;
		}
	}

	return 0;
}

static struct clk_core *__clk_lookup_subtree(const char *name,
                                             struct clk_core *core)
{
	struct clk_core *child;
	struct clk_core *ret;

	if (!rt_strcmp(core->name, name))
		return core;

	hlist_for_each_entry(child, &core->children, child_node) {
		ret = __clk_lookup_subtree(name, child);
		if (ret)
			return ret;
	}

	return NULL;
}

static struct clk_core *clk_core_lookup(const char *name)
{
	struct clk_core *root_clk;
	struct clk_core *ret;

	if (!name)
		return NULL;

	/* search the 'proper' clk tree first */
	hlist_for_each_entry(root_clk, &clk_root_list, child_node) {
		ret = __clk_lookup_subtree(name, root_clk);
		if (ret)
			return ret;
	}

	/* if not found, then search the orphan tree */
	hlist_for_each_entry(root_clk, &clk_orphan_list, child_node) {
		ret = __clk_lookup_subtree(name, root_clk);
		if (ret)
			return ret;
	}

	return NULL;
}

/**
 * of_parse_clkspec() - Parse a DT clock specifier for a given device node
 * @np: device node to parse clock specifier from
 * @index: index of phandle to parse clock out of. If index < 0, @name is used
 * @name: clock name to find and parse. If name is NULL, the index is used
 * @out_args: Result of parsing the clock specifier
 *
 * Parses a device node's "clocks" and "clock-names" properties to find the
 * phandle and cells for the index or name that is desired. The resulting clock
 * specifier is placed into @out_args, or an errno is returned when there's a
 * parsing error. The @index argument is ignored if @name is non-NULL.
 *
 * Example:
 *
 * phandle1: clock-controller@1 {
 *      #clock-cells = <2>;
 * }
 *
 * phandle2: clock-controller@2 {
 *      #clock-cells = <1>;
 * }
 *
 * clock-consumer@3 {
 *      clocks = <&phandle1 1 2 &phandle2 3>;
 *      clock-names = "name1", "name2";
 * }
 *
 * To get a device_node for `clock-controller@2' node you may call this
 * function a few different ways:
 *
 *   of_parse_clkspec(clock-consumer@3, -1, "name2", &args);
 *   of_parse_clkspec(clock-consumer@3, 1, NULL, &args);
 *   of_parse_clkspec(clock-consumer@3, 1, "name2", &args);
 *
 * Return: 0 upon successfully parsing the clock specifier. Otherwise, -ENOENT
 * if @name is NULL or -EINVAL if @name is non-NULL and it can't be found in
 * the "clock-names" property of @np.
 */
static int of_parse_clkspec(const struct dtb_node *np, int index,
                            const char *name, struct fdt_phandle_args *out_args)
{
        int ret = -RT_EINVAL;

        /* Walk up the tree of devices looking for a clock property that matches */
        while (np) {
                /*
                 * For named clocks, first look up the name in the
                 * "clock-names" property.  If it cannot be found, then index
                 * will be an error code and of_parse_phandle_with_args() will
                 * return -EINVAL.
                 */
                if (name)
                        index = dtb_node_property_match_string(np, "clock-names", name);
                ret = dtb_node_parse_phandle_with_args(np, "clocks", "#clock-cells",
                                                 index, out_args);
                if (!ret)
                        break;
                if (name && index >= 0)
                        break;

                /*
                 * No matching clock found on this node.  If the parent node
                 * has a "clock-ranges" property, then we can try one of its
                 * clocks.
                 */
	        np = np->parent;
                if (np && !dtb_node_get_property(np, "clock-ranges", NULL))
                        break;
                index = 0;
        }

        return ret;
}

const char *__clk_get_name(const struct clk *clk)
{
	return !clk ? NULL : clk->core->name;
}

struct clk_hw *__clk_get_hw(struct clk *clk)
{
	return !clk ? NULL : clk->core->hw;
}

const char *clk_hw_get_name(const struct clk_hw *hw)
{
	return hw->core->name;
}

static struct clk_hw *
__of_clk_get_hw_from_provider(struct of_clk_provider *provider,
                              struct fdt_phandle_args *clkspec)
{
        struct clk *clk;

        if (provider->get_hw)
                return provider->get_hw(clkspec, provider->data);

        clk = provider->get(clkspec, provider->data);
        if (IS_ERR(clk))
                return (void *)(clk);
        return __clk_get_hw(clk);
}

static struct clk_hw *
of_clk_get_hw_from_clkspec(struct fdt_phandle_args *clkspec)
{
        struct of_clk_provider *provider;
       	struct clk_hw *hw = ERR_PTR(-EPROBE_DEFER);

        if (!clkspec)
                return ERR_PTR(-RT_EINVAL);

        rt_mutex_take(&of_clk_mutex, RT_WAITING_FOREVER);
        rt_list_for_each_entry(provider, &of_clk_providers, link) {
                if (provider->node == clkspec->np) {
                        hw = __of_clk_get_hw_from_provider(provider, clkspec);
                        if (!IS_ERR(hw))
                                break;
                }
        }
        rt_mutex_release(&of_clk_mutex);

        return hw;
}

/**
 * clk_core_get - Find the clk_core parent of a clk
 * @core: clk to find parent of
 * @p_index: parent index to search for
 *
 * This is the preferred method for clk providers to find the parent of a
 * clk when that parent is external to the clk controller. The parent_names
 * array is indexed and treated as a local name matching a string in the device
 * node's 'clock-names' property or as the 'con_id' matching the device's
 * dev_name() in a clk_lookup. This allows clk providers to use their own
 * namespace instead of looking for a globally unique parent string.
 *
 * For example the following DT snippet would allow a clock registered by the
 * clock-controller@c001 that has a clk_init_data::parent_data array
 * with 'xtal' in the 'name' member to find the clock provided by the
 * clock-controller@f00abcd without needing to get the globally unique name of
 * the xtal clk.
 *
 *      parent: clock-controller@f00abcd {
 *              reg = <0xf00abcd 0xabcd>;
 *              #clock-cells = <0>;
 *      };
 *
 *      clock-controller@c001 {
 *              reg = <0xc001 0xf00d>;
 *              clocks = <&parent>;
 *              clock-names = "xtal";
 *              #clock-cells = <1>;
 *      };
 *
 * Returns: -ENOENT when the provider can't be found or the clk doesn't
 * exist in the provider or the name can't be found in the DT node or
 * in a clkdev lookup. NULL when the provider knows about the clk but it
 * isn't provided on this system.
 * A valid clk_core pointer when the clk can be found in the provider.
 */
static struct clk_core *clk_core_get(struct clk_core *core, unsigned char p_index)
{
	const char *fw_name = core->parents[p_index].fw_name;
	const char *name = core->parents[p_index].name;

	int index = core->parents[p_index].index;
	struct clk_hw *hw = ERR_PTR(-RT_ETIMEOUT);
	/* struct device *dev = core->dev; */
	/* const char *dev_id = dev ? dev_name(dev) : RT_NULL; */
	struct dtb_node *np = core->of_node;
	struct fdt_phandle_args clkspec;

	if (np && (fw_name || index >= 0) &&
			!of_parse_clkspec(np, index, fw_name, &clkspec)) {
		hw = of_clk_get_hw_from_clkspec(&clkspec);
		dtb_node_put(clkspec.np);
	} else if (name) {
		/*
		 * If the DT search above couldn't find the provider fallback to
		 * looking up via clkdev based clk_lookups.
		 */
		hw = clk_find_hw(/* dev_id */ NULL, name);

	}

	if (IS_ERR(hw))
		return (void *)(hw);

	if (!hw)
        	return NULL;

	return hw->core;
}

static rt_uint64_t clk_core_get_rate_nolock(struct clk_core *core)
{
	if (!core)
		return 0;

	if (!core->num_parents || core->parent)
		return core->rate;

	/*
	 * Clk must have a parent because num_parents > 0 but the parent isn't
	 * known yet. Best to return 0 as the rate of this clk until we can
	 * properly recalc the rate based on the parent's rate.
	 */
	return 0;
}

rt_uint64_t clk_hw_get_rate(const struct clk_hw *hw)
{
        return clk_core_get_rate_nolock(hw->core);
}

static void clk_core_rate_protect(struct clk_core *core)
{
	if (!core)
		return;

	if (core->protect_count == 0)
		clk_core_rate_protect(core->parent);

	core->protect_count++;
}

static void clk_core_rate_unprotect(struct clk_core *core)
{
        if (!core)
                return;

        if (core->protect_count == 0) {
		rt_kprintf("%s already unprotected\n", core->name);
		return;
	}

	if (--core->protect_count > 0)
		return;

	clk_core_rate_unprotect(core->parent);
}

static void clk_core_unprepare(struct clk_core *core)
{
	if (!core)
		return;

	if (core->prepare_count == 0) {
		rt_kprintf("%s already unprepared\n", core->name);
		return;
	}

	if (core->prepare_count == 1 && core->flags & CLK_IS_CRITICAL) {
		rt_kprintf("Unpreparing critical %s\n", core->name);
		return;
	}

        if (core->flags & CLK_SET_RATE_GATE)
                clk_core_rate_unprotect(core);

        if (--core->prepare_count > 0)
                return;

	if (core->enable_count > 0) {
		rt_kprintf("Unpreparing enabled %s\n", core->name);
	}

        if (core->ops->unprepare)
                core->ops->unprepare(core->hw);

        clk_core_unprepare(core->parent);
}

static int clk_core_prepare(struct clk_core *core)
{
	int ret = 0;

	if (!core)
		return 0;

	if (core->prepare_count == 0) {

	ret = clk_core_prepare(core->parent);
	if (ret)
		return ret;

	if (core->ops->prepare)
		ret = core->ops->prepare(core->hw);

	if (ret)
		goto unprepare;
	}

	core->prepare_count++;

	/*
	 * CLK_SET_RATE_GATE is a special case of clock protection
	 * Instead of a consumer claiming exclusive rate control, it is
	 * actually the provider which prevents any consumer from making any
	 * operation which could result in a rate change or rate glitch while
	 * the clock is prepared.
	 */
	if (core->flags & CLK_SET_RATE_GATE)
		clk_core_rate_protect(core);

	return 0;
unprepare:
	clk_core_unprepare(core->parent);
	return ret;
}

static void clk_core_disable(struct clk_core *core)
{
        if (!core)
                return;

        if (core->enable_count == 0) {
		rt_kprintf("%s already disabled\n", core->name);
		return;
	}

        if (core->enable_count == 1 && core->flags & CLK_IS_CRITICAL) {
		rt_kprintf("Disabling critical %s\n", core->name);
		return;
	}

	if (--core->enable_count > 0)
		return;

	if (core->ops->disable)
		core->ops->disable(core->hw);

	clk_core_disable(core->parent);
}

static int clk_core_enable(struct clk_core *core)
{
        int ret = 0;

        if (!core)
                return 0;

        if (core->prepare_count == 0) {
		rt_kprintf("Enabling unprepared %s\n", core->name);
		return -ESHUTDOWN;
	}

	if (core->enable_count == 0) {
		ret = clk_core_enable(core->parent);

		if (ret)
			return ret;

		if (core->ops->enable)
			ret = core->ops->enable(core->hw);

		if (ret) {
			clk_core_disable(core->parent);
			return ret;
		}
	}

	core->enable_count++;
	return 0;
}

static int clk_core_enable_lock(struct clk_core *core)
{
	rt_base_t flags;
	int ret;

	flags = clk_enable_lock();
	ret = clk_core_enable(core);
	clk_enable_unlock(flags);

	return ret;
}

static void clk_core_disable_lock(struct clk_core *core)
{
	rt_base_t flags;

	flags = clk_enable_lock();
        clk_core_disable(core);
	clk_enable_unlock(flags);
}

static void clk_core_fill_parent_index(struct clk_core *core, unsigned char index)
{
	struct clk_parent_map *entry = &core->parents[index];
	struct clk_core *parent;

	if (entry->hw) {
		parent = entry->hw->core;
	} else {
		parent = clk_core_get(core, index);
		if (PTR_ERR(parent) == -RT_ETIMEOUT && entry->name)
			parent = clk_core_lookup(entry->name);
	}

	/*
	 * We have a direct reference but it isn't registered yet?
	 * Orphan it and let clk_reparent() update the orphan status
	 * when the parent is registered.
	 */
	if (!parent)
		parent = ERR_PTR(-EPROBE_DEFER);

	/* Only cache it if it's not an error */
	if (!IS_ERR(parent))
		entry->core = parent;
}

static struct clk_core *clk_core_get_parent_by_index(struct clk_core *core,
                                                         unsigned char index)
{
	if (!core || index >= core->num_parents || !core->parents)
		return NULL;

	if (!core->parents[index].core)
		clk_core_fill_parent_index(core, index);

	return core->parents[index].core;
}

struct clk_hw *
clk_hw_get_parent_by_index(const struct clk_hw *hw, rt_uint32_t index)
{
        struct clk_core *parent;

        parent = clk_core_get_parent_by_index(hw->core, index);

        return !parent ? NULL : parent->hw;
}

static struct clk_core *__clk_init_parent(struct clk_core *core)
{
	unsigned char index = 0;

	if (core->num_parents > 1 && core->ops->get_parent)
		index = core->ops->get_parent(core->hw);

	return clk_core_get_parent_by_index(core, index);
}

static int clk_core_prepare_lock(struct clk_core *core)
{
	int ret;

	clk_prepare_lock();
	ret = clk_core_prepare(core);
	clk_prepare_unlock();

	return ret;
}

static void clk_core_unprepare_lock(struct clk_core *core)
{
	clk_prepare_lock();
	clk_core_unprepare(core);
	clk_prepare_unlock();
}

static int clk_core_prepare_enable(struct clk_core *core)
{
	int ret;

	ret = clk_core_prepare_lock(core);
	if (ret)
		return ret;

	ret = clk_core_enable_lock(core);
	if (ret)
		clk_core_unprepare_lock(core);

	return ret;
}

static void clk_core_disable_unprepare(struct clk_core *core)
{
	clk_core_disable_lock(core);
	clk_core_unprepare_lock(core);
}

/*
 * Update the orphan status of @core and all its children.
 */
static void clk_core_update_orphan_status(struct clk_core *core, bool is_orphan)
{
	struct clk_core *child;

	core->orphan = is_orphan;

	hlist_for_each_entry(child, &core->children, child_node)
		clk_core_update_orphan_status(child, is_orphan);
}

static void clk_reparent(struct clk_core *core, struct clk_core *new_parent)
{
	bool was_orphan = core->orphan;

	hlist_del(&core->child_node);

	if (new_parent) {
		bool becomes_orphan = new_parent->orphan;

		/* avoid duplicate POST_RATE_CHANGE notifications */
		if (new_parent->new_child == core)
			new_parent->new_child = NULL;

		hlist_add_head(&core->child_node, &new_parent->children);

		if (was_orphan != becomes_orphan)
			clk_core_update_orphan_status(core, becomes_orphan);
	} else {
		hlist_add_head(&core->child_node, &clk_orphan_list);
		if (!was_orphan)
			clk_core_update_orphan_status(core, true);
	}

	core->parent = new_parent;
}

static struct clk_core *__clk_set_parent_before(struct clk_core *core,
                                           struct clk_core *parent)
{
	rt_base_t flags;
	struct clk_core *old_parent = core->parent;

	/*
	 * 1. enable parents for CLK_OPS_PARENT_ENABLE clock
	 *
	 * 2. Migrate prepare state between parents and prevent race with
	 * clk_enable().
	 *
	 * If the clock is not prepared, then a race with
	 * clk_enable/disable() is impossible since we already have the
	 * prepare lock (future calls to clk_enable() need to be preceded by
	 * a clk_prepare()).
	 *
	 * If the clock is prepared, migrate the prepared state to the new
	 * parent and also protect against a race with clk_enable() by
	 * forcing the clock and the new parent on.  This ensures that all
	 * future calls to clk_enable() are practically NOPs with respect to
	 * hardware and software states.
	 *
	 * See also: Comment for clk_set_parent() below.
	 */

	/* enable old_parent & parent if CLK_OPS_PARENT_ENABLE is set */
	if (core->flags & CLK_OPS_PARENT_ENABLE) {
		clk_core_prepare_enable(old_parent);
		clk_core_prepare_enable(parent);
	}

	/* migrate prepare count if > 0 */
	if (core->prepare_count) {
		clk_core_prepare_enable(parent);
		clk_core_enable_lock(core);
	}

	/* update the clk tree topology */
	flags = clk_enable_lock();
	clk_reparent(core, parent);
	clk_enable_unlock(flags);

	return old_parent;
}

static void __clk_set_parent_after(struct clk_core *core,
                                   struct clk_core *parent,
                                   struct clk_core *old_parent)
{
	/*
	 * Finish the migration of prepare state and undo the changes done
	 * for preventing a race with clk_enable().
	 */
	if (core->prepare_count) {
		clk_core_disable_lock(core);
		clk_core_disable_unprepare(old_parent);
	}

	/* re-balance ref counting if CLK_OPS_PARENT_ENABLE is set */
	if (core->flags & CLK_OPS_PARENT_ENABLE) {
		clk_core_disable_unprepare(parent);
		clk_core_disable_unprepare(old_parent);
	}
}

static rt_uint64_t clk_recalc(struct clk_core *core,
                                rt_uint64_t parent_rate)
{
	rt_uint64_t rate = parent_rate;

	if (core->ops->recalc_rate/* && !clk_pm_runtime_get(core) */) {
		rate = core->ops->recalc_rate(core->hw, parent_rate);
		/* clk_pm_runtime_put(core); */
	}
	return rate;
}

/**
 * __clk_recalc_rates
 * @core: first clk in the subtree
 * @update_req: Whether req_rate should be updated with the new rate
 * @msg: notification type (see include/linux/clk.h)
 *
 * Walks the subtree of clks starting with clk and recalculates rates as it
 * goes.  Note that if a clk does not implement the .recalc_rate callback then
 * it is assumed that the clock will take on the rate of its parent.
 *
 * clk_recalc_rates also propagates the POST_RATE_CHANGE notification,
 * if necessary.
 */
static void __clk_recalc_rates(struct clk_core *core, bool update_req,
                               rt_uint64_t msg)
{
//	rt_uint64_t old_rate;
	rt_uint64_t parent_rate = 0;
	struct clk_core *child;

//	old_rate = core->rate;

	if (core->parent)
		parent_rate = core->parent->rate;

	core->rate = clk_recalc(core, parent_rate);
	if (update_req)
		core->req_rate = core->rate;
#if 0
        /*
         * ignore NOTIFY_STOP and NOTIFY_BAD return values for POST_RATE_CHANGE
         * & ABORT_RATE_CHANGE notifiers
         */
        if (core->notifier_count && msg)
                __clk_notify(core, msg, old_rate, core->rate);
#endif
	hlist_for_each_entry(child, &core->children, child_node)
		__clk_recalc_rates(child, update_req, msg);
}

static void clk_core_reparent_orphans_nolock(void)
{
	struct clk_core *orphan;
	struct hlist_node *tmp2;

	/*
	 * walk the list of orphan clocks and reparent any that newly finds a
	 * parent.
	 */
	hlist_for_each_entry_safe(orphan, tmp2, &clk_orphan_list, child_node) {
		struct clk_core *parent = __clk_init_parent(orphan);

		/*
		 * We need to use __clk_set_parent_before() and _after() to
		 * properly migrate any prepare/enable count of the orphan
		 * clock. This is important for CLK_IS_CRITICAL clocks, which
		 * are enabled during init but might not have a parent yet.
		 */
		if (parent) {
			/* update the clk tree topology */
			__clk_set_parent_before(orphan, parent);
			__clk_set_parent_after(orphan, parent, NULL);
			__clk_recalc_rates(orphan, true, 0);

			/*
			 * __clk_init_parent() will set the initial req_rate to
			 * 0 if the clock doesn't have clk_ops::recalc_rate and
			 * is an orphan when it's registered.
			 *
			 * 'req_rate' is used by clk_set_rate_range() and
			 * clk_put() to trigger a clk_set_rate() call whenever
			 * the boundaries are modified. Let's make sure
			 * 'req_rate' is set to something non-zero so that
			 * clk_set_rate_range() doesn't drop the frequency.
			 */
			orphan->req_rate = orphan->rate;
		}
	}
}

/**
 * __clk_core_init - initialize the data structures in a struct clk_core
 * @core:       clk_core being initialized
 *
 * Initializes the lists in struct clk_core, queries the hardware for the
 * parent and rate and sets them both.
 */
static int __clk_core_init(struct clk_core *core)
{
	int ret = 0;
	struct clk_core *parent;
	rt_uint64_t rate;

	clk_prepare_lock();

	/*
	 * Set hw->core after grabbing the prepare_lock to synchronize with
	 * callers of clk_core_fill_parent_index() where we treat hw->core
	 * being NULL as the clk not being registered yet. This is crucial so
	 * that clks aren't parented until their parent is fully registered.
	 */
	core->hw->core = core;

	/* check to see if a clock with this name is already registered */
	if (clk_core_lookup(core->name)) {
		rt_kprintf("%s: clk %s already initialized\n", __func__, core->name);
		ret = -RT_EINVAL;
		goto out;
	}

	/* check that clk_ops are sane.  See Documentation/driver-api/clk.rst */
	if (core->ops->set_rate &&
			!((core->ops->round_rate || core->ops->determine_rate) &&
				core->ops->recalc_rate)) {
		rt_kprintf("%s: %s must implement .round_rate or .determine_rate in addition to .recalc_rate\n", __func__, core->name);
		ret = -RT_EINVAL;
		goto out;
	}

	if (core->ops->set_parent && !core->ops->get_parent) {
		rt_kprintf("%s: %s must implement .get_parent & .set_parent\n", __func__, core->name);
		ret = -RT_EINVAL;
		goto out;
	}

	if (core->ops->set_parent && !core->ops->determine_rate) {
		rt_kprintf("%s: %s must implement .set_parent & .determine_rate\n", __func__, core->name);
		ret = -RT_EINVAL;
		goto out;
	}

	if (core->num_parents > 1 && !core->ops->get_parent) {
		rt_kprintf("%s: %s must implement .get_parent as it has multi parents\n", __func__, core->name);
		ret = -RT_EINVAL;
		goto out;
	}

	if (core->ops->set_rate_and_parent &&
			!(core->ops->set_parent && core->ops->set_rate)) {
		rt_kprintf("%s: %s must implement .set_parent & .set_rate\n", __func__, core->name);
		ret = -RT_EINVAL;
		goto out;
	}

	/*
	 * optional platform-specific magic
	 *
	 * The .init callback is not used by any of the basic clock types, but
	 * exists for weird hardware that must perform initialization magic for
	 * CCF to get an accurate view of clock for any other callbacks. It may
	 * also be used needs to perform dynamic allocations. Such allocation
	 * must be freed in the terminate() callback.
	 * This callback shall not be used to initialize the parameters state,
	 * such as rate, parent, etc ...
	 *
	 * If it exist, this callback should called before any other callback of
	 * the clock
	 */
	if (core->ops->init) {
		ret = core->ops->init(core->hw);
		if (ret)
			goto out;
	}

	parent = core->parent = __clk_init_parent(core);

	/*
	 * Populate core->parent if parent has already been clk_core_init'd. If
	 * parent has not yet been clk_core_init'd then place clk in the orphan
	 * list.  If clk doesn't have any parents then place it in the root
	 * clk list.
	 *
	 * Every time a new clk is clk_init'd then we walk the list of orphan
	 * clocks and re-parent any that are children of the clock currently
	 * being clk_init'd.
	 */
	if (parent) {
		hlist_add_head(&core->child_node, &parent->children);
		core->orphan = parent->orphan;
	} else if (!core->num_parents) {
		hlist_add_head(&core->child_node, &clk_root_list);
		core->orphan = false;
	} else {
		hlist_add_head(&core->child_node, &clk_orphan_list);
		core->orphan = true;
	}

	/*
	 * Set clk's rate.  The preferred method is to use .recalc_rate.  For
	 * simple clocks and lazy developers the default fallback is to use the
	 * parent's rate.  If a clock doesn't have a parent (or is orphaned)
	 * then rate is set to zero.
	 */
	if (core->ops->recalc_rate)
		rate = core->ops->recalc_rate(core->hw,
				clk_core_get_rate_nolock(parent));
	else if (parent)
		rate = parent->rate;
	else
		rate = 0;
	core->rate = core->req_rate = rate;

	/*
	 * Enable CLK_IS_CRITICAL clocks so newly added critical clocks
	 * don't get accidentally disabled when walking the orphan tree and
	 * reparenting clocks
	 */
	if (core->flags & CLK_IS_CRITICAL) {
		ret = clk_core_prepare(core);
		if (ret) {
			rt_kprintf("%s: critical clk '%s' failed to prepare\n",
					__func__, core->name);
			goto out;
		}

		ret = clk_core_enable_lock(core);
		if (ret) {
			rt_kprintf("%s: critical clk '%s' failed to enable\n",
					__func__, core->name);
			clk_core_unprepare(core);
			goto out;
		}
	}

	clk_core_reparent_orphans_nolock();
out:
	if (ret) {
		hlist_del_init(&core->child_node);
		core->hw->core = NULL;
	}

	clk_prepare_unlock();

/*
        if (!ret)
                clk_debug_register(core);
*/
	return ret;
}

/**
 * free_clk - Free a clk consumer
 * @clk: clk consumer to free
 *
 * Note, this assumes the clk has been unlinked from the clk_core consumer
 * list.
 */
static void free_clk(struct clk *clk)
{
	/* kfree_const(clk->con_id); */
	rt_free(clk);
}

static struct clk *
__clk_register(/*struct device *dev, */struct dtb_node *np, struct clk_hw *hw)
{
	int ret;
	struct clk_core *core;
	const struct clk_init_data *init = hw->init;

	/*
	 * The init data is not supposed to be used outside of registration path.
	 * Set it to NULL so that provider drivers can't use it either and so that
	 * we catch use of hw->init early on in the core.
	 */
	hw->init = NULL;

	core = rt_malloc(sizeof(*core));
	if (!core) {
		ret = -RT_ENOMEM;
		goto fail_out;
	}

	if (!init->ops) {
		ret = -RT_EINVAL;
		goto fail_ops;
	}
	core->ops = init->ops;

	core->name = init->name;
	/* core->dev = dev; */
	core->of_node = np;
	core->hw = hw;
	core->flags = init->flags;
	core->num_parents = init->num_parents;
	core->min_rate = 0;
	core->max_rate = ULONG_MAX;

	ret = clk_core_populate_parent_map(core, init);
	if (ret)
		goto fail_parents;

	INIT_HLIST_HEAD(&core->clks);

	/*
	 * Don't call clk_hw_create_clk() here because that would pin the
	 * provider module to itself and prevent it from ever being removed.
	 */
	hw->clk = alloc_clk(core, NULL, NULL);
	if (IS_ERR(hw->clk)) {
		ret = PTR_ERR(hw->clk);
		goto fail_create_clk;
	}

	clk_core_link_consumer(core, hw->clk);

	ret = __clk_core_init(core);
	if (!ret)
		return hw->clk;

	clk_prepare_lock();
	clk_core_unlink_consumer(hw->clk);
	clk_prepare_unlock();

	free_clk(hw->clk);
	hw->clk = NULL;

fail_create_clk:
fail_parents:
fail_ops:
fail_out:
	return ERR_PTR(ret);
}

/*
 * of_clk_hw_register - register a clk_hw and return an error code
 * @node: device_node of device that is registering this clock
 * @hw: link to hardware-specific clock data
 *
 * of_clk_hw_register() is the primary interface for populating the clock tree
 * with new clock nodes when a struct device is not available, but a struct
 * device_node is. It returns an integer equal to zero indicating success or
 * less than zero indicating failure. Drivers must test for an error code after
 * calling of_clk_hw_register().
 */
struct clk * of_clk_hw_register(struct dtb_node *node, struct clk_hw *hw)
{
	return __clk_register(/*NULL,*/ node, hw);
}

static void clk_core_reparent_orphans(void)
{
	clk_prepare_lock();
	clk_core_reparent_orphans_nolock();
	clk_prepare_unlock();
}

struct clk_hw *
of_clk_hw_onecell_get(struct fdt_phandle_args *clkspec, void *data)
{
	struct clk_hw_onecell_data *hw_data = data;
	rt_uint32_t idx = clkspec->args[0];

	if (idx >= hw_data->num) {
		rt_kprintf("%s: invalid index %u\n", __func__, idx);
		return ERR_PTR(-RT_EINVAL);
	}

	return hw_data->hws[idx];
}

/**
 * of_clk_add_hw_provider() - Register a clock provider for a node
 * @np: Device node pointer associated with clock provider
 * @get: callback for decoding clk_hw
 * @data: context pointer for @get callback.
 */
int of_clk_add_hw_provider(struct dtb_node *np,
                           struct clk_hw *(*get)(struct fdt_phandle_args *clkspec, void *data),
                           void *data)
{
	struct of_clk_provider *cp;
	int ret = 0;

	if (!np)
		return 0;

	cp = rt_calloc(1, sizeof(*cp));
	if (!cp)
		return -RT_ENOMEM;

	cp->node = dtb_node_get(np);
	cp->data = data;
	cp->get_hw = get;

	rt_mutex_take(&of_clk_mutex, RT_WAITING_FOREVER);
	rt_list_insert_after(&of_clk_providers, &cp->link);
	rt_mutex_release(&of_clk_mutex);
	debug("Added clk_hw provider from %pOF\n", np);

        clk_core_reparent_orphans();

#if 0
        ret = of_clk_set_defaults(np, true);
        if (ret < 0)
                of_clk_del_provider(np);
#endif
        /* fwnode_dev_initialized(&np->fwnode, true); */

        return ret;
}

/**
 * clk_hw_create_clk: Allocate and link a clk consumer to a clk_core given
 * a clk_hw
 * @dev: clk consumer device
 * @hw: clk_hw associated with the clk being consumed
 * @dev_id: string describing device name
 * @con_id: connection ID string on device
 *
 * This is the main function used to create a clk pointer for use by clk
 * consumers. It connects a consumer to the clk_core and clk_hw structures
 * used by the framework and clk provider respectively.
 */
static struct clk *clk_hw_create_clk(struct clk_hw *hw, const char *dev_id, const char *con_id)
{
        struct clk *clk;
        struct clk_core *core;

        /* This is to allow this function to be chained to others */
        if (IS_ERR(hw) || !hw)
                return (void *)(hw);

        core = hw->core;
        clk = alloc_clk(core, dev_id, con_id);
        if (IS_ERR(clk))
                return clk;
        /* clk->dev = dev; */

        clk_core_link_consumer(core, clk);

        return clk;
}

/**
 * clk_hw_get_clk - get clk consumer given an clk_hw
 * @hw: clk_hw associated with the clk being consumed
 * @con_id: connection ID string on device
 *
 * Returns: new clk consumer
 * This is the function to be used by providers which need
 * to get a consumer clk and act on the clock element
 * Calls to this function must be balanced with calls clk_put()
 */
struct clk *clk_hw_get_clk(struct clk_hw *hw, const char *con_id)
{
//	struct device *dev = hw->core->dev;
//	const char *name = dev ? dev_name(dev) : NULL;

        return clk_hw_create_clk(/*dev, */hw, RT_NULL, con_id);
}

static struct clk_hw *of_clk_get_hw(struct dtb_node *np, int index,
                             const char *con_id)
{
	int ret;
	struct clk_hw *hw;
	struct fdt_phandle_args clkspec;

	ret = of_parse_clkspec(np, index, con_id, &clkspec);
	if (ret)
		return ERR_PTR(ret);

	hw = of_clk_get_hw_from_clkspec(&clkspec);
	dtb_node_put(clkspec.np);

	return hw;
}

static struct clk *__of_clk_get(struct dtb_node *np,
                                int index, const char *dev_id,
                                const char *con_id)
{
	struct clk_hw *hw = of_clk_get_hw(np, index, con_id);

	return clk_hw_create_clk(hw, dev_id, con_id);
}

struct clk *of_clk_get(struct dtb_node *np, int index)
{
        return __of_clk_get(np, index, NULL, NULL);
}

/**
 * of_clk_get_by_name() - Parse and lookup a clock referenced by a device node
 * @np: pointer to clock consumer node
 * @name: name of consumer's clock input, or NULL for the first clock reference
 *
 * This function parses the clocks and clock-names properties,
 * and uses them to look up the struct clk from the registered list of clock
 * providers.
*/
struct clk *of_clk_get_by_name(struct dtb_node *np, const char *name)
{
        if (!np)
                return ERR_PTR(-RT_ETIMEOUT);

        return __of_clk_get(np, 0, NULL, name);
}

/**
 * clk_prepare - prepare a clock source
 * @clk: the clk being prepared
 *
 * clk_prepare may sleep, which differentiates it from clk_enable.  In a simple
 * case, clk_prepare can be used instead of clk_enable to ungate a clk if the
 * operation may sleep.  One example is a clk which is accessed over I2c.  In
 * the complex case a clk ungate operation may require a fast and a slow part.
 * It is this reason that clk_prepare and clk_enable are not mutually
 * exclusive.  In fact clk_prepare must be called before clk_enable.
 * Returns 0 on success, -EERROR otherwise.
 */
int clk_prepare(struct clk *clk)
{
	if (!clk)
		return 0;

	return clk_core_prepare_lock(clk->core);
}

/**
 * clk_enable - ungate a clock
 * @clk: the clk being ungated
 *
 * clk_enable must not sleep, which differentiates it from clk_prepare.  In a
 * simple case, clk_enable can be used instead of clk_prepare to ungate a clk
 * if the operation will never sleep.  One example is a SoC-internal clk which
 * is controlled via simple register writes.  In the complex case a clk ungate
 * operation may require a fast and a slow part.  It is this reason that
 * clk_enable and clk_prepare are not mutually exclusive.  In fact clk_prepare
 * must be called before clk_enable.  Returns 0 on success, -EERROR
 * otherwise.
 */
int clk_enable(struct clk *clk)
{
	if (!clk)
		return 0;

	return clk_core_enable_lock(clk->core);
}

/**
 * clk_unprepare - undo preparation of a clock source
 * @clk: the clk being unprepared
 *
 * clk_unprepare may sleep, which differentiates it from clk_disable.  In a
 * simple case, clk_unprepare can be used instead of clk_disable to gate a clk
 * if the operation may sleep.  One example is a clk which is accessed over
 * I2c.  In the complex case a clk gate operation may require a fast and a slow
 * part.  It is this reason that clk_unprepare and clk_disable are not mutually
 * exclusive.  In fact clk_disable must be called before clk_unprepare.
 */
void clk_unprepare(struct clk *clk)
{
	if (IS_ERR(clk) && !clk)
		return;

	clk_core_unprepare_lock(clk->core);
}

/* clk_prepare_enable helps cases using clk_enable in non-atomic context. */
int clk_prepare_enable(struct clk *clk)
{
	int ret;

	ret = clk_prepare(clk);
	if (ret)
		return ret;
	ret = clk_enable(clk);
	if (ret)
		clk_unprepare(clk);

	return ret;
}

/**
 * clk_disable - gate a clock
 * @clk: the clk being gated
 *
 * clk_disable must not sleep, which differentiates it from clk_unprepare.  In
 * a simple case, clk_disable can be used instead of clk_unprepare to gate a
 * clk if the operation is fast and will never sleep.  One example is a
 * SoC-internal clk which is controlled via simple register writes.  In the
 * complex case a clk gate operation may require a fast and a slow part.  It is
 * this reason that clk_unprepare and clk_disable are not mutually exclusive.
 * In fact clk_disable must be called before clk_unprepare.
 */
void clk_disable(struct clk *clk)
{
	if (IS_ERR(clk) || !clk)
		return;

	clk_core_disable_lock(clk->core);
}

/* clk_disable_unprepare helps cases using clk_disable in non-atomic context. */
void clk_disable_unprepare(struct clk *clk)
{
	clk_disable(clk);
	clk_unprepare(clk);
}

static void clk_core_get_boundaries(struct clk_core *core,
                                    rt_uint64_t *min_rate,
                                    rt_uint64_t *max_rate)
{
	struct clk *clk_user;

	*min_rate = core->min_rate;
	*max_rate = core->max_rate;

	hlist_for_each_entry(clk_user, &core->clks, clks_node)
		/* *min_rate = max(*min_rate, clk_user->min_rate); */
		*min_rate = *min_rate > clk_user->min_rate ? *min_rate : clk_user->min_rate;

	hlist_for_each_entry(clk_user, &core->clks, clks_node)
		/* *max_rate = min(*max_rate, clk_user->max_rate); */
		*max_rate = *max_rate < clk_user->max_rate ? *max_rate : clk_user->max_rate;
}

static void clk_core_init_rate_req(struct clk_core * const core,
                                   struct clk_rate_request *req,
                                   rt_uint64_t rate)
{
	struct clk_core *parent;

	if (!req)
		return;

	rt_memset(req, 0, sizeof(*req));
	req->max_rate = ULONG_MAX;

	if (!core)
		return;

	req->core = core;
	req->rate = rate;
	clk_core_get_boundaries(core, &req->min_rate, &req->max_rate);

	parent = core->parent;
	if (parent) {
		req->best_parent_hw = parent->hw;
		req->best_parent_rate = parent->rate;
	} else {
		req->best_parent_hw = NULL;
		req->best_parent_rate = 0;
	}
}

static int clk_core_rate_nuke_protect(struct clk_core *core)
{
	int ret;

	if (!core)
		return -RT_EINVAL;

	if (core->protect_count == 0)
		return 0;

	ret = core->protect_count;
	core->protect_count = 1;
	clk_core_rate_unprotect(core);

	return ret;
}

static bool clk_core_can_round(struct clk_core * const core)
{
	return core->ops->determine_rate || core->ops->round_rate;
}

static bool clk_core_rate_is_protected(struct clk_core *core)
{
	return core->protect_count;
}

static int clk_core_determine_round_nolock(struct clk_core *core,
                                           struct clk_rate_request *req)
{
	long rate;

	if (!core)
		return 0;

	/*
	 * Some clock providers hand-craft their clk_rate_requests and
	 * might not fill min_rate and max_rate.
	 *
	 * If it's the case, clamping the rate is equivalent to setting
	 * the rate to 0 which is bad. Skip the clamping but complain so
	 * that it gets fixed, hopefully.
	 */
	if (!req->min_rate && !req->max_rate)
		rt_kprintf("%s: %s: clk_rate_request has initialized min or max rate.\n",
				__func__, core->name);
	else {
		/* req->rate = clamp(req->rate, req->min_rate, req->max_rate); */
		if (req->rate < req->min_rate)
			req->rate = req->min_rate;
		else if (req->rate > req->max_rate)
			req->rate = req->max_rate;
		else
			;
	}

	/*
	 * At this point, core protection will be disabled
	 * - if the provider is not protected at all
	 * - if the calling consumer is the only one which has exclusivity
	 *   over the provider
	 */
	if (clk_core_rate_is_protected(core)) {
		req->rate = core->rate;
	} else if (core->ops->determine_rate) {
	return core->ops->determine_rate(core->hw, req);
	} else if (core->ops->round_rate) {
		rate = core->ops->round_rate(core->hw, req->rate,
				&req->best_parent_rate);
	if (rate < 0)
		return rate;

	req->rate = rate;
	} else {
		return -RT_EINVAL;
	}

	return 0;
}

static bool clk_core_has_parent(struct clk_core *core, const struct clk_core *parent)
{
	struct clk_core *tmp;
	rt_uint32_t i;

	/* Optimize for the case where the parent is already the parent. */
	if (core->parent == parent)
		return true;
	for (i = 0; i < core->num_parents; i++) {
		tmp = clk_core_get_parent_by_index(core, i);
		if (!tmp)
			continue;

	if (tmp == parent)
		return true;
	}

	return false;
}

static void
clk_core_forward_rate_req(struct clk_core *core,
                          const struct clk_rate_request *old_req,
                          struct clk_core *parent,
                          struct clk_rate_request *req,
                          rt_uint64_t parent_rate)
{
        if (!clk_core_has_parent(core, parent))
		return;

	clk_core_init_rate_req(parent, req, parent_rate);

	if (req->min_rate < old_req->min_rate)
		req->min_rate = old_req->min_rate;

	if (req->max_rate > old_req->max_rate)
		req->max_rate = old_req->max_rate;
}

static int clk_core_round_rate_nolock(struct clk_core *core,
                                      struct clk_rate_request *req)
{
	int ret;

	if (!core) {
		req->rate = 0;
		return 0;
	}

	if (clk_core_can_round(core))
		return clk_core_determine_round_nolock(core, req);

	if (core->flags & CLK_SET_RATE_PARENT) {
		struct clk_rate_request parent_req;

		clk_core_forward_rate_req(core, req, core->parent, &parent_req, req->rate);

		ret = clk_core_round_rate_nolock(core->parent, &parent_req);
		if (ret)
			return ret;

		req->best_parent_rate = parent_req.rate;
		req->rate = parent_req.rate;

		return 0;
	}

	req->rate = core->rate;
	return 0;
}

static void clk_core_rate_restore_protect(struct clk_core *core, int count)
{
	if (!core)
		return;

	if (count == 0)
		return;

	clk_core_rate_protect(core);
	core->protect_count = count;
}

static rt_uint64_t clk_core_req_round_rate_nolock(struct clk_core *core,
                                                     rt_uint64_t req_rate)
{
	int ret, cnt;
	struct clk_rate_request req;

	if (!core)
		return 0;

	/* simulate what the rate would be if it could be freely set */
	cnt = clk_core_rate_nuke_protect(core);
	if (cnt < 0)
		return cnt;

	clk_core_init_rate_req(core, &req, req_rate);

	ret = clk_core_round_rate_nolock(core, &req);

	/* restore the protection */
	clk_core_rate_restore_protect(core, cnt);

	return ret ? 0 : req.rate;
}

static int clk_fetch_parent_index(struct clk_core *core,
                                  struct clk_core *parent)
{
        int i;

        if (!parent)
                return -RT_EINVAL;

        for (i = 0; i < core->num_parents; i++) {
                /* Found it first try! */
                if (core->parents[i].core == parent)
                        return i;

                /* Something else is here, so keep looking */
                if (core->parents[i].core)
                        continue;

                /* Maybe core hasn't been cached but the hw is all we know? */
                if (core->parents[i].hw) {
                        if (core->parents[i].hw == parent->hw)
                                break;

                        /* Didn't match, but we're expecting a clk_hw */
                        continue;
                }

                /* Maybe it hasn't been cached (clk_set_parent() path) */
                if (parent == clk_core_get(core, i))
                        break;

                /* Fallback to comparing globally unique names */
                if (core->parents[i].name &&
                    !strcmp(parent->name, core->parents[i].name))
                        break;
        }

        if (i == core->num_parents)
                return -RT_EINVAL;

        core->parents[i].core = parent;
        return i;
}

static void clk_calc_subtree(struct clk_core *core, rt_uint64_t new_rate,
                             struct clk_core *new_parent, unsigned char p_index)
{
        struct clk_core *child;

        core->new_rate = new_rate;
        core->new_parent = new_parent;
        core->new_parent_index = p_index;
        /* include clk in new parent's PRE_RATE_CHANGE notifications */
        core->new_child = NULL;
        if (new_parent && new_parent != core->parent)
                new_parent->new_child = core;

        hlist_for_each_entry(child, &core->children, child_node) {
                child->new_rate = clk_recalc(child, new_rate);
                clk_calc_subtree(child, child->new_rate, NULL, 0);
        }
}

/*
 * calculate the new rates returning the topmost clock that has to be
 * changed.
 */
static struct clk_core *clk_calc_new_rates(struct clk_core *core,
                                           rt_uint64_t rate)
{
        struct clk_core *top = core;
        struct clk_core *old_parent, *parent;
        rt_uint64_t best_parent_rate = 0;
        rt_uint64_t new_rate;
        rt_uint64_t min_rate;
        rt_uint64_t max_rate;
        int p_index = 0;
        long ret;

        /* sanity */
        if (IS_ERR(core) || !core)
                return NULL;

        /* save parent rate, if it exists */
        parent = old_parent = core->parent;
        if (parent)
                best_parent_rate = parent->rate;

        clk_core_get_boundaries(core, &min_rate, &max_rate);

        /* find the closest rate and parent clk/rate */
        if (clk_core_can_round(core)) {
                struct clk_rate_request req;

                clk_core_init_rate_req(core, &req, rate);

                ret = clk_core_determine_round_nolock(core, &req);
                if (ret < 0)
                        return NULL;

                best_parent_rate = req.best_parent_rate;
                new_rate = req.rate;
                parent = req.best_parent_hw ? req.best_parent_hw->core : NULL;

                if (new_rate < min_rate || new_rate > max_rate)
                        return NULL;
        } else if (!parent || !(core->flags & CLK_SET_RATE_PARENT)) {
                /* pass-through clock without adjustable parent */
                core->new_rate = core->rate;
                return NULL;
        } else {
                /* pass-through clock with adjustable parent */
                top = clk_calc_new_rates(parent, rate);
                new_rate = parent->new_rate;
                goto out;
        }

        /* some clocks must be gated to change parent */
        if (parent != old_parent &&
            (core->flags & CLK_SET_PARENT_GATE) && core->prepare_count) {
                rt_kprintf("%s: %s not gated but wants to reparent\n",
                         __func__, core->name);
                return NULL;
        }

        /* try finding the new parent index */
        if (parent && core->num_parents > 1) {
                p_index = clk_fetch_parent_index(core, parent);
                if (p_index < 0) {
                        rt_kprintf("%s: clk %s can not be parent of clk %s\n",
                                 __func__, parent->name, core->name);
                        return NULL;
                }
        }

        if ((core->flags & CLK_SET_RATE_PARENT) && parent &&
            best_parent_rate != parent->rate)
                top = clk_calc_new_rates(parent, best_parent_rate);

out:
        clk_calc_subtree(core, new_rate, parent, p_index);

        return top;
}

#if 0
/*
 * Notify about rate changes in a subtree. Always walk down the whole tree
 * so that in case of an error we can walk down the whole tree again and
 * abort the change.
 */
static struct clk_core *clk_propagate_rate_change(struct clk_core *core,
                                                  rt_uint64_t event)
{
        struct clk_core *child, *tmp_clk, *fail_clk = NULL;
        int ret = NOTIFY_DONE;

        if (core->rate == core->new_rate)
                return NULL;

#if 0
        if (core->notifier_count) {
                ret = __clk_notify(core, event, core->rate, core->new_rate);
                if (ret & NOTIFY_STOP_MASK)
                        fail_clk = core;
        }
#endif

        hlist_for_each_entry(child, &core->children, child_node) {
                /* Skip children who will be reparented to another clock */
                if (child->new_parent && child->new_parent != core)
                        continue;
                tmp_clk = clk_propagate_rate_change(child, event);
                if (tmp_clk)
                        fail_clk = tmp_clk;
        }

        /* handle the new child who might not be in core->children yet */
        if (core->new_child) {
                tmp_clk = clk_propagate_rate_change(core->new_child, event);
                if (tmp_clk)
                        fail_clk = tmp_clk;
        }

        return fail_clk;
}
#endif

/*
 * walk down a subtree and set the new rates notifying the rate
 * change on the way
 */
static void clk_change_rate(struct clk_core *core)
{
        struct clk_core *child;
        struct hlist_node *tmp;
//        rt_uint64_t old_rate;
        rt_uint64_t best_parent_rate = 0;
        bool skip_set_rate = false;
        struct clk_core *old_parent;
        struct clk_core *parent = NULL;

//        old_rate = core->rate;

        if (core->new_parent) {
                parent = core->new_parent;
                best_parent_rate = core->new_parent->rate;
        } else if (core->parent) {
                parent = core->parent;
                best_parent_rate = core->parent->rate;
        }

        if (core->flags & CLK_SET_RATE_UNGATE) {
                clk_core_prepare(core);
                clk_core_enable_lock(core);
        }

        if (core->new_parent && core->new_parent != core->parent) {
                old_parent = __clk_set_parent_before(core, core->new_parent);

                if (core->ops->set_rate_and_parent) {
                        skip_set_rate = true;
                        core->ops->set_rate_and_parent(core->hw, core->new_rate,
                                        best_parent_rate,
                                        core->new_parent_index);
                } else if (core->ops->set_parent) {
                        core->ops->set_parent(core->hw, core->new_parent_index);
                }

                __clk_set_parent_after(core, core->new_parent, old_parent);
        }

        if (core->flags & CLK_OPS_PARENT_ENABLE)
                clk_core_prepare_enable(parent);

        if (!skip_set_rate && core->ops->set_rate)
                core->ops->set_rate(core->hw, core->new_rate, best_parent_rate);

        core->rate = clk_recalc(core, best_parent_rate);

        if (core->flags & CLK_SET_RATE_UNGATE) {
                clk_core_disable_lock(core);
                clk_core_unprepare(core);
        }

        if (core->flags & CLK_OPS_PARENT_ENABLE)
                clk_core_disable_unprepare(parent);

#if 0
        if (core->notifier_count && old_rate != core->rate)
                __clk_notify(core, POST_RATE_CHANGE, old_rate, core->rate);
#endif

        if (core->flags & CLK_RECALC_NEW_RATES)
                (void)clk_calc_new_rates(core, core->new_rate);

        /*
         * Use safe iteration, as change_rate can actually swap parents
         * for certain clock types.
         */
        hlist_for_each_entry_safe(child, tmp, &core->children, child_node) {
                /* Skip children who will be reparented to another clock */
                if (child->new_parent && child->new_parent != core)
                        continue;
                clk_change_rate(child);
        }

        /* handle the new child who might not be in core->children yet */
        if (core->new_child)
                clk_change_rate(core->new_child);
}

static int clk_core_set_rate_nolock(struct clk_core *core,
                                    rt_uint64_t req_rate)
{
	struct clk_core *top/*, *fail_clk */;
	rt_uint64_t rate;
	int ret = 0;

	if (!core)
		return 0;

	rate = clk_core_req_round_rate_nolock(core, req_rate);

	/* bail early if nothing to do */
	if (rate == clk_core_get_rate_nolock(core))
		return 0;

	/* fail on a direct rate set of a protected provider */
	if (clk_core_rate_is_protected(core))
		return -RT_EBUSY;

	/* calculate new rates and get the topmost changed clock */
	top = clk_calc_new_rates(core, req_rate);
	if (!top)
		return -RT_EINVAL;

#if 0
	/* notify that we are about to change rates */
	fail_clk = clk_propagate_rate_change(top, PRE_RATE_CHANGE);
	if (fail_clk) {
		pr_debug("%s: failed to set %s rate\n", __func__,
				fail_clk->name);
		clk_propagate_rate_change(top, ABORT_RATE_CHANGE);
		ret = -EBUSY;
		goto err;
	}
#endif
	/* change the rates */
	clk_change_rate(top);

	core->req_rate = req_rate;

	return ret;
}

int clk_set_rate(struct clk *clk, rt_uint64_t rate)
{
	int ret;

	if (!clk)
		return 0;

	/* prevent racing with updates to the clock topology */
	clk_prepare_lock();

	if (clk->exclusive_count)
		clk_core_rate_unprotect(clk->core);

	ret = clk_core_set_rate_nolock(clk->core, rate);

	if (clk->exclusive_count)
		clk_core_rate_protect(clk->core);

	clk_prepare_unlock();

	return ret;
}

static rt_uint64_t clk_core_get_rate_recalc(struct clk_core *core)
{
	if (core && (core->flags & CLK_GET_RATE_NOCACHE))
		__clk_recalc_rates(core, false, 0);

	return clk_core_get_rate_nolock(core);
}

rt_uint64_t clk_get_rate(struct clk *clk)
{
	rt_uint64_t rate;

	if (!clk)
		return 0;

	clk_prepare_lock();
	rate = clk_core_get_rate_recalc(clk->core);
	clk_prepare_unlock();

	return rate;
}

static int __clk_set_parent(struct clk_core *core, struct clk_core *parent,
                            unsigned char p_index)
{
	rt_uint64_t flags;
	int ret = 0;
	struct clk_core *old_parent;

	old_parent = __clk_set_parent_before(core, parent);

	/* change clock input source */
	if (parent && core->ops->set_parent)
		ret = core->ops->set_parent(core->hw, p_index);

	if (ret) {
		flags = clk_enable_lock();
		clk_reparent(core, old_parent);
		clk_enable_unlock(flags);
		
		__clk_set_parent_after(core, old_parent, parent);

		return ret;
	}

	__clk_set_parent_after(core, parent, old_parent);

        return 0;
}

static int clk_core_set_parent_nolock(struct clk_core *core,
                                      struct clk_core *parent)
{
	int ret = 0;
	int p_index = 0;

	if (!core)
		return 0;

	if (core->parent == parent)
		return 0;

	/* verify ops for multi-parent clks */
	if (core->num_parents > 1 && !core->ops->set_parent)
		return -RT_ERROR;

	/* check that we are allowed to re-parent if the clock is in use */
	if ((core->flags & CLK_SET_PARENT_GATE) && core->prepare_count)
		return -EBUSY;

	if (clk_core_rate_is_protected(core))
		return -EBUSY;

	/* try finding the new parent index */
	if (parent) {
		p_index = clk_fetch_parent_index(core, parent);
		if (p_index < 0) {
			rt_kprintf("%s: clk %s can not be parent of clk %s\n",
					__func__, parent->name, core->name);
			return p_index;
		}
	}

	/* do the re-parent */
	ret = __clk_set_parent(core, parent, p_index);

	/* propagate rate an accuracy recalculation accordingly */
	if (ret) {
	       	__clk_recalc_rates(core, true, /* ABORT_RATE_CHANGE */0);
	} else {
		__clk_recalc_rates(core, true, /* POST_RATE_CHANGE */0);
	}

	return ret;
}

int clk_hw_set_parent(struct clk_hw *hw, struct clk_hw *parent)
{
	return clk_core_set_parent_nolock(hw->core, parent->core);
}

rt_uint32_t clk_hw_get_num_parents(const struct clk_hw *hw)
{
	return hw->core->num_parents;
}
