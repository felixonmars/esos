/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_JUNO_PPU_H
#define MOD_JUNO_PPU_H

#include <mod_power_domain.h>

#include <fwk_id.h>
#include <fwk_module_idx.h>

#include <stdint.h>

/*!
 * \addtogroup GroupJunoModule Juno Product Modules
 * \{
 */

/*!
 * \defgroup GroupJunoPPU Juno PPU
 * \{
 */

/*!
 * \brief API indices.
 */
enum mod_juno_ppu_api_idx {
    /*!
     * \brief Power domain driver API index.
     */
    MOD_JUNO_PPU_API_IDX_PD,

    /*!
     * \brief Number of defined APIs.
     */
    MOD_JUNO_PPU_API_COUNT,
};

/*!
 * \brief Power Domain driver API identifier.
 */
static const fwk_id_t mod_juno_ppu_api_id_pd =
    FWK_ID_API_INIT(FWK_MODULE_IDX_JUNO_PPU, MOD_JUNO_PPU_API_IDX_PD);

    /*!
 * \brief Module configuration.
 */
struct mod_juno_ppu_config {
    /*!
     * \brief Identifier of the timer alarm.
     *
     * \details Used for polling a core PPU state during system suspend.
     */
    fwk_id_t timer_alarm_id;
};

/*!
 * \brief Element configuration.
 *
 * \details This is the configuration struct for an individal PPU.
 */
struct mod_juno_ppu_element_config {
    /*!
     * \brief Base address for the PPU registers.
     */
    uintptr_t core_reg_base;

    /*!
     * \brief Base address for the PPU registers.
     */
    uintptr_t core_pd_reg_base;

    /*!
     * \brief Power domain type.
     */
    enum mod_pd_type pd_type;

    /*!
     * \brief Identifier of the timer.
     *
     * \details Used for time-out when operating the PPU.
     */
    fwk_id_t timer_id;

    /*!
     * \brief Wakeup IRQ number.
     */
    unsigned int wakeup_irq;

    /*!
     * \brief Wakeup FIQ number.
     */
    /* unsigned int wakeup_fiq; */

    /*!
     * \brief Warm reset request IRQ number.
     */
    unsigned int warm_reset_irq;

};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_JUNO_PPU_H */
