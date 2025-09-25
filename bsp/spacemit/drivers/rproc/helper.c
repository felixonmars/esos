/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <metal/sys.h>
#include <metal/irq.h>

/* Main hw machinery initialization entry point, called from main()*/
/* return 0 on success */
int init_system(void)
{
	struct metal_init_params metal_param = METAL_INIT_DEFAULTS;

	/* Low level abstraction layer for openamp initialization */
	metal_init(&metal_param);

	metal_spacemit_irq_init();

	return 0;
}
