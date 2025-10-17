#ifndef __RT_THREAD_CLK_PROVIDER_H__
#define __RT_THREAD_CLK_PROVIDER_H__

#include <rtdevice.h>

#undef ARRAY_SIZE
#define ARRAY_SIZE(ar)     (sizeof(ar)/sizeof(ar[0]))

/*
 * flags used across common struct clk.  these flags should only affect the
 * top-level framework.  custom flags for dealing with hardware specifics
 * belong in struct clk_foo
 *
 * Please update clk_flags[] in drivers/clk/clk.c when making changes here!
 */
#define CLK_SET_RATE_GATE       (1 << 0) /* must be gated across rate change */
#define CLK_SET_PARENT_GATE     (1 << 1) /* must be gated across re-parent */
#define CLK_SET_RATE_PARENT     (1 << 2) /* propagate rate change up one level */
#define CLK_IGNORE_UNUSED       (1 << 3) /* do not gate even if unused */
                                /* unused */
                                /* unused */
#define CLK_GET_RATE_NOCACHE    (1 << 6) /* do not use the cached clk rate */
#define CLK_SET_RATE_NO_REPARENT (1 << 7) /* don't re-parent on rate change */
#define CLK_GET_ACCURACY_NOCACHE (1 << 8) /* do not use the cached clk accuracy */
#define CLK_RECALC_NEW_RATES    (1 << 9) /* recalc rates after notifications */
#define CLK_SET_RATE_UNGATE     (1 << 10) /* clock needs to run to set rate */
#define CLK_IS_CRITICAL         (1 << 11) /* do not gate, ever */
/* parents need enable during gate/ungate, set rate and re-parent */
#define CLK_OPS_PARENT_ENABLE   (1 << 12)
/* duty cycle call may be forwarded to the parent clock */
#define CLK_DUTY_CYCLE_PARENT   (1 << 13)

struct clk;
struct clk_hw;
struct clk_core;

struct clk_onecell_data {
	struct clk **clks;
	rt_uint32_t clk_num;
};

struct clk_hw_onecell_data {
	rt_uint32_t num;
#if defined(SOC_SPACEMIT_K3)
	struct clk_hw *hws[168];
#elif defined(SOC_SPACEMIT_K1_X)
	struct clk_hw *hws[128];
#else
	struct clk_hw *hws[128];
#endif
};

/**
 * struct clk_rate_request - Structure encoding the clk constraints that
 * a clock user might require.
 *
 * Should be initialized by calling clk_hw_init_rate_request().
 *
 * @core:               Pointer to the struct clk_core affected by this request
 * @rate:               Requested clock rate. This field will be adjusted by
 *                      clock drivers according to hardware capabilities.
 * @min_rate:           Minimum rate imposed by clk users.
 * @max_rate:           Maximum rate imposed by clk users.
 * @best_parent_rate:   The best parent rate a parent can provide to fulfill the
 *                      requested constraints.
 * @best_parent_hw:     The most appropriate parent clock that fulfills the
 *                      requested constraints.
 *
 */
struct clk_rate_request {
	struct clk_core *core;
	rt_uint64_t rate;
	rt_uint64_t min_rate;
	rt_uint64_t max_rate;
	rt_uint64_t best_parent_rate;
	struct clk_hw *best_parent_hw;
};

/**
 * struct clk_parent_data - clk parent information
 * @hw: parent clk_hw pointer (used for clk providers with internal clks)
 * @fw_name: parent name local to provider registering clk
 * @name: globally unique parent name (used as a fallback)
 * @index: parent index local to provider registering clk (if @fw_name absent)
 */
struct clk_parent_data {
	const struct clk_hw     *hw;
	const char              *fw_name;
	const char              *name;
	int                     index;
};

/**
 * struct clk_init_data - holds init data that's common to all clocks and is
 * shared between the clock provider and the common clock framework.
 *
 * @name: clock name
 * @ops: operations this clock supports
 * @parent_names: array of string names for all possible parents
 * @parent_data: array of parent data for all possible parents (when some
 *               parents are external to the clk controller)
 * @parent_hws: array of pointers to all possible parents (when all parents
 *              are internal to the clk controller)
 * @num_parents: number of possible parents
 * @flags: framework-level hints and quirks
 */
