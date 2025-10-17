/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CCU_DDN_H_
#define _CCU_DDN_H_

#include <rtthread.h>
#include <rtdevice.h>
#include <riscv-ops.h>
#include "ccu-common.h"

struct ccu_ddn_tbl {
	rt_uint32_t num;
	rt_uint32_t den;
};

struct ccu_ddn_info {
	rt_uint32_t factor;
	rt_uint32_t num_mask;
	rt_uint32_t den_mask;
	rt_uint32_t num_shift;
	rt_uint32_t den_shift;
};

struct ccu_ddn_config {
	struct ccu_ddn_info * info;
	struct ccu_ddn_tbl * tbl;
	rt_uint32_t tbl_size;
};

#define PLL_DDN_TBL(_num, _den)		\
	{						\
		.num	=	(_num),		\
		.den	=	(_den),			\
	}

struct ccu_ddn {
	rt_uint32_t gate;
	struct ccu_ddn_config  ddn;
	struct ccu_common	common;
};

#define _SPACEMIT_CCU_DDN_CONFIG(_info, _table, _size)	\
	{						\
		.info	= (struct ccu_ddn_info *)_info,			\
		.tbl	= (struct ccu_ddn_tbl *)_table,			\
		.tbl_size	= _size,			\
	}

#define SPACEMIT_CCU_DDN(_struct, _name, _parent, _info, _table, _size,	\
						 _base_type, _reg_ctrl, 	\
						 _flags)				\
	struct ccu_ddn _struct = {					\
		.ddn	= _SPACEMIT_CCU_DDN_CONFIG(_info, _table, _size),	\
		.common = { 					\
			.reg_ctrl		= _reg_ctrl, 			\
			.base_type		= _base_type,		\
			.name			= _name,		\
			.hw.init	= CLK_HW_INIT(_name,	\
								  _parent, \
								  &ccu_ddn_ops, \
								  _flags),	\
		}							\
	}

#define SPACEMIT_CCU_DDN_GATE(_struct, _name, _parent, _info, _table, _size,	\
							 _base_type, _reg_ddn, __reg_gate, _gate_mask, \
							 _flags)				\
	struct ccu_ddn _struct = {					\
		.gate	= _gate_mask,	 \
		.ddn	= _SPACEMIT_CCU_DDN_CONFIG(_info, _table, _size),	\
		.common = { 					\
			.reg_ctrl		= _reg_ddn,			\
			.reg_sel		= __reg_gate,			\
			.base_type		= _base_type,		\
			.name			= _name,		\
			.hw.init	= CLK_HW_INIT(_name,	\
								  _parent, \
								  &ccu_ddn_ops, \
								  _flags),	\
		}							\
	}

static inline struct ccu_ddn *hw_to_ccu_ddn(struct clk_hw *hw)
{
	struct ccu_common *common = hw_to_ccu_common(hw);

	return rt_container_of(common, struct ccu_ddn, common);
}

extern const struct clk_ops ccu_ddn_ops;

#endif
