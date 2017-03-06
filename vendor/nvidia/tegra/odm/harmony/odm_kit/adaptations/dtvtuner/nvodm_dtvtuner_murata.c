/*
 * Copyright (c) 2006 - 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvodm_services.h"
#include "nvodm_query_discovery.h"
#include "nvodm_query_gpio.h"
#include "nvodm_dtvtuner.h"
#include "nvodm_dtvtuner_int.h"
#include "nvodm_dtvtuner_murata.h"


/** 
 * Exported Murata cap data
 */

const NvOdmDtvtunerCap MurataCap =
{
    DTV_MURATA_057_GUID,
    NvOdmDtvtunerType_ISDBT,
    NvOdmDtvtunerConnect_VIP,
    {
        /* Default timing: 188 byte packets */
        0,
        188,
        NvOdmDtvtuner_StoreUpstreamErrorPktsParam_Discard,
        NvOdmDtvtuner_BodyValidSelectParam_Ignore,
        NvOdmDtvtuner_StartSelectParam_Valid,
        NvOdmDtvtuner_ClockPolarityParam_High,
        NvOdmDtvtuner_ErrorPolarityParam_High,
        NvOdmDtvtuner_PsyncPolarityParam_High,
        NvOdmDtvtuner_ValidPolarityParam_High,
        NvOdmDtvtuner_ProtocolSelectParam_TsPsync
    },
};

/** 
 * Internal Murata register data
 */

#define I2CADDR_DEMO 0x22
#define I2CADDR_RFIC 0xc0

#define I2CADDR_PMU 0x90
#define LD0_CTRL2 0x09
#define VLD04_28V 0x60

// 1.3 Soft Reset/Release
    //Please set the following setting 1.6ms after releasing the Hard reset.
static const I2CRegData SoftResetRelease[] = {
    {0x70, 0x00}, //Soft reset
    {0x70, 0xff}, //Release Soft reset
};

// 1.4 Synchronization sequence Reset
static const I2CRegData SynchronizationSequenceReset[] = {
    {0x08, 0x01},
};

// 1.5 Initialization. Number X in InitX indicate section number 1.5.x
static const I2CRegData Init1[] = {
    {0xd0, 0x62},
    {0x1c, 0x00},
};

static const I2CRegData Init2TVUHF[] = {
        {0x28, 0x00},
        {0x29, 0x00},
        {0x2a, 0x00},
        {0x2b, 0x30},
};

static const I2CRegData Init2Radio[] = {
        {0x28, 0x00},
        {0x29, 0x00},
        {0x2a, 0x00},
        {0x2b, 0x3c},
};

static const I2CRegData Init3_4[] = {
    //1.5.3 Clock setting
    {0x28, 0x2a},
    {0x29, 0x0d},
    {0x2a, 0x8b},
    {0x2b, 0x83},
    //1.5.4 AGC (Auto Gain Control) setting
    {0x01, 0x0d},
    {0x04, 0x0a},
    {0x05, 0xa3},
    {0x04, 0x0b},
    {0x05, 0x40},
    {0x04, 0x29},
    {0x05, 0x64},
};

static const I2CRegData Init4TVUHF[] = {
        {0x04, 0x0e},
        {0x05, 0x01},
        {0x04, 0x0f},
        {0x05, 0x80},
};

static const I2CRegData Init4Radio1seg[] = {
        {0x04, 0x0e},
        {0x05, 0x01},
        {0x04, 0x0f},
        {0x05, 0x00},
};

static const I2CRegData Init4Radio3seg[] = {
        {0x04, 0x10},
        {0x05, 0x01},
        {0x04, 0x11},
        {0x05, 0x80},
};

//1.5.5 TS [Transport stream] output setting
static const I2CRegData Init5_TS1segTV[] = {
    {0x50, 0xd1},
    {0x51, 0x62},
    {0x50, 0xd7},
    {0x51, 0xbf},
};
static const I2CRegData Init5_TSRadio[] = {
    {0x50, 0xd1},
    {0x51, 0x62},
    {0x50, 0xd7},
    {0x51, 0x39},
};

static const I2CRegData Init8_12[] = {
    //1.5.8 Timer setting
    {0x28, 0x6d},
    {0x29, 0x00},
    {0x2a, 0x00},
    {0x2b, 0x04},
    {0x28, 0x54},
    {0x29, 0x00},
    {0x2a, 0x00},
    {0x2b, 0x20},
    {0x28, 0x70},
    {0x29, 0x00},
    {0x2a, 0x00},
    {0x2b, 0x00},
    //1.5.9 Mode detection setting
    {0x28, 0x44},
    {0x29, 0x00},
    {0x2a, 0x00},
    {0x2b, 0x50},
    {0x28, 0x47},
    {0x29, 0x00},
    {0x2a, 0x00},
    {0x2b, 0xf0},
    {0x28, 0x49},
    {0x29, 0x00},
    {0x2a, 0x00},
    {0x2b, 0x60},
    {0x28, 0x4b},
    {0x29, 0x00},
    {0x2a, 0x2c},
    {0x2b, 0x0c},
    {0x28, 0x4c},
    {0x29, 0x00},
    {0x2a, 0x00},
    {0x2b, 0x10},
    //1.5.10 AFC (Auto Frequency Control) setting
    {0x17, 0x06},
    {0x18, 0x02},
    {0x1a, 0x07},
    {0x1b, 0x03},
    {0x20, 0x00},
    {0x21, 0x00},
    {0x26, 0xc2},
    {0x27, 0xe2},
    //1.5.11 Synchronization checking
    {0x40, 0x58},
    {0x41, 0x08},
    //1.5.12 FFT Window control
    {0x3b, 0x05},
    {0x3c, 0x0c},
    {0x3b, 0x06},
    {0x3c, 0x0c},
};

//1.5.13 Digital purpose I/O port setting
static const I2CRegData Init13TV[] = {
        {0xeb, 0x41},
};
static const I2CRegData Init13Radio[] = {
        {0xeb, 0x40},
};

//1.6 Received segment setting
static const I2CRegData SegmentTV_1segRadio[] = {
        {0x71, 0x06},
};
static const I2CRegData Segment3segRadio[] = {
        {0x71, 0x07},
};

//1.7.1 Received spectrum polarity setting
static const I2CRegData SetChannel_1[] = {
    {0x09, 0x1e},
};

//1.7.2. Center Frequency offset setting
static const I2CRegData SetChannel_2_1seg[] = {
        {0x28, 0x2d},
        {0x29, 0xea},
        {0x2a, 0x20},
        {0x2b, 0x5c},
};
static const I2CRegData SetChannel_2_3seg[] = {
        {0x28, 0x2d},
        {0x29, 0xea},
        {0x2a, 0x13},
        {0x2b, 0x75},
};

//1.7.3 Sub channel setting
static const I2CRegData Sub_channel_setting_1segTV[] = {
        {0x44, 0x00},
};
    
//1.8 RF IC channel setting
//RF init. section 3.2.1, page 22
static const I2CRegData Rf_init_common[] = {
    {0x0c, 0x49},
    {0x0d, 0x82},
    {0x0f, 0x70},
    {0x14, 0xc0},
    {0x15, 0xc0},
    {0x1b, 0xdc},
    {0x1c, 0xca},
    {0x1d, 0x01},
    {0x24, 0x00},
    {0x28, 0xd1},
    {0x29, 0x31},
    {0x32, 0x02},
    {0x66, 0x0f},
};
static const I2CRegData Rf_init_1segTV[] = {
    {0x0a, 0xfd},
    {0x1a, 0xd6},
    {0x1e, 0xfc},
    {0x3c, 0x4a},
    {0x45, 0x25},
};
static const I2CRegData Rf_init_1segRadio[] = {
    {0x0a, 0xe5},
    {0x1a, 0xa6},
    {0x1e, 0xfc},
    {0x3c, 0xcb},
    {0x45, 0x25},
};
static const I2CRegData Rf_init_3segRadio[] = {
    {0x0a, 0xe5},
    {0x1a, 0xb6},
    {0x1e, 0xf8},
    {0x3c, 0xcb},
    {0x45, 0x2f},
};

