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
 *                 database NvOdmIoAddress entries for the E888
 *                 audio module.
 */

#include "pmu/max8907b/max8907b_supply_info_table.h"

// Audio Codec
static const NvOdmIoAddress s_AudioCodecAddresses[] =
{
    { NvOdmIoModule_Vdd, 0x00, Max8907bPmuSupply_LDO7, 0},  /* AUDIO_PLL etc -> DCD2 */
    { NvOdmIoModule_ExternalClock, 0, 0, 0 },                  // connected to CDEV1
#if 1
    { NvOdmIoModule_Spi, 2,     1, 0 },                      /* FFA Audio codec on SP3- CS1*/
#else
    { NvOdmIoModule_I2c_Pmu, 0, 0x34, 0},          /* FFA Audio codec on DVC*/

#endif
    { NvOdmIoModule_Dap, 0, 0, 0 },                            /* Dap port Index 0 is used for codec*/
};
