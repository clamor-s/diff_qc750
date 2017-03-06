/*
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "focuser.h"
#include "imager_hal.h"
#include "imager_util.h"
#include "nvassert.h"

#define FOCUSER_POSITIONS               (1024)
#define FOCUSER_POSITION_INFINITY       (0)
/* According OV message, the Max, settle time is 250us if VCM clock is 20KHz */
/* So 10ms is long enough for setting position                                */
#define FOCUSER_MAX_FOCUS_SETTLE_TIME   (10)
#define FOCUSER_POSITION_SHIFT          (0)

#define TIME_DIFF(_from, _to) \
           (((_from) <= (_to)) ? ((_to) - (_from)) : \
                                 ((NvU32)~0 - ((_from) - (_to)) + 1))
#define DEBUG_PRINTS 0

NvU16 VcmClkDiv[2];
/**
 * Focuser's context
 */
typedef struct FocuserContextRec
{
    NvOdmImagerI2cConnection I2c;
    ImagerGpioConfig GpioConfig;
    NvOdmImagerPowerLevel PowerLevel;

    NvU32 MaxFocusPosition;       // Maximum allowed focus position
    NvU32 MaxMappedFocusPosition; // Maximum mapped focus position
    NvU32 MaxSettleTime;          // Maximum settling time for focusing.
    NvU32 LastCmdTime;            // the latest focus command was issued.
    NvS32 Position;              // The last settled focus position.
    NvS32 RequestedPosition;     // The last requested focus position.
    NvS32 DelayedFPos;          // Focusing position for delayed request
    NvOdmImagerFocuserPositionMap FocusPosMap;
                            // Focusing positions map provided by config. data
}FocuserContext;


/**
 * Static functions
 */
static NV_INLINE void Focuser_UpdateSettledPosition(FocuserContext *pContext);

static NvBool Focuser_SetPosition(FocuserContext *pContext, NvU32 Position);

static NvBool Focuser_Open(NvOdmImagerHandle hImager);

static void Focuser_Close(NvOdmImagerHandle hImager);

static NvBool
Focuser_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel);

static void
Focuser_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities);

static NvBool
Focuser_SetParameter(
        NvOdmImagerHandle hImager,
        NvOdmImagerParameter Param,
        NvS32 SizeOfValue,
        const void *pValue);

static NvBool
Focuser_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue);


static NV_INLINE void Focuser_UpdateSettledPosition(FocuserContext *pContext)
{
    NvU32 CurrentTime = 0;

    // Settled position has been updated?
    if (pContext->Position == pContext->RequestedPosition)
        return;

    CurrentTime = NvOdmOsGetTimeMS();

    // Previous requested position settled?
    if (TIME_DIFF(pContext->LastCmdTime, CurrentTime) >=
        pContext->MaxSettleTime)
    {                    
        pContext->Position = pContext->RequestedPosition;
    } 
}

/**
 * Focuser_SetPosition.
 * Write the focuser's register to set the position.
 */
static NvBool Focuser_SetPosition(FocuserContext *pContext, NvU32 Position)
{
    NvU16 Value[2];
    NvU32 AbsPosition = FOCUSER_POSITION_SHIFT + Position;

    Value[0] = (NvU8)((AbsPosition >> 4) & 0x3F); //0x30ec
    Value[1] = (NvU8)((AbsPosition & 0xf) << 4);  //0x30ed
    if (AbsPosition == FOCUSER_POSITION_INFINITY)
    {
        Value[0] |= 0x80;
    }

    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x30ec, Value[0]);
    NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x30ed, Value[1]);

#if DEBUG_PRINTS
    NvOdmOsDebugPrintf("Setting focuser position to %d\n", Position);
#endif

    return NV_TRUE;
}


static NvBool FocuserDirectControl(FocuserContext *pContext, 
                                NvU32 cLength, const NvS32 *control)
{
    NvS32 delay, currPos;
    NvU32 cnt;
    NvBool result = NV_TRUE;
  
#if DEBUG_PRINTS
    NvOdmOsDebugPrintf(">>>>>>>> FocuserDirectControl: [%d] {%d} %d-%d\n",
                         cLength,
                         control[NvOdmImagerDirectFocuserControlIndex_Wait],
                         control[NvOdmImagerDirectFocuserControlIndex_Move0],
                         control[NvOdmImagerDirectFocuserControlIndex_Move1]);
#endif

    delay = control[NvOdmImagerDirectFocuserControlIndex_Wait]; // settling delay for control sequence moves.
    cnt = NvOdmImagerDirectFocuserControlIndex_Move0;
    currPos = pContext->RequestedPosition;
    while (cnt < cLength) {
        currPos += control[cnt];
#if DEBUG_PRINTS
        NvOdmOsDebugPrintf("currPos = %d \n", currPos);
#endif
        if (currPos < 0) {
            currPos = 0;
        } else if (currPos > (NvS32)pContext->MaxFocusPosition) {
            currPos = (NvS32)pContext->MaxFocusPosition;
        }        
        cnt += 1;
    }
#if DEBUG_PRINTS
    NvOdmOsDebugPrintf("new currPos = %d \n", currPos);
#endif

    // currPos is what we should set
    result = Focuser_SetPosition(pContext,(NvU32)currPos);
    if (result) {
        pContext->RequestedPosition = currPos;
        pContext->LastCmdTime = NvOdmOsGetTimeMS();
    }
    if (delay > 0) {
        NvOdmOsSleepMS(delay);
    }
    return NV_TRUE;
}

