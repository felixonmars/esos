#ifndef __RT24_CLINT_H__
#define __RT24_CLINT_H__

#include <rtthread.h>
#include <rtdef.h>
#include <rthw.h>
#include <riscv-ops.h>
#include <stdint.h>

#define SysTimer_BASE	0xe4000000		/*!< SysTick Base Address */
#define SysTimer_Mtimer_Pos			(0xbff8)
#define SysTimer_MtimerCmp_Pos			(0x4000)
#define SysTimer_SHIFT_PER_TARGET		27

/**
 * \brief  Get system timer load value
 * \details
 * This function get the system timer current value in MTIMER register.
 * \return  current value(64bit) of system timer MTIMER register.
 * \remarks
 * - Load value is 64bits wide.
 * - \ref SysTimer_SetLoadValue
 */
static inline rt_uint64_t SysTimer_GetLoadValue(void)
{
	unsigned int hartid = read_csr(mhartid);
	volatile rt_uint64_t *ptr = (rt_uint64_t *)(uintptr_t)(SysTimer_BASE + SysTimer_Mtimer_Pos + (hartid << SysTimer_SHIFT_PER_TARGET));

	return *ptr;
}

/**
 * \brief  Set system timer compare value in machined mode
 * \details
 * This function set the system Timer compare value in MTIMERCMP register.
 * \param [in]  value   compare value to set system timer MTIMERCMP register.
 * \remarks
 * - Compare value is 64bits wide.
 * - If compare value is larger than current value timer interrupt generate.
 * - Modify the load value or compare value less to clear the interrupt.
 * - __get_hart_id function can only be accessed in machined mode, or else exception will occur.
 * - \ref SysTimer_GetCompareValue
 */
static inline void SysTimer_SetCompareValue(rt_uint64_t value)
{
	unsigned long hartid = read_csr(mhartid);
	volatile rt_uint64_t *ptr = (rt_uint64_t *)(SysTimer_BASE + SysTimer_MtimerCmp_Pos + (hartid << SysTimer_SHIFT_PER_TARGET));

	*ptr = value;
}

#endif /* __RT24_CLINT_H__ */
