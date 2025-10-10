/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <dtb_head.h>
#include <librpmi.h>
#include "../spacemit-rpmi.h"

static rt_int32_t  _k3_os0_rtc_init(void *priv)
{
	/* platform releated, get the registers or other thing what you want */
	struct spacemit_rpmi_rtc_config *config = priv;

	return 0;
}

/** Set the rtc time */
static enum rpmi_error k3_os0_set_time(void *priv, rpmi_uint32_t year,
				    rpmi_uint32_t mon,
				    rpmi_uint32_t dat,
				    rpmi_uint32_t hour,
				    rpmi_uint32_t min,
				    rpmi_uint32_t second)
{
	return RPMI_SUCCESS;
}

/**
 * Get the rtc time
 **/
static enum rpmi_error k3_os0_get_time(void *priv, rpmi_uint32_t *year,
				    rpmi_uint32_t *mon,
				    rpmi_uint32_t *dat,
				    rpmi_uint32_t *hour,
				    rpmi_uint32_t *min,
				    rpmi_uint32_t *second)
{
	*year = 0;
	*mon = 0;
	*dat = 0;
	*hour = 0;
	*min = 0;
	*second = 0;

	return RPMI_SUCCESS;
}

/** Set the rtc alarm time */
static enum rpmi_error k3_os0_set_alarm(void *priv, rpmi_uint32_t year,
				    rpmi_uint32_t mon,
				    rpmi_uint32_t dat,
				    rpmi_uint32_t hour,
				    rpmi_uint32_t min,
				    rpmi_uint32_t second)
{
	return RPMI_SUCCESS;
}

/**
 * Get the rtc alarm time
 **/
static enum rpmi_error k3_os0_get_alarm(void *priv, rpmi_uint32_t *year,
				    rpmi_uint32_t *mon,
				    rpmi_uint32_t *dat,
				    rpmi_uint32_t *hour,
				    rpmi_uint32_t *min,
				    rpmi_uint32_t *second)
{
	*year = 0;
	*mon = 0;
	*dat = 0;
	*hour = 0;
	*min = 0;
	*second = 0;

	return RPMI_SUCCESS;
}

enum rpmi_error k3_os0_get_alarm_en(void *priv, rpmi_uint32_t *status)
{
	return RPMI_SUCCESS;
}

static enum rpmi_error k3_os0_set_alarm_en(void *priv, rpmi_uint32_t en)
{
	return RPMI_SUCCESS;
}

enum rpmi_error k3_os0_query_pending(void *priv, rpmi_uint32_t *status)
{
	return RPMI_SUCCESS;
}

static enum rpmi_error k3_os0_clear_pending(void *priv)
{
	return RPMI_SUCCESS;
}

static struct rpmi_rtc_platform_ops k3_os0_rtc_pops = {
	.set_time = k3_os0_set_time,
	.get_time = k3_os0_get_time,
	.set_alarm = k3_os0_set_alarm,
	.get_alarm = k3_os0_get_alarm,
	.get_alarm_en = k3_os0_get_alarm_en,
	.set_alarm_en = k3_os0_set_alarm_en,
	.query_pending = k3_os0_query_pending,
	.clear_pending = k3_os0_clear_pending,
};

static struct spacemit_rpmi_rtc_ops k3_os0_rtc_ops = {
	.name = "k3-os0-rpmi-rtc",
	.init = _k3_os0_rtc_init,
	.rtc_ops = &k3_os0_rtc_pops,
};

static rt_int32_t k3_os0_rtc_init(void)
{
	rt_list_init(&k3_os0_rtc_ops.list);

	spacemit_rpmi_rtc_register(&k3_os0_rtc_ops.list);

	return 0;
}
INIT_DEVICE_EXPORT(k3_os0_rtc_init);
