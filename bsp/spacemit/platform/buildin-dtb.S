#include <rtconfig.h>
#include "riscv_encoding.h"

.section .data

.global __irf_start
__irf_start:
#ifdef SOC_SPACEMIT_K1_X
.incbin "./platform/n308/k1-x_deb1.dtb"
#endif

#if defined(SOC_SPACEMIT_K3_CORE0_COM260) || defined(SOC_SPACEMIT_K3_CORE1_COM260)
.incbin "./platform/rt24/k3_com260.dtb"
#endif
#if defined(SOC_SPACEMIT_K3_CORE0_DEB1) || defined(SOC_SPACEMIT_K3_CORE1_DEB1)
.incbin "./platform/rt24/k3_deb1.dtb"
#endif
#if defined(SOC_SPACEMIT_K3_CORE0_EVB) || defined(SOC_SPACEMIT_K3_CORE1_EVB)
.incbin "./platform/rt24/k3_evb.dtb"
#endif
#if defined(SOC_SPACEMIT_K3_CORE0_EVB2_1) || defined(SOC_SPACEMIT_K3_CORE1_EVB2_1)
.incbin "./platform/rt24/k3_evb2_1.dtb"
#endif
#if defined(SOC_SPACEMIT_K3_CORE0_EVB2_2) || defined(SOC_SPACEMIT_K3_CORE1_EVB2_2)
.incbin "./platform/rt24/k3_evb2_2.dtb"
#endif
.global __irf_end
__irf_end:

