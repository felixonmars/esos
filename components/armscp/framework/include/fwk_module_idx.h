#ifndef FWK_MODULE_IDX_H
#define FWK_MODULE_IDX_H

#include <fwk_id.h>

enum fwk_module_idx {
    FWK_MODULE_IDX_CLOCK,
    FWK_MODULE_IDX_DVFS,
    FWK_MODULE_IDX_GTIMER,
    FWK_MODULE_IDX_TIMER,
    FWK_MODULE_IDX_PMIC_VOLTAGE_DOMAIN,
    FWK_MODULE_IDX_VOLTAGE_DOMAIN,
    FWK_MODULE_IDX_JUNO_PPU,
    FWK_MODULE_IDX_SYSTEM_POWER,
    FWK_MODULE_IDX_POWER_DOMAIN,
    FWK_MODULE_IDX_MHU,
    FWK_MODULE_IDX_TRANSPORT,
    FWK_MODULE_IDX_SCMI,
    FWK_MODULE_IDX_SCMI_VOLTAGE_DOMAIN,
    FWK_MODULE_IDX_SCMI_PERF,
    FWK_MODULE_IDX_SCMI_POWER_DOMAIN,
    FWK_MODULE_IDX_SCMI_SYSTEM_POWER,
    FWK_MODULE_IDX_PSU,
    FWK_MODULE_IDX_MOCK_PSU,
    FWK_MODULE_IDX_MOCK_CLOCK,
    FWK_MODULE_IDX_COUNT,
};


static const fwk_id_t fwk_module_id_clock =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_CLOCK);

static const fwk_id_t fwk_module_id_dvfs =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_DVFS);

static const fwk_id_t fwk_module_id_timer =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TIMER);

static const fwk_id_t fwk_module_id_gtimer =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_GTIMER);

static const fwk_id_t fwk_module_id_pmic_voltage_domain =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PMIC_VOLTAGE_DOMAIN);

static const fwk_id_t fwk_module_id_juno_ppu =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_JUNO_PPU);

static const fwk_id_t fwk_module_id_voltage_domain =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_VOLTAGE_DOMAIN);

static const fwk_id_t fwk_module_id_system_power =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SYSTEM_POWER);

static const fwk_id_t fwk_module_id_power_domain =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_POWER_DOMAIN);

static const fwk_id_t fwk_module_id_mhu =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_MHU);

static const fwk_id_t fwk_module_id_transport =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TRANSPORT);

static const fwk_id_t fwk_module_id_scmi =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI);

static const fwk_id_t fwk_module_id_scmi_perf =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_PERF);

static const fwk_id_t fwk_module_id_scmi_voltage_domain =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_VOLTAGE_DOMAIN);

static const fwk_id_t fwk_module_id_scmi_power_domain =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_POWER_DOMAIN);

static const fwk_id_t fwk_module_id_scmi_system_power =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_SYSTEM_POWER);

static const fwk_id_t fwk_module_id_psu =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PSU);

static const fwk_id_t fwk_module_id_mock_psu =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_MOCK_PSU);

static const fwk_id_t fwk_module_id_mock_clock =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_MOCK_CLOCK);

#endif
