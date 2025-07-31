/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SPACEMIT_REGISTER_DEFINATION_H__
#define __SPACEMIT_REGISTER_DEFINATION_H__

#include <rtdef.h>

#if defined(SOC_SPACEMIT_K1_X)
#include "./n308/k1-x/_register_defination.h"
#elif defined(SOC_SPACEMIT_K3_CORE0)
#include "./rt24/k3_core0/_register_defination.h"
#elif defined(SOC_SPACEMIT_K3_CORE1)
#include "./rt24/k3_core1/_register_defination.h"
#endif

#endif
