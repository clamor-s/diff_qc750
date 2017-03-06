/*
 * Copyright (c) 2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "focuser_bh6459.h"
#include "imager_hal.h"
#include "imager_util.h"
#include "nvassert.h"

#define FOCUSER_POSITIONS               (16)
#define FOCUSER_POSITION_INFINITY       (0)
#define FOCUSER_MAX_FOCUS_SETTLE_TIME   (50)
#define FOCUSER_POSITION_SHIFT          (0)

#define FOCUSER_MAX_CALIBRATION_PARAMS  (16)

// These numbers should be set correctly for specific focuser:
#define LENS_FOCAL_LENGTH   (3.871f)
#define LENS_MAX_APERTURE   (1.62f)
#define LENS_FNUMBER        (2.8f)

#define FOCUSER_DEVADDR    0x18
   
#define CYCLE 6 // 5.7 microsec.
//cycle 5.7É s osc[2:0]=3'b000(base cycle 0.1É s)
//ta[7:0]     = 0x0c = 12 count 
//break1[7:0] = 0x0  = 0 count
//tb[7:0]     = 0x11 = 17 count
//break2[7:0] = 0x1c = 28 count

#define CNT_INIT 0x00
#define CNT_INFINITY_BEYOND (-6000)
#define CNT_MACRO_BEYOND    (9000)
#define CNT_INFINITY        950
#define CNT_MACRO           1850
#define CNT_STEP            60

//#define CNT_COMPENSATE_INFINITY (0.80f)
#define CNT_COMPENSATE_INFINITY (1.0f)

// Steps multiplier for "real" device steps:
#define CNTCK 8  // cntck[2:0]

#define DIR_MACRO 0
#define DIR_INFINITY 1

#define TIME_DIFF(_from, _to) \
           (((_from) <= (_to)) ? ((_to) - (_from)) : \
                                 ((NvU32)~0 - ((_from) - (_to)) + 1))

static char *focuserCalibParamsPath[] = 
                { "\\release\\bh6459.dat",
                  0
                };


/**
 * Focuser's context
 */
typedef struct FocuserContextRec
{
    NvOdmImagerI2cConnection I2c;
    ImagerGpioConfig GpioConfig;
    NvOdmImagerPowerLevel PowerLevel;

    NvU32 InfinityCnt;          // Number of cycles to move focuser 
                                // from the edge to Infinity position.
    NvU32 MacroCnt;             // Number of cycles to move focuser 
                                // from the edge to Macro position.
    NvF32 InfinityDirCompensate;                                
    NvU32 MaxPosition;          // Maximum allowed focus position
    NvU32 LastCmdTime;          // Time the latest focus command was executed.
    NvS32 DelayedPosition;      // Position for delayed focusing action
    NvU32 Position;             // The last settled focus position.
    NvOdmImagerFocuserPositionMap Map;
                                // Focusing positions map as provided by config. data
    NvU32 PosCnt;               // Focusing positions counter for recalibration 
    NvU32 RecalibrateLimit;     // Recalibrate the focuser position after
                                // we have done that many focus movements.
    NvU32 RecalibrateTimeDelay; // Recalibrate the focuser position 
                                // if necessary after this period of inactivity
                                // (in millisecs)
} FocuserContext, *PFocuserContext;

/**
 * Static functions
 */
static void Focuser_UpdateSettledPosition(FocuserContext *pFocuser);

static NvBool Focuser_SetPosition(FocuserContext *pFocuser, NvU32 Position);

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

static NvBool focuserGetCalibrationParams(NvU32 maxParams,
                                   NvS32 *params, NvU32 *numParams);


