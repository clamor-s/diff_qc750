//
// Copyright (c) 2007-2008 NVIDIA Corporation.  All Rights Reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation.  Any
// use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation
// is strictly prohibited.
//

/**
 * @file
 * <b>NVIDIA Tegra Boot Loader API</b>
 *
 */

#ifndef INCLUDED_NVBL_OPERATINGMODES_H
#define INCLUDED_NVBL_OPERATINGMODES_H

/**
 * @defgroup nvbl_operatingmodes_group NvBL Operating Modes
 *
 * Provides operating mode information for NVIDIA Boot Loader functions.
 *
 * @ingroup nvbl_group
 * @{
 */

/**
 * Defines the supported operating modes.
 */
typedef enum
{
    /// Specifies development mode; boot loader is validated using a fixed key of zeroes.
    NvBlOperatingMode_NvProduction = 3,

    /// Specifies production mode; boot loader is decrypted using the Secure Boot Key,
    /// then validated using the Secure Boot Key.
    NvBlOperatingMode_OdmProductionSecure,

    /// Specifies ODM production mode; boot loader is validated using the Secure Boot Key.
    NvBlOperatingMode_OdmProductionOpen,

    /// Undefined.
    NvBlOperatingMode_Undefined,

    /// Ignore -- Forces compilers to make 32-bit enums.
    NvBlOperatingMode_Force32 = 0x7FFFFFFF
} NvBlOperatingMode;

/** @} */

#endif // INCLUDED_NVBL_OPERATINGMODES_H

