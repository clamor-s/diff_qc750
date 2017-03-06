/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


/*
 *
 *  This file contains code that is supposed to be common to the supported
 *  sensors.
 *  The use of common code is supposed to serve three purposes:
 *     1.  better code maintainability across various sensors
 *     2.  easier to develop code for new sensors
 *
 *  This file is not to be compiled alone but rather must be included in an ODM
 *  sensor file.  The code in this file contains a set of common functions that
 *  operate on the parameters specific to ODM sensors.
 *
 *  The API for this module consists of only two functions defined with a
 *  #define, namely,
 *     NvBool SENSOR_COMMON_OPEN(NvOdmImagerDeviceHandle sensor)
 *     void SENSOR_COMMON_CLOSE(NvOdmImagerDeviceHandle sensor)
 *
 *  Six static functions are stored in a list of function pointers accessible
 *  outside this module:
 *     hSensor->GetCapabilities = Sensor_Common_GetCapabilities;
 *     hSensor->ListModes       = Sensor_Common_ListModes;
 *     hSensor->SetMode         = Sensor_Common_SetMode;
 *     hSensor->SetPowerLevel   = Sensor_Common_SetPowerLevel;
 *     hSensor->SetParameter    = Sensor_Common_SetParameter;
 *     hSensor->GetParameter    = Sensor_Common_GetParameter;
 *
 *  These two functions are called through acessing the function table in
 *  nvodm_sensorlist.c.
 *
 *  The rest of the static functions in this file are helper functions.
 *
 *  To develop code for a newly adapted sensor, the developer needs to create
 *  two ODM sensor files, for example, an existing sensor which can be used as
 *  reference has files nvodm_sensor_mi5130.c and nvodm_sensor_mi5130.h
 *
 *  Among other things, the sensor C file needs to have the following #define's
 *  at the beginning of the file:
 *
 *     #define NVODM_SENSOR_TYPE          SENSOR_MI5130_DEVID
 *     #define NVODM_SENSOR_MICRON        SENSOR_MI5130_DEVID
 *     #define NVODM_SENSOR_DEVICE_ADDR   SENSOR_MI5130_DEVICE_ADDR
 *
 *  Moreover, the sensor header file needs to have the following #defines':
 *     #define NVODM_SENSOR_8_BIT_DEVICE   0
 *
 *  The developer has the option to define his/her own functions rather than
 *  using those in this file.  This can be achieved by doing something similar
 *  to the following:
 *     #define SENSOR_INIT_PROGRAMMING   Sensor_OV3640_InitProgramming
 *     #define SENSOR_SET_MODE           Sensor_OV3640_SetMode
 *     #define SENSOR_SET_POWER_LEVEL    Sensor_OV3640_SetPowerLevel
 *     #define SENSOR_SELF_TEST          Sensor_OV3640_SelfTest
 *     #define SENSOR_OPEN               Sensor_OV3640_Open
 *
 */


#ifdef NVODM_SENSOR_TYPE

#include "nvodm_imager_guids.h"

#ifndef USE_SLOW_SETMODE
#define USE_SLOW_SETMODE (0)
#endif

#if NVODM_SENSOR_8_BIT_DEVICE
#define NVODM_IMAGER_I2C_READ    NvOdmImagerI2cRead16Data8
#define NVODM_IMAGER_I2C_WRITE   NvOdmImagerI2cWrite16Data8
#else
#define NVODM_IMAGER_I2C_READ    NvOdmImagerI2cRead16
#define NVODM_IMAGER_I2C_WRITE   NvOdmImagerI2cWrite16
#endif

#define WINGBOARD_PMU 0
#define USE_BINNING 0

#define I2C_BUFFER_SIZE 128
#if 0
#define NVODM_WRITEREG_BUFFER(_addr, _data) \
    NVODM_WRITEREG_RETURN_ON_ERROR(_addr, _data)
#else
#define NVODM_WRITEREG_BUFFER(_addr, _data) \
        I2CBufferAddrData[I2CBufferSize++] = \
            ((((_addr) & 0xFFFF) << 16) | ((_data) & 0xFFFF));
#endif

#define NVODM_READREG_RETURN_ON_ERROR(Reg,X) \
    do {\
        if (NVODM_IMAGER_I2C_READ(i2c,(Reg),&X) != NvOdmI2cStatus_Success) {\
            return NV_FALSE;  \
        }}while(0);

#define NVODM_READREG_REPORT_ON_ERROR(Reg,X) \
    do {\
        if (NVODM_IMAGER_I2C_READ(i2c,(Reg),&X) != NvOdmI2cStatus_Success) {\
            NvOdmOsDebugPrintf("I2C Read error for reg %0x\n",(Reg));  \
        }}while(0);


#define NVODM_WRITEREG_RETURN_ON_ERROR(Reg,X) \
    do { \
        NvError RetType; \
        RetType = NVODM_IMAGER_I2C_WRITE(i2c,(Reg),(X)); \
        if (RetType == NvOdmI2cStatus_Timeout) \
        { \
            RetType = NVODM_IMAGER_I2C_WRITE(i2c,(Reg),(X)); \
        } \
        if (RetType != NvOdmI2cStatus_Success) \
        { \
            return NV_FALSE; \
        }}while(0);

#define NVODM_WRITEREG_REPORT_ON_ERROR(Reg,X) \
    do {\
        if (NVODM_IMAGER_I2C_WRITE(i2c,(Reg),(X)) != NvOdmI2cStatus_Success) {\
            NvOdmOsDebugPrintf("I2C Write error for reg %0x\n",(Reg));  \
        }}while(0);

#define NVODM_CLAMP(value,minValue,maxValue)  \
    do {\
        if (value < (minValue)) {\
            value = (minValue);\
        }\
        if (value > (maxValue)) {\
            value = (maxValue);\
        }\
       } while(0);


// Wait until the "live" registers are writen
#define SYNC_FRAME (1)

#if SYNC_FRAME
#define NVODM_SYNC_FRAME(name, status) \
    do {\
        NvU16 FrameStatus, FrameCount=0, i=0; \
        status = NV_FALSE; \
        do \
        { \
            NVODM_READREG_REPORT_ON_ERROR(0x303C, FrameStatus); \
            if ((FrameStatus & 0x1) != 0) \
            { \
                NV_DEBUG_PRINTF(("%s: Waiting for frames to be sync'd (status = 0x%x)\n", name, FrameStatus)); \
                NvOdmOsWaitUS(1000); \
                i++; \
            } \
            else \
            { \
                status = NV_TRUE; \
                NV_DEBUG_PRINTF(("%s: Success, frames sync'd (status = 0x%x)\n", name, FrameStatus)); \
                NV_DEBUG_PRINTF(("Sync after %d attempts\n", i));\
            } \
        } while(status == NV_FALSE && i < 400); \
        if (status == NV_FALSE) \
        { \
            NVODM_READREG_REPORT_ON_ERROR(0x303B, FrameCount); \
            NV_DEBUG_PRINTF(("%s: Frame %d never sync'd\n", FrameCount)); \
        } \
    } while(0);
#else
#define NVODM_SYNC_FRAME(name, status)
#endif

#define ODDINC_TO_SCALE(_o) (((_o) == 1) ? 1 : \
                             ((_o) == 3) ? 2 : \
                             ((_o) == 7) ? 4 : 1)



#define NVODM_UNDEF_VALUE  99999.999f

#define NVODM_ET_RATIO_DELTA 0.05f

/////////////////////////////////////////////////////////
///       PRIVATE FUNCTIONS
/////////////////////////////////////////////////////////

#ifndef SENSOR_GET_CAPABILITIES
#define SENSOR_GET_CAPABILITIES   Sensor_Common_GetCapabilities
static void Sensor_Common_GetCapabilities(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerCapabilities *capabilities)
{
    SENSOR_CAPABILITIES.PreferredModeIndex = gs_PreferredModeIndex;
    NvOdmOsMemcpy(capabilities, &SENSOR_CAPABILITIES, sizeof(SENSOR_CAPABILITIES));
}
#endif

#ifndef SENSOR_LIST_MODES
#define SENSOR_LIST_MODES   Sensor_Common_ListModes
static void Sensor_Common_ListModes(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerSensorMode *modes,
        NvS32 *NumberOfModes)
{

    if (NumberOfModes)
    {
        *NumberOfModes = (NvS32)g_NumberOfResolutions;
        if (modes)
            NvOdmOsMemcpy(modes, &SENSOR_MODES,
                          g_NumberOfResolutions*sizeof(NvOdmImagerSensorMode));
    }
}
#endif

#if 0
#if defined(NVODM_SENSOR_MICRON) && !defined(SENSOR_TEST_REGISTER_SEQ)
#define SENSOR_TEST_REGISTER_SEQ   Sensor_Common_TestRegisterSeq
static NvBool Sensor_Common_TestRegisterSeq(NvOdmImagerI2cConnection *i2c,
        DevTestReg16 *regs,
        NvS32 numRegs, NvU16 initPattern,
        NvU16 *failAddr, NvS32 *cond)
{
    NvU16 patt = initPattern;
    DevTestReg16 *testReg = regs;
    NvU16 rres;
    NvS32 i;
    NvOdmI2cStatus status;

    *failAddr = NVODM_SENSOR_NOREG;

    for (i = 0; i < numRegs; i++) {
        status = NVODM_IMAGER_I2C_WRITE(i2c,testReg->RegAddr,
                patt & testReg->RegMask);
        if (status != NvOdmI2cStatus_Success) {
            *failAddr = testReg->RegAddr;
            *cond = 0;
            NvOdmOsDebugPrintf("SENSOR WRITE REG. FAILED: reg.[0x%x]\n",*failAddr);
            return NV_FALSE;
        }
        testReg++;
        patt ^= 0xFFFF;
    }
    // Now read all registers back:
    patt = initPattern;
    testReg = regs;
    for (i = 0; i < numRegs; i++) {
        status = NvOdmImagerI2cRead16(i2c,testReg->RegAddr,&rres);
        *failAddr = testReg->RegAddr;
        if (status != NvOdmI2cStatus_Success) {
            *cond = 1;
            NvOdmOsDebugPrintf("SENSOR READ REG. FAILED: reg.[0x%x]\n",*failAddr);
            return NV_FALSE;
        }
        if ((rres & testReg->RegMask) != (patt & testReg->RegMask)) {
            *cond = 2;
            NvOdmOsDebugPrintf(
                    "SENSOR COMPARE REG. FAILED: reg.[0x%x] |0x%x| != |0x%x|\n",
                    *failAddr,
                    rres & testReg->RegMask,
                    patt & testReg->RegMask);
            return NV_FALSE;
        }
        testReg++;
        patt ^= 0xFFFF;
    }
    return NV_TRUE;
}
#endif

