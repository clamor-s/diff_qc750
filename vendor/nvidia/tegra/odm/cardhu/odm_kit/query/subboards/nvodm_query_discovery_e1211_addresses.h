/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 *
 * @file
 * <b>NVIDIA APX ODM Kit::
 *         Implementation of the ODM Peripheral Discovery API</b>
 *
 * @b Description: Specifies the peripheral connectivity database Peripheral
 *                 entries for the peripherals on E1294 module.
 */

#define NVODM_PORT(x) ((x) - 'a')

#define OV5650_PINS (NVODM_CAMERA_SERIAL_CSI_D1A | \
                     NVODM_CAMERA_DEVICE_IS_DEFAULT)

static const NvOdmIoAddress s_ffaImagerOV5650Addresses[] =
{
    { NvOdmIoModule_I2c,  0x02, 0x6C, 0 },
    { NvOdmIoModule_Gpio, 27, 0 | NVODM_IMAGER_RESET_AL, 0 },
    { NvOdmIoModule_Gpio, 27, 5 | NVODM_IMAGER_POWERDOWN, 0 },
    { NvOdmIoModule_Vdd,  0x00, CardhuPowerRails_VDD_1V8_CAM1, 0 },
    // Add TPS65911 ldo6 1200 ON
    //{ NvOdmIoModule_Vdd, 0x00, TPS6586xPmuSupply_LDO6, 0 },
    { NvOdmIoModule_VideoInput, 0x00, OV5650_PINS, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};
static const NvOdmIoAddress s_ffaImagerOV2710Addresses[] =
{
//    { NvOdmIoModule_Gpio, NVODM_GPIO_CAMERA_PORT, 3, 0 },
    { NvOdmIoModule_I2c, 0x08, 0x6C, 0 },
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_SERIAL_CSI_D1A, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};
static const NvOdmIoAddress s_ffaImagerOV5650SAddresses[] =
{
    { NvOdmIoModule_I2c, 0x06, 0x6C, 0 },
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_SERIAL_CSI_D1A, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};
static const NvOdmIoAddress s_ffaImagerOV5640Addresses[] =
{
    { NvOdmIoModule_I2c, 0x08, 0x78, 0 },
    { NvOdmIoModule_VideoInput, 0x00, NVODM_CAMERA_SERIAL_CSI_D1B, 0 },
    { NvOdmIoModule_ExternalClock, 2, 0, 0 } // CSUS
};

