/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>
#ifdef RT_USING_ARMSCP_MODULE
#include <fwk_core.h>
#endif

extern void fwk_waiting_for_event(void);

int main(void)
{

    rt_kprintf("##########Hello World############\n");

#ifdef RT_USING_ARMSCP_MODULE
    while (1) {
	 /* process the event */
         fwk_process_event_queue();

	 /* wait for the messages send by CPUX */
	 fwk_waiting_for_event();
    }
#endif
    return 0;
}

/*@}*/
