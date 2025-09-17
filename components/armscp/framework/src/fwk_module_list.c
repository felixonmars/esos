#include <rtconfig.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#ifdef BUILD_HAS_MOD_DVFS
extern struct fwk_module module_clock;
extern struct fwk_module module_dvfs;
extern struct fwk_module module_psu;
extern struct fwk_module module_mock_psu;
extern struct fwk_module module_mock_clock;
#endif

#ifdef BUILD_HAS_MOD_VOLTAGE_DOMAIN
extern struct fwk_module module_pmic_voltage_domain;
extern struct fwk_module module_voltage_domain;
#endif

#ifdef BUILD_HAS_MOD_POWER_DOMAIN
extern struct fwk_module module_juno_ppu;
extern struct fwk_module module_power_domain;
#endif

#ifdef BUILD_HAS_MOD_SYSTEM_POWER
extern struct fwk_module module_system_power;
#endif

#ifdef BUILD_HAS_MOD_MHU
extern struct fwk_module module_mhu;
#endif

#ifdef BUILD_HAS_MOD_TRANSPORT
extern struct fwk_module module_transport;
#endif

extern struct fwk_module module_scmi;

#ifdef BUILD_HAS_MOD_SCMI_POWER_DOMAIN
extern struct fwk_module module_scmi_power_domain;
#endif

#ifdef BUILD_HAS_MOD_SCMI_VOLTAGE_DOMAIN
extern struct fwk_module module_scmi_voltage_domain;
#endif

#ifdef BUILD_HAS_MOD_SCMI_PERF
extern struct fwk_module module_scmi_perf;
#endif

#ifdef BUILD_HAS_MOD_SCMI_SYSTEM_POWER
extern struct fwk_module module_scmi_system_power;
#endif

#ifdef BUILD_HAS_MOD_TIMER
extern struct fwk_module module_timer;
extern struct fwk_module module_gtimer;
#endif

struct fwk_module *module_table[FWK_MODULE_IDX_COUNT] = {
#ifdef BUILD_HAS_MOD_DVFS
	&module_clock,
	&module_dvfs,
#endif
#ifdef BUILD_HAS_MOD_TIMER
	&module_gtimer,
	&module_timer,
#endif
#ifdef BUILD_HAS_MOD_VOLTAGE_DOMAIN
        &module_pmic_voltage_domain,
	&module_voltage_domain,
#endif
#ifdef BUILD_HAS_MOD_POWER_DOMAIN
	&module_juno_ppu,
#endif
#ifdef BUILD_HAS_MOD_SYSTEM_POWER
	&module_system_power,
#endif
#ifdef BUILD_HAS_MOD_POWER_DOMAIN
	&module_power_domain,
#endif
#ifdef BUILD_HAS_MOD_MHU
	&module_mhu,
#endif
#ifdef BUILD_HAS_MOD_TRANSPORT
	&module_transport,
#endif
	&module_scmi,
#ifdef BUILD_HAS_MOD_SCMI_VOLTAGE_DOMAIN
	&module_scmi_voltage_domain,
#endif
#ifdef BUILD_HAS_MOD_SCMI_PERF
	&module_scmi_perf,
#endif
#ifdef BUILD_HAS_MOD_SCMI_POWER_DOMAIN
	&module_scmi_power_domain,
#endif
#ifdef BUILD_HAS_MOD_SCMI_SYSTEM_POWER
	&module_scmi_system_power,
#endif
#ifdef BUILD_HAS_MOD_DVFS
	&module_psu,
	&module_mock_psu,
	&module_mock_clock,
#endif
};

#ifdef BUILD_HAS_MOD_DVFS
extern struct fwk_module_config config_clock;
extern struct fwk_module_config config_psu;
extern struct fwk_module_config config_mock_psu;
extern struct fwk_module_config config_dvfs;
extern struct fwk_module_config config_mock_clock;
#endif

#ifdef BUILD_HAS_MOD_VOLTAGE_DOMAIN
extern struct fwk_module_config config_pmic_voltage_domain;
extern struct fwk_module_config config_voltage_domain;
#endif

#ifdef BUILD_HAS_MOD_POWER_DOMAIN
extern struct fwk_module_config config_juno_ppu;
extern struct fwk_module_config config_power_domain;
#endif

#ifdef BUILD_HAS_MOD_SYSTEM_POWER
extern struct fwk_module_config config_system_power;
#endif

#ifdef BUILD_HAS_MOD_MHU
extern struct fwk_module_config config_mhu;
#endif

extern struct fwk_module_config config_scmi;

#ifdef BUILD_HAS_MOD_TRANSPORT
extern struct fwk_module_config config_transport;
#endif

#ifdef BUILD_HAS_MOD_SCMI_VOLTAGE_DOMAIN
extern struct fwk_module_config config_scmi_voltage_domain;
#endif

#ifdef BUILD_HAS_MOD_SCMI_POWER_DOMAIN
extern struct fwk_module_config config_scmi_power_domain;
#endif

#ifdef BUILD_HAS_MOD_SCMI_PERF
extern struct fwk_module_config config_scmi_perf;
#endif

#ifdef BUILD_HAS_MOD_SCMI_SYSTEM_POWER
extern struct fwk_module_config config_scmi_system_power;
#endif

#ifdef BUILD_HAS_MOD_TIMER
extern struct fwk_module_config config_timer;
extern struct fwk_module_config config_gtimer;
#endif

struct fwk_module_config *module_config_table[FWK_MODULE_IDX_COUNT] = {
#ifdef BUILD_HAS_MOD_DVFS
	&config_clock,
	&config_dvfs,
#endif
#ifdef BUILD_HAS_MOD_TIMER
	&config_gtimer,
	&config_timer,
#endif
#ifdef BUILD_HAS_MOD_VOLTAGE_DOMAIN
	&config_pmic_voltage_domain,
	&config_voltage_domain,
#endif
#ifdef BUILD_HAS_MOD_POWER_DOMAIN
	&config_juno_ppu,
#endif
#ifdef BUILD_HAS_MOD_SYSTEM_POWER
	&config_system_power,
#endif
#ifdef BUILD_HAS_MOD_POWER_DOMAIN
	&config_power_domain,
#endif
#ifdef BUILD_HAS_MOD_MHU
	&config_mhu,
#endif
#ifdef BUILD_HAS_MOD_TRANSPORT
	&config_transport,
#endif
	&config_scmi,
#ifdef BUILD_HAS_MOD_SCMI_VOLTAGE_DOMAIN
	&config_scmi_voltage_domain,
#endif
#ifdef BUILD_HAS_MOD_SCMI_PERF
	&config_scmi_perf,
#endif
#ifdef BUILD_HAS_MOD_SCMI_POWER_DOMAIN
	&config_scmi_power_domain,
#endif
#ifdef BUILD_HAS_MOD_SCMI_SYSTEM_POWER
	&config_scmi_system_power,
#endif
#ifdef BUILD_HAS_MOD_DVFS
	&config_psu,
	&config_mock_psu,
	&config_mock_clock,
#endif
};
