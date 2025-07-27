/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	config.h
 * @brief	Generated configuration settings for libmetal.
 */

#ifndef __METAL_CONFIG__H__
#define __METAL_CONFIG__H__
#include <rtconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Library major version number. */
#define METAL_VER_MAJOR		/* @PROJECT_VERSION_MAJOR@ */1

/** Library minor version number. */
#define METAL_VER_MINOR		/* @PROJECT_VERSION_MINOR@ */5

/** Library patch level. */
#define METAL_VER_PATCH		/* @PROJECT_VERSION_PATCH@ */0

/** Library version string. */
#define METAL_VER		/* "@PROJECT_VERSION@" */"1.5.0"

/** System type (linux, generic, ...). */
#define METAL_SYSTEM		/* "@PROJECT_SYSTEM@" */ "rtthread"
/* #define METAL_SYSTEM_@PROJECT_SYSTEM_UPPER@ */
#define METAL_SYSTEM_RTTHREAD

/** Processor type (arm, x86_64, ...). */
#define METAL_PROCESSOR		/* "@PROJECT_PROCESSOR@" */"riscv"
/* #define METAL_PROCESSOR_@PROJECT_PROCESSOR_UPPER@ */
#define METAL_PROCESSOR_RISCV

#if defined(SOC_SPACEMIT_K1_X)
/** Machine type (zynq, zynqmp, ...). */
#define METAL_MACHINE		/* "@PROJECT_MACHINE@" */ "k1x"
/* #define METAL_MACHINE_@PROJECT_MACHINE_UPPER@ */
#define METAL_MACHINE_K1X
#elif defined(SOC_SPACEMIT_K3)
/** Machine type (zynq, zynqmp, ...). */
#define METAL_MACHINE		/* "@PROJECT_MACHINE@" */ "k3"
/* #define METAL_MACHINE_@PROJECT_MACHINE_UPPER@ */
#define METAL_MACHINE_K3
#endif

/* #cmakedefine HAVE_STDATOMIC_H */
/* #cmakedefine HAVE_FUTEX_H */
/* #cmakedefine HAVE_PROCESSOR_ATOMIC_H */
/* #cmakedefine HAVE_PROCESSOR_CPU_H */
/* #define HAVE_PROCESSOR_ATOMIC_H */
#define HAVE_PROCESSOR_CPU_H

#ifdef __cplusplus
}
#endif

#endif /* __METAL_CONFIG__H__ */