/**
 * Focuser_Open
 * Initialize Focuser's private context.
 */
static NvBool Focuser_Open(NvOdmImagerHandle hImager)
{
    FocuserContext *pFocuserContext = NULL;
    NvU32 Divider;
    NvOdmImagerCapabilities SensorCaps;
    NvOdmImagerI2cConnection *pI2c = NULL;
    
    if (!hImager || !hImager->pFocuser)
        return NV_FALSE;

    pFocuserContext = (FocuserContext *)NvOdmOsAlloc(sizeof(FocuserContext));
    if (!pFocuserContext)
        goto fail;

    NvOdmOsMemset(pFocuserContext, 0, sizeof(FocuserContext));
    
    // Setup the i2c bit widths so we can call the
    // generalized write/read utility functions.
    // This is done here, since it will vary from sensor to sensor.
    pI2c = &pFocuserContext->I2c;
    pI2c->AddressBitWidth = 16;
    pI2c->DataBitWidth = 16;

    pFocuserContext->PowerLevel = NvOdmImagerPowerLevel_Off;
    pFocuserContext->MaxFocusPosition = FOCUSER_POSITIONS - 1;
    pFocuserContext->MaxMappedFocusPosition = 1;    
    pFocuserContext->MaxSettleTime = FOCUSER_MAX_FOCUS_SETTLE_TIME;
    pFocuserContext->LastCmdTime = 0;
    pFocuserContext->Position = FOCUSER_POSITION_INFINITY;
    pFocuserContext->RequestedPosition = FOCUSER_POSITION_INFINITY;
    pFocuserContext->FocusPosMap.mapPositionsNum = 1;
    pFocuserContext->FocusPosMap.map[0] = FOCUSER_POSITION_INFINITY;
    pFocuserContext->DelayedFPos = (-1);
    hImager->pFocuser->pPrivateContext = pFocuserContext;

    // Prepare to set VCM clock to 20Khz
    NvOdmImagerGetCapabilities(hImager, &SensorCaps);
    Divider = (SensorCaps.ClockProfiles[0].ExternalClockKHz+19)/20;
    VcmClkDiv[0] = (NvU16)(Divider & 0xff);
    VcmClkDiv[1] = (NvU16)((Divider>>8) & 0xf);

    return NV_TRUE;

fail:
    NvOdmOsFree(pFocuserContext);
    return NV_FALSE;

}

/**
 * Focuser_Close
 * Free focuser's context and resources.
 */
static void Focuser_Close(NvOdmImagerHandle hImager)
{
    FocuserContext *pContext = NULL;

    if (!hImager || ! hImager->pFocuser || !hImager->pFocuser->pPrivateContext)
        return;

    pContext = (FocuserContext*)hImager->pFocuser->pPrivateContext;
    NvOdmOsFree(pContext);
    hImager->pFocuser->pPrivateContext = NULL;
}

/**
 * Focuser_SetPowerLevel
 * Set the focuser's power level.
 */ 
static NvBool
Focuser_SetPowerLevel(
    NvOdmImagerHandle hImager,
    NvOdmImagerPowerLevel PowerLevel)
{
    FocuserContext *pContext =
        (FocuserContext*)hImager->pFocuser->pPrivateContext;
    NvBool Status = NV_TRUE;
    const NvOdmPeripheralConnectivity *pConnections;
    NvU16 tmp;
    NvS32 dPos;

    if (pContext->PowerLevel == PowerLevel)
        return NV_TRUE;

    NV_ASSERT(hImager->pFocuser->pConnections);
    pConnections = hImager->pFocuser->pConnections;

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_On);
            if (!Status)
            {
                NvOdmOsDebugPrintf("NvOdmImagerPowerLevel_On failed.\n");
                return NV_FALSE;
            }

            // Should have had a i2c handle
            NV_ASSERT(pContext->I2c.hOdmI2c);
