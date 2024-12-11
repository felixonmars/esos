#include <rtthread.h>
#include <rtdevice.h>

static rt_list_t clocks = RT_LIST_OBJECT_INIT(clocks);
extern struct rt_mutex clocks_mutex;
/*
 * Find the correct struct clk for the device and connection ID.
 * We do slightly fuzzy matching here:
 *  An entry with a NULL ID is assumed to be a wildcard.
 *  If an entry has a device ID, it must match
 *  If an entry has a connection ID, it must match
 * Then we take the most specific entry - with the following
 * order of precedence: dev+con > dev only > con only.
 */
static struct clk_lookup *clk_find(const char *dev_id, const char *con_id)
{
        struct clk_lookup *p, *cl = NULL;
        int match, best_found = 0, best_possible = 0;

        if (dev_id)
                best_possible += 2;
        if (con_id)
                best_possible += 1;

        rt_list_for_each_entry(p, &clocks, node) {
                match = 0;
                if (p->dev_id) {
                        if (!dev_id || rt_strcmp(p->dev_id, dev_id))
                                continue;
                        match += 2;
                }
                if (p->con_id) {
                        if (!con_id || rt_strcmp(p->con_id, con_id))
                                continue;
                        match += 1;
                }

                if (match > best_found) {
                        cl = p;
                        if (match != best_possible)
                                best_found = match;
                        else
                                break;
                }
        }

        return cl;
}

struct clk_hw *clk_find_hw(const char *dev_id, const char *con_id)
{
	struct clk_lookup *cl;
	struct clk_hw *hw = ERR_PTR(-RT_ETIMEOUT);

	rt_mutex_take(&clocks_mutex, RT_WAITING_FOREVER);
	cl = clk_find(dev_id, con_id);
	if (cl)
		hw = cl->clk_hw;
	rt_mutex_release(&clocks_mutex);

	return hw;
}

static void __clkdev_add(struct clk_lookup *cl)
{
	rt_mutex_take(&clocks_mutex, RT_WAITING_FOREVER);
	rt_list_insert_after(&clocks, &cl->node);
	rt_mutex_release(&clocks_mutex);
}

#define MAX_DEV_ID      32
#define MAX_CON_ID      32

struct clk_lookup_alloc {
        struct clk_lookup cl;
        char    dev_id[MAX_DEV_ID];
        char    con_id[MAX_CON_ID];
};

static struct clk_lookup *
vclkdev_alloc(struct clk_hw *hw, const char *con_id, const char *dev_fmt,
        va_list ap)
{
        struct clk_lookup_alloc *cla;

        cla = rt_calloc(1, sizeof(*cla));
        if (!cla)
                return NULL;

        cla->cl.clk_hw = hw;
        if (con_id) {
                rt_strncpy(cla->con_id, con_id, sizeof(cla->con_id));
                cla->cl.con_id = cla->con_id;
        }

        if (dev_fmt) {
                rt_vsnprintf(cla->dev_id, sizeof(cla->dev_id), dev_fmt, ap);
                cla->cl.dev_id = cla->dev_id;
        }

        return &cla->cl;
}

static struct clk_lookup *
vclkdev_create(struct clk_hw *hw, const char *con_id, const char *dev_fmt,
        va_list ap)
{
	struct clk_lookup *cl;

	cl = vclkdev_alloc(hw, con_id, dev_fmt, ap);
	if (cl)
		__clkdev_add(cl);

	return cl;
}

static struct clk_lookup *__clk_register_clkdev(struct clk_hw *hw,
                                                const char *con_id,
                                                const char *dev_id, ...)
{
	struct clk_lookup *cl;
	va_list ap;

	va_start(ap, dev_id);
	cl = vclkdev_create(hw, con_id, dev_id, ap);
	va_end(ap);

	return cl;
}

static int do_clk_register_clkdev(struct clk_hw *hw,
        struct clk_lookup **cl, const char *con_id, const char *dev_id)
{
	if (IS_ERR(hw))
		return PTR_ERR(hw);
	/*
	 * Since dev_id can be NULL, and NULL is handled specially, we must
	 * pass it as either a NULL format string, or with "%s".
	 */
	if (dev_id)
		*cl = __clk_register_clkdev(hw, con_id, "%s", dev_id);
	else
		*cl = __clk_register_clkdev(hw, con_id, NULL);

	return *cl ? 0 : -RT_ENOMEM;
}

/**
 * clk_hw_register_clkdev - register one clock lookup for a struct clk_hw
 * @hw: struct clk_hw to associate with all clk_lookups
 * @con_id: connection ID string on device
 * @dev_id: format string describing device name
 *
 * con_id or dev_id may be NULL as a wildcard, just as in the rest of
 * clkdev.
 *
 * To make things easier for mass registration, we detect error clk_hws
 * from a previous clk_hw_register_*() call, and return the error code for
 * those.  This is to permit this function to be called immediately
 * after clk_hw_register_*().
 */
int clk_hw_register_clkdev(struct clk_hw *hw, const char *con_id,
        const char *dev_id)
{
	struct clk_lookup *cl;

	return do_clk_register_clkdev(hw, &cl, con_id, dev_id);
}

