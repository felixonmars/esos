#ifndef __RPMI_PLATFORM_DEFIN_H__
#define __RPMI_PLATFORM_DEFIN_H__

#include <librpmi.h>
#include <dtb_node.h>
#include <librpmi_env.h>
#include <rtdevice.h>
#include <rtdef.h>

#define HSM_SUSP_BASE_MASK		0x7fffffff
#define HSM_SUSP_NON_RET_BIT		0x80000000
#define HSM_SUSP_PLAT_BASE		0x10000000

#define HSM_SUSPEND_RET_DEFAULT		0x00000000
#define HSM_SUSPEND_RET_PLATFORM	HSM_SUSP_PLAT_BASE
#define HSM_SUSPEND_RET_LAST		HSM_SUSP_BASE_MASK
#define HSM_SUSPEND_NON_RET_DEFAULT	HSM_SUSP_NON_RET_BIT
#define HSM_SUSPEND_NON_RET_PLATFORM	(HSM_SUSP_NON_RET_BIT | HSM_SUSP_PLAT_BASE)
#define HSM_SUSPEND_NON_RET_LAST	(HSM_SUSP_NON_RET_BIT | HSM_SUSP_BASE_MASK)

#define MAX_HSM_SUSPEND_TYPE		6

#define HSM_SUSPEND_CPU_RET		HSM_SUSP_PLAT_BASE
#define HSM_SUSPEND_CLUSTER_RET		(HSM_SUSPEND_CPU_RET | 0x1000000)
#define HSM_SUSPEND_CPU_NON_RET		HSM_SUSPEND_NON_RET_PLATFORM
#define HSM_SUSPEND_CLUSTER_NON_RET	(HSM_SUSPEND_CPU_NON_RET | 0x1000000)
#define HSM_SUSPEND_HOME_SCREEN_NON_RET	(HSM_SUSPEND_CPU_NON_RET | 0x2000000)

#define HSM_SUSPEND_MAX_HARTIDS		16

/* RPMI HSM structures  */
struct spacemit_rpmi_hsm_config {
	rt_uint32_t hartids[HSM_SUSPEND_MAX_HARTIDS];
	struct rpmi_hsm_suspend_type stype[MAX_HSM_SUSPEND_TYPE];
	int hartcnt;
	int type_cnt;
	int support_syssup;
	struct dtb_node *node;
	struct rpmi_hsm_platform_ops *hsm_ops;
	struct rpmi_syssusp_platform_ops *syssup_ops;
	/* reserved for future use */
	void *priv;
};

struct spacemit_rpmi_hsm_ops {
	char *name;
	int (*init)(void *priv);
	struct rpmi_hsm_platform_ops *hsm_ops;
	struct rpmi_syssusp_platform_ops *syssup_ops;
	rt_list_t list;
};

/* RPMI clock structures */
struct spacemit_rpmi_clk_config {
	int num_clk;
	struct rpmi_clock_data *clk_data;
	struct dtb_node *node;
	struct rpmi_clock_platform_ops *ops;
	/* reserved for future use */
	void *priv;
};

struct spacemit_rpmi_clk_ops {
	char *name;
	int (*init)(void *priv);
	struct rpmi_clock_platform_ops *clk_ops;
	rt_list_t list;
};

/* RPMI voltage structures */
struct spacemit_rpmi_voltage_config {
	int domain_count;
	struct rpmi_voltage_data *voltage_data;
	struct dtb_node *node;
	struct rpmi_voltage_platform_ops *ops;
	/* reserved for future use */
	void *priv;
};

struct spacemit_rpmi_voltage_ops {
	char *name;
	int (*init)(void *priv);
	struct rpmi_voltage_platform_ops *voltage_ops;
	rt_list_t list;
};

/* RPMI sysreset structures */
struct spacemit_rpmi_sysreset_config {
	int reset;
};

struct spacemit_rpmi_config {
	rpmi_uintptr_t shmem_base;        /* Shared memory base address */
	rpmi_uint32_t shmem_size;         /* Shared memory size */
	rpmi_uint32_t slot_size;          /* Message slot size */
	rpmi_uint32_t a2p_queue_size;     /* AP to RCPU queue size */
	rpmi_uint32_t p2a_queue_size;     /* RCPU to AP queue size */
	struct spacemit_rpmi_hsm_config hsm_config;
	struct spacemit_rpmi_clk_config clk_config;
	struct spacemit_rpmi_voltage_config voltage_config;
	struct spacemit_rpmi_sysreset_config sysreset_config;
};

struct spacemit_rpmi_func {
	int (*rmpi_get_configuration)(struct dtb_node *mode, void *config, char *match);
	int (*rpmi_register_service)(void *config, struct rpmi_context *cntx);
};

/* Define RPMI private data structure */
struct spacemit_rpmi_priv {
	struct dtb_node *node;
	struct mbox_client client;
	struct mbox_chan *chan; /* Changed to match the header definition */
	struct rpmi_context *cntx;
	rt_thread_t tid;
	rt_sem_t sem;
	struct spacemit_rpmi_config config;
};

int spacemit_rpmi_hsm_register(rt_list_t *node);
int spacemit_rpmi_clk_register(rt_list_t *node);
int spacemit_rpmi_voltage_register(rt_list_t *node);

#endif /* __RPMI_PLATFORM_DEFIN_H__ */
