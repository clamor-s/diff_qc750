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
 * @b Description: Specifies the peripheral connectivity database NvOdmIoAddress entries
 *                 for the E911 5MP + 1.3MP + VGA VI Camera module.
 */
#include "nvodm_query_gpio.h"
#include "../include/nvodm_imager_guids.h"
#include "pmu/max8907b/max8907b_supply_info_table.h"

#define NVODM_PORT(x) ((x) - 'a')
/* VGP5 is apparently inverted on some boards.
 * For E912- A01, you may need to change the VGP5_RESET_AL line to:
 * NVODM_CAMERA_VGP5_RESET
 * If you find other boards for which it needs to be inverted, please
 * add your information to this comment.
 */
#define OV5650_PINS (NVODM_CAMERA_SERIAL_CSI_D1A | \
                     NVODM_CAMERA_DEVICE_IS_DEFAULT)
static const NvOdmIoAddress s_ffaImagerOV5650Addresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x6C, 0 }, 
    { NvOdmIoModule_Gpio, NVODM_GPIO_CAMERA_PORT, 5 | NVODM_IMAGER_RESET_AL, 0 },
    { NvOdmIoModule_Gpio, NVODM_PORT('t'), 3 | NVODM_IMAGER_POWERDOWN, 0 },
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO18, 0 }, //VDDIO_VI
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO9, 0 }, //AVDD_CAM1
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO13, 0 }, //VDDIO_AF
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO17, 0 }, //VDDIO_MIPI
    { NvOdmIoModule_VideoInput, 0x00, OV5650_PINS, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};

static const NvOdmIoAddress s_ffaImagerOV5650StereoAddresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x6C, 0 }, 
    { NvOdmIoModule_I2c_Pmu,  0x00, 0x6C, 0 }, 
    { NvOdmIoModule_Gpio, NVODM_GPIO_CAMERA_PORT, 5 | NVODM_IMAGER_RESET_AL, 0 },
    { NvOdmIoModule_Gpio, NVODM_GPIO_CAMERA_PORT, 3 | NVODM_IMAGER_POWERDOWN_AL, 0 },
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO18, 0 }, //VDDIO_VI
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO9, 0 }, //AVDD_CAM1
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO13, 0 }, //VDDIO_AF
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO17, 0 }, //VDDIO_MIPI
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_SERIAL_CSI_D1A, 0 }, 
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};

// OV5650 focuser
static const NvOdmIoAddress s_ffaImagerAD5820Addresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x18, 0 },  // focuser i2c
};

// OV5650 flash
static const NvOdmIoAddress s_ffaFlashLTC3216Addresses[] =
{
    { NvOdmIoModule_Gpio, NVODM_GPIO_CAMERA_PORT, 3 | NVODM_IMAGER_FLASH0, 0 },  // Flash 200mA
    { NvOdmIoModule_Gpio, NVODM_GPIO_CAMERA_PORT, 6 | NVODM_IMAGER_FLASH1, 0 }   // Flash 600mA
};

// For SEMCO VGA
#define SOC380_PINS (NVODM_CAMERA_PARALLEL_VD0_TO_VD7)
static const NvOdmIoAddress s_ffaImagerSOC380Addresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x78, 0 },
    { NvOdmIoModule_Gpio, NVODM_GPIO_CAMERA_PORT, 4 | NVODM_IMAGER_POWERDOWN_AL, 0 },
    { NvOdmIoModule_Gpio, NVODM_GPIO_CAMERA_PORT, 0 | NVODM_IMAGER_RESET_AL, 0 },
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO18, 0}, //VDDIO_VI
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO9, 0 }, //AVDD_CAM2
    { NvOdmIoModule_Vdd,  0x00, Max8907bPmuSupply_LDO13, 0}, //VDDIO_AF
    { NvOdmIoModule_VideoInput, 0x00, SOC380_PINS, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};

static const NvOdmIoAddress s_CommonImagerAddresses[] =
{
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};


