/*
 * Arm SCP/MCP Software
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     String management.
 */

#include <rtthread.h>
#include <fwk_mm.h>
#include <stdlib.h>

void fwk_str_memset(void *dest, int ch, size_t count)
{
    void *ret;

    if (dest == NULL) {
	    rt_kprintf("%s:%d hang\n", __func__, __LINE__);
	    while (1);
    }

    ret = memset(dest, ch, count);
    if (ret != dest) {
	    rt_kprintf("%s:%d hang\n", __func__, __LINE__);
	    while (1);
    }
}

void fwk_str_memcpy(void *dest, const void *src, size_t count)
{
    void *ret;

    if ((dest == NULL) || (src == NULL)) {
	    rt_kprintf("%s:%d hang\n", __func__, __LINE__);
	    while (1);
    }

    ret = memcpy(dest, src, count);
    if (ret != dest) {
	    rt_kprintf("%s:%d hang\n", __func__, __LINE__);
	    while (1);
    }
}

void fwk_str_strncpy(char *dest, const char *src, size_t count)
{
    char *ch;

    if ((dest == NULL) || (src == NULL)) {
	    rt_kprintf("%s:%d hang\n", __func__, __LINE__);
	    while (1);
    }

    ch = strncpy(dest, src, count);
    if (ch != dest) {
	    rt_kprintf("%s:%d hang\n", __func__, __LINE__);
	    while (1);
    }
}