//RF Power up with no sequencer go:
static const I2CRegData Rf_Powerup_No_Sequencer_Go[] = {
         {0x09, 0xf0},
};
//RF Power up:
static const I2CRegData Rf_Powerup[] = {
         {0x09, 0xf4},
};

//1.9 Release the Synchronized Sequence reset
static const I2CRegData Release_Sync_Seq_reset[] = {
    {0x08, 0x00},
};

#define NVODM_MURATA_WRITE_DEMOD_SEQUENCE(RegSeq) \
    do {\
        NvU32 i, count; \
        count = NV_ARRAY_SIZE(RegSeq); \
        for (i = 0; i < count; i++) \
        { \
            NvOdmTVI2cWrite8(pHW->hI2C, I2CADDR_DEMO, \
                             RegSeq[i].RegAddr, RegSeq[i].RegValue); \
        } \
    }while(0)

#define NVODM_MURATA_WRITE_RFIC_SEQUENCE(RegSeq) \
    do {\
        NvU32 i, count; \
        count = NV_ARRAY_SIZE(RegSeq); \
        for (i = 0; i < count; i++) \
        { \
            NvOdmMurataRFWrite8(pHW->hI2C, \
                             RegSeq[i].RegAddr, RegSeq[i].RegValue); \
        } \
    }while(0)

/** Helper functions
*/
static NvU8 NvOdmMurataRFReg44Val(NvU32 subChannel)
{
    //valid subChannel range is from 0 to 41

    NvU8 reg44;
    if (subChannel == 41) 
        reg44 = 0x90;
    else
        reg44 = (NvU8)((((subChannel + 1) / 3) * 0x10 + 0x90) & 0xff);
    
    return reg44;
}

static NvBool NvOdmMurataRFWrite8(NvOdmServicesI2cHandle hI2C, 
                                  NvU8 RegAddr, NvU8 RegValue)
{
    NvU8 WriteBuffer[] = {0x00, 0x00,
                          0xfe};
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;

    //First put Murata into i2c bypass mode:
    NvOdmTVI2cWrite8(hI2C, I2CADDR_DEMO, 0xfe, 0x00);

    WriteBuffer[0] = RegAddr;
    WriteBuffer[1] = RegValue;

    TransactionInfo.Address = I2CADDR_RFIC;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = NV_ARRAY_SIZE(WriteBuffer);

    Error = NvOdmI2cTransaction(hI2C, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NVODMTVTUNER_PRINTF(("NvOdmMurataRFWrite8 Fail: reg 0x%x\n", RegAddr));
        return Error;
    }
    return Error;
}

static NvBool NvOdmMurataSetRF1SegTV(NvOdmServicesI2cHandle hI2C, NvU32 channel)
{
    NvU8 WriteBuffer[] = {0x0e, 0x00,
                          0x33, 0x76,
                          0x34, 0x49,
                          0x35, 0x24,
                          0x36, 0x92,
                          0x6b, 0x00,
                          0xfe};
    NvOdmI2cStatus Error;
#if 1 //less efficient, but should work
    NvU8 i;

    WriteBuffer[3] = ((channel - 13) * 3) / 2 + 0x76;
    WriteBuffer[5] = (channel & 0x01) ? 0x49 : 0xc9;
    for (i = 0; i < 12; i += 2)
    {
        Error = NvOdmMurataRFWrite8(hI2C, WriteBuffer[i], WriteBuffer[i+1]);
    }

#else //More effencient, but does not work yet

    NvOdmI2cTransactionInfo TransactionInfo;

    //First put Murata into i2c bypass mode:
    NvOdmTVI2cWrite8(hI2C, I2CADDR_DEMO, 0xfe, 0x00);

    // Magic numbers come from Murata's application note.
    WriteBuffer[3] = ((channel - 13) * 3) / 2 + 0x76;
    WriteBuffer[5] = (channel & 0x01) ? 0x49 : 0xc9;

    TransactionInfo.Address = I2CADDR_RFIC;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = NV_ARRAY_SIZE(WriteBuffer);

    Error = NvOdmI2cTransaction(hI2C, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NVODMTVTUNER_PRINTF(("NvOdmMurataSetRF1SegTV Fail: channel %d\n", channel));
        return Error;
    }
#endif
    return Error;
}

