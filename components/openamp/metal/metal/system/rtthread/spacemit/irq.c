/*
 * Copyright (c) 2016 - 2017, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	rtthread/spacemit/irq.c
 * @brief	rtthread libmetal Xilinx irq controller definitions.
 */

#include <metal/errno.h>
#include <metal/irq_controller.h>
#include <metal/log.h>
#include <metal/mutex.h>
#include <metal/list.h>
#include <metal/alloc.h>
#include <metal/sys.h>
#include <metal/system/rtthread/spacemit/sys.h>

#define MAX_IRQS	128

static struct metal_irq irqs[MAX_IRQS]; /**< Linux IRQs array */

static void metal_spacemit_irq_set_enable(struct metal_irq_controller *irq_cntr,
				      int irq, unsigned int state)
{
	if (irq < irq_cntr->irq_base ||
	    irq >= irq_cntr->irq_base + irq_cntr->irq_num) {
		metal_log(METAL_LOG_ERROR, "%s: invalid irq %d\n",
			  __func__, irq);
		return;
	} else if (state == METAL_IRQ_ENABLE) {
		sys_irq_enable((unsigned int)irq);
	} else {
		sys_irq_disable((unsigned int)irq);
	}
}

int metal_spacemit_irq_register(struct metal_irq_controller *irq_cntr,
					int irq, metal_irq_handler hd,
					void *arg)
{
	rt_hw_interrupt_install(irq, (rt_isr_handler_t)hd, arg, NULL);

	return 0;
}

/**< Spacemit common platform IRQ controller */
static METAL_IRQ_CONTROLLER_DECLARE(spacemit_irq_cntr,
				    0, MAX_IRQS,
				    NULL,
				    metal_spacemit_irq_set_enable, metal_spacemit_irq_register,
				    irqs);

int metal_spacemit_irq_init(void)
{
	int ret;

	ret =  metal_irq_register_controller(&spacemit_irq_cntr);
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "%s: register irq controller failed.\n",
			  __func__);
		return ret;
	}
	return 0;
}
