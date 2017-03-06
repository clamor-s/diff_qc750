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
 *                 database NvOdmIoAddress entries for the E936
 *                 ISDB-T module.
 */

#include "pmu/max8907b/max8907b_supply_info_table.h"

// VIP raw bitstream addresses
static const NvOdmIoAddress s_ffaVIPBitstreamAddresses[] =
{
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LX_V3, 0  },
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO9 , 0},
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO18, 0 },
    // Reset. vgp0 and vgp5:
    { NvOdmIoModule_Gpio, 27, 1, 0 },      // vgp[0] - Port BB(27), Pin 1
    { NvOdmIoModule_Gpio, 'd'-'a', 2, 0 }, // vgp[5] - Port D, Pin 2
};