struct clk_init_data {
	const char              *name;
	const struct clk_ops    *ops;
	/* Only one of the following three should be assigned */
	const char              * const *parent_names;
	const struct clk_parent_data    *parent_data;
	const struct clk_hw             **parent_hws;
	unsigned char                   num_parents;
	rt_uint64_t           flags;
};

/**
 * struct clk_hw - handle for traversing from a struct clk to its corresponding
 * hardware-specific structure.  struct clk_hw should be declared within struct
 * clk_foo and then referenced by the struct clk instance that uses struct
 * clk_foo's clk_ops
 *
 * @core: pointer to the struct clk_core instance that points back to this
 * struct clk_hw instance
 *
 * @clk: pointer to the per-user struct clk instance that can be used to call
 * into the clk API
 *
 * @init: pointer to struct clk_init_data that contains the init data shared
 * with the common clock framework. This pointer will be set to NULL once
 * a clk_register() variant is called on this clk_hw pointer.
 */
struct clk_hw {
	struct clk_core *core;
	struct clk *clk;
	const struct clk_init_data *init;
};

struct clk_div_table {
	rt_uint32_t    val;
	rt_uint32_t    div;
};

/**
 * struct clk_divider - adjustable divider clock
 *
 * @hw:         handle between common and hardware-specific interfaces
 * @reg:        register containing the divider
 * @shift:      shift to the divider bit field
 * @width:      width of the divider bit field
 * @table:      array of value/divider pairs, last entry should have div = 0
 * @lock:       register lock
 *
 * Clock with an adjustable divider affecting its output frequency.  Implements
 * .recalc_rate, .set_rate and .round_rate
 *
 * @flags:
 * CLK_DIVIDER_ONE_BASED - by default the divisor is the value read from the
 *      register plus one.  If CLK_DIVIDER_ONE_BASED is set then the divider is
 *      the raw value read from the register, with the value of zero considered
 *      invalid, unless CLK_DIVIDER_ALLOW_ZERO is set.
 * CLK_DIVIDER_POWER_OF_TWO - clock divisor is 2 raised to the value read from
 *      the hardware register
 * CLK_DIVIDER_ALLOW_ZERO - Allow zero divisors.  For dividers which have
 *      CLK_DIVIDER_ONE_BASED set, it is possible to end up with a zero divisor.
 *      Some hardware implementations gracefully handle this case and allow a
 *      zero divisor by not modifying their input clock
 *      (divide by one / bypass).
 * CLK_DIVIDER_HIWORD_MASK - The divider settings are only in lower 16-bit
 *      of this register, and mask of divider bits are in higher 16-bit of this
 *      register.  While setting the divider bits, higher 16-bit should also be
 *      updated to indicate changing divider bits.
 * CLK_DIVIDER_ROUND_CLOSEST - Makes the best calculated divider to be rounded
 *      to the closest integer instead of the up one.
 * CLK_DIVIDER_READ_ONLY - The divider settings are preconfigured and should
 *      not be changed by the clock framework.
 * CLK_DIVIDER_MAX_AT_ZERO - For dividers which are like CLK_DIVIDER_ONE_BASED
 *      except when the value read from the register is zero, the divisor is
 *      2^width of the field.
 * CLK_DIVIDER_BIG_ENDIAN - By default little endian register accesses are used
 *      for the divider register.  Setting this flag makes the register accesses
 *      big endian.
 */
struct clk_divider {
        struct clk_hw   hw;
        void		*reg;
        unsigned char	shift;
        unsigned char	width;
        unsigned char	flags;
        const struct clk_div_table	*table;
	rt_base_t	*lock;
};

/*
 * DOC: Basic clock implementations common to many platforms
 *
 * Each basic clock hardware type is comprised of a structure describing the
 * clock hardware, implementations of the relevant callbacks in struct clk_ops,
 * unique flags for that hardware type, a registration function and an
 * alternative macro for static initialization
 */

/**
 * struct clk_fixed_rate - fixed-rate clock
 * @hw:         handle between common and hardware-specific interfaces
 * @fixed_rate: constant frequency of clock
 * @fixed_accuracy: constant accuracy of clock in ppb (parts per billion)
 * @flags:      hardware specific flags
 *
 * Flags:
 * * CLK_FIXED_RATE_PARENT_ACCURACY - Use the accuracy of the parent clk
 *                                    instead of what's set in @fixed_accuracy.
 */