static NvBool NvOdmMurataRFRead8(NvOdmServicesI2cHandle hI2C, 
                                  NvU8 RegAddr, NvU8 *RegValue)
{
    NvU8 ReadBuffer[1];
    NvOdmI2cStatus Error;    
    NvOdmI2cTransactionInfo TransactionInfo;

    //First put Murata into i2c bypass mode:
    NvOdmTVI2cWrite8(hI2C, I2CADDR_DEMO, 0xfe, 0x00);

    //Then write register to be read to reg 0xfb
    NvOdmTVI2cWrite8(hI2C, I2CADDR_RFIC, 0xfb, RegAddr);

    //Do we need this again?
    NvOdmTVI2cWrite8(hI2C, I2CADDR_DEMO, 0xfe, 0x00);

    //read 1 byte:
    NvOdmOsMemset(ReadBuffer, 0, sizeof(ReadBuffer));  
    
    TransactionInfo.Address = (I2CADDR_RFIC | 0x1);
    TransactionInfo.Buf = ReadBuffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 1;

    // Read data from the specified offset
    Error = NvOdmI2cTransaction(hI2C, &TransactionInfo, 1, I2CSPEED,
                                        I2C_WAIT_LIMIT);
    if (Error != NvOdmI2cStatus_Success) {
        NVODMTVTUNER_PRINTF(("NvOdmMurataRFRead8: Receive Failed\n"));  
        return Error;
    }
    *RegValue = ReadBuffer[0];
    return Error;
}

static void NvOdmMurataSetLNA(NvOdmServicesI2cHandle hI2C)
{
    NvU8 gpio_dat, ifagcdac, agc_att_rf = 0;

    NvOdmTVI2cRead8(hI2C, I2CADDR_DEMO, 0xeb, &gpio_dat);
    //read sub reg 0x3a:
    NvOdmTVI2cWrite8(hI2C, I2CADDR_DEMO, 0x04, 0x3a);
    NvOdmTVI2cRead8(hI2C, I2CADDR_DEMO, 0x05, &ifagcdac);

    //read 0x3d of RF
    NvOdmMurataRFRead8(hI2C, 0x3d, &agc_att_rf);
    agc_att_rf &= 0x3f; //only bit[5:0] are valid

    if (((gpio_dat & 0x01) == 0x01) && (ifagcdac <= 0x52) && (agc_att_rf <= 0x07))
        NvOdmTVI2cWrite8(hI2C, I2CADDR_DEMO, 0xeb, gpio_dat & 0xfe);
    else if (((gpio_dat & 0x01) == 0x00) && (ifagcdac >= 0x6e) && (agc_att_rf >= 0x3f))
        NvOdmTVI2cWrite8(hI2C, I2CADDR_DEMO, 0xeb, gpio_dat | 0x01);
}

// Toggle Murata's gpio to set antenna for optimal TV reception.
// Radio antenna is always available and do not need to be 
// programmed.
static void NvOdmMurataSetAntenna(
         NvOdmDeviceHWContext *pHW,
         NvU32 channel)
{
    NvOdmISDBTSegment seg = pHW->CurrentSeg;
    NvU8 gpio_dat;

    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
            NvOdmTVI2cRead8(pHW->hI2C, I2CADDR_DEMO, 0xeb, &gpio_dat);
            gpio_dat &= 0xf9; //masked out gpio 1 and 2
            if (channel < 25)
                gpio_dat |= 0x02;
            else if ((channel >= 25) && (channel < 38))
                gpio_dat |= 0x00;
            else if ((channel >= 38) && (channel < 60))
                gpio_dat |= 0x06;
            else 
                gpio_dat |= 0x04;
            NvOdmTVI2cWrite8(pHW->hI2C, I2CADDR_DEMO, 0xeb, gpio_dat);
            break;
        default:
            break;
    }
}

