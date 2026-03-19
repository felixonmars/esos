#include <rtconfig.h>

.section .data
.global __esos_lite_start
__esos_lite_start:
.incbin "../../binary/esos_lite.bin"
.global __esos_lite_end
__esos_lite_end:
