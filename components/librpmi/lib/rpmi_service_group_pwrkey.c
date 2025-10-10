/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Spacemit.
 */

#include <librpmi.h>
#include "librpmi_internal.h"
#include "librpmi_internal_list.h"

#ifdef DEBUG
#define DPRINTF(msg...)		rpmi_env_printf(msg)
#else
#define DPRINTF(msg...)
#endif

/** RPMI Pwrkey Service Group instance */
struct rpmi_pwrkey_group {
	/* Common pwrkey platform operations (called with holding the lock)*/
	const struct rpmi_pwrkey_platform_ops *ops;
	/* Private data of platform pwrkey operations */
	void *ops_priv;
	struct rpmi_service_group group;
};

static enum rpmi_error
rpmi_pwrkey_sg_query_pending(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	rpmi_uint16_t resp_dlen;
	rpmi_uint32_t status;
	struct rpmi_pwrkey_group *pwrkeygrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	pwrkeygrp->ops->query_pending(pwrkeygrp->ops_priv, &status);

	resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)status);

	resp_dlen = sizeof(*resp);

done:
	*response_datalen = resp_dlen;
	return RPMI_SUCCESS;
}

static enum rpmi_error
rpmi_pwrkey_sg_clear_pending(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	rpmi_uint16_t resp_dlen;
	enum rpmi_error status;
	struct rpmi_pwrkey_group *pwrkeygrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;
	rpmi_uint32_t clear = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[0]);

	status = pwrkeygrp->ops->clear_pending(pwrkeygrp->ops_priv, clear);
	if (status) {
		resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)status);
		resp_dlen = sizeof(*resp);
		goto done;
	}

	resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)RPMI_SUCCESS);

	resp_dlen = sizeof(*resp);

done:
	*response_datalen = resp_dlen;
	return RPMI_SUCCESS;
}

static struct rpmi_service rpmi_pwrkey_services[RPMI_PWRKEY_SRV_ID_MAX_COUNT] = {
	[RPMI_PWRKEY_SRV_ENABLE_NOTIFICATION] = {
		.service_id = RPMI_PWRKEY_SRV_ENABLE_NOTIFICATION,
		.min_a2p_request_datalen = 4,
		.process_a2p_request = NULL,
	},
	[RPMI_PWRKEY_SRV_QUERY_PENDING] = {
		.service_id = RPMI_PWRKEY_SRV_QUERY_PENDING,
		.min_a2p_request_datalen = 4,
		.process_a2p_request = rpmi_pwrkey_sg_query_pending,
	},
	[RPMI_PWRKEY_SRV_CLR_PENDING] = {
		.service_id = RPMI_PWRKEY_SRV_CLR_PENDING,
		.min_a2p_request_datalen = 4,
		.process_a2p_request = rpmi_pwrkey_sg_clear_pending,
	},
};

struct rpmi_service_group *
rpmi_service_group_pwrkey_create(const struct rpmi_pwrkey_platform_ops *ops, void *ops_priv)
{
	struct rpmi_pwrkey_group *pwrkeygrp;
	struct rpmi_service_group *group;

	/* All critical parameters should be non-NULL */
	if (!ops) {
		DPRINTF("%s: invalid parameters\n", __func__);
		return NULL;
	}

	/* Allocate pwrkey service group */
	pwrkeygrp = rpmi_env_zalloc(sizeof(*pwrkeygrp));
	if (!pwrkeygrp) {
		DPRINTF("%s: failed to allocate pwrkey service group instance\n",
			__func__);
		return NULL;
	}

	pwrkeygrp->ops = ops;
	pwrkeygrp->ops_priv = ops_priv;

	group = &pwrkeygrp->group;
	group->name = "pwrkey";
	group->servicegroup_id = RPMI_SRVGRP_PWRKEY;
	group->servicegroup_version =
		RPMI_BASE_VERSION(RPMI_SPEC_VERSION_MAJOR, RPMI_SPEC_VERSION_MINOR);
	/* Allowed for both M-mode and S-mode RPMI context */
	group->privilege_level_bitmap = RPMI_PRIVILEGE_M_MODE_MASK | RPMI_PRIVILEGE_S_MODE_MASK;
	group->max_service_id = RPMI_PWRKEY_SRV_ID_MAX_COUNT;
	group->services = rpmi_pwrkey_services;
	group->lock = rpmi_env_alloc_lock();
	group->priv = pwrkeygrp;

	return group;
}

void rpmi_service_group_pwrkey_destroy(struct rpmi_service_group *group)
{
	struct rpmi_pwrkey_group *pwrkeygrp;

	if (!group) {
		DPRINTF("%s: invalid parameters\n", __func__);
		return;
	}

	pwrkeygrp = group->priv;

	rpmi_env_free_lock(group->lock);
	rpmi_env_free(group->priv);
}
