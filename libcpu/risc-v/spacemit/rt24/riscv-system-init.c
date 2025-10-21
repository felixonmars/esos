#include <rthw.h>
#include <rtdef.h>
#include <riscv-ops.h>
#include <clint.h>
#include <spacemit_sdk_soc.h>
#include <register_defination.h>
#include "riscv_encoding.h"

#define SYSTICK_TICK_CONST	(SOC_TIMER_FREQ / RT_TICK_PER_SECOND)

static volatile unsigned long tick_cycles = 0;

void rt_hw_tick_isr(void)
{
	rt_uint64_t value;

	rt_tick_increase();

	value = SysTimer_GetLoadValue() + tick_cycles;
	SysTimer_SetCompareValue(value);
}

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
	rt_uint64_t value;

	/* disable dcache */
	/* clear_csr(0x7c1, 0x1); */

	/* initilaze the interrupt */
	rt_hw_interrupt_init();

	/* initialize the timer */
	clear_csr(mie, MIP_MTIP);

	/* calculate the tick cycles */
	tick_cycles = SYSTICK_TICK_CONST;

	value = SysTimer_GetLoadValue() + tick_cycles;
	SysTimer_SetCompareValue(value);

	/* Enable the Timer bit & external bit in MIE */
	set_csr(mie, MIP_MTIP | MIP_MEIP);
}