struct clk_fixed_rate {
	struct          clk_hw hw;
	rt_uint64_t   fixed_rate;
	/* rt_uint64_t   fixed_accuracy; */
	rt_uint64_t   flags;
};

#define BITS_PER_LONG 32

#ifndef GENMASK
#define GENMASK(h, l) \
        (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#endif

#define __KERNEL_DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define DIV_ROUND_UP __KERNEL_DIV_ROUND_UP

/**
 * do_div - returns 2 values: calculate remainder and update new dividend
 * @n: uint64_t dividend (will be updated)
 * @base: uint32_t divisor
 *
 * Summary:
 * ``uint32_t remainder = n % base;``
 * ``n = n / base;``
 *
 * Return: (uint32_t)remainder
 *
 * NOTE: macro parameter @n is evaluated multiple times,
 * beware of side effects!
 */
# define do_div(n,base) ({                                      \
        rt_uint32_t __base = (base);                               \
        rt_uint32_t __rem;                                         \
        __rem = ((rt_uint64_t)(n)) % __base;                       \
        (n) = ((rt_uint64_t)(n)) / __base;                         \
        __rem;                                                  \
 })

#define DIV_ROUND_DOWN_ULL(ll, d) \
        ({ rt_uint64_t _tmp = (ll); do_div(_tmp, d); _tmp; })

#define DIV_ROUND_UP_ULL(ll, d) \
        DIV_ROUND_DOWN_ULL((rt_uint64_t)(ll) + (d) - 1, (d))

#define clk_div_mask(width)     ((1 << (width)) - 1)
#define to_clk_divider(_hw) rt_container_of(_hw, struct clk_divider, hw)

#define CLK_DIVIDER_ONE_BASED           (1 << 0)
#define CLK_DIVIDER_POWER_OF_TWO        (1 << 1)
#define CLK_DIVIDER_ALLOW_ZERO          (1 << 2)
#define CLK_DIVIDER_HIWORD_MASK         (1 << 3)
#define CLK_DIVIDER_ROUND_CLOSEST       (1 << 4)
#define CLK_DIVIDER_READ_ONLY           (1 << 5)
#define CLK_DIVIDER_MAX_AT_ZERO         (1 << 6)
#define CLK_DIVIDER_BIG_ENDIAN          (1 << 7)