/**
 * Exported functions
 */

void NvOdmMurataSetPowerLevel(
        NvOdmDeviceHWContext *pHW,
        NvOdmDtvtunerPowerLevel PowerLevel)
{
    //For GPIO
    NvOdmServicesGpioHandle hGpio;
    NvOdmGpioPinHandle hPin;
    NvU32 i;
    NvU32 VddId;
    NvOdmServicesPmuVddRailCapabilities vddrailcap;
    NvU32 settletime, settletimeMax = 0;

    hGpio = NvOdmGpioOpen();
    switch (PowerLevel) {
        case NvOdmDtvtunerPowerLevel_On:
            for( i = 0; i < pHW->pPeripheralConnectivity->NumAddress; i++)
            {
                switch(pHW->pPeripheralConnectivity->AddressList[i].Interface)
                {
                    case NvOdmIoModule_I2c:
                        break;
                    case NvOdmIoModule_Gpio:
                        // In order to power it on we need to set vgp0 and vgp5 to low
                        hPin = NvOdmGpioAcquirePinHandle(hGpio,
                            pHW->pPeripheralConnectivity->AddressList[i].Instance,
                            pHW->pPeripheralConnectivity->AddressList[i].Address);
                        NvOdmGpioConfig(hGpio, hPin, NvOdmGpioPinMode_Output);
                        NvOdmGpioSetState(hGpio, hPin, NvOdmGpioPinActiveState_Low);
                        NvOdmGpioReleasePinHandle(hGpio, hPin);
                        break;
                    case NvOdmIoModule_Vdd:
                        VddId = pHW->pPeripheralConnectivity->AddressList[i].Address;
                        NvOdmServicesPmuGetCapabilities(pHW->hPmu,
                                                        VddId,
                                                        &vddrailcap);
                        NvOdmServicesPmuSetVoltage(pHW->hPmu, VddId,
                                                   vddrailcap.requestMilliVolts,
                                                   &settletime);
                        if (settletime > settletimeMax)
                            settletimeMax = settletime;
                        break;
                    default:
                        break;
                }
            }
            NvOdmOsWaitUS(settletimeMax);  // wait to settle VDD power

            //Access Murata needs i2c:
            if(!pHW->hI2C)
                pHW->hI2C = NvOdmI2cOpen(NvOdmIoModule_I2c, 1);
            if (!pHW->hI2C) {
                NVODMTVTUNER_PRINTF(("!! NvOdmI2cOpen fail !!\n"));
            }

            // Open second I2C (necessary for wing board REV D)
            if (!pHW->hI2CPMU)
            {
                pHW->hI2CPMU = NvOdmI2cOpen(NvOdmIoModule_I2c_Pmu, 0);
            }
            if (!pHW->hI2CPMU) 
            {
                NVODMTVTUNER_PRINTF(("!! NvOdmI2cOpen fail !!\n"));
            }
            break;
        case NvOdmDtvtunerPowerLevel_Suspend:
            break;
        case NvOdmDtvtunerPowerLevel_Off:
            for( i = 0; i < pHW->pPeripheralConnectivity->NumAddress; i++)
            {
                switch(pHW->pPeripheralConnectivity->AddressList[i].Interface)
                {
                    case NvOdmIoModule_Gpio:
                        break;
                    case NvOdmIoModule_Vdd:
                        VddId = pHW->pPeripheralConnectivity->AddressList[i].Address;
                        NvOdmServicesPmuGetCapabilities(pHW->hPmu,
                                                        VddId,
                                                        &vddrailcap);
                        NvOdmServicesPmuSetVoltage(pHW->hPmu, VddId, NVODM_VOLTAGE_OFF, &settletime);
                        if (settletime > settletimeMax)
                            settletimeMax = settletime;
                        break;
                    default:
                        break;
                }
            }
            NvOdmOsWaitUS(settletimeMax);  // wait to settle VDD power
            break;
        default:
            break;
    }

    NvOdmGpioClose(hGpio);
    return;
}

