#include <rthw.h>
#include <rtdef.h>
#include <riscv-ops.h>
#include <clint.h>
#include <spacemit_sdk_soc.h>
#include <register_defination.h>
#include "riscv_encoding.h"

#define SYSTICK_TICK_CONST	(SOC_TIMER_FREQ / RT_TICK_PER_SECOND)

static volatile unsigned long tick_cycles = 0;

#if defined(BSP_TEST_RT) && defined(RT_USING_FINSH)

#define RT_TEST_NUMBER  500
volatile rt_uint64_t tick_last_cmp = 0;
volatile unsigned int tick_latency_count = 0;
volatile unsigned int tick_latency_max_count = 0;
unsigned long long tick_latency_buf[RT_TEST_NUMBER];

static int rt_tick_latency(void)
{
	int i;
	unsigned long long all = 0, lat_min = (unsigned long long)-1, lat_max = 0;

	tick_latency_count = 0;
	tick_latency_max_count = RT_TEST_NUMBER;

	rt_kprintf("Collecting %d tick latency samples...\n", RT_TEST_NUMBER);

	while (tick_latency_count < RT_TEST_NUMBER)
		rt_thread_delay(10);

	tick_latency_max_count = 0;

	for (i = 0; i < RT_TEST_NUMBER; i++) {
		all += tick_latency_buf[i];
		if (tick_latency_buf[i] < lat_min) lat_min = tick_latency_buf[i];
		if (tick_latency_buf[i] > lat_max) lat_max = tick_latency_buf[i];
	}

	rt_kprintf("Tick IRQ latency: avg=%lldns, min=%lldns, max=%lldns\n",
		all * 1000000000ULL / SOC_TIMER_FREQ / RT_TEST_NUMBER,
		lat_min * 1000000000ULL / SOC_TIMER_FREQ,
		lat_max * 1000000000ULL / SOC_TIMER_FREQ);

	return 0;
}

MSH_CMD_EXPORT(rt_tick_latency, "measure tick interrupt latency");

#endif

void rt_hw_tick_isr(void)
{
	rt_uint64_t value;

#if defined(BSP_TEST_RT) && defined(RT_USING_FINSH)

	if (tick_last_cmp != 0 && tick_latency_count < tick_latency_max_count) {
		tick_latency_buf[tick_latency_count] = SysTimer_GetLoadValue() - tick_last_cmp;
		tick_latency_count++;
	}
#endif

	rt_tick_increase();
	value = SysTimer_GetLoadValue() + tick_cycles;
	SysTimer_SetCompareValue(value);

#if defined(BSP_TEST_RT) && defined(RT_USING_FINSH)
	tick_last_cmp = value;
#endif
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
