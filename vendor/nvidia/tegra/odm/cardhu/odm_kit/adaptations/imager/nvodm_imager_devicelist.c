/*
 * Copyright (c) 2006-2010 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_imager_device.h"
#include "nvodm_query_discovery.h"
#include "sensors/nvodm_sensor_private_guids.h"
#include "nvodm_imager_guids.h" // located in query/include

#ifdef NV_IMAGER_HOST_ID
#include "sensors/nvodm_sensor_host.h"
#include "sensors/nvodm_sensor_null.h"
#endif

#ifdef MI5130_GUID
#include "sensors/nvodm_sensor_mi5130.h"
#endif

static NvOdmImagerDeviceFptrs gs_DeviceTable[] = {
    // Always available, the host input
#ifdef NV_IMAGER_HOST_ID
    {NV_IMAGER_HOST_ID, Sensor_Host_Open, Sensor_Host_Close},
#endif
    // real sensors
#ifdef MI5130_GUID
    {MI5130_GUID, Sensor_MI5130_Open, Sensor_MI5130_Close},
#endif
    // real sensors
#ifdef SENSOR_BAYER_OV5630_GUID
    {SENSOR_BAYER_OV5630_GUID , SensorBayerOV5630_Open, SensorBayerOV5630_Close},
#endif
    // virtual sensors (for testing)
#ifdef NV_IMAGER_NULLYUV_ID
    {NV_IMAGER_HOST_ID, Sensor_Host_Open, Sensor_Host_Close},
    {NV_IMAGER_NULLYUV_ID, Sensor_NullYuv_Open, Sensor_Null_Close},
    {NV_IMAGER_NULLBAYER_ID, Sensor_NullBayer_Open, Sensor_Null_Close},
#endif
};

// Will replace the other function
void
NvOdmImagerGetDeviceFptrs(
    NvU64 GUID,
    NvOdmImagerDeviceFptrs *pDeviceFptrs,
    NvBool *VirtualDevice)
{
    NvU32 i;
    for (i = 0; i < NV_ARRAY_SIZE(gs_DeviceTable); i++)
    {
        if (gs_DeviceTable[i].DeviceGUID == GUID)
        {
            *pDeviceFptrs = gs_DeviceTable[i];
            *VirtualDevice = ((GUID >> 56ULL) & 0xFF) == 'v';
            return;
        }
    }
    pDeviceFptrs->DeviceGUID = 0; // clear first to indicate failure
    return;
}

NvS32 NvOdmImagerCountDevices(void)
{
    return NV_ARRAY_SIZE(gs_DeviceTable);
}

void NvOdmImagerGetDeviceTable(NvOdmImagerDeviceTable *pTable)
{
    *pTable = gs_DeviceTable;
}