#if defined(NVODM_SENSOR_MICRON) && !defined(SENSOR_TEST_REGISTER_READ_WRITE)
#define SENSOR_TEST_REGISTER_READ_WRITE   Sensor_Common_TestRegisters_ReadWrite
static NvBool Sensor_Common_TestRegisters_ReadWrite(
        NvOdmImagerI2cConnection *i2c)
{
    NvS32 n;
    NvU16 initPattern = 0xA5A5;
    NvU16 failAddr;
    NvS32 failCond;
    DevTestReg16 *testReg = &regReadWriteTestSeq[0];
    NvBool res;

    NvOdmOsDebugPrintf("Starting I2C registers Read/Write test:\n");
    n = 0;
    while (testReg[n].RegAddr != NVODM_SENSOR_NOREG) {
        n += 1;
    }

    res = Sensor_Common_TestRegisterSeq(i2c,
            testReg,n,initPattern,
            &failAddr,&failCond);
    NvOdmOsDebugPrintf("Finished I2C registers Read/Write test: %s\n",
            res? "Success":"Fail");
    return res;
}
#endif
#endif

#ifndef SENSOR_INIT_PROGRAMMING
#define SENSOR_INIT_PROGRAMMING   Sensor_Common_InitProgramming
static NvBool Sensor_Common_InitProgramming(
        NvOdmImagerI2cConnection *i2c,
        NvU32 index,
        SensorContext *ctx)
{
    DevCtrlReg16 *seq = 0;
    NvU32 I2CBufferAddrData[I2C_BUFFER_SIZE];
    NvU32 I2CBufferSize = 0;
    NvOdmI2cStatus Status;

    if (i2c->hOdmI2c == NULL) {
        i2c->hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, i2c->I2cPort);
        if (!i2c->hOdmI2c) {
            return NV_FALSE;
        }
    }

    if (index == 0)
    {
        NvS32 i = 0;
        NvS32 ProfileIndexChosen = -1;
        NvF32 MaxClockRatio = 0.0f;

        // Find the clock profile that has the largest clock ratio that is
        // less than or equal to the clock ratio given by camera driver.
        for (i = 0; i < NVODMIMAGER_CLOCK_PROFILE_MAX; i++)
        {
            if (SENSOR_CAPABILITIES.ClockProfiles[i].ClockMultiplier <=
                ctx->shadow.SensorClockRatio &&
                MaxClockRatio < SENSOR_CAPABILITIES.ClockProfiles[i].
                    ClockMultiplier)
            {
                MaxClockRatio = SENSOR_CAPABILITIES.ClockProfiles[i].
                                ClockMultiplier;
                ProfileIndexChosen = i;
            }
        }

        // can't find a matching clock profile
        if (ProfileIndexChosen < 0 ||
            ProfileIndexChosen >= NVODMIMAGER_CLOCK_PROFILE_MAX)
            return NV_FALSE;

        if (SENSOR_CAPABILITIES.ClockProfiles[ProfileIndexChosen].
            ClockMultiplier == 1.0f)
        {
            seq = &SENSOR_INIT_SEQUENCE[0];
#if 0
            NvOdmOsDebugPrintf("Using (mclk=pclk) settings because %f == 1.0\n",
                               ctx->shadow.SensorClockRatio);
#endif

        }
        // Assume there are only two clock profiles at most and the one whose
        // clock ratio is not 1 uses the fast sequence.
        // TODO: each sequence should be associated to one clock profile.
        else
        {
            seq = &SENSOR_INIT_SEQUENCE_FAST[0];
#if 0
            NvOdmOsDebugPrintf("Using (mclk<pclk) settings because %f != 1.0\n",
                               ctx->shadow.SensorClockRatio);
#endif
        }
    } else {
        return NV_FALSE;
    }

    while (seq->RegAddr != 0xFFFF)
    {
        // delay?
        if (seq->RegAddr == 0xFFFE)
        {
            // on delay, flush the i2c buffer first
            if (I2CBufferSize)
            {
                Status = NvOdmImagerI2cWriteBuffer16(i2c,
                                                     I2CBufferAddrData,
                                                     I2CBufferSize);
                if (Status != NvOdmI2cStatus_Success)
                {
                    NvOdmOsDebugPrintf("I2C WriteBuffer failed\n");
                    return NV_FALSE;
                }
                I2CBufferSize = 0; // clear buffer
            }
            NvOdmOsWaitUS(seq->RegValue * 1000);
        }
        // I2C writes
        else
            NVODM_WRITEREG_BUFFER(seq->RegAddr,seq->RegValue);

        seq += 1;
    }
    if (I2CBufferSize)
    {
        Status = NvOdmImagerI2cWriteBuffer16(i2c,
                                             I2CBufferAddrData,
                                             I2CBufferSize);
        if (Status != NvOdmI2cStatus_Success)
        {
            NvOdmOsDebugPrintf("I2C WriteBuffer failed\n");
            return NV_FALSE;
        }
    }

    NvOdmOsWaitUS(333000);

    return NV_TRUE;
}
#endif

#if defined(NVODM_SENSOR_MICRON) && !USE_SLOW_SETMODE
static NvBool Sensor_Common_ProgramSensorMode(
    NvOdmImagerDeviceHandle hSensor,
    SMIAScalerSettings *pOldSettings,
    SMIAScalerSettings *pNewSettings,
    NvS32 NewFrameLength,
    NvS32 NewCoarse,
    NvOdmImagerI2cConnection *i2c)
{
    NvBool Status = NV_TRUE;
    NvBool WriteFrameSizeFirst;
    //NvS32 newFrameLines = pNewMode->shadow.FrameLengthLines;
    NvU32 I2CBufferAddrData[I2C_BUFFER_SIZE];
    NvU32 I2CBufferSize = 0;
    NvOdmI2cStatus i2cStatus;

    WriteFrameSizeFirst = (pOldSettings == NULL) ||
         (pOldSettings->XOutputSize < pNewSettings->XOutputSize);


   /**
     * Although we are writing registers between GROUP_PARAM_HOLD=1 and
     * GROUP_PARAM_HOLD=0, apparently the order of register writes does
     * matter from experiments. So, if we are changing to a bigger resolution,
     * change FrameLengthLines and LineLengthPck first. If we are changing to
     * a smaller resolution, change FrameLengthLines and LineLengthPck last.
 */
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0);
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 1);

        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_EXPOSURE_COARSE,
                                       NewCoarse);// frame length line

    if (WriteFrameSizeFirst)
    {
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_FRAME_LENGTH_LINES,
                                       NewFrameLength);// frame length line
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_LINE_LENGTH_PCK,
                                       pNewSettings->LineLengthPck);
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_X_ODD_INC,
                                       pNewSettings->XOddInc);// x odd inc
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_Y_ODD_INC,
                                       pNewSettings->YOddInc);// y odd inc
        if (USE_BINNING)
        {
            if((pNewSettings->XOddInc > 1) && (pNewSettings->YOddInc > 1))
            {
                NVODM_WRITEREG_BUFFER(0x3040, (1 << 10) |
                                              (pNewSettings->XOddInc << 5) |
                                              (pNewSettings->YOddInc << 2));
            }
            else
            {
                NVODM_WRITEREG_BUFFER(0x3040, 0x24);
            }
        }
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_X_OUTPUT_SIZE,
                                       pNewSettings->XOutputSize);// x output size
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_Y_OUTPUT_SIZE,
                                       pNewSettings->YOutputSize);// y output size
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_X_ADDR_START,
                                       pNewSettings->XAddrStart);// x start
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_Y_ADDR_START,
                                       pNewSettings->YAddrStart);// y start
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_X_ADDR_END,
                                       pNewSettings->XAddrEnd);// x end
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_Y_ADDR_END,
                                       pNewSettings->YAddrEnd);// y end
    }
    else
    {
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_X_OUTPUT_SIZE,
                                       pNewSettings->XOutputSize);// x output size
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_Y_OUTPUT_SIZE,
                                       pNewSettings->YOutputSize);// y output size
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_X_ADDR_START,
                                       pNewSettings->XAddrStart);// x start
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_Y_ADDR_START,
                                       pNewSettings->YAddrStart);// y start
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_X_ADDR_END,
                                       pNewSettings->XAddrEnd);// x end
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_Y_ADDR_END,
                                       pNewSettings->YAddrEnd);// y end
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_X_ODD_INC,
                                       pNewSettings->XOddInc);// x odd inc
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_Y_ODD_INC,
                                       pNewSettings->YOddInc);// y odd inc
        if (USE_BINNING)
        {
            if((pNewSettings->XOddInc > 1) && (pNewSettings->YOddInc > 1))
            {
                NVODM_WRITEREG_BUFFER(0x3040, (1 << 10) |
                                               (pNewSettings->XOddInc << 5) |
                                               (pNewSettings->YOddInc << 2));
            }
            else
            {
                NVODM_WRITEREG_BUFFER(0x3040, 0x24);
            }
        }
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_FRAME_LENGTH_LINES,
                                       NewFrameLength);// frame length line
        NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_LINE_LENGTH_PCK,
                                       pNewSettings->LineLengthPck);
    }

    NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0);

    if (I2CBufferSize)
    {
        i2cStatus = NvOdmImagerI2cWriteBuffer16(i2c,
                                                I2CBufferAddrData,
                                                I2CBufferSize);
        if (i2cStatus != NvOdmI2cStatus_Success)
        {
            NvOdmOsDebugPrintf("I2C WriteBuffer failed\n");
            return NV_FALSE;
        }
    }

    return Status;
}
#endif

/*
 * Recalculate exposure limits
 * for current sensor settings.
 * (we depend on: VtPixClkFreqHz
 *                LineLengthPck
 * )
 */
#if defined(NVODM_SENSOR_MICRON)
static void
Sensor_Common_RecalculateExposureRange(NvOdmImagerSensorShadow *pShadow)
{
    NvF32 Freq = pShadow->VtPixClkFreqHz_F;
    NvF32 LineLength = pShadow->LineLengthPck_F;

    pShadow->MaxExposure = ((NVODM_SENSOR_REG_MAX_FRAME_LINES - 1) *
                          LineLength + NVODM_SENSOR_DEFAULT_EXPOS_FINE) / Freq;
    pShadow->MinExposure = (NVODM_SENSOR_DEFAULT_EXPOS_FINE) / Freq;
}
#endif


/*
 * Recalculate frame rate limits
 * for current sensor settings.
 * (we depend on: VtPixClkFreqHz
 *                LineLengthPck
 *                FrameLengthLines range
 * )
 */
#if defined(NVODM_SENSOR_MICRON)
static void
Sensor_Common_RecalculateFrameRateRange(NvOdmImagerSensorShadow *pShadow)
{
    NvF32 Freq = pShadow->VtPixClkFreqHz_F;
    NvF32 LineLength = pShadow->LineLengthPck_F;

    pShadow->MaxFrameRate = Freq / (pShadow->MinFrameLengthLines * LineLength);
    pShadow->MinFrameRate = Freq / ((NvF32)NVODM_SENSOR_REG_MAX_FRAME_LINES * LineLength);
}
#endif

#if defined(NVODM_SENSOR_MICRON) && !defined(SENSOR_WRITE_EXPOSURE)
#define SENSOR_WRITE_EXPOSURE   Sensor_Common_WriteExposure

