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

/** RPMI RTC Service Group instance */
struct rpmi_rtc_group {
	/* Common rtc platform operations (called with holding the lock)*/
	const struct rpmi_rtc_platform_ops *ops;
	/* Private data of platform clock operations */
	void *ops_priv;
	struct rpmi_service_group group;
};

static enum rpmi_error
rpmi_rtc_sg_set_time(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	enum rpmi_error status;
	struct rpmi_rtc_group *rtcgrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	rpmi_uint32_t year = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[0]);
	rpmi_uint32_t mon = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[1]);
	rpmi_uint32_t data = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[2]);
	rpmi_uint32_t hour = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[3]);
	rpmi_uint32_t min = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[4]);
	rpmi_uint32_t second = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[5]);

	status = rtcgrp->ops->set_time(rtcgrp->ops_priv, year, mon, data, hour, min, second);
	resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)status);

	*response_datalen = sizeof(*resp);

	return RPMI_SUCCESS;
}

static enum rpmi_error
rpmi_rtc_sg_get_time(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	rpmi_uint16_t resp_dlen;
	enum rpmi_error status;
	rpmi_uint32_t year, mon, data, hour, min, second;
	struct rpmi_rtc_group *rtcgrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	status = rtcgrp->ops->get_time(rtcgrp->ops_priv, &year, &mon, &data, &hour, &min, &second);
	if (status) {
		resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)status);
		resp_dlen = sizeof(*resp);
		goto done;
	}

	resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)RPMI_SUCCESS);
	resp[1] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)year);
	resp[2] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)mon);
	resp[3] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)data);
	resp[4] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)hour);
	resp[5] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)min);
	resp[6] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)second);

	resp_dlen = 7 * sizeof(*resp);

done:
	*response_datalen = resp_dlen;
	return RPMI_SUCCESS;
}

static enum rpmi_error
rpmi_rtc_sg_set_alarm(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	enum rpmi_error status;
	struct rpmi_rtc_group *rtcgrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	rpmi_uint32_t year = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[0]);
	rpmi_uint32_t mon = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[1]);
	rpmi_uint32_t data = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[2]);
	rpmi_uint32_t hour = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[3]);
	rpmi_uint32_t min = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[4]);
	rpmi_uint32_t second = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[5]);

	status = rtcgrp->ops->set_alarm(rtcgrp->ops_priv, year, mon, data, hour, min, second);
	resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)status);

	*response_datalen = sizeof(*resp);

	return RPMI_SUCCESS;
}

static enum rpmi_error
rpmi_rtc_sg_get_alarm(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	rpmi_uint16_t resp_dlen;
	enum rpmi_error status;
	rpmi_uint32_t year, mon, data, hour, min, second;
	struct rpmi_rtc_group *rtcgrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	status = rtcgrp->ops->get_alarm(rtcgrp->ops_priv, &year, &mon, &data, &hour, &min, &second);
	if (status) {
		resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)status);
		resp_dlen = sizeof(*resp);
		goto done;
	}

	resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)RPMI_SUCCESS);
	resp[1] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)year);
	resp[2] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)mon);
	resp[3] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)data);
	resp[4] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)hour);
	resp[5] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)min);
	resp[6] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)second);

	resp_dlen = 7 * sizeof(*resp);

done:
	*response_datalen = resp_dlen;
	return RPMI_SUCCESS;
}

static enum rpmi_error
rpmi_rtc_sg_get_alarm_en(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	rpmi_uint16_t resp_dlen;
	rpmi_uint32_t status;
	struct rpmi_rtc_group *rtcgrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	rtcgrp->ops->get_alarm_en(rtcgrp->ops_priv, &status);

	resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)status);

	resp_dlen = sizeof(*resp);

done:
	*response_datalen = resp_dlen;
	return RPMI_SUCCESS;
}