static NvBool FocuserMovePos(
                         PFocuserContext focuser, 
                         NvS32 steps, NvBool waitForDone)
{
    NvOdmI2cStatus res = NvOdmI2cStatus_Success;
    NvU32 waitime, cnt;
    NvU8  data;
    NvU32 dir = DIR_MACRO;

    if (steps == 0) {
        return NV_TRUE;
    }
    if (steps < 0) { // Moving to INFINITY
        steps = -steps;
        dir = DIR_INFINITY;
        steps = (NvS32)((NvF32)steps * focuser->InfinityDirCompensate);
    }

    res = NvOdmImagerI2cWrite(&focuser->I2c,0x85,steps & 0xFF);
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x86,(steps >> 8) & 0xFF);
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x80,0x04|dir); // (,start=1, dir)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }

    if (! waitForDone) {
        focuser->PosCnt += 1;
        return NV_TRUE;
    }
    
    waitime = (CNTCK * steps * CYCLE);
    NvOdmOsWaitUS(waitime);

    cnt = 0;
    do {
        data = 0;
        if ((NvOdmImagerI2cRead(&focuser->I2c,0x8A,&data)) != NvOdmI2cStatus_Success) {
            return NV_FALSE;
        }
        cnt++;
    } while (((data & 0x2) == 0) && (cnt <= 5));

    res = NvOdmImagerI2cWrite(&focuser->I2c,0x80,0x00); //(Init=0,start=0, dir=0)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }

    focuser->PosCnt += 1;
    return NV_TRUE;
}

static NvBool FocuserRecalibrate(PFocuserContext focuser, NvU32 Position)
{

    // Set focus position to infinity.  
    FocuserMovePos(focuser,CNT_INFINITY_BEYOND,NV_TRUE);
    FocuserMovePos(focuser,Position,NV_TRUE);
    focuser->PosCnt = 0;

    return NV_TRUE;
}

static NvBool FocuserDirectControl(PFocuserContext focuser, 
                                NvU32 cLength, const NvS32 *control)
{
    NvS32 delay;
    NvU32 cnt;
    NvBool result = NV_TRUE;
    
    delay = control[0]; // settling delay for control sequence moves.
    cnt = 1;

    while (cnt < cLength) {
        result = FocuserMovePos(focuser,control[cnt],NV_TRUE);
        if (! result) {
            return result;
        }
        if (delay > 0) {
            NvOdmOsSleepMS(delay);
        }
        cnt += 1;
    }    
    return NV_TRUE;
}

static NvBool FocuserInitSequence(PFocuserContext focuser)
{
    NvOdmI2cStatus res = NvOdmI2cStatus_Success;
    NvS32 cnt = 0;
    NvS32 maxCnt = 32;
    NvU8 data;
    
    // init sequence
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x81,0x0C);  //(Ta=12)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x82,0x00); //(Brake1=0)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x83,0x11); //(Tb=17)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x84,0x1C); //(Brake2=28)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x87,0x03); //(cntck=8)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x85,CNT_INIT & 0xFF);
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x86,(CNT_INIT>>8) & 0xFF);
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x80,0x08); //(Init=1,start=0, dir=0)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }
    
    // Wait for device init sequence to finish:
    while (cnt < maxCnt) {
        NvOdmOsWaitUS(1000);
        data = 0;
        if ((NvOdmImagerI2cRead(&focuser->I2c,0x8A,&data)) != NvOdmI2cStatus_Success) {
            return NV_FALSE;
        }
        if (data == 0x1) {
            break;
        }
        cnt++;
    }
    
    res = NvOdmImagerI2cWrite(&focuser->I2c,0x80,0x00); //(Init=0,start=0, dir=0)
    if (res != NvOdmI2cStatus_Success) {
        return NV_FALSE;
    }

    return NV_TRUE;
}

/**
 * Focuser_SetPosition.
 */
static NvBool Focuser_SetPosition(FocuserContext *pFocuser, NvU32 Position)
{
    NvS32 currPos = pFocuser->Map.map[pFocuser->Position];
    NvS32 reqPos = pFocuser->Map.map[Position];
    NvBool status;
    NvU32 CurrentTime;

    CurrentTime = NvOdmOsGetTimeMS();

    if ((pFocuser->PosCnt > pFocuser->RecalibrateLimit) && 
        (TIME_DIFF(pFocuser->LastCmdTime,CurrentTime) > 
         pFocuser->RecalibrateTimeDelay)) {
        status = FocuserRecalibrate(pFocuser,reqPos);
    } else {
        status = FocuserMovePos(pFocuser,reqPos-currPos,NV_TRUE);
    }
    if (status) {
        pFocuser->Position = Position;
        pFocuser->LastCmdTime = CurrentTime;
    }
    return status;
}