/**
 * struct clk_ops -  Callback operations for hardware clocks; these are to
 * be provided by the clock implementation, and will be called by drivers
 * through the clk_* api.
 *
 * @prepare:    Prepare the clock for enabling. This must not return until
 *              the clock is fully prepared, and it's safe to call clk_enable.
 *              This callback is intended to allow clock implementations to
 *              do any initialisation that may sleep. Called with
 *              prepare_lock held.
 *
 * @unprepare:  Release the clock from its prepared state. This will typically
 *              undo any work done in the @prepare callback. Called with
 *              prepare_lock held.
 *
 * @is_prepared: Queries the hardware to determine if the clock is prepared.
 *              This function is allowed to sleep. Optional, if this op is not
 *              set then the prepare count will be used.
 *
 * @unprepare_unused: Unprepare the clock atomically.  Only called from
 *              clk_disable_unused for prepare clocks with special needs.
 *              Called with prepare mutex held. This function may sleep.
 *
 * @enable:     Enable the clock atomically. This must not return until the
 *              clock is generating a valid clock signal, usable by consumer
 *              devices. Called with enable_lock held. This function must not
 *              sleep.
 *
 * @disable:    Disable the clock atomically. Called with enable_lock held.
 *              This function must not sleep.
 *
 * @is_enabled: Queries the hardware to determine if the clock is enabled.
 *              This function must not sleep. Optional, if this op is not
 *              set then the enable count will be used.
 *
 * @disable_unused: Disable the clock atomically.  Only called from
 *              clk_disable_unused for gate clocks with special needs.
 *              Called with enable_lock held.  This function must not
 *              sleep.
 *
 * @save_context: Save the context of the clock in prepration for poweroff.
 *
 * @restore_context: Restore the context of the clock after a restoration
 *              of power.
 *
 * @recalc_rate: Recalculate the rate of this clock, by querying hardware. The
 *              parent rate is an input parameter.  It is up to the caller to
 *              ensure that the prepare_mutex is held across this call. If the
 *              driver cannot figure out a rate for this clock, it must return
 *              0. Returns the calculated rate. Optional, but recommended - if
 *              this op is not set then clock rate will be initialized to 0.
 *
 * @round_rate: Given a target rate as input, returns the closest rate actually
 *              supported by the clock. The parent rate is an input/output
 *              parameter.
 *
 * @determine_rate: Given a target rate as input, returns the closest rate
 *              actually supported by the clock, and optionally the parent clock
 *              that should be used to provide the clock rate.
 *
 * @set_parent: Change the input source of this clock; for clocks with multiple
 *              possible parents specify a new parent by passing in the index
 *              as a u8 corresponding to the parent in either the .parent_names
 *              or .parents arrays.  This function in affect translates an
 *              array index into the value programmed into the hardware.
 *              Returns 0 on success, -EERROR otherwise.
 *
 * @get_parent: Queries the hardware to determine the parent of a clock.  The
 *              return value is a u8 which specifies the index corresponding to
 *              the parent clock.  This index can be applied to either the
 *              .parent_names or .parents arrays.  In short, this function
 *              translates the parent value read from hardware into an array
 *              index.  Currently only called when the clock is initialized by
 *              __clk_init.  This callback is mandatory for clocks with
 *              multiple parents.  It is optional (and unnecessary) for clocks
 *              with 0 or 1 parents.
 *
 * @set_rate:   Change the rate of this clock. The requested rate is specified
 *              by the second argument, which should typically be the return
 *              of .round_rate call.  The third argument gives the parent rate
 *              which is likely helpful for most .set_rate implementation.
 *              Returns 0 on success, -EERROR otherwise.
 *
 * @set_rate_and_parent: Change the rate and the parent of this clock. The
 *              requested rate is specified by the second argument, which
 *              should typically be the return of .round_rate call.  The
 *              third argument gives the parent rate which is likely helpful
 *              for most .set_rate_and_parent implementation. The fourth
 *              argument gives the parent index. This callback is optional (and
 *              unnecessary) for clocks with 0 or 1 parents as well as
 *              for clocks that can tolerate switching the rate and the parent
 *              separately via calls to .set_parent and .set_rate.
 *              Returns 0 on success, -EERROR otherwise.
 *
 * @recalc_accuracy: Recalculate the accuracy of this clock. The clock accuracy
 *              is expressed in ppb (parts per billion). The parent accuracy is
 *              an input parameter.
 *              Returns the calculated accuracy.  Optional - if this op is not
 *              set then clock accuracy will be initialized to parent accuracy
 *              or 0 (perfect clock) if clock has no parent.
 *
 * @init:       Perform platform-specific initialization magic.
 *              This is not used by any of the basic clock types.
 *              This callback exist for HW which needs to perform some
 *              initialisation magic for CCF to get an accurate view of the
 *              clock. It may also be used dynamic resource allocation is
 *              required. It shall not used to deal with clock parameters,
 *              such as rate or parents.
 *              Returns 0 on success, -EERROR otherwise.
 *
 * @terminate:  Free any resource allocated by init.
 *
 *
 * The clk_enable/clk_disable and clk_prepare/clk_unprepare pairs allow
 * implementations to split any work between atomic (enable) and sleepable
 * (prepare) contexts.  If enabling a clock requires code that might sleep,
 * this must be done in clk_prepare.  Clock enable code that will never be
 * called in a sleepable context may be implemented in clk_enable.
 *
 * Typically, drivers will call clk_prepare when a clock may be needed later
 * (eg. when a device is opened), and clk_enable when the clock is actually
 * required (eg. from an interrupt). Note that clk_prepare MUST have been
 * called before clk_enable.
 */

