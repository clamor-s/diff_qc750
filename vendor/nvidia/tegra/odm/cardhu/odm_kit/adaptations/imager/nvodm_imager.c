/*
 * Copyright (c) 2006-2010 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_imager.h"
#include "nvodm_imager_int.h"
#include "nvodm_imager_guids.h"
#include "nvodm_services.h"

#include "nvos.h"

#define HACK_FOR_EEPROM_WAR 0

// Board Info.  0xB -> 11, 0xF -> 15, so 0xB0F is E1115's id.
#define UNKNOWN_BOARDINFO_IDENTIFIER                     0xFFFF
#define VIP_EXTCAMBOARD_BOARDINFO_IDENTIFIER_E911        0x090B

// gs_CameraBoardList is the list of boards that we detect
// as new boards or devices (voyager?) get added, just add to this list
static NvU32 gs_CameraBoardList[] = {
    VIP_EXTCAMBOARD_BOARDINFO_IDENTIFIER_E911};

/** Implementation for the NvOdm Imager */

static NvOdmImagerDevice
       nvOdmImagerParmTypeTable[NvOdmImagerParameter_Num];
static NvBool nvOdmImagerParmTypeTableInit = NV_FALSE;

// DetectBoard iterates thru the board list given to it
// Returns UNKNOWN_BOARDINFO_IDENTIFIER if no matching board is found
static NvU32
DetectBoard(NvU32 *BoardList, NvU32 BoardCount, NvOdmBoardInfo *pBoardConnected)
{
    NvU32 board = UNKNOWN_BOARDINFO_IDENTIFIER;
    NvBool Detected;
    NvU32 i;
    for (i = 0; i < BoardCount; i++)
    {
        Detected = NvOdmPeripheralGetBoardInfo(BoardList[i],
                                              pBoardConnected);
        if (Detected)
        {
            board = BoardList[i];
            break;
        }
    }
    return board;
}



static void nvOdmImagerInitParmTypeTable(void)
{
    NvS32 i;

    for (i = 0; i < NvOdmImagerParameter_Num; i++) {
        nvOdmImagerParmTypeTable[i] = NvOdmImagerDevice_Sensor;
    }
}


NvBool
NvOdmImagerGetDevices(NvS32 *count, NvOdmImagerHandle *imager)
{
    NvBool bStatus;
    NvS32 i = 0;
    NvOdmImagerDeviceTable DeviceTable = NULL;
    NvOdmImagerRec *hImager = NULL;
    NvS32 DeviceCount;

    DeviceCount = NvOdmImagerCountDevices();

    if (*count == 0) {
        *count = DeviceCount;
        return NV_TRUE;
    }

    *count = NV_MIN(*count, DeviceCount);

    NvOdmImagerGetDeviceTable(&DeviceTable);

    for (i = 0; i < *count; i++) {
        imager[i] = NvOdmOsAlloc(sizeof(NvOdmImagerRec));
        if (imager[i] == NULL) {
            // Resource leak! ASD
            return NV_FALSE;
        }
        NvOdmOsMemset(imager[i], 0, sizeof(NvOdmImagerRec));
        hImager = (NvOdmImagerRec *)imager[i];
        {
            hImager->devices = NvOdmImagerDevice_SensorMask;
            hImager->fptrs[NvOdmImagerDevice_Sensor] = DeviceTable[i];
#if 0
            hImager->sensorConnections = NvOdmPeripheralGetGuid(
                                      currSensorTable->SensorGUID);
#endif
            bStatus = hImager->fptrs[NvOdmImagerDevice_Sensor].
                Open(&hImager->contexts[NvOdmImagerDevice_Sensor]);
            if (!bStatus) {
                NvOdmOsDebugPrintf("Imager open error\n");
                return NV_FALSE;
            }
        }
    }

    //nvOdmImagerAssemble(*count,imager,"Micron 5130");
    return NV_TRUE;
}