static NvBool
Sensor_Common_ComputeCoarseAndFrameLengthForExposure(
    NvS32 *pCoarse,
    NvS32 *pFrameLength,
    NvF32 Freq,
    NvS32 LineLength,
    NvF32 Exposure,
    NvF32 MaxFrameRate,
    NvS32 MaxFrameLength,
    NvS32 MinFrameLength)
{
    NvS32 RequestedMinFrameLengthLines = 0;
    NvS32 NewCoarse;
    NvS32 NewFrameLength;

    NewCoarse = (NvS32)((Freq * Exposure) / (NvF32)LineLength);
    NewFrameLength = NewCoarse + 1;

    // 1. Don't exceed max frame rate.
    if (MaxFrameRate > 0.0f)
    {
        RequestedMinFrameLengthLines =
            (NvS32)((NvF32)Freq / (LineLength * MaxFrameRate));

        if (NewFrameLength < RequestedMinFrameLengthLines)
            NewFrameLength = RequestedMinFrameLengthLines;
    }

    // 2. Don't exceeed MacFrameLength
    if (NewFrameLength > MaxFrameLength)
    {
        NewFrameLength = MaxFrameLength;
    }
    else if (NewFrameLength < MinFrameLength)
    {
        NewFrameLength = MinFrameLength;
    }

    if (NewCoarse > NewFrameLength - 1)
        NewCoarse = NewFrameLength - 1;

    if (pCoarse)
        *pCoarse = NewCoarse;

    if (pFrameLength)
        *pFrameLength = NewFrameLength;

    return NV_TRUE;
}

static NvBool
Sensor_Common_WriteExposure(NvOdmImagerDeviceHandle sensor,
                          const NvF32 *exposureTime,
                          NvBool *wasChanged)
{
    // formula is: (pg 100)
    // coarse * LineLengthPck + fine = int_time * freq
    // but our input is inverse of seconds, so:
    // coarse = ((1/int_time * freq) - fine) / LineLengthPck
    // coarse = ((1/int_time * freq)/LineLengthPck - fine / LineLengthPck
    // coarse = freq/(LineLengthPck*int_time) - (0x204/0xAAC)
    // coarse = freq/(LineLengthPck*int_time) - (0x0)
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
    SensorContext *ctx = hSensor->prv;
    NvF32 freq = ctx->shadow.VtPixClkFreqHz_F;
    NvF32 lineLength = ctx->shadow.LineLengthPck_F;
    NvF32 expTime = exposureTime ? *exposureTime : NVODM_UNDEF_VALUE;
    NvS32 coarse;
    NvS32 prevCoarse = ctx->shadow.currCoarse;
    NvU32 I2CBufferAddrData[I2C_BUFFER_SIZE];
    NvU32 I2CBufferSize = 0;


    if (expTime == NVODM_UNDEF_VALUE || freq == 0)
    {
        // Invalid clock or invalid exposure time.
        // Most likely, we're still trying to bootstrap.
        // Just set coarse = max
        coarse = ctx->shadow.FrameLengthLines-1;
        NVODM_WRITEREG_RETURN_ON_ERROR(
            NVODM_SENSOR_REG_EXPOSURE_COARSE,
            coarse);
        ctx->shadow.currExposure = NVODM_UNDEF_VALUE;
        ctx->shadow.currCoarse = coarse;
        prevCoarse = (-1);
    }
    else
    {
        NvS32 NewFrameLengthLines = 0;
        NvBool Status;
        NvBool FrameLengthLineChanged, CoarseChanged;

        Status = Sensor_Common_ComputeCoarseAndFrameLengthForExposure(
                   &coarse, &NewFrameLengthLines, ctx->shadow.VtPixClkFreqHz_F,
                   ctx->shadow.LineLengthPck, expTime,
                   ctx->shadow.RequestedMaxFrameRate,
                   ctx->shadow.MaxFrameLengthLines,
                   ctx->shadow.MinFrameLengthLines);
        if (!Status)
        {
            NV_ASSERT("ODM: Can't compute coarse and frame length!");
            return Status;
        }

        FrameLengthLineChanged = (NewFrameLengthLines !=
                                  ctx->shadow.FrameLengthLines);
        CoarseChanged = (coarse != prevCoarse);

        if (FrameLengthLineChanged || CoarseChanged)
        {
            NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 1);

            if (FrameLengthLineChanged)
            {
                NVODM_WRITEREG_RETURN_ON_ERROR(
                    NVODM_SENSOR_REG_FRAME_LENGTH_LINES, NewFrameLengthLines);
                ctx->shadow.FrameLengthLines = NewFrameLengthLines;
                ctx->shadow.FrameLengthLines_F = (NvF32)NewFrameLengthLines;
                ctx->shadow.currFrameRate = freq / (NewFrameLengthLines *
                                                    lineLength);
            }

            if (CoarseChanged)
            {
                NVODM_WRITEREG_RETURN_ON_ERROR(
                    NVODM_SENSOR_REG_EXPOSURE_COARSE, coarse);
                ctx->shadow.currExposure =
                    ((NvF32)coarse * lineLength +
                     NVODM_SENSOR_DEFAULT_EXPOS_FINE) / freq;
                ctx->shadow.currCoarse = coarse;

                NV_DEBUG_PRINTF(("ODM: Set Exposure to %f (%f) (Coarse = %d)\n",
                    ctx->shadow.currExposure, *exposureTime,
                    ctx->shadow.currCoarse));
            }

            NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_GROUP_PARAM_HOLD, 0);
        }

    }

    if (wasChanged)
    {
        *wasChanged = (coarse != prevCoarse);
    }

    NV_ASSERT(*exposureTime > 0.0);

    if (I2CBufferSize)
    {
        NvOdmI2cStatus i2cStatus = NvOdmImagerI2cWriteBuffer16(i2c,
                                        I2CBufferAddrData, I2CBufferSize);
        if (i2cStatus != NvOdmI2cStatus_Success)
        {
            NvOdmOsDebugPrintf("I2C WriteBuffer failed\n");
            return NV_FALSE;
        }
    }

    return NV_TRUE;
}
#endif


#if defined(NVODM_SENSOR_MICRON) && !defined(SENSOR_WRITE_FRAME_RATE)
static NvBool
Sensor_Common_WriteFrameRate(
        NvOdmImagerDeviceHandle sensor,
        NvF32 *FrameRate,
        NvBool WriteNotQuery);
#endif

/* *********************
 *        Gain
 * *********************
 */

#if defined(NVODM_SENSOR_MICRON) && !defined(SENSOR_READ_GAIN)
#define SENSOR_READ_GAIN   Sensor_Common_ReadGain
static NvBool
Sensor_Common_ReadGains(NvOdmImagerDeviceHandle sensor, NvF32 *Gains)
{
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    SensorContext *ctx = (SensorContext *)hSensor->prv;
    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
    NvU16 gainTemp;

    NVODM_READREG_RETURN_ON_ERROR(NVODM_SENSOR_REG_ANALOG_GAIN_R, gainTemp);
    Gains[0] = ctx->shadow.currGains[0] = NVODM_SENSOR_GAIN_TO_F32(gainTemp);
    NVODM_READREG_RETURN_ON_ERROR(NVODM_SENSOR_REG_ANALOG_GAIN_GR, gainTemp);
    Gains[1] = ctx->shadow.currGains[1] = NVODM_SENSOR_GAIN_TO_F32(gainTemp);
    NVODM_READREG_RETURN_ON_ERROR(NVODM_SENSOR_REG_ANALOG_GAIN_GB, gainTemp);
    Gains[2] = ctx->shadow.currGains[2] = NVODM_SENSOR_GAIN_TO_F32(gainTemp);
    NVODM_READREG_RETURN_ON_ERROR(NVODM_SENSOR_REG_ANALOG_GAIN_B, gainTemp);
    Gains[3] = ctx->shadow.currGains[3] = NVODM_SENSOR_GAIN_TO_F32(gainTemp);

    return NV_TRUE;
}
#endif

#if defined(NVODM_SENSOR_MICRON) && !defined(SENSOR_WRITE_GAINS)
#define SENSOR_WRITE_GAINS   Sensor_Common_WriteGains
static NvBool
Sensor_Common_WriteGains(NvOdmImagerDeviceHandle sensor, const NvF32 *Gains)
{
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
    SensorContext *ctx = (SensorContext *)hSensor->prv;
    NvF32 maxGain = ctx->shadow.MaxSensorGain;
    NvS32 sensorGains[4]; // What we actually program to the sensor
    NvF32 setGains[4];
    int i;
    NvU32 I2CBufferAddrData[I2C_BUFFER_SIZE];
    NvU32 I2CBufferSize = 0;
    NvOdmI2cStatus i2cStatus;
    NvBool GainChanged = NV_FALSE;

    // can't write gains if we are in test pattern mode.
    if (ctx->TestPatternMode)
        return NV_TRUE;

    for(i = 0; i < 4; i++) {
        // Why are we checking this?  The previous layer should have
        // already checked this.
        if (Gains[i] > maxGain) {
            sensorGains[i] = NVODM_SENSOR_F32_TO_GAIN(maxGain);
            setGains[i] = maxGain;
        } else {
            sensorGains[i] = NVODM_SENSOR_F32_TO_GAIN(Gains[i]);
            setGains[i] = Gains[i];
        }

        if (setGains[i] != ctx->shadow.currGains[i])
            GainChanged = NV_TRUE;
    }

    if (!GainChanged)
        return NV_TRUE;

    NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_GROUP_PARAM_HOLD,1);
    NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_ANALOG_GAIN_R,
        sensorGains[0]);
    ctx->shadow.currGains[0] = setGains[0];
    NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_ANALOG_GAIN_GR,
        sensorGains[1]);
    ctx->shadow.currGains[1] = setGains[1];
    NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_ANALOG_GAIN_GB,
        sensorGains[2]);
    ctx->shadow.currGains[2] = setGains[2];
    NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_ANALOG_GAIN_B,
        sensorGains[3]);
    ctx->shadow.currGains[3] = setGains[3];
    NVODM_WRITEREG_BUFFER(NVODM_SENSOR_REG_GROUP_PARAM_HOLD,0);

    if (I2CBufferSize)
    {
        i2cStatus = NvOdmImagerI2cWriteBuffer16(i2c,
                                                I2CBufferAddrData,
                                                I2CBufferSize);
        if (i2cStatus != NvOdmI2cStatus_Success)
        {
            NvOdmOsDebugPrintf("I2C WriteBuffer failed\n");
            return NV_FALSE;
        }
    }

    NV_DEBUG_PRINTF(("ODM: Set gains to %f, %f, %f, %f (%f, %f, %f, %f)\n",
        ctx->shadow.currGains[0], ctx->shadow.currGains[1],
        ctx->shadow.currGains[2], ctx->shadow.currGains[3],
        Gains[0], Gains[1], Gains[2], Gains[3]));

    return NV_TRUE;
}
#endif