static enum rpmi_error
rpmi_rtc_sg_set_alarm_en(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	rpmi_uint16_t resp_dlen;
	enum rpmi_error status;
	struct rpmi_rtc_group *rtcgrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	rpmi_uint32_t en = rpmi_to_xe32(trans->is_be,
			       ((const rpmi_uint32_t *)request_data)[0]);

	status = rtcgrp->ops->set_alarm_en(rtcgrp->ops_priv, en);
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

static enum rpmi_error
rpmi_rtc_sg_query_pending(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	rpmi_uint16_t resp_dlen;
	rpmi_uint32_t status;
	struct rpmi_rtc_group *rtcgrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	rtcgrp->ops->query_pending(rtcgrp->ops_priv, &status);

	resp[0] = rpmi_to_xe32(trans->is_be, (rpmi_uint32_t)status);

	resp_dlen = sizeof(*resp);

done:
	*response_datalen = resp_dlen;
	return RPMI_SUCCESS;
}

static enum rpmi_error
rpmi_rtc_sg_clear_pending(struct rpmi_service_group *group,
			 struct rpmi_service *service,
			 struct rpmi_transport *trans,
			 rpmi_uint16_t request_datalen,
			 const rpmi_uint8_t *request_data,
			 rpmi_uint16_t *response_datalen,
			 rpmi_uint8_t *response_data)
{
	rpmi_uint16_t resp_dlen;
	enum rpmi_error status;
	struct rpmi_rtc_group *rtcgrp = group->priv;
	rpmi_uint32_t *resp = (void *)response_data;

	status = rtcgrp->ops->clear_pending(rtcgrp->ops_priv);
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

static struct rpmi_service rpmi_rtc_services[RPMI_RTC_SRV_MAX_COUNT] = {
	[RPMI_RTC_SRV_ENABLE_NOTIFICATION] = {
		.service_id = RPMI_RTC_SRV_ENABLE_NOTIFICATION,
		.min_a2p_request_datalen = 4,
		.process_a2p_request = NULL,
	},
	[RPMI_RTC_SRV_SET_TIME] = {
		.service_id = RPMI_RTC_SRV_SET_TIME,
		.min_a2p_request_datalen = 24,
		.process_a2p_request = rpmi_rtc_sg_set_time,
	},
	[RPMI_RTC_SRV_GET_TIME] = {
		.service_id = RPMI_RTC_SRV_GET_TIME,
		.min_a2p_request_datalen = 28,
		.process_a2p_request = rpmi_rtc_sg_get_time,
	},
	[RPMI_RTC_SRV_SET_ALARM] = {
		.service_id = RPMI_RTC_SRV_SET_ALARM,
		.min_a2p_request_datalen = 24,
		.process_a2p_request = rpmi_rtc_sg_set_alarm,
	},
	[RPMI_RTC_SRV_GET_ALARM] = {
		.service_id = RPMI_RTC_SRV_GET_ALARM,
		.min_a2p_request_datalen = 28,
		.process_a2p_request = rpmi_rtc_sg_get_alarm,
	},
	[RPMI_RTC_SRV_ALARM_GET_EN] = {
		.service_id = RPMI_RTC_SRV_ALARM_GET_EN,
		.min_a2p_request_datalen = 4,
		.process_a2p_request = rpmi_rtc_sg_get_alarm_en,
	},
	[RPMI_RTC_SRV_ALARM_SET_EN] = {
		.service_id = RPMI_RTC_SRV_ALARM_SET_EN,
		.min_a2p_request_datalen = 4,
		.process_a2p_request = rpmi_rtc_sg_set_alarm_en,
	},
	[RPMI_RTC_SRV_QUERY_PENDING] = {
		.service_id = RPMI_RTC_SRV_QUERY_PENDING,
		.min_a2p_request_datalen = 4,
		.process_a2p_request = rpmi_rtc_sg_query_pending,
	},
	[RPMI_RTC_SRV_CLR_PENDING] = {
		.service_id = RPMI_RTC_SRV_CLR_PENDING,
		.min_a2p_request_datalen = 4,
		.process_a2p_request = rpmi_rtc_sg_clear_pending,
	},
};

struct rpmi_service_group *
rpmi_service_group_rtc_create(const struct rpmi_rtc_platform_ops *ops, void *ops_priv)
{
	struct rpmi_rtc_group *rtcgrp;
	struct rpmi_service_group *group;

	/* All critical parameters should be non-NULL */
	if (!ops) {
		DPRINTF("%s: invalid parameters\n", __func__);
		return NULL;
	}

	/* Allocate clock service group */
	rtcgrp = rpmi_env_zalloc(sizeof(*rtcgrp));
	if (!rtcgrp) {
		DPRINTF("%s: failed to allocate rtc service group instance\n",
			__func__);
		return NULL;
	}

	rtcgrp->ops = ops;
	rtcgrp->ops_priv = ops_priv;

	group = &rtcgrp->group;
	group->name = "rtc";
	group->servicegroup_id = RPMI_SRVGRP_RTC;
	group->servicegroup_version =
		RPMI_BASE_VERSION(RPMI_SPEC_VERSION_MAJOR, RPMI_SPEC_VERSION_MINOR);
	/* Allowed for both M-mode and S-mode RPMI context */
	group->privilege_level_bitmap = RPMI_PRIVILEGE_M_MODE_MASK | RPMI_PRIVILEGE_S_MODE_MASK;
	group->max_service_id = RPMI_RTC_SRV_MAX_COUNT;
	group->services = rpmi_rtc_services;
	group->lock = rpmi_env_alloc_lock();
	group->priv = rtcgrp;

	return group;
}

void rpmi_service_group_rtc_destroy(struct rpmi_service_group *group)
{
	struct rpmi_rtc_group *rtcgrp;

	if (!group) {
		DPRINTF("%s: invalid parameters\n", __func__);
		return;
	}

	rtcgrp = group->priv;

	rpmi_env_free_lock(group->lock);
	rpmi_env_free(group->priv);
}