void
NvOdmImagerReleaseDevices(NvS32 count, NvOdmImagerHandle *imager)
{
    NvS32 i, j;
    NvOdmImagerRec *hImager = NULL;

    for (i = 0; i < count; i++) {
        hImager = (NvOdmImagerRec *)imager[i];
        for (j = 0; j < NvOdmImagerDevice_Num; j++)
        {
            if (*hImager->fptrs[j].Close != NULL)
                (*hImager->fptrs[j].Close)(&hImager->contexts[j]);
        }
        NvOdmOsFree(imager[i]);
    }
}

#define NVODM_IMAGER_MAX_CLOCKS 3

#if HACK_FOR_EEPROM_WAR
#include "../../adaptations/pmu/nvodm_pmu_pcf50626_supply_info.h"
#include "../../query/subboards/nvodm_query_discovery_e911_addresses.h"
static NvOdmPeripheralConnectivity s_Peripherals_Local[] =
{
#include "../../query/subboards/nvodm_query_discovery_e911_peripherals.h"
};
#endif

NvBool
NvOdmImagerOpen(NvU64 ImagerGUID, NvOdmImagerHandle *phImager)
{
    NvOdmImagerRec *hImager;
    NvU64 SensorGUID;
    NvBool VirtualSensor, bStatus;
    char guidstr[9] = {0};
    const NvOdmPeripheralConnectivity *sensorConnections=NULL;
    NvU32 ClockInstances[NVODM_IMAGER_MAX_CLOCKS];
    NvU32 ClockFrequencies[NVODM_IMAGER_MAX_CLOCKS];
    NvU32 NumClocks;

    hImager = NvOdmOsAlloc(sizeof(NvOdmImagerRec));
    if (hImager == NULL)
        return NV_FALSE;
    NvOdmOsMemset(hImager, 0, sizeof(NvOdmImagerRec));

    if ((ImagerGUID == 0) || (ImagerGUID == 1))
    {
        NvOdmPeripheralSearch FindImagerAttr =
            NvOdmPeripheralSearch_PeripheralClass;
        NvU32 FindImagerVal = (NvU32)NvOdmPeripheralClass_Imager;
        NvU32 count;
        NvU64 GUIDs[3];
        NvOdmBoardInfo BoardInfo;
        NvU32 CameraBoardDetected;

        // Find out what kind of boards we are running on
        CameraBoardDetected = DetectBoard(gs_CameraBoardList,
                                  NV_ARRAY_SIZE(gs_CameraBoardList),
                                  &BoardInfo);

        if (CameraBoardDetected != UNKNOWN_BOARDINFO_IDENTIFIER)
        {
            NvOsDebugPrintf("Detected Board E%d%d SKU %d Fab %d Rev %d\n",
                            (BoardInfo.BoardID >> 8) & 0xFF,
                            BoardInfo.BoardID & 0x00FF,
                            BoardInfo.SKU,
                            BoardInfo.Fab,
                            BoardInfo.Revision);
        }

        count = NvOdmPeripheralEnumerate(&FindImagerAttr,
                                         &FindImagerVal, 1,
                                         GUIDs, 3);
        if ((ImagerGUID == 0) && (count > 0))
        {
            SensorGUID = GUIDs[0];
        }
        else if ((ImagerGUID == 1) && (count > 1))
        {
            SensorGUID = GUIDs[1];
        }
        else
        {
            NvOdmOsFree(hImager);
            return NV_FALSE;
        }
    }
    else
    {
        // use the specified guid
        SensorGUID = ImagerGUID;
    }

    // setup for sensor
    NvOdmImagerGetDeviceFptrs(SensorGUID,
                               &hImager->fptrs[NvOdmImagerDevice_Sensor],
                               &VirtualSensor);
    // Try a local version of the query
    if (hImager->fptrs[NvOdmImagerDevice_Sensor].DeviceGUID != SensorGUID)
    {
#if HACK_FOR_EEPROM_WAR
        sensorConnections = s_Peripherals_Local;
        SensorGUID = s_Peripherals_Local[0].Guid;
        NvOdmImagerGetDeviceFptrs(SensorGUID,
                                  &hImager->fptrs[NvOdmImagerDevice_Sensor],
                                  &VirtualSensor);
#endif
    }
    else
    {
        sensorConnections = NvOdmPeripheralGetGuid(SensorGUID);
    }

    if ((sensorConnections == NULL) && (!VirtualSensor))
    {
        //NV_ASSERT(!"No driver code found for this sensor");
        NvOdmOsMemcpy(guidstr, &(SensorGUID), sizeof(guidstr)-1);
        guidstr[8] = 0;
        NvOdmOsDebugPrintf("Error, guid %s not found\n", guidstr);
        NV_ASSERT(!"Error in guid");
        NvOdmOsFree(hImager);
        return NV_FALSE;
    }

    if (!VirtualSensor)
    {
        bStatus = NvOdmExternalClockConfig(
                  COMMONIMAGER_GUID, NV_FALSE, ClockInstances,
                  ClockFrequencies, &NumClocks);
        if (bStatus == NV_FALSE)
        {
            NV_ASSERT(bStatus);
            NvOdmOsFree(hImager);
            return NV_FALSE;
        }
    }
    bStatus = hImager->fptrs[NvOdmImagerDevice_Sensor].Open(
                    &hImager->contexts[NvOdmImagerDevice_Sensor]);
    if (!bStatus)
    {
        if (!VirtualSensor)
        {
            bStatus = NvOdmExternalClockConfig(
                COMMONIMAGER_GUID, NV_TRUE, ClockInstances,
                ClockFrequencies, &NumClocks);
        }
        NV_ASSERT(bStatus);
        NvOdmOsFree(hImager);
        return NV_FALSE;
    }
    hImager->contexts[NvOdmImagerDevice_Sensor].connections = sensorConnections;

    hImager->devices = NvOdmImagerDevice_SensorMask;
    *phImager = (NvOdmImagerHandle)hImager;

    return NV_TRUE;
}