static void focuserSetCalibrationParams(FocuserContext *pFocuser,
                               NvS32 stepsInf,
                               NvS32 stepsMacro,
                               NvS32 dirComp)
{
    if (stepsInf != 0) {
        pFocuser->InfinityCnt = stepsInf;
    }
    if (stepsMacro != 0) {
        pFocuser->MacroCnt = stepsMacro;
    }
    if (dirComp != 0) {
        pFocuser->InfinityDirCompensate = (NvF32)dirComp / 100.0f;
    }
}

/**
 * Focuser_Open
 * Initialize Focuser's private context.
 */
static NvBool Focuser_Open(NvOdmImagerHandle hImager)
{
    FocuserContext *pFocuserContext = NULL;
    NvOdmImagerI2cConnection *pI2c = NULL;
    
    //NvS32 calParams[FOCUSER_MAX_CALIBRATION_PARAMS];
    //NvU32 parmNum;

    if (!hImager || !hImager->pFocuser) {
        return NV_FALSE;
    }

    pFocuserContext = (FocuserContext *)NvOdmOsAlloc(sizeof(FocuserContext));
    if (!pFocuserContext) {
        goto fail;
    }
    NvOdmOsMemset(pFocuserContext, 0, sizeof(FocuserContext));

    // Setup the i2c bit widths so we can call the
    // generalized write/read utility functions.
    // This is done here, since it will vary from sensor to sensor.
    pI2c = &pFocuserContext->I2c;
    pI2c->AddressBitWidth = 8;
    pI2c->DataBitWidth = 8;

    pFocuserContext->PowerLevel = NvOdmImagerPowerLevel_Off;

    pFocuserContext->InfinityCnt = 950;
    pFocuserContext->MacroCnt = 1850;
    pFocuserContext->InfinityDirCompensate = 1.0f;
    
    // Construct some sane default positions map:
    pFocuserContext->Map.mapPositionsNum = 1;
    pFocuserContext->Map.map[0] = pFocuserContext->InfinityCnt;
    
    pFocuserContext->MaxPosition = 0;
    pFocuserContext->Position = 0;
    pFocuserContext->PosCnt = 0;
    pFocuserContext->LastCmdTime = 0;
    pFocuserContext->RecalibrateLimit = 1024;
    pFocuserContext->RecalibrateTimeDelay = 1000;
    pFocuserContext->DelayedPosition = (-1);

    hImager->pFocuser->pPrivateContext = pFocuserContext;

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
    FocuserContext *pFocuser = NULL;

    if (!hImager || ! hImager->pFocuser || !hImager->pFocuser->pPrivateContext) {
        return;
    }

    pFocuser = (FocuserContext*)hImager->pFocuser->pPrivateContext;
    NvOdmOsFree(pFocuser);
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
    NvOdmGpioPinHandle  hPin;
    NvU32   Port;
    NvU32   Pin;
    NvU32   Index;
    NvOdmServicesGpioHandle hOdmGpio;
    
    FocuserContext *pFocuser =
        (FocuserContext *)hImager->pFocuser->pPrivateContext;
    NvBool Status = NV_TRUE;
    const NvOdmPeripheralConnectivity *pConnections;

    NvS32 dPos;

    if (pFocuser->PowerLevel == PowerLevel)
        return NV_TRUE;

    NV_ASSERT(hImager->pFocuser->pConnections);
    pConnections = hImager->pFocuser->pConnections;
    if (! pConnections) {
        return NV_FALSE;
    }

    switch(PowerLevel)
    {
        case NvOdmImagerPowerLevel_On:
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pFocuser->I2c, &pFocuser->GpioConfig,
                         NvOdmImagerPowerLevel_On);
            if (!Status) {
                return NV_FALSE;
            }
            for (Index = 0 ; Index < pConnections->NumAddress ; ++Index)
                if(pConnections->AddressList[Index].Interface == NvOdmIoModule_Gpio)
                {
                    hOdmGpio = NvOdmGpioOpen();

                    if (hOdmGpio == NULL)
                    {
                        return NV_FALSE;
                    }
                    Port = pConnections->AddressList[Index].Instance;
                    Pin  = pConnections->AddressList[Index].Address;

                    hPin = NvOdmGpioAcquirePinHandle(hOdmGpio, Port, Pin);
                    if (hPin == NULL)
                    {
                        return NV_FALSE;
                    }
                    NvOdmGpioSetState(hOdmGpio, hPin, 1);
                    NvOdmOsWaitUS(1000);
                    NvOdmGpioReleasePinHandle(hOdmGpio, hPin);
                    NvOdmGpioClose(hOdmGpio);
                }

            // Should have had a i2c handle
            NV_ASSERT(pFocuser->I2c.hOdmI2c);
            if (! pFocuser->I2c.hOdmI2c) {
                return NV_FALSE;
            }
            
            if (! FocuserInitSequence(pFocuser)) {
                return NV_FALSE;
            }
            // Set focus position to infinity.  
            FocuserMovePos(pFocuser,CNT_INFINITY_BEYOND,NV_TRUE);
            FocuserMovePos(pFocuser,pFocuser->InfinityCnt,NV_TRUE);
            
            pFocuser->Position = FOCUSER_POSITION_INFINITY;
            pFocuser->PosCnt = 0;
            pFocuser->LastCmdTime = NvOdmOsGetTimeMS();

            dPos = pFocuser->DelayedPosition;
            if ((dPos >= 0) && ((NvU32)dPos <= pFocuser->MaxPosition)) {
                Focuser_SetPosition(pFocuser,(NvU32)dPos);
                pFocuser->DelayedPosition = (-1);
            }
            break;

        case NvOdmImagerPowerLevel_Off:
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pFocuser->I2c, &pFocuser->GpioConfig,
                         NvOdmImagerPowerLevel_Off);
            for (Index = 0 ; Index < pConnections->NumAddress ; ++Index)
                if(pConnections->AddressList[Index].Interface == NvOdmIoModule_Gpio)
                {
                    hOdmGpio = NvOdmGpioOpen();

                    if (hOdmGpio == NULL)
                    {
                        return NV_FALSE;
                    }
                    Port = pConnections->AddressList[Index].Instance;
                    Pin  = pConnections->AddressList[Index].Address;

                    hPin = NvOdmGpioAcquirePinHandle(hOdmGpio, Port, Pin);
                    if (hPin == NULL)
                    {
                        return NV_FALSE;
                    }
                    NvOdmGpioSetState(hOdmGpio, hPin, 0);
                    NvOdmGpioReleasePinHandle(hOdmGpio, hPin);
                    NvOdmGpioClose(hOdmGpio);
                }
            break;
        case NvOdmImagerPowerLevel_Standby:
            Status = SetPowerLevelWithPeripheralConnectivity(pConnections,
                         &pFocuser->I2c, &pFocuser->GpioConfig,
                         NvOdmImagerPowerLevel_Off);
            break;
        default:
            NV_ASSERT(!"Unknown power level");
            return NV_FALSE;
    }

    pFocuser->PowerLevel = PowerLevel;

    return Status;
}


