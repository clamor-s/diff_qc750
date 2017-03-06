/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA Tegra ODM Kit: ODM Service for Pinmux Configuration Interface</b>
 *
 * @b Description: Defines functions for applying pinmux configurations.
 */

/**
 * @defgroup nvodm_pinmux_group Pinmux Configuration Interface
 *
 * Defines functions for configuring Pinmux settings.
 *
 * @ingroup nvodm_adaptation
 * @{
 */

#ifndef NVODM_PINMUX_INIT_H
#define NVODM_PINMUX_INIT_H

#include "nverror.h"

#if defined(__cplusplus)
extern "C"
{
#endif
/**
 * Sets the pinmux configuration.
 * This function obtains the settings from the pinmux table,
 * which is in the board configuration file for the board.
 * @retval NvSuccess on success.
 */

NvError NvOdmPinmuxInit(void);

/** @} */

#if defined(__cplusplus)
}
#endif

#endif