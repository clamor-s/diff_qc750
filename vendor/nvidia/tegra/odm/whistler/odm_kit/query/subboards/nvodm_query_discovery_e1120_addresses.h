/*
 * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA APX ODM Kit::
 *         Implementation of the ODM Peripheral Discovery API</b>
 *
 * @b Description: Specifies the peripheral connectivity 
 *                 database NvOdmIoAddress entries for the E1120
 *                 AP20 Development System Motherboard.
 */

#include "pmu/max8907b/max8907b_supply_info_table.h"

static const NvOdmIoAddress s_enc28j60EthernetAddresses[] =
{
    { NvOdmIoModule_Spi, 1, 1, 0 },
    { NvOdmIoModule_Gpio, (NvU32)'c'-'a', 1, 0 }
};

static const NvOdmIoAddress s_SdioAddresses[] =
{
    { NvOdmIoModule_Sdio, 0x0, 0x0, 0 },
    { NvOdmIoModule_Sdio, 0x2, 0x0, 0 },
    { NvOdmIoModule_Sdio, 0x3, 0x0, 0 },
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO12, 0 }, /* VDDIO_SDIO -> VOUT12 */
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO5, 0 } /* VCORE_MMC -> VOUT05 */
};

static const NvOdmIoAddress s_VibAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x0, Max8907bPmuSupply_LDO16, 0 },
};