/**
 * Focuser_GetCapabilities
 * Get focuser's capabilities.
 */
static void Focuser_GetCapabilities(
    NvOdmImagerHandle hImager,
    NvOdmImagerCapabilities *pCapabilities)
{
    FocuserContext *pFocuser =
        (FocuserContext*)hImager->pFocuser->pPrivateContext;
    NvU32 numPos = 1;
    
    if (!pCapabilities) {
        return;
    }

    if (pFocuser != NULL) {
        numPos = pFocuser->Map.mapPositionsNum;
    }
    pCapabilities->NumberOfFocusingPositions = numPos;
}

/**
 * Focuser_SetParameter
 * Set focuser's parameter
 */
static NvBool Focuser_SetParameter(
        NvOdmImagerHandle hImager,
        NvOdmImagerParameter Param,
        NvS32 SizeOfValue,
        const void *pValue)
{
    NvBool Status = NV_TRUE; 
    FocuserContext *pFocuser =
        (FocuserContext*)hImager->pFocuser->pPrivateContext;

    if (Param == NvOdmImagerParameter_FocuserPosition) {
        NvU32 Position = *((NvU32*)pValue);

        if (Position > pFocuser->MaxPosition) {
            return NV_FALSE;
        }
        if (Position == pFocuser->Position) {
            return NV_TRUE;
        }
        if (pFocuser->PowerLevel != NvOdmImagerPowerLevel_On) {
            pFocuser->DelayedPosition = Position;
            return NV_TRUE;
        }
        Status = Focuser_SetPosition(pFocuser, Position);
        return Status;
    } else if (Param == NvOdmImagerParameter_DirectFocuserControl) {
        const NvOdmImagerDirectFocuserControl *fCtrl;
    
        if (SizeOfValue < sizeof(NvOdmImagerDirectFocuserControl)) {
            return NV_FALSE;
        }
        if (pFocuser->PowerLevel != NvOdmImagerPowerLevel_On) {
            return NV_FALSE;
        }
        fCtrl = (NvOdmImagerDirectFocuserControl *)pValue;
        // Check control sequence format:
        if ((fCtrl->control[0] < 2) || (fCtrl->control[1] != 0x0)) {
            return NV_FALSE;            
        }
        Status = FocuserDirectControl(pFocuser,
                                  (NvU32)(fCtrl->control[0]-2),
                                  &fCtrl->control[2]);
        return Status;
    } else if (Param == NvOdmImagerParameter_FocuserPositionMap) {
        NvOdmImagerFocuserPositionMap *fMap;
        
        if (SizeOfValue != sizeof(NvOdmImagerFocuserPositionMap)) {
            return NV_FALSE;
        }
        if (pValue == NULL) {
            return NV_FALSE;
        }
        if (pFocuser->PowerLevel == NvOdmImagerPowerLevel_On) {
            return NV_FALSE;
        }
        fMap = (NvOdmImagerFocuserPositionMap *)pValue;
        // Check positions map to be well-formed?
        pFocuser->Map = *fMap;
        pFocuser->MaxPosition = pFocuser->Map.mapPositionsNum-1;
        pFocuser->InfinityCnt = pFocuser->Map.map[0];
        pFocuser->MacroCnt = pFocuser->Map.map[pFocuser->MaxPosition];
        return Status;
    } else {
        return NV_FALSE;
    }
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
    NvF32 *pFValue = (NvF32*)pValue;
    NvU32 *pUValue = (NvU32*)pValue;
    FocuserContext *pFocuser =
        (FocuserContext*)hImager->pFocuser->pPrivateContext;

    switch(Param)
    {
        case NvOdmImagerParameter_FocuserPosition:
        {
            *pUValue = pFocuser->Position;
            break;
        }

        case NvOdmImagerParameter_FocalLength:
            *pFValue = LENS_FOCAL_LENGTH;
            break;

        case NvOdmImagerParameter_MaxAperture:
            *pFValue = LENS_MAX_APERTURE;
            break;

        case NvOdmImagerParameter_FNumber:
            *pFValue = LENS_FNUMBER;
            break;

        case NvOdmImagerParameter_FocuserPositionsLimits:
            pUValue[0] = 0;
            pUValue[1] = pFocuser->MaxPosition;
            break;

        case NvOdmImagerParameter_FocuserCapabilities:
            if (SizeOfValue < sizeof(NvOdmImagerFocuserCapabilities)) {
                return NV_FALSE;
            }
            {   NvOdmImagerFocuserCapabilities *caps =
                  (NvOdmImagerFocuserCapabilities *)pValue;
                caps->actuatorRange = (NvU32)(-CNT_INFINITY_BEYOND);
                caps->settleTime = 0; // we do synchronous positioning
            }
            break;
        
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

static NvBool focuserParseDecNumber(const char *str, NvS32 *num, NvS32 *len)
{
    const char *curr = str, *digs;
    NvBool neg = NV_FALSE;
    NvS32 res = 0;
    
    if (*curr == '-') {
        neg = NV_TRUE;
        curr++;
    } else if (*curr == '+') {
        curr++;
    }
    digs = curr;
    while ((*curr >= '0') && (*curr <= '9')) {
        res = res * 10 + (*curr - '0');
        curr++;
    }
    if (neg) {
        res = -res;
    }
    *len = curr - str;
    *num = res;
    return (curr - digs) > 0;
}

static NvBool focuserParseCalibrationParams(const char *str,
                     NvU32 maxParams,
                     NvS32 *params, NvU32 *numParams) 
{
    NvU32 inx = 0;
    const char *curr = str;
    NvS32 num,len;

    *numParams = 0;
    if (*curr == '{') {
        curr++;
    }
    while (inx < maxParams) {
        if ((*curr == '+') || (*curr == '-') ||
            ((*curr >= '0') && (*curr <= '9'))) {
            if (!focuserParseDecNumber(curr,&num,&len)) {
                return NV_FALSE;
            }
            params[inx++] = num;
            curr += len;
            if (*curr == ',') {
                curr++;
            } else if (*curr == '}') {
                *numParams = inx;
                return NV_TRUE;
            } else {
                return NV_FALSE;
            }
        } else {
            return NV_FALSE;
        }
    }
    return NV_FALSE;
}

static void focuserReadCalibrationString(const char *buf, size_t bLen,
                       char *trgStr, NvU32 maxStrLength)
{   NvU32 inx = 0;
    const char *endBuf = buf+bLen;
    char ch;
    
    while (buf < endBuf) {
        ch = *buf++;
        if ((ch != ' ') && (ch != '\t') && (ch != '\n') &&
            (ch != 0x0D) && (ch != 0x0A)) {
            trgStr[inx++] = ch;
            if (inx == (maxStrLength-1)) {
                break;
            }
        }
    }
    trgStr[inx] = 0;
}

static NvBool focuserGetCalibrationString(const char **paths,
                         char *trgStr, NvU32 maxStrLength)
{   const char *fName = NULL;
    NvOdmOsStatType fStat;
    NvBool result;
    NvOdmOsFileHandle cFile = NULL;
    char buf[128];
    size_t reqLen, readLen;

    *trgStr = 0;
    if (paths == NULL) {
        return NV_FALSE;
    }
    fName = *paths++;
    while (fName != NULL) {
        result = NvOdmOsStat(fName,&fStat);
        if (result && (fStat.type == NvOdmOsFileType_File)) {
            result = NvOdmOsFopen(fName,NVODMOS_OPEN_READ,&cFile);
            if (result) {
                reqLen = (size_t)fStat.size;
                if (reqLen > (size_t)sizeof(buf)) {
                    reqLen = sizeof(buf);
                }
                result = NvOdmOsFread(cFile,&buf[0],reqLen,&readLen);
                if (result && (reqLen == readLen)) {
                    focuserReadCalibrationString(&buf[0],readLen,trgStr,maxStrLength);
                    return NV_TRUE;
                }
            }
        }
        fName = *paths++;
    }
    return NV_FALSE;    
}

static NvBool focuserGetCalibrationParams(NvU32 maxParams,
                                   NvS32 *params, NvU32 *numParams)
{   char str[128];

    *numParams = 0;
    if (!focuserGetCalibrationString(focuserCalibParamsPath,
                                     &str[0],sizeof(str))) {
        return NV_FALSE;
    }
    return focuserParseCalibrationParams(str,maxParams,params,numParams);
}