#ifndef SENSOR_SET_MODE
#define SENSOR_SET_MODE   Sensor_Common_SetMode

static NV_INLINE NvBool Sensor_Common_ChooseModeIndex(
    NvSize Resolution,
    NvU32 *pIndex)
{
    NvU32 index;

    for (index = 0; index < g_NumberOfResolutions; index++)
    {
        if (RESOLUTION_MATCHES(Resolution.width, Resolution.height,
            &SENSOR_MODES[index]))
        {
            *pIndex = index;
            return NV_TRUE;
        }
    }

    return NV_FALSE;
}

static NvBool Sensor_Common_SetMode(
    NvOdmImagerDeviceHandle sensor,
    const SetModeParameters *pParameters,
    NvOdmImagerSensorMode *pSelectedMode,
    SetModeParameters *pResult)
{
    NvBool Status = NV_TRUE;

#ifdef NVODM_SENSOR_MICRON
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    SensorContext *pContext = (SensorContext *)hSensor->prv;
    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
    SMIAScalerSettings *pOldScaler;
    SMIAScalerSettings *pNewScaler;
    NvBool MatchFound = NV_FALSE;
    NvU32 index;
    NvF32 Freq, Exposure;
    NvU32 WaitTimeMs = 0;
    NvS32 NewCoarse, NewFrameLength;

    // TODO: find the closest and Framerate < PeakFrameRate, and handle that.
    MatchFound = Sensor_Common_ChooseModeIndex(pParameters->Resolution, &index);
    if (!MatchFound)
        return NV_FALSE;

    NV_ASSERT(index < g_NumberOfResolutions);

    if (pSelectedMode)
        *pSelectedMode =  SENSOR_MODES[index];

    if (pContext->CurrentModeIndex == index)
        return NV_TRUE;

    NV_DEBUG_PRINTF(("ODM: Changing resolution to %dx%d\n",
        pParameters->Resolution.width, pParameters->Resolution.height));

    pOldScaler = (pContext->CurrentModeIndex == INVALID_MODE_INDEX) ?
                 NULL : &ScalerSettings[pContext->CurrentModeIndex];
    pNewScaler = &ScalerSettings[index];

    // 1. Compute the new coarse and frame length
    if (pParameters->Exposure == 0.0)
        Exposure = pContext->shadow.MinExposure;
    else
        Exposure = pParameters->Exposure;
    Status = Sensor_Common_ComputeCoarseAndFrameLengthForExposure(
                 &NewCoarse, &NewFrameLength,
                 pContext->shadow.VtPixClkFreqHz_F,
                 pNewScaler->LineLengthPck, Exposure,
                 pContext->shadow.RequestedMaxFrameRate,
                 pContext->shadow.MaxFrameLengthLines,
                 pNewScaler->FrameLengthLines);
    NV_ASSERT(Status);

    // 2. Apply the new scaler settings.
    Status = Sensor_Common_ProgramSensorMode(sensor, pOldScaler, pNewScaler,
                 NewFrameLength, NewCoarse, i2c);
    if (!Status)
    {
        NvOdmOsDebugPrintf("NvOdmImager: Failed to set resolution\n");
        NvOdmOsDebugPrintf("NvOdmImager: sensor is unresponsive\n");
        return NV_FALSE;
    }

    Freq = pContext->shadow.VtPixClkFreqHz_F;

    // 3. update sensor's shadow
    pContext->shadow.LineLengthPck = pNewScaler->LineLengthPck;
    pContext->shadow.LineLengthPck_F = (NvF32)pContext->shadow.LineLengthPck;
    pContext->shadow.FrameLengthLines = NewFrameLength;
    pContext->shadow.FrameLengthLines_F =
        (NvF32)pContext->shadow.FrameLengthLines;
    pContext->shadow.MinFrameLengthLines = pNewScaler->FrameLengthLines;
    pContext->shadow.currExposure = (NvF32)(pContext->shadow.currCoarse *
                                            pContext->shadow.LineLengthPck +
                                            NVODM_SENSOR_DEFAULT_EXPOS_FINE) /
                                            Freq;
    pContext->shadow.currFrameRate = Freq /
                                     (pContext->shadow.FrameLengthLines_F *
                                       pContext->shadow.LineLengthPck_F);
    pContext->shadow.currCoarse = NewCoarse;
    pContext->CurrentModeIndex = index;

    Sensor_Common_RecalculateExposureRange(&pContext->shadow);
    Sensor_Common_RecalculateFrameRateRange(&pContext->shadow);

    // 4. Set the desired gains
    if (pParameters->Gains[0] != 0.0 && pParameters->Gains[1] != 0.0 &&
        pParameters->Gains[2] != 0.0 && pParameters->Gains[3] != 0.0)
    {
        Sensor_Common_WriteGains(sensor, pParameters->Gains);
    }

    // 5. Wait 2 frames for gains/exposure to take effect.
    WaitTimeMs = (NvU32)(2000.0 / (NvF32)(pContext->shadow.currFrameRate));
    NvOsSleepMS(WaitTimeMs);


    NV_DEBUG_PRINTF(("Mode: Start %d,%d End %d,%d Size %d,%d Time %d %d\n",
                       ScalerSettings[index].XAddrStart,
                       ScalerSettings[index].YAddrStart,
                       ScalerSettings[index].XAddrEnd,
                       ScalerSettings[index].YAddrEnd,
                       ScalerSettings[index].XOutputSize,
                       ScalerSettings[index].YOutputSize,
                       ScalerSettings[index].LineLengthPck,
                       ScalerSettings[index].FrameLengthLines));

    if (pResult)
    {
        pResult->Resolution = SENSOR_MODES[index].ActiveDimensions;
        pResult->Exposure = pContext->shadow.currExposure;
        NvOdmOsMemcpy(pResult->Gains, &pContext->shadow.currGains, sizeof(NvF32) * 4);
    }

#endif
    return Status;
}
#endif

#ifndef SENSOR_COMMON_HARDRESET
#define SENSOR_COMMON_HARDRESET   Sensor_Common_HardReset
static NvBool Sensor_Common_HardReset(NvOdmImagerDeviceHandle hSensor)
{
    NvOdmImagerDeviceContext *pImagerCtx = (NvOdmImagerDeviceContext *)hSensor;
    NvOdmImagerI2cConnection *i2c = &pImagerCtx->i2c;
    SensorContext *pSensorCtx = (SensorContext *)pImagerCtx->prv;
    NvBool Ret = NV_TRUE;
    NvU32 CurrentModeIndex = pSensorCtx->CurrentModeIndex;
    SetModeParameters ModeParameters;

    // 1. Disable streaming
    NV_DEBUG_PRINTF(("Disable streaming\n"));
    NVODM_WRITEREG_RETURN_ON_ERROR(NVODM_SENSOR_REG_STREAMING_CONTROL,
        NVODM_SENSOR_STREAMING_CONTROL_OFF);

    // 2. Wait for the soft standby state is reached.
    NV_DEBUG_PRINTF(("Wait for the soft standby state is reached\n"));
    NvOdmOsWaitUS(1000);

    // 3. Assert RESET_BAR (active LOW) to reset the sensor
    NV_DEBUG_PRINTF(("Assert RESET_BAR\n"));
    NvOdmGpioSetState(pSensorCtx->hGpio,
                      pSensorCtx->hPin[pSensorCtx->PinsUsed],
                      NvOdmGpioPinActiveState_High);
    NvOdmGpioSetState(pSensorCtx->hGpio,
                      pSensorCtx->hPin[pSensorCtx->PinsUsed],
                      NvOdmGpioPinActiveState_Low);
    NvOdmOsWaitUS(1000);
    NvOdmGpioSetState(pSensorCtx->hGpio,
                      pSensorCtx->hPin[pSensorCtx->PinsUsed],
                      NvOdmGpioPinActiveState_High);
    NvOdmOsWaitUS(1000); // allow pin changes to settle

    // 4. Initial programming.
    NV_DEBUG_PRINTF(("Initial programming\n"));
#if !USE_SLOW_SETMODE
    SENSOR_INIT_PROGRAMMING(i2c, 0, pSensorCtx);
#endif
    pSensorCtx->CurrentModeIndex = INVALID_MODE_INDEX;
    NvOdmOsWaitUS(1000); // allow init programming settle time

    // 5. Set the sensor to the previous mode.
    NvOdmOsMemset(&ModeParameters, 0, sizeof(SetModeParameters));
    ModeParameters.Resolution = SENSOR_MODES[CurrentModeIndex].ActiveDimensions;
    ModeParameters.Exposure = pSensorCtx->shadow.currExposure;
    NvOdmOsMemcpy(&ModeParameters.Gains, &pSensorCtx->shadow.currGains, sizeof(NvF32) * 4);
    NV_DEBUG_PRINTF(("SENSOR_SET_MODE\n"));
    Ret = SENSOR_SET_MODE(
               hSensor,
               &ModeParameters,
               NULL,
               NULL);

    // TODO: other stuff: gains and exposure time...
    NV_DEBUG_PRINTF(("Hard resetting...Done\n"));

    return Ret;
}
#endif

#ifndef SENSOR_SET_POWER_LEVEL
#define SENSOR_SET_POWER_LEVEL   Sensor_Common_SetPowerLevel
static void AssignGpio(
    NvOdmImagerDeviceContext *hSensor,
    ImagerGpio Gpio)
{
    NvU32 code, tmp;

    Gpio.enable = NV_TRUE;
    code = hSensor->PinMappings >> (Gpio.pin * NVODM_CAMERA_GPIO_PIN_WIDTH);
    tmp = code & (NVODM_CAMERA_GPIO_PIN_MASK <<
                  NVODM_CAMERA_GPIO_PIN_SHIFT);
    switch (tmp)
    {
        case NVODM_CAMERA_RESET(0):
            Gpio.activeHigh = NV_TRUE;
            hSensor->Gpios[NvOdmImagerGpio_Reset] = Gpio;
            break;
        case NVODM_CAMERA_RESET_AL(0):
            Gpio.activeHigh = NV_FALSE;
            hSensor->Gpios[NvOdmImagerGpio_Reset] = Gpio;
            break;
        case NVODM_CAMERA_POWERDOWN(0):
            Gpio.activeHigh = NV_TRUE;
            hSensor->Gpios[NvOdmImagerGpio_Powerdown] = Gpio;
            break;
        case NVODM_CAMERA_POWERDOWN_AL(0):
            Gpio.activeHigh = NV_FALSE;
            hSensor->Gpios[NvOdmImagerGpio_Powerdown] = Gpio;
            break;
        case NVODM_CAMERA_FLASH_LOW(0):
            Gpio.activeHigh = NV_TRUE;
            hSensor->Gpios[NvOdmImagerGpio_FlashLow] = Gpio;
            break;
        case NVODM_CAMERA_FLASH_HIGH(0):
            Gpio.activeHigh = NV_TRUE;
            hSensor->Gpios[NvOdmImagerGpio_FlashHigh] = Gpio;
            break;
        case NVODM_CAMERA_MCLK(0):
            if (Gpio.pin == 0)
                hSensor->Gpios[NvOdmImagerGpio_MCLK] = Gpio;
            else if (Gpio.pin == 6)
                hSensor->Gpios[NvOdmImagerGpio_PWM] = Gpio;
            else
                NV_ASSERT(!"Unknown GPIO config");
            break;
        case NVODM_CAMERA_UNUSED:
            break;
        default:
            NV_ASSERT(!"Undefined gpio");
    }
}

