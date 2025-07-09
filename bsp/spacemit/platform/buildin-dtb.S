#include <rtconfig.h>
#include "riscv_encoding.h"

.section    .data

.global __irf_start
__irf_start:
#ifdef SOC_SPACEMIT_K1_X
.incbin "./platform/n308/k1-x/dts/k1-x_deb1.dtb"
#endif
#ifdef SOC_SPACEMIT_K3
.incbin "./platform/rt24/k3/dts/k3_deb1.dtb"
#endif
.global __irf_end
__irf_end:

