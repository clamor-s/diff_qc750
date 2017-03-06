/*
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "imager_hal.h"
#include "sensor_bayer_ar0832.h"
#include "sensor_bayer_ov5630.h"
#include "sensor_bayer_ov5650.h"
#include "sensor_bayer_ov9726.h"
#include "sensor_bayer_ov2710.h"
#include "sensor_bayer_ov14810.h"
#include "focuser_ar0832.h"
#include "focuser_sh532u.h"
#include "focuser_ad5820.h"
#include "focuser_nvc.h"
#include "flash_ltc3216.h"
#include "flash_ssl3250a.h"
#include "flash_tps61050.h"
#include "torch_nvc.h"
#include "sensor_yuv_soc380.h"
#include "sensor_yuv_ov5640.h"
#include "sensor_yuv_hm2057.h"
#include "sensor_yuv_gc0308.h"
#include "sensor_yuv_t8ev5.h"
#include "sensor_null.h"
#include "sensor_host.h"

#define HAL_DB_PRINTF(...)
//#define HAL_DB_PRINTF(...)  NvOdmOsPrintf(__VA_ARGS__)
#define NVODM_IMAGER_MAX_CLOCKS (3)

typedef struct DeviceHalTableRec
{
    NvU64 GUID;
    pfnImagerGetHal pfnGetHal;
}DeviceHalTable;

typedef enum
{
    NvOdmImagerSubDeviceType_Sensor,
    NvOdmImagerSubDeviceType_Focuser,
    NvOdmImagerSubDeviceType_Flash,
} NvOdmImagerSubDeviceType;

static NV_INLINE NvOdmImagerSubDeviceType
GetSubDeviceForParameter(
    NvOdmImagerParameter Param);

char *guidStr(NvU64 guid);

/**
 * g_SensorHalTable
 * Consist of currently supported sensor guids and their GetHal functions.
 * The sensors listed here must have a peripherial connection.
 */
DeviceHalTable g_SensorHalTable[] =
{
    {SENSOR_BAYER_OV5630_GUID,   SensorBayerOV5630_GetHal},
    {SENSOR_BAYER_OV5650_GUID,   SensorBayerOV5650_GetHal},
    {SENSOR_BYRRI_AR0832_GUID,   SensorBayerAR0832_GetHal},
    {SENSOR_BYRLE_AR0832_GUID,   SensorBayerAR0832_GetHal},
    {SENSOR_BYRST_OV5630_GUID,   SensorBayerOV5630_GetHal},
    {SENSOR_BYRST_OV5650_GUID,   SensorBayerOV5650_GetHal},
    {SENSOR_BYRST_AR0832_GUID,   SensorBayerAR0832_GetHal},
    {SENSOR_BAYER_OV9726_GUID,   SensorBayerOV9726_GetHal},
    {SENSOR_BAYER_OV2710_GUID,   SensorBayerOV2710_GetHal},
    {SENSOR_YUV_SOC380_GUID,     SensorYuvSOC380_GetHal},
    {SENSOR_YUV_OV5640_GUID,     SensorYuvOV5640_GetHal},
    {SENSOR_BAYER_OV14810_GUID,  SensorBayerOV14810_GetHal},
    {SENSOR_YUV_HM2057_GUID,  SensorYuv_Hm2057_GetHal},//jimmy add
    {SENSOR_YUV_GC0308_GUID,  SensorYuv_Gc0308_GetHal},//jimmy add
    {SENSOR_T8EV5_GUID,  SensorYuv_T8EV5_GetHal},//jimmy add
};

/**
 * g_VirtualSensorHalTable
 * Consist of currently supported virtual sensor guids and their GetHal
 * functions. The virtual sensors listed here do not need a peripherial
 * connection.
 */
DeviceHalTable g_VirtualSensorHalTable[] =
{
    // default virtual sensor listed first
    {NV_IMAGER_NULLYUV_ID,   SensorNullYuv_GetHal},
    {NV_IMAGER_NULLBAYER_ID, SensorNullBayer_GetHal},
    {NV_IMAGER_HOST_ID,      SensorHost_GetHal},
};

/**
 * g_FocuserHalTable
 * Consist of currently supported focuser guids and their GetHal functions.
 */
DeviceHalTable g_FocuserHalTable[] =
{
#if (BUILD_FOR_AOS == 0)
    {FOCUSER_SH532U_GUID, FocuserSH532U_GetHal},
#endif
    {FOCUSER_AD5820_GUID, FocuserAD5820_GetHal},
    {FOCUSER_AR0832_GUID, FocuserAR0832_GetHal},
    {FOCUSER_NVC_GUID, FocuserNvc_GetHal},
    {FOCUSER_NVC_GUID1, FocuserNvc_GetHal},
    {FOCUSER_NVC_GUID2, FocuserNvc_GetHal},
    {FOCUSER_NVC_GUID3, FocuserNvc_GetHal},
};

/**
 * g_FlashHalTable
 * Currently supported flash guids and their GetHal functions.
 */
DeviceHalTable g_FlashHalTable[] =
{
    {FLASH_LTC3216_GUID, FlashLTC3216_GetHal},
    {FLASH_SSL3250A_GUID, FlashSSL3250A_GetHal},
    {FLASH_TPS61050_GUID, FlashTPS61050_GetHal},
    {TORCH_NVC_GUID, TorchNvc_GetHal},
};

char *guidStr(NvU64 guid);

NvBool
NvOdmImagerGetDevices(
    NvS32 *pCount,
    NvOdmImagerHandle *phImagers)
{
    return NV_FALSE;
}

void
NvOdmImagerReleaseDevices(
    NvS32 Count,
    NvOdmImagerHandle *phImagers)
{
    NvS32 i;
    NvOdmImager *pImager = NULL;

    for (i = 0; i < Count; i++)
    {
        // Close sensor
        pImager = (NvOdmImager *)phImagers[i];
        if (pImager->pSensor)
            pImager->pSensor->pfnClose(pImager);

        //Close focuser
        if (pImager->pFocuser)
            pImager->pFocuser->pfnClose(pImager);

        //Close flash
        if (pImager->pFlash)
            pImager->pFlash->pfnClose(pImager);

        NvOdmOsFree(pImager->pSensor);
        NvOdmOsFree(pImager->pFocuser);
        NvOdmOsFree(pImager->pFlash);
        NvOdmOsFree(pImager);
    }
}

char *guidStr(NvU64 guid)
{
    static char guidstr[9];
    int i;
    for(i=7;i>=0;i--)
    {
        guidstr[i] = (char)(0xFFULL & guid);
        guid /= 0x100;
    }
    guidstr[8] = 0;
    return guidstr;
}

NvBool
NvOdmImagerOpen(
    NvU64 ImagerGUID,
    NvOdmImagerHandle *phImager)
{
    NvOdmImager *pImager = NULL;
    NvBool Result = NV_TRUE;
    pfnImagerGetHal pfnGetHal = NULL;
    NvU32 ClockInstances[NVODM_IMAGER_MAX_CLOCKS];
    NvU32 ClockFrequencies[NVODM_IMAGER_MAX_CLOCKS];
    NvU32 NumClocks;
    NvOdmImagerCapabilities SensorCaps;
    NvU32 i;
    NvBool NeedConnectivity = NV_TRUE;
    NvBool VirtualImager = NV_FALSE;

    *phImager = NULL;
    HAL_DB_PRINTF("%s +++, ImagerGUID(%s)\n", __func__, guidStr(ImagerGUID));
    pImager = (NvOdmImager*)NvOdmOsAlloc(sizeof(NvOdmImager));
    if (!pImager){
        HAL_DB_PRINTF("%s %d: couldn't allocate memory for an imager\n", __func__, __LINE__);
        goto fail;
    }

    NvOdmOsMemset(pImager, 0, sizeof(NvOdmImager));

    //find the sensor guid for ImagerGUID =0 to NVODM_IMAGER_MAX_REAL_IMAGER_GUID
    if (ImagerGUID < NVODM_IMAGER_MAX_REAL_IMAGER_GUID)
    {
        NvOdmPeripheralSearch FindImagerAttr =
            NvOdmPeripheralSearch_PeripheralClass;
        NvU32 FindImagerVal = (NvU32)NvOdmPeripheralClass_Imager;
        NvU32 Count;
        NvU64 GUIDs[NVODM_IMAGER_MAX_REAL_IMAGER_GUID];

        Count = NvOdmPeripheralEnumerate(&FindImagerAttr,
                                         &FindImagerVal, 1,
                                         GUIDs, NVODM_IMAGER_MAX_REAL_IMAGER_GUID);
        if (Count == 0)
        {
            // if no sensor in database, fall back to the default
            // virtual sensor
            HAL_DB_PRINTF("%s %d: no sensors found\n", __func__, __LINE__);
            ImagerGUID = g_VirtualSensorHalTable[0].GUID;
        }
        else if (Count > ImagerGUID)
        {
            ImagerGUID = GUIDs[ImagerGUID];
            HAL_DB_PRINTF("using ImagerGUID '%s' (%d)\n", guidStr(ImagerGUID), Count);
        }
        else
        {
            HAL_DB_PRINTF("%s %d: no sensors found\n", __func__, __LINE__);
            NV_ASSERT(!"Could not find any devices");
            goto fail;
        }
    }

    HAL_DB_PRINTF("real using ImagerGUID '%s'\n", guidStr(ImagerGUID));
    // search real sensors first.
    for (i = 0; i < NV_ARRAY_SIZE(g_SensorHalTable); i++)
    {
        if (ImagerGUID == g_SensorHalTable[i].GUID)
        {
            pfnGetHal = g_SensorHalTable[i].pfnGetHal;
            VirtualImager = NV_FALSE;
            HAL_DB_PRINTF("%s %d: found a sensor (%d)\n", __func__, __LINE__, i);
            break;
        }
    }

    // if no real sensor found, search virtual sensors
    if (!pfnGetHal)
    {
        HAL_DB_PRINTF("%s %d: couldn't get the sensor HAL table for an imager\n",
                        __func__, __LINE__);
        for (i = 0; i < NV_ARRAY_SIZE(g_VirtualSensorHalTable); i++)
        {
            if (ImagerGUID == g_VirtualSensorHalTable[i].GUID)
            {
                pfnGetHal = g_VirtualSensorHalTable[i].pfnGetHal;
                VirtualImager = NV_TRUE;
                NeedConnectivity = NV_FALSE;
                break;
            }
        }
    }

    if (!pfnGetHal)
        goto fail;

    // Get sensor HAL
    pImager->pSensor =
        (NvOdmImagerSensor*)NvOdmOsAlloc(sizeof(NvOdmImagerSensor));
    if (!pImager->pSensor)
        goto fail;

    NvOdmOsMemset(pImager->pSensor, 0, sizeof(NvOdmImagerSensor));


    Result = pfnGetHal(pImager);
    if (!Result)
        goto fail;

    if (VirtualImager)
        pImager->pSensor->GUID = g_VirtualSensorHalTable[i].GUID;
    else
        pImager->pSensor->GUID = g_SensorHalTable[i].GUID;

    if (NeedConnectivity)
    {
        // Get Sensor's connections
        pImager->pSensor->pConnections = NvOdmPeripheralGetGuid(ImagerGUID);
        if (!pImager->pSensor->pConnections)
        {
            goto fail;
        }

        // Get the clocks
        Result = NvOdmExternalClockConfig(
            ImagerGUID, NV_FALSE, ClockInstances,
            ClockFrequencies, &NumClocks);
        if (Result == NV_FALSE)
            goto fail;
    }

    // Open the sensor
    Result = pImager->pSensor->pfnOpen(pImager);
    if (!Result)
    {
        Result = NvOdmExternalClockConfig(
                    ImagerGUID, NV_TRUE, ClockInstances,
                    ClockFrequencies, &NumClocks);
        NV_ASSERT(Result);
        goto fail;
    }

    /* --- Focuser code --- */
    // Get sensor's focuser
    NvOdmImagerGetCapabilities(pImager, &SensorCaps);

    // No focuser?
    if (SensorCaps.FocuserGUID == 0)
        goto checkFlash;

    // Find focuser's GetHal
    pfnGetHal = NULL;
    for (i = 0; i < NV_ARRAY_SIZE(g_FocuserHalTable); i++)
    {
        if (SensorCaps.FocuserGUID == g_FocuserHalTable[i].GUID)
        {
            pfnGetHal = g_FocuserHalTable[i].pfnGetHal;
            break;
        }
    }

    HAL_DB_PRINTF("Using FocuserGUID '%s'\n", guidStr(SensorCaps.FocuserGUID));
    if (!pfnGetHal) {
        HAL_DB_PRINTF("%s %d: couldn't get the sensor HAL table for an imager\n", __func__, __LINE__);
        goto fail;
    }

    // Get focuser's HAL
    pImager->pFocuser =
        (NvOdmImagerSubdevice*)NvOdmOsAlloc(sizeof(NvOdmImagerSubdevice));
    if (!pImager->pFocuser)
        goto fail;

    NvOdmOsMemset(pImager->pFocuser, 0, sizeof(NvOdmImagerSubdevice));

    Result = pfnGetHal(pImager);
    if (!Result)
        goto fail;

    pImager->pFocuser->GUID = g_FocuserHalTable[i].GUID;
    Result = pImager->pFocuser->pfnOpen(pImager);
    if (!Result)
        goto fail;

checkFlash:
    /* --- Flash code --- */
    // No flash?
    if (SensorCaps.FlashGUID == 0)
        goto done;

    // Find flash's GetHal
    pfnGetHal = NULL;
    for (i = 0; i < NV_ARRAY_SIZE(g_FlashHalTable); i++)
    {
        if (SensorCaps.FlashGUID == g_FlashHalTable[i].GUID)
        {
            pfnGetHal = g_FlashHalTable[i].pfnGetHal;
            break;
        }
    }

    HAL_DB_PRINTF("Using FlashGUID '%s'\n", guidStr(SensorCaps.FlashGUID));
    if (!pfnGetHal) {
        HAL_DB_PRINTF("%s %d: couldn't get HAL table for flash\n", __func__, __LINE__);
        goto fail;
    }

    // Get flash's HAL
    pImager->pFlash =
        (NvOdmImagerSubdevice*)NvOdmOsAlloc(sizeof(NvOdmImagerSubdevice));
    if (!pImager->pFlash)
        goto fail;

    NvOdmOsMemset(pImager->pFlash, 0, sizeof(NvOdmImagerSubdevice));

    Result = pfnGetHal(pImager);
    if (!Result)
        goto fail;

    pImager->pFlash->GUID = g_FlashHalTable[i].GUID;
    Result = pImager->pFlash->pfnOpen(pImager);
    if (!Result)
    {
        NvOsDebugPrintf("%s %d: cannot open flash driver: error(%d)\n",
                __func__, __LINE__, Result);
        // Still returns TRUE even if kernel flash driver is not available.
        // Just treat this as if the sensor does not have flash capability,
        // i.e. set flash GUID to 0 in the odm flash driver.
        goto done;
    }

    pImager->pFlash->pConnections =
        NvOdmPeripheralGetGuid(SensorCaps.FlashGUID);
    if (!pImager->pFlash->pConnections)
        goto fail;

done:
    *phImager = pImager;
    return NV_TRUE;

fail:
    HAL_DB_PRINTF("%s FAILED!\n", __func__);
    NvOdmImagerClose(pImager);
    return NV_FALSE;
}

void NvOdmImagerClose(NvOdmImagerHandle hImager)
{
    NvBool Status;
    NvU32 ClockInstances[NVODM_IMAGER_MAX_CLOCKS];
    NvU32 ClockFrequencies[NVODM_IMAGER_MAX_CLOCKS];
    NvU32 NumClocks;

    if (!hImager)
        return;

    // Close the focuser
    if (hImager->pFocuser)
        hImager->pFocuser->pfnClose(hImager);

    // Close the flash
    if (hImager->pFlash)
        hImager->pFlash->pfnClose(hImager);

    // Close the sensor
    if (hImager->pSensor)
    {
        hImager->pSensor->pfnClose(hImager);

        if (hImager->pSensor->pConnections != NULL)
        {
            Status = NvOdmExternalClockConfig(
                hImager->pSensor->GUID,
                NV_TRUE, ClockInstances, ClockFrequencies, &NumClocks);
            NV_ASSERT(Status);
        }
    }

    // Cleanup
    NvOdmOsFree(hImager->pSensor);
    NvOdmOsFree(hImager->pFocuser);
    NvOdmOsFree(hImager->pFlash);
    NvOdmOsFree(hImager);
}

void
NvOdmImagerGetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
    // Get sensor capabilities
    if (hImager->pSensor)
    {
        hImager->pSensor->pfnGetCapabilities(hImager, pCapabilities);
        if (pCapabilities->CapabilitiesEnd != NVODM_IMAGER_CAPABILITIES_END)
        {
            HAL_DB_PRINTF(
                "Warning, this NvOdm adaptation %s is out of date, the ",
                pCapabilities->identifier);
            HAL_DB_PRINTF(
                "last value in the caps should be 0x%x, but is 0x%x\n",
                NVODM_IMAGER_CAPABILITIES_END,
                pCapabilities->CapabilitiesEnd);
        }
    }

    // Get focuser capabilities
    if (hImager->pFocuser)
    {
        hImager->pFocuser->pfnGetCapabilities(hImager, pCapabilities);
    }

    // Get flash capabilities
    if (hImager->pFlash)
    {
        hImager->pFlash->pfnGetCapabilities(hImager, pCapabilities);
    }
}



void
NvOdmImagerListSensorModes(
    NvOdmImagerHandle hImager,
    NvOdmImagerSensorMode *pModes,
    NvS32 *pNumberOfModes)
{
    if (hImager->pSensor)
    {
        hImager->pSensor->pfnListModes(hImager, pModes, pNumberOfModes);
    }
    else
        *pNumberOfModes = 0;
}