static NvBool Sensor_Common_SetPowerLevel(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerPowerLevel powerLevel)
{
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    SensorContext *ctx;
#if !USE_SLOW_SETMODE
    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
#endif
    NvBool FrameSyncSuccess = NV_TRUE;

    ctx = (SensorContext *)hSensor->prv;

    if (powerLevel == NvOdmImagerPowerLevel_On)
    {
        SetModeParameters ModeParameters;
        NvU32 i;

        if (hSensor->CurrentPowerLevel == NvOdmImagerPowerLevel_On)
            return NV_TRUE;


#if WINGBOARD_PMU
        // Wingboard PMU
        {
            NvOdmServicesI2cHandle Handle;
            NvU32 DeviceAddr = 0x90;
            NvU8 WriteBuffer[2];
            NvOdmI2cStatus status = NvOdmI2cStatus_Success;
            NvOdmI2cTransactionInfo TransactionInfo;

            NvOdmOsDebugPrintf("**************************************\n");
            NvOdmOsDebugPrintf("* Change Wingboard PMU settings      *\n");
            Handle = NvOdmI2cOpen(NvOdmIoModule_I2c, 0x4);
            if (Handle != 0)
            {
                WriteBuffer[0] = 0x09;
                WriteBuffer[1] = 0x30; // trial and error found this value
                TransactionInfo.Address = DeviceAddr;
                TransactionInfo.Buf = &WriteBuffer[0];
                TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
                TransactionInfo.NumBytes = 2;

                status = NvOdmI2cTransaction(Handle, &TransactionInfo, 1,
                                             400, NV_WAIT_INFINITE);
                if (status == NvOdmI2cStatus_Success)
                {
                    NvOdmOsDebugPrintf(
                               "* Write of 0x%.2x <= 0x%.2x succeeded *\n",
                              WriteBuffer[0], WriteBuffer[1]);
                }
                else
                {
                    switch (status)
                    {
                        case NvOdmI2cStatus_Timeout:
                            NvOdmOsDebugPrintf(
                                "* Write of 0x%.2x <= 0x%.2x timed out *\n",
                              WriteBuffer[0], WriteBuffer[1]);
                            break;
                        case NvOdmI2cStatus_SlaveNotFound:
                        default:
                            NvOdmOsDebugPrintf(
                                "* Write of 0x%.2x <= 0x%.2x failed   *\n",
                              WriteBuffer[0], WriteBuffer[1]);
                            break;
                    }
                }
            }
            else
            {
                NvOdmOsDebugPrintf("* No device handle created           *\n");
            }
            NvOdmI2cClose(Handle);
            NvOdmOsDebugPrintf("**************************************\n");
        }
#endif

        if (hSensor->connections)
        {
            NvU32 UnassignedGpioCount = 0;
            ImagerGpio UnassignedGpio[NvOdmImagerGpio_Num];
            ImagerGpio NewGpio;
            NvOdmServicesPmuHandle hPmu;
            hPmu = NvOdmServicesPmuOpen();

            NewGpio.enable = NV_FALSE;
            NewGpio.port = 0;
            NewGpio.pin = 0;
            NewGpio.activeHigh = NV_FALSE;
            for (i = 0; i < NvOdmImagerGpio_Num; i++) {
                UnassignedGpio[i] = NewGpio;
            }

            for (i = 0; i < hSensor->connections->NumAddress; i++)
            {
                switch (hSensor->connections->AddressList[i].Interface)
                {
                    case NvOdmIoModule_Vdd:
                        {
                            NvOdmServicesPmuVddRailCapabilities cap;
                            NvU32 settle_us;
                            /* address is the vdd rail id */
                            NvOdmServicesPmuGetCapabilities( hPmu,
                               hSensor->connections->AddressList[i].Address,
                               &cap );
                            /* set the rail voltage to the recommended */
                            NvOdmServicesPmuSetVoltage( hPmu,
                               hSensor->connections->AddressList[i].Address,
                               cap.requestMilliVolts,
                               &settle_us );
                            /* wait for rail to settle */
                            NvOdmOsWaitUS( settle_us );
                        }
                        break;
                    case NvOdmIoModule_I2c:
                        {
                            hSensor->i2c.DeviceAddr =
                                hSensor->connections->AddressList[i].Address;
                            hSensor->i2c.I2cPort =
                                hSensor->connections->AddressList[i].Instance;
                            if (hSensor->i2c.I2cPort != 2)
                            {
                                NvOdmOsDebugPrintf("Warning, you are not ");
                                NvOdmOsDebugPrintf("using the I2c port that ");
                                NvOdmOsDebugPrintf("is for the imager\n");
                            }
                        }
                        break;
                    case NvOdmIoModule_Gpio:
                        {
                            if (hSensor->PinMappings == 0)
                            {
                                NV_ASSERT(UnassignedGpioCount <
                                          NvOdmImagerGpio_Num);
                                UnassignedGpio[UnassignedGpioCount].port =
                                    hSensor->connections->AddressList[i].Instance;
                                UnassignedGpio[UnassignedGpioCount].pin =
                                    hSensor->connections->AddressList[i].Address;
                                UnassignedGpioCount++;
                            }
                            else
                            {
                                NewGpio.port =
                                 hSensor->connections->AddressList[i].Instance;
                                NewGpio.pin =
                                 hSensor->connections->AddressList[i].Address;
                                NewGpio.enable = NV_TRUE;
                                AssignGpio(hSensor, NewGpio);
                            }
                        }
                        break;
                    case NvOdmIoModule_VideoInput:
                        {
                            NvU32 code;
                            NV_ASSERT(hSensor->PinMappings == 0);
                            hSensor->PinMappings =
                                hSensor->connections->AddressList[i].Address;
                            while (UnassignedGpioCount)
                            {
                                UnassignedGpioCount--;
                                AssignGpio(hSensor,
                                           UnassignedGpio[UnassignedGpioCount]);
                            }
                            // Check the data connection
                            code = ((hSensor->PinMappings >>
                                     NVODM_CAMERA_DATA_PIN_SHIFT)
                                    & NVODM_CAMERA_DATA_PIN_MASK);
                            switch (code)
                            {
                                case NVODM_CAMERA_PARALLEL_VD0_TO_VD9:
                                case NVODM_CAMERA_PARALLEL_VD0_TO_VD7:
                                case NVODM_CAMERA_SERIAL_CSI_D1A:
                                case NVODM_CAMERA_SERIAL_CSI_D2A:
                                case NVODM_CAMERA_SERIAL_CSI_D1A_D2A:
                                case NVODM_CAMERA_SERIAL_CSI_D1B:
                                default:
                                    break;
                            }
                        }
                        break;
                    default:
                        NV_ASSERT("! Unknown NvOdmIoAddress used");
                        break;
                }
            }
            NvOdmServicesPmuClose(hPmu);

            if (hSensor->Gpios[NvOdmImagerGpio_Powerdown].enable)
            {
                NvU32 port = hSensor->Gpios[NvOdmImagerGpio_Powerdown].port;
                NvU32 pin = hSensor->Gpios[NvOdmImagerGpio_Powerdown].pin;
                NvOdmGpioPinActiveState level;
                level = NvOdmGpioPinActiveState_Low;
                if (hSensor->Gpios[NvOdmImagerGpio_Powerdown].activeHigh)
                {
                    level = NvOdmGpioPinActiveState_High;
                }
                if (ctx->hGpio == NULL)
                    ctx->hGpio = NvOdmGpioOpen();
                NV_ASSERT(ctx->PinsUsed < MAX_GPIO_PINS &&
                          "Not enough handles in the context for this pin");
                ctx->hPin[ctx->PinsUsed] =
                    NvOdmGpioAcquirePinHandle(ctx->hGpio, port, pin);
                NvOdmGpioSetState(ctx->hGpio, ctx->hPin[ctx->PinsUsed], level);
                ctx->PinsUsed++;
            }
            if (hSensor->Gpios[NvOdmImagerGpio_Reset].enable)
            {
                NvU32 port = hSensor->Gpios[NvOdmImagerGpio_Reset].port;
                NvU32 pin = hSensor->Gpios[NvOdmImagerGpio_Reset].pin;
                NvOdmGpioPinActiveState levelA, levelB;
                levelA = NvOdmGpioPinActiveState_Low;
                levelB = NvOdmGpioPinActiveState_High;
                if (!hSensor->Gpios[NvOdmImagerGpio_Reset].activeHigh)
                {
                    levelA = NvOdmGpioPinActiveState_High;
                    levelB = NvOdmGpioPinActiveState_Low;
                }

                if (ctx->hGpio == NULL)
                    ctx->hGpio = NvOdmGpioOpen();
                NV_ASSERT(ctx->PinsUsed < MAX_GPIO_PINS &&
                          "Not enough handles in the context for this pin");
                ctx->hPin[ctx->PinsUsed] =
                    NvOdmGpioAcquirePinHandle(ctx->hGpio, port, pin);
                NvOdmGpioSetState(ctx->hGpio, ctx->hPin[ctx->PinsUsed], levelA);
                NvOdmGpioSetState(ctx->hGpio, ctx->hPin[ctx->PinsUsed], levelB);
                NvOdmOsWaitUS(1000);
                NvOdmGpioSetState(ctx->hGpio, ctx->hPin[ctx->PinsUsed], levelA);
                ctx->PinsUsed++;
            }
        }
        else
        {
            NvU32 port = NVODM_GPIO_CAMERA_PORT;
            NvU32 pin = 5; // VGP5

            // Turn power on from the PMU
            NvOdmServicesPmuHandle hPmu = NvOdmServicesPmuOpen();
            NvU32 latchTime = 0;
            // D7REG
            NvOdmServicesPmuSetVoltage(hPmu, 15, 2800, &latchTime);
            NvOdmOsWaitUS(latchTime);
            // RF1REG
            NvOdmServicesPmuSetVoltage(hPmu, 5, 2800, &latchTime);
            NvOdmOsWaitUS(latchTime);
            // RF3REG
            NvOdmServicesPmuSetVoltage(hPmu, 7, 1800, &latchTime);
            NvOdmOsWaitUS(latchTime);
            NvOdmServicesPmuClose(hPmu);

            // In order to power on the sensor we need to toggle vgp5 active high
            // We possibly want to do the open's at Init time. Just putting
            // the code here for reference of what we intend to do for Vail.
            if (ctx->hGpio == NULL)
                ctx->hGpio = NvOdmGpioOpen();
            NV_ASSERT(ctx->PinsUsed < MAX_GPIO_PINS &&
                      "Not enough handles in the context for this pin");
            ctx->hPin[ctx->PinsUsed] =
                NvOdmGpioAcquirePinHandle(ctx->hGpio, port, pin);

            NvOdmGpioSetState(ctx->hGpio,
                              ctx->hPin[ctx->PinsUsed],
                              NvOdmGpioPinActiveState_High);
            NvOdmGpioSetState(ctx->hGpio,
                              ctx->hPin[ctx->PinsUsed],
                              NvOdmGpioPinActiveState_Low);
            NvOdmOsWaitUS(1000);
            NvOdmGpioSetState(ctx->hGpio,
                              ctx->hPin[ctx->PinsUsed],
                              NvOdmGpioPinActiveState_High);
            ctx->PinsUsed++;
        }

        NvOdmOsWaitUS(1000); // allow pin changes to settle
#if !USE_SLOW_SETMODE
        SENSOR_INIT_PROGRAMMING(i2c, 0, ctx);
#endif
        ctx->CurrentModeIndex = INVALID_MODE_INDEX;
        NvOdmOsWaitUS(1000); // allow init programming settle time

        NvOdmOsMemset(&ModeParameters, 0, sizeof(SetModeParameters));
        ModeParameters.Resolution = SENSOR_MODES[gs_PreferredModeIndex].ActiveDimensions;
        ModeParameters.Exposure = ctx->shadow.MinExposure;
        for (i = 0; i < 4; i++)
            ModeParameters.Gains[i] = ctx->shadow.MinSensorGain;

        NV_DEBUG_PRINTF(("SENSOR_SET_MODE\n"));

        FrameSyncSuccess = SENSOR_SET_MODE(hSensor, &ModeParameters, NULL, NULL);
    }
    else if (powerLevel == NvOdmImagerPowerLevel_Standby)
    {
        if (hSensor->CurrentPowerLevel == NvOdmImagerPowerLevel_Standby)
            return NV_TRUE;
        if (hSensor->Gpios[NvOdmImagerGpio_Powerdown].enable)
        {
            NvOdmGpioPinActiveState level;
            NvOdmGpioGetState(ctx->hGpio, ctx->hPin[0], (NvU32 *)&level);
            NvOdmGpioSetState(ctx->hGpio, ctx->hPin[0], !level);
        }
    }
    else
    {
        NvU32 i;

        if (hSensor->CurrentPowerLevel == NvOdmImagerPowerLevel_Off)
            return NV_TRUE;

        /// 1. Turn sensor to standby mode via i2c
        /// 2. Turn off PMU voltages
        /// 3. Assert !reset (aka reset_bar)
        NVODM_WRITEREG_RETURN_ON_ERROR(
                                       NVODM_SENSOR_REG_MODE_SELECT,
                                       NVODM_SENSOR_MODE_STANDBY);

        // wait for the standby 0.1 sec should be enough
        NvOdmOsWaitUS(100000);

        if (hSensor->connections)
        {
            NvOdmServicesPmuHandle hPmu;
            hPmu = NvOdmServicesPmuOpen();
            for (i = 0; i < hSensor->connections->NumAddress; i++)
            {
                switch (hSensor->connections->AddressList[i].Interface)
                {
                    case NvOdmIoModule_Vdd:
                        {
                            NvU32 settle_us;
                            NvU32 Millivolts;
                            /* set the rail voltage to the recommended */
                            NvOdmServicesPmuSetVoltage(hPmu,
                               hSensor->connections->AddressList[i].Address,
                               NVODM_VOLTAGE_OFF,
                               &settle_us);
                            /* wait for rail to settle */
                            NvOdmOsWaitUS( settle_us );
                            /* make sure it is off */
                            NvOdmServicesPmuGetVoltage( hPmu,
                               hSensor->connections->AddressList[i].Address,
                               &Millivolts);
                        }
                        break;
                    default:
                        break;
                }
            }
            NvOdmServicesPmuClose(hPmu);
        }
        /// Required delay
        NvOdmOsWaitUS(5000);
        for (i = 0; i < 2; i++)
        {
            if (ctx->hPin[i])
            {
                NvOdmGpioPinActiveState level;
                NvOdmGpioGetState(ctx->hGpio, ctx->hPin[i], (NvU32 *)&level);
                NvOdmGpioSetState(ctx->hGpio, ctx->hPin[i], !level);
            }
            NvOdmGpioReleasePinHandle(ctx->hGpio, ctx->hPin[i]);
        }
        NvOdmGpioClose(ctx->hGpio);
        hSensor->CurrentPowerLevel = powerLevel;
    }

    if (FrameSyncSuccess)
        hSensor->CurrentPowerLevel = powerLevel;

    return FrameSyncSuccess;
}