#if DEBUG_PRINTS
            NvOdmOsDebugPrintf("==================== FOCUSER POWER ON!\n");
#endif
            // Set VCM CLOCK to 20KHz
            NvOdmImagerI2cRead(&pContext->I2c, 0x30ef, &tmp);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x30ee, VcmClkDiv[0]);
            NVODM_WRITE_RETURN_ON_ERROR(&pContext->I2c, 0x30ef, (VcmClkDiv[1]|(tmp&0xf0)));

            // Move the position to infinity
            Focuser_SetPosition(pContext, FOCUSER_POSITION_INFINITY);
            NvOdmOsWaitUS(50*1000);

            // Check for delayed focusing request:
            dPos = pContext->DelayedFPos;
            if ((dPos >= 0) && ((NvU32)dPos <= pContext->MaxMappedFocusPosition)) 
            {
               Status = Focuser_SetPosition(pContext, (NvU32)pContext->FocusPosMap.map[dPos]);
               if (Status) 
               {
                  pContext->RequestedPosition = dPos;
                  pContext->LastCmdTime = NvOdmOsGetTimeMS();
               }
               pContext->DelayedFPos = (-1);
            }
            break;

        case NvOdmImagerPowerLevel_Off:
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Off);
            break;
        case NvOdmImagerPowerLevel_Standby:
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pContext->I2c, &pContext->GpioConfig,
                         NvOdmImagerPowerLevel_Off);
        default:
            NV_ASSERT(!"Unknown power level");
            return NV_FALSE;
    }

    pContext->PowerLevel = PowerLevel;

    return Status;
}


/**
 * Focuser_GetCapabilities
 * Get focuser's capabilities.
 */
static void
Focuser_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{

    FocuserContext *pContext =
        (FocuserContext*)hImager->pFocuser->pPrivateContext;
    NvU32 numPos = 1;

    if (!pCapabilities)
        return;
    
    if (pContext != NULL) {
        numPos =  pContext->FocusPosMap.mapPositionsNum;
    }

    pCapabilities->NumberOfFocusingPositions = 
                  numPos;
}

/**
 * Focuser_SetParameter
 * Set focuser's parameter
 */
static NvBool
Focuser_SetParameter(
        NvOdmImagerHandle hImager,
        NvOdmImagerParameter Param,
        NvS32 SizeOfValue,
        const void *pValue)
{
    NvBool Status = NV_TRUE; 
    FocuserContext *pContext =
        (FocuserContext*)hImager->pFocuser->pPrivateContext;

    if (Param == NvOdmImagerParameter_FocuserPosition)
    {
        NvU32 Position = *((NvU32*)pValue);
        NvU32 CurrentTime = 0;
        
        if (Position > pContext->MaxMappedFocusPosition)
        {
#if DEBUG_PRINTS
            NvOdmOsDebugPrintf("Set Focuser position error = %d > %d\n", 
                 Position, pContext->MaxMappedFocusPosition);
#endif
            return NV_FALSE;
        }

        if (Position == (NvU32)pContext->RequestedPosition)
        {
#if DEBUG_PRINTS
            NvOdmOsDebugPrintf("Set Focuser position ignored = %d = %d\n", 
              Position, pContext->RequestedPosition);
#endif
            return NV_TRUE;
        }

        if (pContext->PowerLevel != NvOdmImagerPowerLevel_On) {
            pContext->DelayedFPos = (NvS32)Position;
            NvOdmOsDebugPrintf("SetFocuserPosition ignored: power off\n");
            return NV_TRUE;
        }

        Status = Focuser_SetPosition(pContext, (NvU32)pContext->FocusPosMap.map[Position]);

        CurrentTime = NvOdmOsGetTimeMS();

        // Previous requested position settled?
        Focuser_UpdateSettledPosition(pContext);

        // This requested position set?
        if (Status)
        {
            pContext->RequestedPosition = Position;
            pContext->LastCmdTime = CurrentTime;
        }
        return Status;
    }else if (Param == NvOdmImagerParameter_DirectFocuserControl) {
        const NvOdmImagerDirectFocuserControl *fCtrl;
    
        if (SizeOfValue < sizeof(NvOdmImagerDirectFocuserControl)) {
            return NV_FALSE;
        }
        if (pContext->PowerLevel != NvOdmImagerPowerLevel_On) {
            return NV_FALSE;
        }
        fCtrl = (NvOdmImagerDirectFocuserControl *)pValue;
        // Check control sequence format:
        if ((fCtrl->control[NvOdmImagerDirectFocuserControlIndex_Size] < 2) || 
            (fCtrl->control[NvOdmImagerDirectFocuserControlIndex_Version] !=
             NVODMIMAGER_DIRECT_FOCUSER_CONTROL_VERSION)) {
            return NV_FALSE;            
        }
        Status = FocuserDirectControl(pContext,
                                  (NvU32)(fCtrl->control[NvOdmImagerDirectFocuserControlIndex_Size]-2),
                                  fCtrl->control);
        return Status;
    }
    else if (Param == NvOdmImagerParameter_FocuserPositionMap) {
        NvOdmImagerFocuserPositionMap *fMap;

        if (SizeOfValue != sizeof(NvOdmImagerFocuserPositionMap)) {
            return NV_FALSE;
        }
        if (pValue == NULL) {
            return NV_FALSE;
        }
        if (pContext->PowerLevel == NvOdmImagerPowerLevel_On) {
            return NV_FALSE;
        }
        fMap = (NvOdmImagerFocuserPositionMap *)pValue;
        // Check positions map to be well-formed?
        pContext->FocusPosMap = *fMap;
        pContext->MaxMappedFocusPosition = pContext->FocusPosMap.mapPositionsNum-1;

#if DEBUG_PRINTS
        NvOdmOsDebugPrintf("Focuser_OV8810VCM_SetParameter: ");
        NvOdmOsDebugPrintf("focusing map [%d] %d %d %d %d\n",
                           pContext->FocusPosMap.mapPositionsNum,
                           pContext->FocusPosMap.map[0],
                           pContext->FocusPosMap.map[1],
                           pContext->FocusPosMap.map[2],
                           pContext->FocusPosMap.map[3]);
#endif
        return Status;
    }
    else
        return NV_FALSE;
}