void
NvOdmImagerClose(NvOdmImagerHandle hImager)
{
    NvOdmImagerRec *pImager = (NvOdmImagerRec *)hImager;
    NvBool bStatus;
    NvU32 i;
    NvU32 ClockInstances[NVODM_IMAGER_MAX_CLOCKS];
    NvU32 ClockFrequencies[NVODM_IMAGER_MAX_CLOCKS];
    NvU32 NumClocks;
    const NvOdmPeripheralConnectivity *sensorConnections;
    for (i = 0; i < NvOdmImagerDevice_Num; i++)
    {
        if (*pImager->fptrs[i].Close != NULL)
        (*pImager->fptrs[i].Close)(&pImager->contexts[i]);
    }
    sensorConnections = NvOdmPeripheralGetGuid(
            pImager->fptrs[NvOdmImagerDevice_Sensor].DeviceGUID);
    if (sensorConnections != NULL)
    {
        bStatus = NvOdmExternalClockConfig(COMMONIMAGER_GUID,
            NV_TRUE, ClockInstances, ClockFrequencies, &NumClocks);
        NV_ASSERT(bStatus);
    }
    NvOdmOsFree(pImager);
}

/* HAL Redirection */

void NvOdmImagerGetCapabilities(
        NvOdmImagerHandle imager,
        NvOdmImagerCapabilities *capabilities)
{
    NvOdmImagerRec *hImager = (NvOdmImagerRec *)imager;
    if ((hImager->devices & NvOdmImagerDevice_SensorMask) != 0) {
        hImager->contexts[NvOdmImagerDevice_Sensor].GetCapabilities(&hImager->contexts[NvOdmImagerDevice_Sensor],capabilities);
        if (capabilities->CapabilitiesEnd != NVODM_IMAGER_CAPABILITIES_END)
        {
            NvOdmOsPrintf("Warning, this NvOdm adaptation %s is out of date, the ",
                       capabilities->identifier);
            NvOdmOsPrintf("last value in the caps should be 0x%x, but is 0x%x\n",
                       NVODM_IMAGER_CAPABILITIES_END,
                       capabilities->CapabilitiesEnd);
        }
    }
}

