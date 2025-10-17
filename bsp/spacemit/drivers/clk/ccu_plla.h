/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CCU_PLLA_H_
#define _CCU_PLLA_H_

#include <rtthread.h>
#include <rtdevice.h>
#include "ccu-common.h"

struct ccu_plla_rate_tbl {
	rt_uint64_t rate;
	rt_uint32_t swcr1;
	rt_uint32_t swcr2;
	rt_uint32_t swcr3;
};

struct ccu_plla_config {
	struct ccu_plla_rate_tbl * rate_tbl;
	rt_uint32_t tbl_size;
	rt_uint32_t *lock_base;
	rt_uint32_t reg_lock;
	rt_uint32_t lock_enable_bit;
};

#define PLLA_RATE(_rate, _swcr1, _swcr2, _swcr3) \
	{									\
		.rate	= _rate,						\
		.swcr1	= _swcr1,						\
		.swcr2	= _swcr2,						\
		.swcr3	= _swcr3,						\
	}

struct ccu_plla {
	struct ccu_plla_config	pll;
	struct ccu_common	common;
};

#define _SPACEMIT_CCU_PLLA_CONFIG(_table, _size, _reg_lock, _lock_enable_bit)	\
	{						\
		.rate_tbl	= (struct ccu_plla_rate_tbl *)_table,			\
		.tbl_size	= _size,			\
		.reg_lock 	= _reg_lock,        \
		.lock_enable_bit	= _lock_enable_bit,			\
	}

#define SPACEMIT_CCU_PLLA(_struct, _name, _table, _size,	\
						 _base_type, _reg_ctrl, _reg_sel, _reg_xtc,\
						 _reg_lock, _lock_enable_bit, _is_pll,	\
						 _flags)				\
	struct ccu_plla _struct = {					\
		.pll	= _SPACEMIT_CCU_PLLA_CONFIG(_table, _size, _reg_lock, _lock_enable_bit),	\
		.common = { 					\
			.reg_ctrl		= _reg_ctrl, 			\
			.reg_sel		= _reg_sel, 			\
			.reg_xtc		= _reg_xtc, 			\
			.base_type		= _base_type,	   \
			.is_pll 		= _is_pll,       \
			.hw.init	= CLK_HW_INIT_NO_PARENT(_name,	\
								  &ccu_plla_ops, \
								  _flags),	\
		}							\
	}


static inline struct ccu_plla *hw_to_ccu_plla(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return rt_container_of(common, struct ccu_plla, common);
}

extern const struct clk_ops ccu_plla_ops;

#endif