/**
 * Focuser_GetParameter
 * Get focuser's parameter
 */
static NvBool
Focuser_GetParameter(
    NvOdmImagerHandle hImager,
    NvOdmImagerParameter Param,
    NvS32 SizeOfValue,
    void *pValue)
{
    NvU32 *pUValue = (NvU32*)pValue;
    FocuserContext *pContext =
        (FocuserContext*)hImager->pFocuser->pPrivateContext;

    switch(Param)
    {
        case NvOdmImagerParameter_FocuserPosition:
        {
            // The last requested position has been settled?
            Focuser_UpdateSettledPosition(pContext);
            *pUValue = pContext->Position;
            break;
        }

        case NvOdmImagerParameter_FocuserPositionsLimits:
            pUValue[0] = 0;
            pUValue[1] = pContext->MaxMappedFocusPosition;
            break;

        case NvOdmImagerParameter_FocuserCapabilities:
            if (SizeOfValue < sizeof(NvOdmImagerFocuserCapabilities))
            {
                return NV_FALSE;
            }
            {
            NvOdmImagerFocuserCapabilities *caps =
                              (NvOdmImagerFocuserCapabilities *)pValue;
            NvOdmOsMemset(caps, 0, sizeof(NvOdmImagerFocuserCapabilities));
            caps->actuatorRange = FOCUSER_POSITIONS; 
            caps->settleTime = FOCUSER_MAX_FOCUS_SETTLE_TIME; // we do synchronous positioning
            }
            break;

        case NvOdmImagerParameter_FocalLength:
        case NvOdmImagerParameter_MaxAperture:
        case NvOdmImagerParameter_FNumber:
        default:
            return NV_FALSE;
    }

    return NV_TRUE;
}


/**
 * Focuser_GetHal.
 * return the hal functions associated with focuser
 */
NvBool Focuser_GetHal(NvOdmImagerHandle hImager)
{
    if (!hImager || !hImager->pFocuser)
        return NV_FALSE;

    hImager->pFocuser->pfnOpen = Focuser_Open;
    hImager->pFocuser->pfnClose = Focuser_Close;
    hImager->pFocuser->pfnGetCapabilities = Focuser_GetCapabilities;
    hImager->pFocuser->pfnSetPowerLevel = Focuser_SetPowerLevel;
    hImager->pFocuser->pfnSetParameter = Focuser_SetParameter;
    hImager->pFocuser->pfnGetParameter = Focuser_GetParameter;

    return NV_TRUE;
}

NvBool
ResetFocuserPosition(NvOdmImagerHandle hImager)
{
    NvU32 pos;
    FocuserContext *pContext =
        (FocuserContext*)hImager->pFocuser->pPrivateContext;

    pos = pContext->RequestedPosition;
    pContext->RequestedPosition = FOCUSER_POSITION_INFINITY;
    return Focuser_SetParameter(hImager, 
        NvOdmImagerParameter_FocuserPosition,
        sizeof(NvU32),
        &pos);
}