#endif


/* *********************
 *      Frame Rate
 * *********************
 */
#if defined(NVODM_SENSOR_MICRON) && !defined(SENSOR_WRITE_FRAME_RATE)
#define SENSOR_WRITE_FRAME_RATE   Sensor_Common_WriteFrameRate
static NvBool
Sensor_Common_WriteFrameRate(
        NvOdmImagerDeviceHandle sensor,
        NvF32 *FrameRate,
        NvBool WriteNotQuery)
{
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    SensorContext *ctx = (SensorContext *)hSensor->prv;
    NvF32 freq = ctx->shadow.VtPixClkFreqHz_F;
    NvF32 lineLength = ctx->shadow.LineLengthPck_F;
    static NvF32 prev = 0.0f;

    NV_ASSERT(!WriteNotQuery);

    ctx->shadow.currFrameRate = freq / (ctx->shadow.FrameLengthLines_F * lineLength);
    if (FrameRate != 0) {
        *FrameRate = ctx->shadow.currFrameRate;
        if (prev != *FrameRate) {
            NV_DEBUG_PRINTF(("Sensor Theoretical Frame Rate is %f\n", *FrameRate));
            //NvOdmOsDebugPrintf("Sensor Theoretical Frame Rate is %f\n", *FrameRate);
        }
    }
    //Sensor_Common_RecalculateExposureRange(sensor);

    return NV_TRUE;
}
#endif


/* ****************************************************************
 * Update internal timing info so exposure calculations are correct
 * ****************************************************************
 */
#if defined(NVODM_SENSOR_MICRON) && !defined(SENSOR_SET_EXTERNAL_CLOCK)
#define SENSOR_SET_EXTERNAL_CLOCK   Sensor_Common_SetExternalClock
static NvBool
Sensor_Common_SetExternalClock(NvOdmImagerDeviceHandle sensor,
                               const NvOdmImagerClockProfile *pNewClockProfile)
{
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    SensorContext *ctx = (SensorContext *)hSensor->prv;
    NvF32 fClock;
    NvF32 oldClock = ctx->shadow.VtPixClkFreqHz_F;

    ctx->shadow.SensorInputClock = pNewClockProfile->ExternalClockKHz;
    ctx->shadow.SensorClockRatio = pNewClockProfile->ClockMultiplier;

    fClock = pNewClockProfile->ExternalClockKHz * 1000.0f *
             (NvF32)ctx->shadow.currPllMult /
              (NvF32)((NvS32)ctx->shadow.currPllPreDiv *
                      ctx->shadow.currPllVtPixDiv *
                      ctx->shadow.currPllVtSysDiv);

    ctx->shadow.VtPixClkFreqHz_F = fClock;
    ctx->shadow.VtPixClkFreqHz = (NvS32)fClock;

    if(ctx->shadow.currExposure == NVODM_UNDEF_VALUE)
    {
        NvS32 coarse =
            (NVODM_SENSOR_DEFAULT_EXPOS_COARSE > ctx->shadow.FrameLengthLines-1) ?
            ctx->shadow.FrameLengthLines-1 :
            NVODM_SENSOR_DEFAULT_EXPOS_COARSE;
        ctx->shadow.currExposure = (coarse *
            ctx->shadow.LineLengthPck_F + NVODM_SENSOR_DEFAULT_EXPOS_FINE) /
            ctx->shadow.VtPixClkFreqHz_F;
    }
    else
    {
        NV_ASSERT(oldClock != 0);
        ctx->shadow.currExposure *= oldClock / ctx->shadow.VtPixClkFreqHz_F;
    }

    // Recalculate our sensor limits
    // which depend on the clock rate:
    Sensor_Common_RecalculateExposureRange(&ctx->shadow);
    Sensor_Common_RecalculateFrameRateRange(&ctx->shadow);

    return NV_TRUE;
}
#endif


#ifndef SENSOR_SELF_TEST
#define SENSOR_SELF_TEST   Sensor_Common_SelfTest
static NvBool Sensor_Common_SelfTest(
    NvOdmImagerDeviceHandle sensor,
    NvOdmImagerSelfTest *pTest)
{
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
    NvOdmI2cStatus AckReturn;
#if NVODM_SENSOR_8_BIT_DEVICE
    NvU8 value[2];
#else
    NvU16 value;
#endif
    NvBool retVal;

    SENSOR_SET_POWER_LEVEL(sensor, NvOdmImagerPowerLevel_On);

    if (i2c->hOdmI2c == NULL) {
        i2c->hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 1);
        if (i2c->hOdmI2c == NULL)
        {
            NvOdmOsDebugPrintf("NvOdmI2cOpen failed\n");
            return NV_FALSE;
        }
    }

    switch (*pTest)
    {
        case NvOdmImagerSelfTest_Existence:
            AckReturn = NvOdmImagerI2cAck(i2c);
            if (AckReturn == NvOdmI2cStatus_Success)
            {
                return NV_TRUE;
            }
            else if (AckReturn == NvOdmI2cStatus_Timeout)
            {
                NvOdmOsDebugPrintf("Ack Timeout\n");
                return NV_FALSE;
            }
            else if (AckReturn == NvOdmI2cStatus_SlaveNotFound)
            {
                NvOdmOsDebugPrintf("Ack Failed: Slave Not Found\n");
                return NV_FALSE;
            }
            else
            {
                NvOdmOsDebugPrintf("I2C transaction failed\n");
                return NV_FALSE;
            }
        case NvOdmImagerSelfTest_DeviceId:
            NVODM_READREG(retVal);
            return retVal;
        case NvOdmImagerSelfTest_InitValidate:
        case NvOdmImagerSelfTest_FullTest:
            // none implemented, so return true
            return NV_TRUE;
        default:
            return NV_FALSE;
    }
}
#endif

