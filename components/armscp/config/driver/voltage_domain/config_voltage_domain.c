#include <config_pmic_voltage_domain.h>
#include <mod_pmic_voltage_domain.h>
#include <mod_voltage_domain.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stdint.h>

static const struct fwk_element voltage_domain_element_table[] = {
    [0] = {
        .name = "LITTLE_VOLTD",
        .data = &((const struct mod_voltd_dev_config) {
        .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_PMIC_VOLTAGE_DOMAIN,
                CONFIG_MOCK_VOLTAGE_DOMAIN_ELEMENT_IDX_VLITTLE),
        .api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_PMIC_VOLTAGE_DOMAIN,
                MOD_MOCK_VOLTAGE_DOMAIN_API_IDX_VOLTD),
        }),
    },
    [1] = {
        .name = "GPU_VOLTD",
        .data = &((const struct mod_voltd_dev_config) {
        .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_PMIC_VOLTAGE_DOMAIN,
                CONFIG_MOCK_VOLTAGE_DOMAIN_ELEMENT_IDX_VGPU),
        .api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_PMIC_VOLTAGE_DOMAIN,
                MOD_MOCK_VOLTAGE_DOMAIN_API_IDX_VOLTD),
        }),
    },
};

static const struct fwk_element *voltage_domain_get_element_table(
    fwk_id_t module_id)
{
    return voltage_domain_element_table;
}

const struct fwk_module_config config_voltage_domain = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(voltage_domain_get_element_table),
};