NvBool NvOdmMurataGetCap(
          NvOdmDeviceHWContext *pHW,
          NvOdmDtvtunerCap *pCap)
{
    NvOdmOsMemcpy(pCap, &MurataCap, sizeof(NvOdmDtvtunerCap));
    return NV_TRUE;
}

NvBool NvOdmMurataInit(
          NvOdmDeviceHWContext *pHW,
          NvOdmISDBTSegment seg,
          NvU32 channel)
{
    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(SoftResetRelease);

    // It is necessary to program this pin so that data coming from Murata's board
    // can be received by Concord (2.80V triggers output state
    NvOdmTVI2cWrite8(pHW->hI2CPMU, I2CADDR_PMU, LD0_CTRL2, VLD04_28V);

    //For Murata, init sequence has dependencies on segment settings:
    return NvOdmMurataSetSegmentChannel(pHW, seg, channel, 0);
}

NvBool NvOdmMurataSetSegmentChannel(
         NvOdmDeviceHWContext *pHW,
         NvOdmISDBTSegment seg,
         NvU32 channel,
         NvU32 PID)
{
    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(SynchronizationSequenceReset);
    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init1);
    
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init2TVUHF);
            break;
        case NvOdmISDBTtuner1SegRadioVHF:
        case NvOdmISDBTtuner3SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init2Radio);
            break;
        default:
            break;
    }
    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init3_4);
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init4TVUHF);
            break;
        case NvOdmISDBTtuner1SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init4Radio1seg);
            break;
        case NvOdmISDBTtuner3SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init4Radio3seg);
            break;
        default:
            break;
    }
    //Spec 11.3.5: TS [Transport stream] output setting
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init5_TS1segTV);
            break;
        case NvOdmISDBTtuner1SegRadioVHF:
        case NvOdmISDBTtuner3SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init5_TSRadio);
            break;
        default:
            break;
    }

    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init8_12);
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init13TV);
            break;
        case NvOdmISDBTtuner1SegRadioVHF:
        case NvOdmISDBTtuner3SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Init13Radio);
            break;
        default:
            break;
    }

    //Now init to a working seg/channel
    // set segment:
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
        case NvOdmISDBTtuner1SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(SegmentTV_1segRadio);
            break;
        case NvOdmISDBTtuner3SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Segment3segRadio);
            break;
        default:
            break;
    }
    pHW->CurrentSeg = seg;

    // set channel:
    NvOdmMurataSetChannel(pHW, channel, 0);

    return NV_TRUE;
}

NvBool NvOdmMurataSetChannel(
         NvOdmDeviceHWContext *pHW,
         NvU32 channel,
         NvU32 PID)
{
    NvOdmISDBTSegment seg = pHW->CurrentSeg;
    
    //Toggle Murata's gpio to set antenna for optimal reception:
    NvOdmMurataSetAntenna(pHW, channel);

    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(SynchronizationSequenceReset);

    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(SetChannel_1);
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
        case NvOdmISDBTtuner1SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(SetChannel_2_1seg);
            break;
        case NvOdmISDBTtuner3SegRadioVHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(SetChannel_2_3seg);
            break;
        default:
            break;
    }
    //sub channel:
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
            NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Sub_channel_setting_1segTV);
            break;
        case NvOdmISDBTtuner1SegRadioVHF:
        case NvOdmISDBTtuner3SegRadioVHF:
            NvOdmTVI2cWrite8(pHW->hI2C, I2CADDR_DEMO,
                             0x44, NvOdmMurataRFReg44Val(channel));
            break;
        default:
            break;
    }
    //RF-IC channel setting:

    //init RF:
    NVODM_MURATA_WRITE_RFIC_SEQUENCE(Rf_init_common);
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
            NVODM_MURATA_WRITE_RFIC_SEQUENCE(Rf_init_1segTV);
            break;
        case NvOdmISDBTtuner1SegRadioVHF:
            NVODMTVTUNER_PRINTF(("!! 1seg Radio 7CHSeg8 not implemented !!\n"));
            NVODM_MURATA_WRITE_RFIC_SEQUENCE(Rf_init_1segRadio);
            break;
        case NvOdmISDBTtuner3SegRadioVHF:
            NVODM_MURATA_WRITE_RFIC_SEQUENCE(Rf_init_3segRadio);
            break;
        default:
            break;
    }
    //power up w/o go:
    NVODM_MURATA_WRITE_RFIC_SEQUENCE(Rf_Powerup_No_Sequencer_Go);
    //channel change:
    switch(seg) {
        case NvOdmISDBTtuner1SegTVUHF:
            NvOdmMurataSetRF1SegTV(pHW->hI2C, channel);
            break;
        case NvOdmISDBTtuner1SegRadioVHF:
            NVODMTVTUNER_PRINTF(("!! 1seg Radio not implemented !!\n"));
            break;
        case NvOdmISDBTtuner3SegRadioVHF:
            NVODMTVTUNER_PRINTF(("!! 3seg Radio not implemented !!\n"));
            break;
        default:
            break;
    }
    //Power up:
    NVODM_MURATA_WRITE_RFIC_SEQUENCE(Rf_Powerup);

    //finishing up:
    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(Release_Sync_Seq_reset);
    if (seg == NvOdmISDBTtuner1SegTVUHF)
        NvOdmMurataSetLNA(pHW->hI2C);

    pHW->CurrentChannel = channel;
    return NV_TRUE;
}

