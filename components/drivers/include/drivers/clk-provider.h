#ifndef __RT_THREAD_CLK_PROVIDER_H__
#define __RT_THREAD_CLK_PROVIDER_H__

#include <rtdevice.h>

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
	unsigned int clk_num;
};

struct clk_hw_onecell_data {
	unsigned int num;
	struct clk_hw *hws[];
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
	unsigned long rate;
	unsigned long min_rate;
	unsigned long max_rate;
	unsigned long best_parent_rate;
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
	unsigned long           flags;
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
        unsigned long   (*recalc_rate)(struct clk_hw *hw,
                                        unsigned long parent_rate);
        long            (*round_rate)(struct clk_hw *hw, unsigned long rate,
                                        unsigned long *parent_rate);
        int             (*determine_rate)(struct clk_hw *hw,
                                          struct clk_rate_request *req);
        int             (*set_parent)(struct clk_hw *hw, unsigned char index);
        unsigned char   (*get_parent)(struct clk_hw *hw);
        int             (*set_rate)(struct clk_hw *hw, unsigned long rate,
                                    unsigned long parent_rate);
        int             (*set_rate_and_parent)(struct clk_hw *hw,
                                    unsigned long rate,
                                    unsigned long parent_rate, unsigned char index);
        int             (*init)(struct clk_hw *hw);
        void            (*terminate)(struct clk_hw *hw);
};

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

#endif /* __RT_THREAD_CLK_PROVIDER_H__ */