NvBool
NvOdmImagerSetSensorMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult)
{
    if (!hImager->pSensor)
        return NV_FALSE;

    return hImager->pSensor->pfnSetMode(hImager, pParameters, pSelectedMode,
           pResult);
}

NvBool
NvOdmImagerSetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerDeviceMask Devices,
    NvOdmImagerPowerLevel PowerLevel)
{
    NvBool Status = NV_TRUE;

    if (!hImager)
        return NV_FALSE;

    if (hImager->pSensor)
    {
        Status = hImager->pSensor->pfnSetPowerLevel(hImager, PowerLevel);
        if (!Status)
            return Status;
    }

    if (hImager->pFocuser)
    {
        Status = hImager->pFocuser->pfnSetPowerLevel(hImager, PowerLevel);
        if (!Status)
            return Status;
    }

    if (hImager->pFlash)
    {
        Status = hImager->pFlash->pfnSetPowerLevel(hImager, PowerLevel);
        if (!Status)
            return Status;
    }

    return Status;
}

static NV_INLINE NvOdmImagerSubDeviceType
GetSubDeviceForParameter(
    NvOdmImagerParameter Param)
{
    switch(Param)
    {
        case NvOdmImagerParameter_FocuserLocus:
        case NvOdmImagerParameter_FocalLength:
        case NvOdmImagerParameter_MaxAperture:
        case NvOdmImagerParameter_FNumber:
        case NvOdmImagerParameter_FocuserCapabilities:
        case NvOdmImagerParameter_FocuserStereo:
            return NvOdmImagerSubDeviceType_Focuser;

        case NvOdmImagerParameter_FlashCapabilities:
        case NvOdmImagerParameter_FlashLevel:
        case NvOdmImagerParameter_TorchCapabilities:
        case NvOdmImagerParameter_TorchLevel:
        case NvOdmImagerParameter_FlashPinState:
            return NvOdmImagerSubDeviceType_Flash;

        default:
            return NvOdmImagerSubDeviceType_Sensor;
    }
}

NvBool
NvOdmImagerSetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    const void *pValue)
{
    // Forward to appropriate sub-device
    switch(GetSubDeviceForParameter(Param))
    {
        case NvOdmImagerSubDeviceType_Sensor:
            if (!hImager->pSensor)
                return NV_FALSE;
            return hImager->pSensor->pfnSetParameter(
                hImager, Param, SizeOfValue, pValue);

        case NvOdmImagerSubDeviceType_Focuser:
            if (!hImager->pFocuser)
                return NV_FALSE;
            return hImager->pFocuser->pfnSetParameter(
                hImager, Param, SizeOfValue, pValue);

        case NvOdmImagerSubDeviceType_Flash:
            if (!hImager->pFlash)
                return NV_FALSE;
            return hImager->pFlash->pfnSetParameter(
                hImager, Param, SizeOfValue, pValue);

        default:
            NV_ASSERT(!"Impossible code path");
            return NV_FALSE;
    }
}

NvBool
NvOdmImagerGetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
    NvOdmImagerSubDeviceType SubDevice = NvOdmImagerSubDeviceType_Sensor;

    // Find the proper sub-device for the parameters
    // If the sub-device doesn't exist, always choose the sensor
    switch(GetSubDeviceForParameter(Param))
    {
        case NvOdmImagerSubDeviceType_Focuser:
            if (hImager->pFocuser)
                SubDevice = NvOdmImagerSubDeviceType_Focuser;
            break;
        case NvOdmImagerSubDeviceType_Flash:
            if (hImager->pFlash)
                SubDevice = NvOdmImagerSubDeviceType_Flash;
            break;
        default:
            break;
    }

    // Forward to appropriate sub-device
    switch(SubDevice)
    {
        case NvOdmImagerSubDeviceType_Sensor:
            if (!hImager->pSensor)
                return NV_FALSE;
            return hImager->pSensor->pfnGetParameter(
                hImager, Param, SizeOfValue, pValue);

        case NvOdmImagerSubDeviceType_Focuser:
            if (!hImager->pFocuser)
                return NV_FALSE;
            return hImager->pFocuser->pfnGetParameter(
                hImager, Param, SizeOfValue, pValue);

        case NvOdmImagerSubDeviceType_Flash:
            if (!hImager->pFlash)
                return NV_FALSE;
            return hImager->pFlash->pfnGetParameter(
                hImager, Param, SizeOfValue, pValue);

        default:
            NV_ASSERT(!"Impossible code path");
            return NV_FALSE;
    }
}

void
NvOdmImagerGetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerDevice Device,
    NvOdmImagerPowerLevel *pPowerLevel)
{
    NV_ASSERT("!Not Implemented!");
}
