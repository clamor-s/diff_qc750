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
 *                 database NvOdmIoAddress entries for the E1215
 *                 Voyager Main Board.
 */

#include "pmu/max8907b/max8907b_supply_info_table.h"

// Accelerometer
static const NvOdmIoAddress s_ffaAccelerometerAddresses[] =
{
    { NvOdmIoModule_I2c,  0x00, 0x3A, 0 }, /* I2C device address is 0x3A */
//    { NvOdmIoModule_Gpio, 0x0, 0x0, 0 }, /* FIXME - need appropriate setting for Voyager (if any) */
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LX_V3, 0  },  /* IO_ACC -> V3 */
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO1, 0 }   /* VCORE_ACC -> VOUT1 */
};

