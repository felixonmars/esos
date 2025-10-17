/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CCU_SPACEMIT_COMMON_H_
#define _CCU_SPACEMIT_COMMON_H_

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#define BIT(x)		(1 << x)

enum ccu_base_type{
	BASE_TYPE_MPMU       = 0,
	BASE_TYPE_APMU       = 1,
	BASE_TYPE_APBC       = 2,
	BASE_TYPE_APBS       = 3,
	BASE_TYPE_CIU        = 4,
	BASE_TYPE_DCIU       = 5,
	BASE_TYPE_DDRC       = 6,
	BASE_TYPE_APBC2      = 7,
	BASE_TYPE_RCPU       = 8,
	BASE_TYPE_RCPU1      = 9,
	BASE_TYPE_RCPU2      = 10,
	BASE_TYPE_RCPU3      = 11,
	BASE_TYPE_RCPU4      = 12,
	BASE_TYPE_RCPU5      = 13,
	BASE_TYPE_RCPU6      = 14,
};

enum {
	CLK_DIV_TYPE_1REG_NOFC_V1 = 0,
	CLK_DIV_TYPE_1REG_FC_V2,
	CLK_DIV_TYPE_2REG_NOFC_V3,
	CLK_DIV_TYPE_2REG_FC_V4,
	CLK_DIV_TYPE_1REG_FC_DIV_V5,
	CLK_DIV_TYPE_1REG_FC_MUX_V6,
};

struct ccu_common {
	void *base;
	enum ccu_base_type base_type;
	rt_uint32_t	reg_type;
	rt_uint32_t	reg_ctrl;
	rt_uint32_t	reg_sel;
	rt_uint32_t	reg_xtc;
	rt_uint32_t	fc;
	bool	is_pll;
	const char		*name;
	const struct clk_ops	*ops;
	const char		* const *parent_names;
	rt_uint8_t num_parents;
	unsigned long	flags;
	struct rt_spinlock lock;
	struct clk_hw	hw;
};

struct spacemit_k1x_clk {
	void	*mpmu_base;
	void	*apmu_base;
	void	*apbc_base;
	void	*apbs_base;
	void	*ciu_base;
	void	*dciu_base;
	void	*ddrc_base;
	void	*apbc2_base;
	void	*rcpu_base;
	void	*rcpu1_base;
	void	*rcpu2_base;
	void	*rcpu3_base;
	void	*rcpu4_base;
	void	*rcpu5_base;
	void	*rcpu6_base;
};

struct clk_hw_table {
	char	*name;
	rt_uint32_t	clk_hw_id;
};

extern rt_ubase_t g_cru_lock;

static inline struct ccu_common *hw_to_ccu_common(struct clk_hw *hw)
{
	return rt_container_of(hw, struct ccu_common, hw);
}

#endif /* _CCU_SPACEMIT_COMMON_H_ */
