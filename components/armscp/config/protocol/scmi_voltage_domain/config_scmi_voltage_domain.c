#include <juno_scmi.h>

#include <config_pmic_voltage_domain.h>
#include <mod_scmi_voltage_domain.h>
#include <mod_voltage_domain.h>

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stddef.h>

static struct mod_scmi_voltd_device scmi_voltd_device[] = {
    {
        .element_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_VOLTAGE_DOMAIN,
			CONFIG_MOCK_VOLTAGE_DOMAIN_ELEMENT_IDX_VLITTLE),
        .config = 0,
    },
    {
        .element_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_VOLTAGE_DOMAIN,
			CONFIG_MOCK_VOLTAGE_DOMAIN_ELEMENT_IDX_VGPU),
        .config = 0,
    },
};

static const struct mod_scmi_voltd_agent scmi_voltd_agent_table[JUNO_SCMI_AGENT_IDX_COUNT] = {
    [JUNO_SCMI_AGENT_IDX_OSPM] = {
        .device_table = scmi_voltd_device,
        .domain_count = FWK_ARRAY_SIZE(scmi_voltd_device),
    },
    [JUNO_SCMI_AGENT_IDX_PSCI] = { 0 /* No access */ },
};

struct fwk_module_config config_scmi_voltage_domain = {
    .data = &((struct mod_scmi_voltd_config){
        .agent_table = scmi_voltd_agent_table,
        .agent_count = FWK_ARRAY_SIZE(scmi_voltd_agent_table),
    }),
};

