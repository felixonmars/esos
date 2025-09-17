/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Interrupt management.
 */

#ifndef FWK_INTERRUPT_H
#define FWK_INTERRUPT_H

#include <rtconfig.h>
#include <rthw.h>
#include <fwk_macros.h>
#include <fwk_status.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

/*!
 * \addtogroup GroupLibFramework Framework
 * \{
 */

/*!
 * \defgroup GroupInterrupt Interrupt Management
 * \{
 */

/*!
 * \brief Non-maskable interrupt identifier.
 *
 * \note This identifier can only be used in specific functions of this API.
 *      Please refer to the individual function documentation for more details.
 */
#define FWK_INTERRUPT_NMI UINT_MAX

/*!
 * \brief Non-existing interrupt identifier.
 *
 * \details Non-existing interrupt identifier to be used by configuration data
 *      when an interrupt identifier is expected but the system does not support
 *      it.
 *
 * \note This identifier cannot be used with the Interrupt Management component
 *      API. Its intention is to provide a standard way to describe an
 *      unconnected (or invalid) IRQ to the drivers.
 */
#define FWK_INTERRUPT_NONE (UINT_MAX - 1)

/*!
 * \brief Exception identifier.
 *
 * \details Exception interrupt identifier.
 *
 * \note This identifier can only be used in specific functions of this API.
 *      Please refer to the individual function documentation for more details.
 */
#define FWK_INTERRUPT_EXCEPTION (UINT_MAX - 2)

/*!
 * \brief Enable interrupts.
 *
 * \param flags Value returned by fwk_interrupt_global_disable.
 */
inline static void fwk_interrupt_global_enable(unsigned int flags)
{
    rt_hw_interrupt_enable(flags);
}

/*!
 * \brief Disable interrupts.
 *
 * \retval Opaque value to be passed when enabling.
 */
inline static unsigned int fwk_interrupt_global_disable(void)
{
    return rt_hw_interrupt_disable();
}

/*!
 * \brief Test whether an interrupt is enabled.
 *
 * \param interrupt Interrupt number.
 * \param [out] enabled \c true if the interrupt is enabled, else \c false.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_is_enabled(unsigned int interrupt, bool *enabled);

/*!
 * \brief Enable interrupt.
 *
 * \param interrupt Interrupt number.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_enable(unsigned int interrupt);

/*!
 * \brief Disable interrupt.
 *
 * \param interrupt Interrupt number.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_disable(unsigned int interrupt);

/*!
 * \brief Check if an interrupt is pending.
 *
 * \param interrupt Interrupt number.
 * \param [out] pending \c true if the interrupt is pending, else \c false.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_is_pending(unsigned int interrupt, bool *pending);

/*!
 * \brief Change interrupt status to pending.
 *
 * \param interrupt Interrupt number.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_set_pending(unsigned int interrupt);

/*!
 * \brief Clear the interrupt pending status.
 *
 * \param interrupt Interrupt number.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_clear_pending(unsigned int interrupt);

/*!
 * \brief Assign an interrupt service routine that receives a parameter to an
 *     interrupt.
 *
 * \param interrupt Interrupt number. This function accepts FWK_INTERRUPT_NMI as
 *      an interrupt number.
 * \param isr Pointer to the interrupt service routine function.
 * \param param Parameter that should be passed to the isr when it is called.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_set_isr_param(unsigned int interrupt,
                                void (*isr)(int interrupt, void *param),
                                void *param);

/*!
 * \brief Check if in interrupt context.
 *
 * \retval true if in an interrupt context.
 * \retval false not in an interrupt context.
 */
bool fwk_is_interrupt_context(void);

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* FWK_INTERRUPT_H */