void NvOdmImagerListSensorModes(
        NvOdmImagerHandle imager,
        NvOdmImagerSensorMode *modes,
        NvS32 *NumberOfModes)
{
    NvOdmImagerRec *hImager = (NvOdmImagerRec *)imager;
    if ((hImager->devices & NvOdmImagerDevice_SensorMask) == 0) {
        *NumberOfModes = 0;
        return;
    }
    hImager->contexts[NvOdmImagerDevice_Sensor].ListModes(&hImager->contexts[NvOdmImagerDevice_Sensor],
                              modes,NumberOfModes);
}

NvBool
NvOdmImagerSetSensorMode(
    NvOdmImagerHandle hImager,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult)
{
    NvOdmImagerRec *pImager = (NvOdmImagerRec *)hImager;
    if ((pImager->devices & NvOdmImagerDevice_SensorMask) == 0) {
        return NV_FALSE;
    }
    return pImager->contexts[NvOdmImagerDevice_Sensor].SetMode(
                             &pImager->contexts[NvOdmImagerDevice_Sensor],
                             pParameters,
                             pSelectedMode,
                             pResult);
}

NvBool NvOdmImagerSetPowerLevel(
        NvOdmImagerHandle imager,
        NvOdmImagerDeviceMask devices,
        NvOdmImagerPowerLevel PowerLevel)
{
    NvOdmImagerRec *hImager = (NvOdmImagerRec *)imager;
    NvOdmImagerDeviceMask ourDevs;
    NvBool result = NV_TRUE;

    if (hImager == NULL && PowerLevel == NvOdmImagerPowerLevel_Off)
    {
        return NV_TRUE;
    }
    else if (hImager == NULL)
    {
        return NV_FALSE;
    }

    ourDevs = hImager->devices & devices;

    if (ourDevs & NvOdmImagerDevice_SensorMask) {
        result = hImager->contexts[NvOdmImagerDevice_Sensor].SetPowerLevel(&hImager->contexts[NvOdmImagerDevice_Sensor],PowerLevel);
    }
    return result;
}

NvBool NvOdmImagerSetParameter(
        NvOdmImagerHandle imager,
        NvOdmImagerParameter param,
        NvS32 SizeOfValue,
        const void *value)
{
    NvOdmImagerRec *hImager = (NvOdmImagerRec *)imager;
    NvOdmImagerDevice trgDevice;

    if (! nvOdmImagerParmTypeTableInit) {
        nvOdmImagerInitParmTypeTable();
        nvOdmImagerParmTypeTableInit = NV_TRUE;
    }
    trgDevice = nvOdmImagerParmTypeTable[param];

    if ((trgDevice == NvOdmImagerDevice_Sensor) ||
        (trgDevice == NvOdmImagerDevice_Num)) {
        if (hImager->devices & NvOdmImagerDevice_SensorMask) {
            return hImager->contexts[NvOdmImagerDevice_Sensor].SetParameter(&hImager->contexts[NvOdmImagerDevice_Sensor],
                           param,SizeOfValue,value);
        } else {
            return NV_FALSE;
        }
    }
    return NV_FALSE;
}

NvBool NvOdmImagerGetParameter(
        NvOdmImagerHandle imager,
        NvOdmImagerParameter param,
        NvS32 SizeOfValue,
        void *value)
{
    NvOdmImagerRec *hImager = (NvOdmImagerRec *)imager;
    NvOdmImagerDevice trgDevice;

    if (! nvOdmImagerParmTypeTableInit) {
        nvOdmImagerInitParmTypeTable();
        nvOdmImagerParmTypeTableInit = NV_TRUE;
    }
    trgDevice = nvOdmImagerParmTypeTable[param];

    if (trgDevice == NvOdmImagerDevice_Sensor) {
        if (hImager->devices & NvOdmImagerDevice_SensorMask) {
            return hImager->contexts[NvOdmImagerDevice_Sensor].GetParameter(&hImager->contexts[NvOdmImagerDevice_Sensor],
                           param,SizeOfValue,value);
        } else {
            return NV_FALSE;
        }
    }

    return NV_FALSE;
}
