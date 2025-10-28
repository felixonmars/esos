#include <rtconfig.h>
#include "riscv_encoding.h"

#if defined(SOC_SPACEMIT_K1_X)
.section .data
#elif defined(SOC_SPACEMIT_K3)
.section .dtb_table
#endif

.global __irf_start
__irf_start:
#ifdef SOC_SPACEMIT_K1_X
.incbin "./platform/n308/k1-x/dts/k1-x_deb1.dtb"
#endif
#ifdef SOC_SPACEMIT_K3_CORE0
.incbin "./platform/rt24/k3_core0/dts/k3_deb1.dtb"
#endif
#ifdef SOC_SPACEMIT_K3_CORE1
.incbin "./platform/rt24/k3_core1/dts/k3_deb1.dtb"
#endif
.global __irf_end
__irf_end:

