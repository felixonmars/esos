/*
 * Arm SCP/MCP Software
 * Copyright (c) 2019-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "juno_mhu.h"
#include "juno_scmi.h"
#include <register_defination.h>

#include <config_power_domain.h>

#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stddef.h>
#include <stdint.h>

static const struct fwk_element element_table[] = {
    /* JUNO_SCMI_SERVICE_IDX_OSPM_A2P */
    { .name = "",
      .data =
          &(struct mod_transport_channel_config){
              .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
              .policies = MOD_TRANSPORT_POLICY_INIT_MAILBOX,
              .out_band_mailbox_address = (uintptr_t)SCMI_PAYLOAD_NS_A2P_BASE,
              .out_band_mailbox_size = SCMI_PAYLOAD_SIZE,
              .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                  FWK_MODULE_IDX_MHU,
                  JUNO_MHU_DEVICE_IDX,
                  0),
              .driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MHU, 0),
#ifdef BUILD_HAS_MOD_POWER_DOMAIN
              .pd_source_id = FWK_ID_ELEMENT_INIT(
                  FWK_MODULE_IDX_POWER_DOMAIN,
                  POWER_DOMAIN_IDX_SYSTOP),
#endif
          } },

    /* JUNO_SCMI_SERVICE_IDX_PSCI_A2P */
    { .name = "",
      .data =
          &(struct mod_transport_channel_config){
              .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
              .policies =
                  (MOD_TRANSPORT_POLICY_INIT_MAILBOX |
                   MOD_TRANSPORT_POLICY_SECURE),
              .out_band_mailbox_address = (uintptr_t)SCMI_PAYLOAD_S_A2P_BASE,
              .out_band_mailbox_size = SCMI_PAYLOAD_SIZE,
              .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                  FWK_MODULE_IDX_MHU,
                  JUNO_MHU_DEVICE_IDX,
                  1),
              .driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MHU, 0),
#ifdef BUILD_HAS_MOD_POWER_DOMAIN
              .pd_source_id = FWK_ID_ELEMENT_INIT(
                  FWK_MODULE_IDX_POWER_DOMAIN,
                  POWER_DOMAIN_IDX_SYSTOP),
#endif
          } },

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    /* JUNO_SCMI_SERVICE_IDX_OSPM_P2A */
    { .name = "",
      .data =
          &(struct mod_transport_channel_config){
              .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_REQUESTER,
              .policies = MOD_TRANSPORT_POLICY_INIT_MAILBOX,
              .out_band_mailbox_address = (uintptr_t)SCMI_PAYLOAD_HIGH_P2A_BASE,
              .out_band_mailbox_size = SCMI_PAYLOAD_SIZE,
              .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                  FWK_MODULE_IDX_MHU,
                  JUNO_MHU_DEVICE_IDX_NS_H,
                  1),
              .driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_MHU, 0),
#ifdef BUILD_HAS_MOD_POWER_DOMAIN
//              .pd_source_id = FWK_ID_ELEMENT_INIT(
//                  FWK_MODULE_IDX_POWER_DOMAIN,
//                  POWER_DOMAIN_IDX_SYSTOP),
#endif
	  } },
#endif
    [JUNO_SCMI_SERVICE_IDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

struct fwk_module_config config_transport = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