struct clk_ops {
        int             (*prepare)(struct clk_hw *hw);
        void            (*unprepare)(struct clk_hw *hw);
        int             (*is_prepared)(struct clk_hw *hw);
        void            (*unprepare_unused)(struct clk_hw *hw);
        int             (*enable)(struct clk_hw *hw);
        void            (*disable)(struct clk_hw *hw);
        int             (*is_enabled)(struct clk_hw *hw);
        void            (*disable_unused)(struct clk_hw *hw);
        int             (*save_context)(struct clk_hw *hw);
        void            (*restore_context)(struct clk_hw *hw);
        rt_uint64_t   (*recalc_rate)(struct clk_hw *hw,
                                        rt_uint64_t parent_rate);
        long            (*round_rate)(struct clk_hw *hw, rt_uint64_t rate,
                                        rt_uint64_t *parent_rate);
        int             (*determine_rate)(struct clk_hw *hw,
                                          struct clk_rate_request *req);
        int             (*set_parent)(struct clk_hw *hw, unsigned char index);
        unsigned char   (*get_parent)(struct clk_hw *hw);
        int             (*set_rate)(struct clk_hw *hw, rt_uint64_t rate,
                                    rt_uint64_t parent_rate);
        int             (*set_rate_and_parent)(struct clk_hw *hw,
                                    rt_uint64_t rate,
                                    rt_uint64_t parent_rate, unsigned char index);
        int             (*init)(struct clk_hw *hw);
        void            (*terminate)(struct clk_hw *hw);
};


#define CLK_HW_INIT_NO_PARENT(_name, _ops, _flags)      \
        (&(struct clk_init_data) {                      \
                .flags          = _flags,               \
                .name           = _name,                \
                .parent_names   = NULL,                 \
                .num_parents    = 0,                    \
                .ops            = _ops,                 \
        })

#define CLK_HW_INIT(_name, _parent, _ops, _flags)               \
        (&(struct clk_init_data) {                              \
                .flags          = _flags,                       \
                .name           = _name,                        \
                .parent_names   = (const char *[]) { _parent }, \
                .num_parents    = 1,                            \
                .ops            = _ops,                         \
        })

#define CLK_HW_INIT_PARENTS(_name, _parents, _ops, _flags)      \
        (&(struct clk_init_data) {                              \
                .flags          = _flags,                       \
                .name           = _name,                        \
                .parent_names   = _parents,                     \
                .num_parents    = ARRAY_SIZE(_parents),         \
                .ops            = _ops,                         \
        })

const char *__clk_get_name(const struct clk *clk);
struct clk_hw *__clk_get_hw(struct clk *clk);
const char *clk_hw_get_name(const struct clk_hw *hw);
rt_uint64_t clk_hw_get_rate(const struct clk_hw *hw);
void clk_disable_unprepare(struct clk *clk);
int clk_prepare_enable(struct clk *clk);
struct clk *of_clk_get_by_name(struct dtb_node *np, const char *name);
struct clk *of_clk_get(struct dtb_node *np, int index);
int of_clk_add_hw_provider(struct dtb_node *np,
		struct clk_hw *(*get)(struct fdt_phandle_args *clkspec, void *data),
		void *data);
struct clk_hw *
of_clk_hw_onecell_get(struct fdt_phandle_args *clkspec, void *data);
struct clk * of_clk_hw_register(struct dtb_node *node, struct clk_hw *hw);
rt_uint64_t divider_recalc_rate(struct clk_hw *hw, rt_uint64_t parent_rate,
				rt_uint32_t val,
				const struct clk_div_table *table,
				rt_uint64_t flags, rt_uint64_t width);
struct clk_hw *
clk_hw_get_parent_by_index(const struct clk_hw *hw, rt_uint32_t index);
rt_uint64_t clk_get_rate(struct clk *clk);
int clk_set_rate(struct clk *clk, rt_uint64_t rate);
struct clk *clk_hw_get_clk(struct clk_hw *hw, const char *con_id);
int clk_hw_set_parent(struct clk_hw *hw, struct clk_hw *parent);
rt_uint32_t clk_hw_get_num_parents(const struct clk_hw *hw);

struct clk_hw *__clk_hw_register_fixed_rate(/*struct device *dev, */
		struct dtb_node *np, const char *name,
		const char *parent_name, const struct clk_hw *parent_hw,
		const struct clk_parent_data *parent_data, rt_uint64_t flags,
		rt_uint64_t fixed_rate/*, rt_uint64_t fixed_accuracy */,
		rt_uint64_t clk_fixed_flags/*, bool devm */);

#endif /* __RT_THREAD_CLK_PROVIDER_H__ */