NvBool NvOdmMurataGetSignalStat(
         NvOdmDeviceHWContext *pHW,
         NvOdmDtvtunerStat *SignalStat)
{
    NvU8 data = 0, data0 = 0, data1 = 0, data2 = 0;
    NvU32 data32 = 0;
    NvS32 count = 0;

#define STAT_COLLECT_TIMEMS 500

    //measure BER, before Viterbi,
    //per (188*8)*STAT_COLLECT_TIMEMS/10 = 75200 = 0x0125c0 bits:

    I2CRegData vberset[] = {
        {0x50, 0xa4}, //layer A
        {0x51, 0x01},
        {0x50, 0xa5},
        {0x51, 0x25},
        {0x50, 0xa6},
        {0x51, 0xc0},
        {0x50, 0xa7}, //layer B
        {0x51, 0x01},
        {0x50, 0xa8},
        {0x51, 0x25},
        {0x50, 0xa9},
        {0x51, 0xc0},
        {0x52, 0x01}, //Turn Viterbi bit on
        {0x53, 0x00}, //Reset BER start bit
        {0x53, 0x03}, //Release BER start bit
    };

    // This can be called anytime, but i2c is available only at run/power_on state.
    if(!pHW->hI2C)
    {
        SignalStat->SignalStrength = 0;
        SignalStat->BitErrorRate = 100000;
        return NV_FALSE;
    }

    //setup:
    NVODM_MURATA_WRITE_DEMOD_SEQUENCE(vberset);

    //wait till stat is collected:
    while ((count < 4) && (data == 0x00))
    {
        NvOdmOsSleepMS(STAT_COLLECT_TIMEMS);
        NvOdmTVI2cRead8(pHW->hI2C, I2CADDR_DEMO, 0x54, &data);
        data &= 0x03;
        count++;
    }
    //Get data out:
    if (data == 0x00)
    {
        // Timeout happens when there is no signal.
        SignalStat->SignalStrength = 0;
        SignalStat->BitErrorRate = 100000;
        return NV_FALSE;
    }
    else
    {
        // Read layer A only. No data from layer B.
        NvOdmTVI2cRead8(pHW->hI2C, I2CADDR_DEMO, 0x55, &data0);
        NvOdmTVI2cRead8(pHW->hI2C, I2CADDR_DEMO, 0x56, &data1);
        NvOdmTVI2cRead8(pHW->hI2C, I2CADDR_DEMO, 0x57, &data2);
        data32 = ((NvU32)data0 << 16) | ((NvU32)data1 << 8) | ((NvU32)data2);
        SignalStat->BitErrorRate = (data32 * 1000) / 752;

        // SignalStrength not implemented yet:
        SignalStat->SignalStrength = data32;
    }
    return NV_TRUE;
}
