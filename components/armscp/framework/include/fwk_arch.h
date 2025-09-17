/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Framework API for the architecture layer.
 */

#ifndef FWK_ARCH_H
#define FWK_ARCH_H

#include <rtconfig.h>
/*!
 * \addtogroup GroupLibFramework Framework
 * \{
 */

/*!
 * \defgroup GroupArch Architecture Interface
 * \{
 */

/*!
 * \brief Initialize the framework library.
 *
 * \param driver Pointer to an initialization driver used to perform the
 *      initialization.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_PANIC Unrecoverable initialization error.
 */
int fwk_arch_init(void);

/*!
 * \brief Stop the framework library.
 *
 * \details Before terminating the SCP-firmware, the modules and their elements
 * get the opportunity to release or reset some resources.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PANIC Unrecoverable error.
 */
int fwk_arch_deinit(void);

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* FWK_ARCH_H */