#ifndef SENSOR_SET_PARAMETER
#define SENSOR_SET_PARAMETER   Sensor_Common_SetParameter
static NvBool Sensor_Common_SetParameter(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerParameter param,
        NvS32 sizeOfValue,
        const void *value)
{
    NvBool result = NV_TRUE;

#ifdef NVODM_SENSOR_MICRON
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
    SensorContext *ctx = hSensor->prv;
    const NvF32 *pFloatValue = value;
    NvF32 fVal;
    int i;
#endif

    switch(param)
    {
        case NvOdmImagerParameter_SelfTest:
            result = SENSOR_SELF_TEST(sensor,
                (NvOdmImagerSelfTest *)value);
        break;

#ifdef NVODM_SENSOR_MICRON
        case NvOdmImagerParameter_SensorExposure:
            if(sizeOfValue == sizeof(NvF32))
            {
                fVal = *pFloatValue;
                if (fVal > ctx->shadow.MaxExposure) {
                    fVal = ctx->shadow.MaxExposure;
                } else if (fVal < ctx->shadow.MinExposure) {
                    fVal = ctx->shadow.MinExposure;
                }
                result = Sensor_Common_WriteExposure(sensor,&fVal,NULL);
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorGain:
            if(sizeOfValue == 4 * sizeof(NvF32))
            {
                for(i = 0; i < 4; i++)
                {
                    if (pFloatValue[i] > ctx->shadow.MaxSensorGain ||
                        pFloatValue[i] < ctx->shadow.MinSensorGain)
                    {
                        return NV_FALSE;
                    }
                }

                result = Sensor_Common_WriteGains(sensor, pFloatValue);
                for(i = 0; i < 4; i++)
                {
                    ctx->shadow.currGains[i] = pFloatValue[i];
                }
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorInputClock:
            if(sizeOfValue == sizeof(NvOdmImagerClockProfile))
            {
                NvOdmImagerClockProfile *pProfile;
                pProfile = (NvOdmImagerClockProfile *)value;
                Sensor_Common_SetExternalClock(sensor,pProfile);
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_MaxSensorFrameRate:
            if(sizeOfValue == sizeof(NvF32))
            {
                ctx->shadow.RequestedMaxFrameRate = *pFloatValue;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_TestMode:
            if (sizeOfValue == sizeof (NvBool))
            {
                NvBool TestMode = *((NvBool *)value);
                NvOdmI2cStatus Status = NvOdmI2cStatus_Success;
                if (TestMode)
                {
                    NvF32 Gains[4];
                    NvU32 i;

                    // reset gains
                    for (i = 0; i < 4; i++)
                        Gains[i] = ctx->shadow.MinSensorGain;

                    result = SENSOR_WRITE_GAINS(sensor, Gains);
                    if (!result)
                        return NV_FALSE;

                    // 0 = normal, 1 = solid, 2 = color bars
                    // 3 = fade color bars, 256 = walking ones
                    Status = NVODM_IMAGER_I2C_WRITE(i2c,
                                                    NVODM_SENSOR_REG_TEST_MODE,
                                                    NVODM_SENSOR_REG_TEST_COLOR_BARS);
                }
                else
                {
                    Status = NVODM_IMAGER_I2C_WRITE(i2c,
                                                    NVODM_SENSOR_REG_TEST_MODE,
                                                    NVODM_SENSOR_REG_TEST_NORMAL);
                }

                if (Status != NvOdmI2cStatus_Success)
                    return NV_FALSE;

                ctx->TestPatternMode = TestMode;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;
#endif
        case NvOdmImagerParameter_Reset:
            if (sizeOfValue == sizeof(NvOdmImagerReset))
            {
                NvOdmImagerReset Reset = *(NvOdmImagerReset*)value;
                switch(Reset)
                {
                    case NvOdmImagerReset_Hard:
                        result = SENSOR_COMMON_HARDRESET(sensor);
                        break;

                    case NvOdmImagerReset_Soft:
                    default:
                        NV_ASSERT(!"Not Implemented!");
                        result = NV_FALSE;
                }
            }
            else
            {
                NV_ASSERT(!"Bad size");
                result = NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_OptimizeResolutionChange:
#ifndef SENSOR_CAP_OPTIMIZE_RESOLUTION_CHANGE
// SENSOR_CAP_OPTIMIZE_RESOLUTION_CHANGE: can the sensor
// do fast resolution change.
// set this to 1 so nothing will be changed for now.
#define SENSOR_CAP_OPTIMIZE_RESOLUTION_CHANGE (1)
#endif

            if (sizeOfValue == sizeof(NvBool))
            {
                NvBool Optimize = *(NvBool*)value;

                // TODO: store this parameter and choose FastSetMode or
                //       SlowSetMode based on it.

                if (Optimize && !SENSOR_CAP_OPTIMIZE_RESOLUTION_CHANGE)
                    result = NV_FALSE;

            }
            else
            {
                NV_ASSERT(sizeOfValue == sizeof(NvBool));
                result = NV_FALSE;
            }

            break;

        default:
            return NV_TRUE;
    }
    return result;
}
#endif

#ifndef SENSOR_GET_PARAMETER
#define SENSOR_GET_PARAMETER   Sensor_Common_GetParameter
static NvBool Sensor_Common_GetParameter(
        NvOdmImagerDeviceHandle sensor,
        NvOdmImagerParameter param,
        NvS32 sizeOfValue,
        void *value)
{
    NvBool result = NV_TRUE;

#ifdef NVODM_SENSOR_MICRON
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    SensorContext *ctx = (SensorContext *)hSensor->prv;
    NvF32 *pFloatValue = value;
    NvU32 *pIntValue = value;
#endif
    int i;

    NV_ASSERT(value);
    if(!value)
        return NV_FALSE;

    switch(param)
    {
        case NvOdmImagerParameter_RegionUsedByCurrentResolution:
            if (sizeOfValue == sizeof(NvOdmImagerRegion))
            {
#ifdef NVODM_SENSOR_MICRON
                if (ctx->CurrentModeIndex == INVALID_MODE_INDEX)
                {
                    // No info available until we've actually set a mode.
                    return NV_FALSE;
                }
                else
                {
                    SMIAScalerSettings *Current = &ScalerSettings[ctx->CurrentModeIndex];
                    SMIAScalerSettings *Max = &ScalerSettings[g_NumberOfResolutions - 1];
                    NvOdmImagerRegion *pRegion = (NvOdmImagerRegion*)value;

                    if (Max->XAddrStart > Current->XAddrStart)
                        pRegion->RegionStart.x = 0;
                    else
                        pRegion->RegionStart.x = Current->XAddrStart - Max->XAddrStart;
                    if (Max->YAddrStart > Current->YAddrStart)
                        pRegion->RegionStart.y = 0;
                    else
                        pRegion->RegionStart.y = Current->YAddrStart - Max->YAddrStart;
                    // X/Y Odd Inc will be zero if uninitialized, it which case
                    // returning 1 as X/YScale is reasonable
                    NV_ASSERT(Current->XOddInc == 1 || Current->XOddInc == 3 ||
                        Current->XOddInc == 7 || Current->XOddInc == 0);
                    NV_ASSERT(Current->YOddInc == 1 || Current->YOddInc == 3 ||
                        Current->YOddInc == 7 || Current->YOddInc == 0);
                    pRegion->xScale = ODDINC_TO_SCALE(Current->XOddInc);
                    pRegion->yScale = ODDINC_TO_SCALE(Current->YOddInc);
                }
#endif
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_CalibrationData:
            if (sizeOfValue == sizeof(NvOdmImagerCalibrationData))
            {
                NvOdmImagerCalibrationData *pCalibration =
                    (NvOdmImagerCalibrationData*)value;
                pCalibration->CalibrationData = sensorCalibrationData;
                pCalibration->NeedsFreeing = NV_FALSE;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_CalibrationOverrides:
            if (sizeOfValue == sizeof(NvOdmImagerCalibrationData))
            {
                // Try to fstat the overrides file, allocate a buffer
                // of the appropriate size, and read it in.
                static const char *files[] =
                {
                    "\\release\\ap15_overrides.isp",
                    "\\Storage Card\\ap15_overrides.isp",
                    "\\NandFlash\\ap15_overrides.isp",
                    "ap15_overrides.isp",
                    "\\release\\camera_overrides.isp",
                    "\\Storage Card\\camera_overrides.isp",
                    "\\NandFlash\\camera_overrides.isp",
                    "camera_overrides.isp",
                };
                NvOdmImagerCalibrationData *pCalibration =
                    (NvOdmImagerCalibrationData*)value;
                NvOdmOsStatType stat;
                NvOdmOsFileHandle file = NULL;
                // use a tempString so we can properly respect const-ness
                char *tempString;

                for(i = 0; i < NV_ARRAY_SIZE(files); i++)
                {
                    if(NvOdmOsStat(files[i], &stat) && stat.size > 0)
                        break;
                }

                if(i < NV_ARRAY_SIZE(files))
                {
                    tempString = NvOdmOsAlloc((size_t)stat.size + 1);
                    if(tempString == NULL)
                    {
                        NV_ASSERT(tempString != NULL &&
                            "Couldn't alloc memory to read config file");
                        result = NV_FALSE;
                        break;
                    }
                    if(!NvOdmOsFopen(files[i], NVODMOS_OPEN_READ, &file))
                    {
                        NV_ASSERT(!"Failed to open a file that fstatted just fine");
                        result = NV_FALSE;
                        NvOdmOsFree(tempString);
                        break;
                    }
                    NvOdmOsFread(file, tempString, (size_t)stat.size, NULL);
                    tempString[stat.size] = '\0';
                    NvOdmOsFclose(file);
                    pCalibration->CalibrationData = tempString;
                    pCalibration->NeedsFreeing = NV_TRUE;
                }
                else
                {
                    pCalibration->CalibrationData = NULL;
                    pCalibration->NeedsFreeing = NV_FALSE;
                }
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

#ifdef NVODM_SENSOR_MICRON
        // Read/write params
        case NvOdmImagerParameter_SensorExposure:
            if (sizeOfValue == sizeof(NvF32)) {
                *pFloatValue = ctx->shadow.currExposure;
            } else {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorGain:
            if(sizeOfValue == 4 * sizeof(NvF32))
            {
                if (ctx->shadow.currGains[0] == NVODM_UNDEF_VALUE)
                {
                    result = Sensor_Common_ReadGains(sensor, ctx->shadow.currGains);
                }
                for(i = 0; i < 4; i++)
                {
                    pFloatValue[i] = ctx->shadow.currGains[i];
                }
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorFrameRate:
            if(sizeOfValue == sizeof(NvF32))
            {
                NvF32 fR;
                result = Sensor_Common_WriteFrameRate(sensor,&fR,NV_FALSE);
                *pFloatValue = fR;
                //NvOsDebugPrintf("NvOdmImagerParameter_SensorFrameRate %f\n", fR);
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorInputClock:
            if(sizeOfValue == sizeof(NvOdmImagerClockProfile))
            {
                NvOdmImagerClockProfile *pProfile;
                pProfile = (NvOdmImagerClockProfile *)value;
                pProfile->ExternalClockKHz = ctx->shadow.SensorInputClock;
                pProfile->ClockMultiplier = ctx->shadow.SensorClockRatio;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        // Read-only params
        case NvOdmImagerParameter_SensorExposureLimits:
            if(sizeOfValue == 2 * sizeof(NvF32))
            {
                pFloatValue[0] = ctx->shadow.MinExposure;
                pFloatValue[1] = ctx->shadow.MaxExposure;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorGainLimits:
            if(sizeOfValue == 2 * sizeof(NvF32))
            {
                pFloatValue[0] = ctx->shadow.MinSensorGain;
                pFloatValue[1] = ctx->shadow.MaxSensorGain;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_MaxSensorFrameRate:
            if(sizeOfValue == sizeof(NvF32))
            {
                pFloatValue[0] = ctx->shadow.RequestedMaxFrameRate;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;
        case NvOdmImagerParameter_SensorFrameRateLimits:
            if(sizeOfValue == 2 * sizeof(NvF32))
            {
                pFloatValue[0] = ctx->shadow.MinFrameRate;
                pFloatValue[1] = ctx->shadow.MaxFrameRate;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorFrameRateLimitsAtResolution:
            if(sizeOfValue == sizeof(NvOdmImagerFrameRateLimitAtResolution))
            {
                NvOdmImagerFrameRateLimitAtResolution *pData;
                NvU32 Index;
                NvBool MatchFound = NV_FALSE;
                NvF32 Freq = ctx->shadow.VtPixClkFreqHz_F;
                NvF32 LineLength;

                pData = (NvOdmImagerFrameRateLimitAtResolution *)value;
                MatchFound =
                    Sensor_Common_ChooseModeIndex(pData->Resolution, &Index);

                if (!MatchFound)
                {
                    pData->MinFrameRate = 0.0f;
                    pData->MaxFrameRate = 0.0f;
                    break;
                }

                LineLength = ScalerSettings[Index].LineLengthPck;

                pData->MinFrameRate = Freq /
                    ((NvF32)NVODM_SENSOR_REG_MAX_FRAME_LINES * LineLength);
                pData->MaxFrameRate = Freq /
                    (ScalerSettings[Index].FrameLengthLines * LineLength);
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorClockLimits:
            if(sizeOfValue == 2 * sizeof(NvF32))
            {
                pIntValue[0] = ctx->shadow.MinSensorInputClock;
                pIntValue[1] = ctx->shadow.MaxSensorInputClock;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

        case NvOdmImagerParameter_SensorExposureLatchTime:
            if(sizeOfValue == sizeof(NvU32))
            {
                *pIntValue = 1;
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;

#if 0
        case NvOdmImagerParameter_FlashCapabilities:
            if(sizeOfValue == sizeof(NvOdmImagerFlashCapabilities))
            {
                NvOdmImagerFlashCapabilities *flash = (NvOdmImagerFlashCapabilities *)value;
                flash->NumberOfFlashPins = 0;
                if (hSensor->Gpios[NvOdmImagerGpio_FlashLow].enable)
                {
                    flash->GpioPin[flash->NumberOfFlashPins] =
                        hSensor->Gpios[NvOdmImagerGpio_FlashLow].pin;
                    // TODO: how to get this best, maybe #defines
                    flash->FlashMilliamps[flash->NumberOfFlashPins] = 200;
                    flash->NumberOfFlashPins++;
                }
                if (hSensor->Gpios[NvOdmImagerGpio_FlashHigh].enable)
                {
                    flash->GpioPin[flash->NumberOfFlashPins] =
                        hSensor->Gpios[NvOdmImagerGpio_FlashHigh].pin;
                    // TODO: how to get this best, maybe #defines
                    flash->FlashMilliamps[flash->NumberOfFlashPins] = 600;
                    flash->NumberOfFlashPins++;
                }

                // TODO: remove this
                if (hSensor->connections == NULL)
                {
                    flash->GpioPin[0] = 3;
                    flash->GpioPin[1] = 6;
                    flash->NumberOfFlashPins = 2;
                }
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;
#endif

        case NvOdmImagerParameter_ExpectedValues:
            {
                NvOdmImagerExpectedValuesPtr *pValuesPtr;
                pValuesPtr = (NvOdmImagerExpectedValuesPtr *)value;
#ifdef SENSOR_EXPECTED_VALUES
                *pValuesPtr = & (SENSOR_EXPECTED_VALUES);
#else
                *pValuesPtr = NULL;
                result = NV_FALSE;
#endif
            }
            break;
        case NvOdmImagerParameter_DeviceStatus:
            if(sizeOfValue == sizeof(NvOdmImagerDeviceStatus))
            {
                NvOdmImagerDeviceStatus *pStatus;
                pStatus = (NvOdmImagerDeviceStatus *)value;
#ifdef SENSOR_REGISTER_LIST
                {
                    NvU32 bucket = 0, addr;
                    NvU16 val;
                    NvU32 i = 0, j;
                    NvOdmImagerI2cConnection *i2c = &hSensor->i2c;
                    for (bucket = 0; bucket < SENSOR_REGISTER_LIST_COUNT; bucket++)
                    {
                        for (j = 0; j < SENSOR_REGISTER_LIST[bucket].Count; j++)
                        {
                            addr = SENSOR_REGISTER_LIST[bucket].Start + j*2;
                            NVODM_IMAGER_I2C_READ(i2c, addr, &val);
                            pStatus->Values[i++] = val;
                            //NvOdmOsDebugPrintf("<0x%.4x> = 0x%.4x\n", addr, val);
                            if (i >= MAX_STATUS_VALUES)
                                break;
                        }
                        if (i >= MAX_STATUS_VALUES)
                            break;
                    }
                    pStatus->Count = i;
                }
#endif
            }
            else
            {
                NV_ASSERT(!"Bad size");
                return NV_FALSE;
            }
            break;
#endif

        case NvOdmImagerParameter_LinesPerSecond:
            if (sizeOfValue == sizeof (NvF32))
            {
                NvF32 YScale = (NvF32)ODDINC_TO_SCALE(
                                ScalerSettings[ctx->CurrentModeIndex].YOddInc);

                if (!USE_BINNING)
                    YScale = 1.0f;

                *((NvF32*)value) = ctx->shadow.VtPixClkFreqHz_F /
                                   (ctx->shadow.LineLengthPck_F * YScale);

            }
            else
            {
                NV_ASSERT(!"Bad size");
            }
            break;
        default:
            return NV_FALSE;
    }
    return result;
}
#endif




//////////////////////////////////////////////////////////
//   PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////

#ifndef SENSOR_OPEN
#define SENSOR_OPEN   SENSOR_COMMON_OPEN
NvBool SENSOR_COMMON_OPEN(NvOdmImagerDeviceHandle sensor)
{
    NvOdmImagerDeviceContext *hSensor;
    SensorContext *ctx;
#if defined(NVODM_SENSOR_MICRON)
    int i;
#endif

    hSensor = (NvOdmImagerDeviceContext *)sensor;
    hSensor->i2c.DeviceAddr = NVODM_SENSOR_DEVICE_ADDR;
    hSensor->i2c.I2cPort    = 1; // default is AP15's camera i2c port
    hSensor->GetCapabilities = SENSOR_GET_CAPABILITIES;
    hSensor->ListModes       = SENSOR_LIST_MODES;
    hSensor->SetMode         = SENSOR_SET_MODE;
    hSensor->SetPowerLevel   = SENSOR_SET_POWER_LEVEL;
    hSensor->SetParameter    = SENSOR_SET_PARAMETER;
    hSensor->GetParameter    = SENSOR_GET_PARAMETER;

    ctx = (SensorContext*)NvOdmOsAlloc(sizeof(SensorContext));
    if (ctx == NULL) {
        return NV_FALSE;
    }

    NvOdmOsMemset(ctx, 0, sizeof(SensorContext));

    ctx->hGpio = NULL;
    ctx->hPin[0] = NULL;
    ctx->hPin[1] = NULL;
    hSensor->prv = (void *)ctx;

#if defined(NVODM_SENSOR_MICRON)
    ctx->shadow.MinSensorInputClock =  NVODM_SENSOR_REG_MIN_INPUT_CLOCK;
    ctx->shadow.MaxSensorInputClock = NVODM_SENSOR_REG_MAX_INPUT_CLOCK;
    ctx->shadow.SensorClockRatio = 1.0;

    ctx->shadow.MinSensorGain = NVODM_SENSOR_GAIN_TO_F32(NVODM_SENSOR_MIN_GAIN);
    ctx->shadow.MaxSensorGain = NVODM_SENSOR_GAIN_TO_F32(NVODM_SENSOR_MAX_GAIN);

    ctx->shadow.MaxFrameLengthLines = NVODM_SENSOR_REG_MAX_FRAME_LINES;
    ctx->shadow.MinFrameLengthLines = ScalerSettings[gs_PreferredModeIndex].FrameLengthLines;
    ctx->shadow.LineLengthPck = NVODM_SENSOR_REG_LINE_LENGTH_PACK;
    ctx->shadow.LineLengthPck_F = (NvF32)ctx->shadow.LineLengthPck;
    ctx->shadow.FrameLengthLines = NVODM_SENSOR_REG_FRAME_LINES;
    ctx->shadow.FrameLengthLines_F = (NvF32)ctx->shadow.FrameLengthLines;
    ctx->shadow.VtPixClkFreqHz = ctx->shadow.MinSensorInputClock * 1000; // in Hz
    ctx->shadow.VtPixClkFreqHz_F = (NvF32)ctx->shadow.VtPixClkFreqHz;


    ctx->CurrentModeIndex = INVALID_MODE_INDEX;  // an invalid index

    ctx->shadow.currPllPreDiv = NVODM_SENSOR_PLL_PRE_DIV;
    ctx->shadow.currPllMult = NVODM_SENSOR_PLL_MULT;
    ctx->shadow.currPllVtPixDiv = NVODM_SENSOR_PLL_VT_PIX_DIV;
    ctx->shadow.currPllVtSysDiv = NVODM_SENSOR_PLL_VT_SYS_DIV;
    ctx->shadow.currPllOpPixDiv = NVODM_SENSOR_PLL_OP_PIX_DIV;
    ctx->shadow.currPllOpSysDiv = NVODM_SENSOR_PLL_OP_SYS_DIV;
    ctx->shadow.currPllRowspeed = NVODM_SENSOR_ROWSPEED;
    ctx->shadow.currFrameRate = 30.f;
    ctx->shadow.currExposure = NVODM_UNDEF_VALUE;
    ctx->shadow.currCoarse = (-1);
         // To be actually set on first Sensor_Common_WriteExposure()
    for (i = 0; i < 4; i++) {
        ctx->shadow.currGains[i] = 1.0f; //??? Or take sensor Init.values ASD
    }
    for (i = 0; i < 4; i++) {
        ctx->analogGainBalance[i] = 1.0f;
    }
#endif

    return NV_TRUE;
}
#endif

#ifndef SENSOR_CLOSE
#define SENSOR_CLOSE   SENSOR_COMMON_CLOSE
void SENSOR_COMMON_CLOSE(NvOdmImagerDeviceHandle sensor)
{
    NvOdmImagerDeviceContext *hSensor = (NvOdmImagerDeviceContext *)sensor;
    NvOdmI2cClose(hSensor->i2c.hOdmI2c);
    NvOdmOsFree(hSensor->prv);
    hSensor->prv = NULL;
}

#endif

#endif
