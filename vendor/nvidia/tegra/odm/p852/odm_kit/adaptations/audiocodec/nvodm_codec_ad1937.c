 /**
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/*
 * NVIDIA APX ODM Kit for Wolfson 8903 Implementation
 *
 * Note: Please search "FIXME" for un-implemented part/might_be_wrong part
 */



#include "nvodm_services.h"
#include "nvodm_query.h"
#include "nvodm_audiocodec.h"
#include "audiocodec_hal.h"
#include "nvodm_query_discovery.h"

#define AD1937_CODEC_GUID NV_ODM_GUID('a','d','e','v','1','9','3','7')

/* I2C Settings */
#define AD1937_I2C_TIMEOUT   1000    // 1 seconds
#define AD1937_I2C_SCLK      100     // 400KHz

/* I2C addresses */
#define I2C_MAXIM_AUDIO_CLOCK_ADDR 0xC0
#define I2C_AUDIO_CONTROL_EXP_ADDR 0x4E

/* Values for MAX9458 CLOCK */
#define MAX9458_SAMPLING_FREQ_48K  (3<<0)
#define MAX9458_SAMPLING_FREQ_44K  (2<<0)
#define MAX9458_CLKOUT1_EN         (1<<5)
#define MAX9458_SAMPLE_RATE_DOUBLE (1<<4)

#define GET_PORT_TYPE(PortName) (PortName >> 16)
#define GET_PORT_ID(PortName) (PortName &0xFFFF)

/* Register & Bit definitions */
/*
 *  Definitions for Audio registers for AD1937
 */
#define PLL_CLK_CTRL0           0x00
#define PLL_CLK_CTRL1           0x01
#define DAC_CTRL0               0x02
#define DAC_CTRL1               0x03
#define DAC_CTRL2               0x04
#define DAC_CHN_MUTES           0x05
#define DAC1_LVOL               0x06
#define DAC1_RVOL               0x07
#define DAC2_LVOL               0x08
#define DAC2_RVOL               0x09
#define DAC3_LVOL               0x0A
#define DAC3_RVOL               0x0B
#define DAC4_LVOL               0x0C
#define DAC4_RVOL               0x0D
#define ADC_CTRL0               0x0E
#define ADC_CTRL1               0x0F
#define ADC_CTRL2               0x10

/* PLL and CLK Control 0 register definitions */
#define PWR_DOWN            (1<<0)
#define MCLKI_RATE_512X     (1<<2)
#define MCLKO_RATE_512X     (2<<3)
#define MCLKO_OFF           (3<<3)
#define XTAL_DISABLE        (3<<3)
#define DAC_ADC_EN          (1<<7)

/* PLL and CLK Control 1 register definitions */
#define DAC_MCLK_SEL    (1<<0)
#define ADC_MCLK_SEL    (1<<1)
#define VREF_DISABLE    (1<<2)
#define PLL_LOCK        (1<<3) /* Read-only */

/* DAC Control 0 register definitions */
#define DAC_PWR_DOWN        (PWR_DOWN)
#define DAC_TDM_SINGLE_LINE (1<<6)
#define DAC_TDM_AUX_MODE    (2<<6)
#define DAC_TDM_DUAL_LINE   (3<<6)
#define DAC_TDM_SINGLE_LINE (1<<6)


/* DAC Control 1 register definitions */
#define FS_POL_HIGH     (1<<3)  /* FS Polarity Active High */
#define FS_MASTER       (1<<4)
#define BCLK_MASTER     (1<<5)
#define BCLK_INTERNAL   (1<<6)
#define BCLK_POL_INVERT (1<<7) /* Inverted bclk polarity */


/* DAC Control 2 register definitions */
#define MST_MUTE        (1<<0) /* Master Mute */
#define DAC_WORD_SIZE16 (3<<3) /* 16 bit words */
#define DAC_WORD_SIZE24 (0<<3) /* 16 bit words */
#define DAC_WORD_SIZE20 (1<<3) /* 16 bit words */
#define DAC_POL_INVERT  (1<<5) /* Inverted DAC output polarity */

/* DAC Individual Channel Mutes register definitions */
#define DAC1_LMUTE  (1<<0)
#define DAC1_RMUTE  (1<<1)
#define DAC2_LMUTE  (1<<2)
#define DAC2_RMUTE  (1<<3)
#define DAC3_LMUTE  (1<<4)
#define DAC3_RMUTE  (1<<5)
#define DAC4_LMUTE  (1<<6)
#define DAC4_RMUTE  (1<<7)

/* ADC Control 0 register definitions */
#define ADC_PWR_DOWN    (PWR_DOWN)
#define HIGH_PASS_EN    (1<<1)
#define ADC1_LMUTE      (1<<2)
#define ADC1_RMUTE      (1<<3)
#define ADC2_LMUTE      (1<<4)
#define ADC2_RMUTE      (1<<5)

/* ADC Control 1 register definitions */
#define ADC_WORD_SIZE16 (3<<0)
#define ADC_WORD_SIZE24 (0<<0)

/* ADC Control 2 register definitions */
#define ADC_FS_MASTER     (1<<3)
#define ADC_BCLK_MASTER   (1<<6)
#define ADC_BCLK_INTERNAL (1<<7)

#define ADC1_LMUTE (1<<2)
#define ADC1_RMUTE (1<<3)
#define ADC2_LMUTE (1<<4)
#define ADC2_RMUTE (1<<5)
/*
 * Wolfson codec register sequences.
 */
typedef NvU32 WCodecRegIndex;

/*
 * The codec control variables.
 */
typedef struct
{

    NvU32 HPOutVolLeft;
    NvU32 HPOutVolRight;

    NvU32 AdcSamplingRate;
    NvU32 DacSamplingRate;


    NvU32 LineInVolLeft;
    NvU32 LineInVolRight;
} WolfsonCodecControls;

typedef struct AD1937AudioCodecRec
{
    NvOdmServicesI2cHandle hOdmService;
    WolfsonCodecControls WCodecControl;
    NvOdmQueryI2sACodecInterfaceProp WCodecInterface;
    NvU32   RegValues[173];
} AD1937AudioCodec, *AD1937AudioCodecHandle;

static AD1937AudioCodec s_AD1937 = {0};       /* Unique audio codec for the whole system*/

static void DumpAD1937(AD1937AudioCodecHandle hOdmAudioCodec);
static NvBool InitializeDigitalInterface(NvOdmPrivAudioCodecHandle hOdmAudioCodec);
static NvBool InitializeAnalogAudioPath(NvOdmPrivAudioCodecHandle hOdmAudioCodec);
static NvBool EnableInput( AD1937AudioCodecHandle hOdmAudioCodec, NvU32  IsMic, NvBool IsPowerOn);

static NvBool
ReadAD1937Register(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    WCodecRegIndex RegIndex,
    NvU32 *Data)
{
    NvOdmI2cStatus I2cTransStatus = NvOdmI2cStatus_Timeout;
    NvOdmI2cTransactionInfo TransactionInfo[2];
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;
    NvU8 ReadBuffer[2] = {0};

    ReadBuffer[0] = (RegIndex & 0xFF);

    TransactionInfo[0].Address = hAudioCodec->WCodecInterface.DeviceAddress;
    TransactionInfo[0].Buf = ReadBuffer;
    TransactionInfo[0].Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo[0].NumBytes = 1;
    TransactionInfo[1].Address = (hAudioCodec->WCodecInterface.DeviceAddress | 0x1);
    TransactionInfo[1].Buf = ReadBuffer;
    TransactionInfo[1].Flags = 0;
    TransactionInfo[1].NumBytes = 1;

    // Read data from PMU at the specified offset
    I2cTransStatus = NvOdmI2cTransaction(hAudioCodec->hOdmService, &TransactionInfo[0], 2,
            AD1937_I2C_SCLK, AD1937_I2C_TIMEOUT);

    *Data = ReadBuffer[0];
    if (I2cTransStatus == 0)
        NvOdmOsDebugPrintf("Read 0X%02X = 0X%04X\n", RegIndex, *Data);
    else
        NvOdmOsDebugPrintf("0x%02x Read Error: %08x\n", RegIndex, I2cTransStatus);
    return (NvBool)(I2cTransStatus == 0);
}

static NvBool
WriteI2CPeripheral(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Addr,
    NvU32 Data)
{
    NvOdmI2cStatus I2cTransStatus;
    NvOdmI2cTransactionInfo TransactionInfo;
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;
    NvU8    Buffer[3] = {0};

    /* Pack the data, p70 */
    Buffer[0] =  Data & 0xFF;

    TransactionInfo.Address = Addr;
    TransactionInfo.Buf = (NvU8*)Buffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 1;
    // write the Accelator offset (from where data is to be read)
    I2cTransStatus = NvOdmI2cTransaction(hAudioCodec->hOdmService, &TransactionInfo, 1, AD1937_I2C_SCLK,
                    AD1937_I2C_TIMEOUT);
    // HW- BUG!! If timeout, again retransmit the data.
    if (I2cTransStatus == NvOdmI2cStatus_Timeout)
        I2cTransStatus = NvOdmI2cTransaction(hAudioCodec->hOdmService, &TransactionInfo, 1, AD1937_I2C_SCLK,
                        AD1937_I2C_TIMEOUT);

    if (I2cTransStatus)
        NvOdmOsDebugPrintf("\t --- Failed(0x%08x)\n", I2cTransStatus);
    else
        NvOdmOsDebugPrintf("Write 0X%02X = 0X%04X", 0, Data);

    return (NvBool)(I2cTransStatus == 0);
}

static NvBool
ReadI2CPeripheral(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Addr,
    NvU32 *Data)
{
    NvOdmI2cStatus I2cTransStatus;
    NvOdmI2cTransactionInfo TransactionInfo;
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;
    NvU8    Buffer[3] = {0};

    /* Pack the data, p70 */

    TransactionInfo.Address = Addr;
    TransactionInfo.Buf = (NvU8*)Buffer;
    TransactionInfo.Flags = 0;
    TransactionInfo.NumBytes = 1;
    // write the Accelator offset (from where data is to be read)
    I2cTransStatus = NvOdmI2cTransaction(hAudioCodec->hOdmService, &TransactionInfo, 1, AD1937_I2C_SCLK,
                    AD1937_I2C_TIMEOUT);
    if (I2cTransStatus)
        NvOdmOsDebugPrintf("\t --- Failed(0x%08x)\n", I2cTransStatus);
    else
        NvOdmOsDebugPrintf("Write 0X%02X = 0X%04X", 0, Data);

    *Data = Buffer[0] & 0xFF;
    return (NvBool)(I2cTransStatus == 0);
}


static NvBool
WriteAD1937Register(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    WCodecRegIndex RegIndex,
    NvU32 Data)
{
    NvOdmI2cStatus I2cTransStatus;
    NvOdmI2cTransactionInfo TransactionInfo;
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;
    NvU8    Buffer[3] = {0};


    /* Pack the data, p70 */
    Buffer[0] =  RegIndex & 0xFF;
    Buffer[1] =  Data     & 0xFF;

    TransactionInfo.Address = hAudioCodec->WCodecInterface.DeviceAddress;
    TransactionInfo.Buf = (NvU8*)Buffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;
    // write the Accelator offset (from where data is to be read)
    I2cTransStatus = NvOdmI2cTransaction(hAudioCodec->hOdmService, &TransactionInfo, 1, AD1937_I2C_SCLK,
                    AD1937_I2C_TIMEOUT);

    // HW- BUG!! If timeout, again retransmit the data.
    if (I2cTransStatus == NvOdmI2cStatus_Timeout)
        I2cTransStatus = NvOdmI2cTransaction(hAudioCodec->hOdmService, &TransactionInfo, 1, AD1937_I2C_SCLK,
                        AD1937_I2C_TIMEOUT);

    if (I2cTransStatus)
        NvOdmOsDebugPrintf("\t --- Failed(0x%08x)\n", I2cTransStatus);
    else
        NvOdmOsDebugPrintf("Write 0X%02X = 0X%04X", RegIndex, Data);

    return (NvBool)(I2cTransStatus == 0);
}

static NvBool
AD1937_ReadRegister(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    WCodecRegIndex RegIndex,
    NvU32 *Data)
{
    return ReadAD1937Register(hOdmAudioCodec, RegIndex, Data);
}

static NvBool
AD1937_WriteRegister(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    WCodecRegIndex RegIndex,
    NvU32 Data)
{
    return WriteAD1937Register(hOdmAudioCodec, RegIndex, Data);
}


static NvBool
SetHeadphoneOutVolume(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Channel,
    NvU32 LeftVolume,
    NvU32 RightVolume)
{
    NvBool IsSuccess = NV_TRUE;
    NvU32 LeftVolReg;
    NvU32 RightVolReg;
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;

    if (Channel & NvOdmAudioSpeakerType_FrontLeft)
        LeftVolReg = 255 - LeftVolume;
    else
        LeftVolReg = (hAudioCodec->WCodecControl.HPOutVolLeft);

    if (Channel & NvOdmAudioSpeakerType_FrontRight)
        RightVolReg = 255 - RightVolume;
    else
        RightVolReg = (hAudioCodec->WCodecControl.HPOutVolRight);

    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC1_LVOL, LeftVolReg);
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC1_RVOL, RightVolReg);

    hAudioCodec->WCodecControl.HPOutVolLeft  = LeftVolReg;
    hAudioCodec->WCodecControl.HPOutVolRight = RightVolReg;

    return IsSuccess;
}

static NvBool
SetLineInVolume(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Channel,
    NvU32 LeftVolume,
    NvU32 RightVolume)
{
    /* There are no registers to support this */
    return NV_TRUE;
}


static NvBool
SetHeadphoneOutMute(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Channel,
    NvBool IsMute)
{
    NvU32 dacCtrl2Reg;
    NvBool IsSuccess = NV_TRUE;
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;

    if (IsMute)
    {
        IsSuccess &= AD1937_ReadRegister(hOdmAudioCodec, DAC_CTRL2, &dacCtrl2Reg);
         dacCtrl2Reg |= MST_MUTE;
        IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC_CTRL2, dacCtrl2Reg);

    }
    else
    {
        IsSuccess &= AD1937_ReadRegister(hOdmAudioCodec, DAC_CTRL2, &dacCtrl2Reg);
         dacCtrl2Reg &= ~MST_MUTE;
        IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC_CTRL2, dacCtrl2Reg);

    }
    return IsSuccess;
}


static NvBool
SetLineInMute(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 Channel,
    NvBool IsMute)
{
    NvU32 LeftVol;
    NvU32 RightVol;
    NvBool IsSuccess = NV_TRUE;
    NvU32 adcCtrlReg;

    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;
    if (IsMute)
    {
        IsSuccess &= AD1937_ReadRegister(hOdmAudioCodec, ADC_CTRL0, &adcCtrlReg);
        adcCtrlReg |= ADC1_LMUTE;
        adcCtrlReg |= ADC1_RMUTE;
        adcCtrlReg |= ADC2_LMUTE;
        adcCtrlReg |= ADC2_RMUTE;
        IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, ADC_CTRL0, adcCtrlReg );
    }
    else
    {
        IsSuccess &= AD1937_ReadRegister(hOdmAudioCodec, ADC_CTRL0, &adcCtrlReg);
        adcCtrlReg &= ~ADC1_LMUTE;
        adcCtrlReg &= ~ADC1_RMUTE;
        adcCtrlReg &= ~ADC2_LMUTE;
        adcCtrlReg &= ~ADC2_RMUTE;
        IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, ADC_CTRL0, adcCtrlReg );
    }

    return IsSuccess;
}

static NvBool SetMicrophoneInVolume(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvU32 MicGain)
{
    NvBool IsSuccess = NV_TRUE;
    /* No Registers to support this */
    return IsSuccess;
}

static NvBool SetMicrophoneInMute(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvBool IsMute)
{
    /* FIXME: Disable Mic Input path */
    return NV_TRUE;
}

/*
 * Shuts down the audio codec driver.
 */
static NvBool ShutDownCodec(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    /* Quick Shutdown */
    NvBool IsSuccess=NV_TRUE;

    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, ADC_CTRL0, ADC_PWR_DOWN );
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC_CTRL0, DAC_PWR_DOWN );
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, PLL_CLK_CTRL0, PWR_DOWN );

    return IsSuccess;

}



static NvBool ResetCodec(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    /* Reset 8903 */
    NvBool IsSuccess=NV_TRUE;
    IsSuccess &= ShutDownCodec(hOdmAudioCodec);
    IsSuccess &= InitializeDigitalInterface(hOdmAudioCodec);
    IsSuccess &= InitializeAnalogAudioPath(hOdmAudioCodec);
    return IsSuccess;
}

static NvBool SetCodecPower(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvBool IsPowerOn)
{
    NvBool IsSuccess = NV_TRUE;
    NvU32 data;
    if (IsPowerOn) {
        /* Enable MCLK from MAX9485 Chip */
        IsSuccess &= WriteI2CPeripheral(hOdmAudioCodec, I2C_MAXIM_AUDIO_CLOCK_ADDR, (MAX9458_SAMPLING_FREQ_48K | MAX9458_CLKOUT1_EN | MAX9458_SAMPLE_RATE_DOUBLE));
        /* IO Expander -> route TEST to DAP1 */
        IsSuccess &= WriteI2CPeripheral(hOdmAudioCodec, I2C_AUDIO_CONTROL_EXP_ADDR , 0x2d ); /* Route to DAP1 */
        NvOdmOsSleepMS(800);    /* Wait 800ms */
    }
    else {
        IsSuccess &= ShutDownCodec(hOdmAudioCodec);
    }
    return IsSuccess;
}


static NvBool InitializeDigitalInterface(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    NvBool IsSuccess = NV_TRUE;
    NvU32 AudioIF1Reg = 0;
    NvU32 SampleContReg = 0;
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;

    /* Enable DAC and ADC */
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, PLL_CLK_CTRL0, DAC_ADC_EN | MCLKI_RATE_512X | MCLKO_RATE_512X );
    /* DAC & ADC reference clk to MCLK */
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, PLL_CLK_CTRL1, (DAC_MCLK_SEL | ADC_MCLK_SEL | VREF_DISABLE));
    /* DAC */
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC_CTRL0, 0 ); //I2S Stereo Mode
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC_CTRL1, FS_MASTER | BCLK_MASTER | BCLK_INTERNAL);
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC_CTRL2, DAC_WORD_SIZE16 );

       return IsSuccess;
}

static NvBool InitializeHeadphoneOut(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    NvBool IsSuccess = NV_TRUE;

    IsSuccess &= SetHeadphoneOutVolume(hOdmAudioCodec,
                (NvOdmAudioSpeakerType_FrontLeft | NvOdmAudioSpeakerType_FrontRight),
                255, 255);

    return IsSuccess;
}


static NvBool InitializeAnalogAudioPath(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    NvU32 DacCtrlReg = 0;
    NvU32 MixOutCtrlDacL = 0;
    NvU32 MixOutCtrlDacR = 0;
    NvU32 MixOutCtrlDacSpk = 0;
    NvU32 MixOutReg = 0;
    NvU32 MixSpkReg = 0;
    NvBool IsSuccess = NV_TRUE;

    /* ADC */
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, ADC_CTRL0, 0x00 );
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, ADC_CTRL1, ADC_WORD_SIZE16 );
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, ADC_CTRL2, 0x00 );

    return IsSuccess;
}

static NvBool SetDacMute(NvOdmPrivAudioCodecHandle hOdmAudioCodec, NvBool IsMute)
{
    return SetHeadphoneOutMute(hOdmAudioCodec,0,IsMute);
}

/*
 * Sets the PCM size for the audio data.
 */
static NvBool
SetPcmSize(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortType PortType,
    NvU32 PcmSize)
{
    NvU32 dacCtrl2;
    NvBool IsSuccess = NV_TRUE;

    IsSuccess &= AD1937_ReadRegister(hOdmAudioCodec,DAC_CTRL2,&dacCtrl2);

    switch(PcmSize)
    {
        case 16 : dacCtrl2 |= DAC_WORD_SIZE16;break;
        case 24 : dacCtrl2 |= DAC_WORD_SIZE24;break;
        case 20 : dacCtrl2 |= DAC_WORD_SIZE20;break;
        default:
            break;
    }
    IsSuccess &= AD1937_WriteRegister(hOdmAudioCodec, DAC_CTRL2, dacCtrl2 );
    return IsSuccess;
}

/*
 * Sets the sampling rate for the audio playback and record path.
 */
static NvBool
SetSamplingRate(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortType PortType,
    NvU32 SamplingRate)
{
    NvBool IsSuccess = NV_TRUE;
    NvU32 SamplingContReg = 0;
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;

    ReadI2CPeripheral(hOdmAudioCodec, I2C_MAXIM_AUDIO_CLOCK_ADDR,&SamplingContReg);
    switch (SamplingRate)
    {
        case 8000:
            return NV_FALSE;

        case 32000:
            return NV_FALSE;

        case 44100:
            if(SamplingContReg & 3 == MAX9458_SAMPLING_FREQ_44K )
                return NV_TRUE;
            SamplingContReg &= ~3;
            SamplingContReg |= MAX9458_SAMPLING_FREQ_44K;
            IsSuccess &= WriteI2CPeripheral(hOdmAudioCodec, I2C_MAXIM_AUDIO_CLOCK_ADDR,SamplingContReg);
            break;

        case 48000:
            if(SamplingContReg & 3 == MAX9458_SAMPLING_FREQ_48K )
                return NV_TRUE;
            SamplingContReg &= ~3;
            SamplingContReg |= MAX9458_SAMPLING_FREQ_48K;
            IsSuccess &= WriteI2CPeripheral(hOdmAudioCodec, I2C_MAXIM_AUDIO_CLOCK_ADDR,SamplingContReg);
            break;

        case 88200:
            return NV_FALSE;

        case 96000:
            return NV_FALSE;

        default:
            return NV_FALSE;
    }

    if (IsSuccess)
    {
        hAudioCodec->WCodecControl.AdcSamplingRate = SamplingRate;
        hAudioCodec->WCodecControl.DacSamplingRate = SamplingRate;
    }
    return IsSuccess;
}

/*
 * Sets the input analog line to the input to ADC.
 */
static NvBool
SetInputPortIO(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 PortId,
    NvOdmAudioConfigInOutSignal *pConfigIO)
{
    NvBool IsSuccess = NV_TRUE;

    if (PortId != 0)
        return NV_FALSE;

    if ((pConfigIO->InSignalId != 0) || (pConfigIO->OutSignalId != 0))
        return NV_FALSE;

    if ((pConfigIO->InSignalType != NvOdmAudioSignalType_LineIn)
         && (pConfigIO->InSignalType != NvOdmAudioSignalType_MicIn))
        return NV_FALSE;

    if (pConfigIO->InSignalType & NvOdmAudioSignalType_LineIn)
    {
        IsSuccess &= EnableInput(hOdmAudioCodec, 0, pConfigIO->IsEnable);
    }

    if (pConfigIO->InSignalType & NvOdmAudioSignalType_MicIn)
    {
        IsSuccess &= EnableInput(hOdmAudioCodec, 1, pConfigIO->IsEnable);
    }

    return InitializeDigitalInterface(hOdmAudioCodec);;
}

/*
 * Sets the output analog line from the DAC.
 */
static NvBool
SetOutputPortIO(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 PortId,
    NvOdmAudioConfigInOutSignal *pConfigIO)
{
    if (PortId != 0)
        return NV_FALSE;

    if ((pConfigIO->InSignalId != 0) || (pConfigIO->OutSignalId != 0))
        return NV_FALSE;

    if (pConfigIO->InSignalType != NvOdmAudioSignalType_Digital_I2s)
        return NV_FALSE;

    if ((pConfigIO->InSignalType != NvOdmAudioSignalType_LineOut)
         && (pConfigIO->InSignalType != NvOdmAudioSignalType_HeadphoneOut))
        return NV_FALSE;

    return InitializeAnalogAudioPath(hOdmAudioCodec);
}

/*
 * Sets the codec bypass.
 */
static NvBool
SetBypass(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioSignalType InputAnalogLineType,
    NvU32 InputLineId,
    NvOdmAudioSignalType OutputAnalogLineType,
    NvU32 OutputLineId,
    NvU32 IsEnable)
{
    NvBool IsSuccess = NV_TRUE;
    NvU32 CtrlReg = 0;

    /* Do Codec Bypass */
    return NV_TRUE;
}


/*
 * Sets the side attenuation.
 */
static NvBool
SetSideToneAttenuation(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvU32 SideToneAtenuation,
    NvBool IsEnabled)
{
    return NV_TRUE;
}

/*
 * Sets the power status of the specified devices of the audio codec.
 */
static NvBool
SetPowerStatus(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioSignalType SignalType,
    NvU32 SignalId,
    NvOdmAudioSpeakerType Channel,
    NvBool IsPowerOn)
{
    return NV_TRUE;
}

static NvBool OpenAD1937Codec(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    NvBool IsSuccess = NV_TRUE;

    //DumpAD1937(hOdmAudioCodec);

    if (IsSuccess)
        IsSuccess = SetCodecPower(hOdmAudioCodec, NV_TRUE);

    if (IsSuccess)
        IsSuccess = InitializeDigitalInterface(hOdmAudioCodec);

    if (IsSuccess)
        IsSuccess = InitializeAnalogAudioPath(hOdmAudioCodec);

    if (IsSuccess)
        IsSuccess = SetMicrophoneInVolume(hOdmAudioCodec,1);

    if (IsSuccess)
        IsSuccess = InitializeHeadphoneOut(hOdmAudioCodec);

    if (IsSuccess)
        IsSuccess = SetDacMute(hOdmAudioCodec, NV_FALSE);

    if (IsSuccess)
        IsSuccess = SetPowerStatus(hOdmAudioCodec, NvOdmAudioSignalType_HeadphoneOut, 0,
            (NvOdmAudioSpeakerType_FrontLeft | NvOdmAudioSpeakerType_FrontRight),
                NV_TRUE);

    //DumpAD1937(hOdmAudioCodec);

    if (!IsSuccess)
        ShutDownCodec(hOdmAudioCodec);


    return IsSuccess;
}

static const NvOdmAudioPortCaps * ACAD1937GetPortCaps(void)
{
    static const NvOdmAudioPortCaps s_PortCaps =
        {
            1,    // MaxNumberOfInputPort;
            1     // MaxNumberOfOutputPort;
        };
    return &s_PortCaps;
}


static const NvOdmAudioOutputPortCaps *
ACAD1937GetOutputPortCaps(
    NvU32 OutputPortId)
{
    static const NvOdmAudioOutputPortCaps s_OutputPortCaps =
            {
                1,          // MaxNumberOfDigitalInput;
                1,          // MaxNumberOfLineOut;
                1,          // MaxNumberOfHeadphoneOut;
                2,          // MaxNumberOfSpeakers;
                NV_FALSE,   // IsShortCircuitCurrLimitSupported;
                NV_FALSE,   // IsNonLinerVolumeSupported;
                0,          // MaxVolumeInMilliBel;
                0,          // MinVolumeInMilliBel;
            };

    if (OutputPortId == 0)
        return &s_OutputPortCaps;
    return NULL;
}


static const NvOdmAudioInputPortCaps *
ACAD1937GetInputPortCaps(
    NvU32 InputPortId)
{
    static const NvOdmAudioInputPortCaps s_InputPortCaps =
            {
                3,          // MaxNumberOfLineIn;
                1,          // MaxNumberOfMicIn;
                0,          // MaxNumberOfCdIn;
                0,          // MaxNumberOfVideoIn;
                0,          // MaxNumberOfMonoInput;
                1,          // MaxNumberOfOutput;
                NV_FALSE,   // IsSideToneAttSupported;
                NV_FALSE,   // IsNonLinerGainSupported;
                0,          // MaxVolumeInMilliBel;
                0           // MinVolumeInMilliBel;
            };

    if (InputPortId == 0)
        return &s_InputPortCaps;
    return NULL;
}

static void DumpAD1937(AD1937AudioCodecHandle hOdmAudioCodec)
{
    int i = 0;
    NvU32 Data = 0;
    for (i=0; i<17; i++)
    {
        Data = 0;
        AD1937_ReadRegister(hOdmAudioCodec, i, &Data);
        NvOsDebugPrintf("Reg#%d Value=%x \n",i,Data);
        hOdmAudioCodec->RegValues[i] = Data;
    }
}


static NvBool ACAD1937Open(NvOdmPrivAudioCodecHandle hOdmAudioCodec,
                        const NvOdmQueryI2sACodecInterfaceProp *pI2sCodecInt,
                        void* hCodecNotifySem )
{
    const NvOdmPeripheralConnectivity *pPerConnectivity = NULL;
    NvU32 Index;
    NvU32 I2CInstance =0, Interface = 0;
    AD1937AudioCodecHandle hAD1937 = (AD1937AudioCodecHandle)hOdmAudioCodec;

    hAD1937->hOdmService = NULL;

    // Codec interface paramater
    pPerConnectivity = NvOdmPeripheralGetGuid(AD1937_CODEC_GUID);
    if (pPerConnectivity == NULL)
        goto ErrorExit;

    // Search for the Io interfacing module
    for (Index = 0; Index < pPerConnectivity->NumAddress; ++Index)
    {
        if (pPerConnectivity->AddressList[Index].Interface == NvOdmIoModule_I2c)    /* FIXME: change to normal I2C */
        {
            break;
        }
    }

    // If IO module is not found then return fail.
    if (Index == pPerConnectivity->NumAddress)
        goto ErrorExit;

    I2CInstance = pPerConnectivity->AddressList[Index].Instance;
    Interface  = pPerConnectivity->AddressList[Index].Interface;

    hAD1937->WCodecInterface.DeviceAddress = pPerConnectivity->AddressList[Index].Address;
    hAD1937->WCodecInterface.IsCodecMasterMode = pI2sCodecInt->IsCodecMasterMode;
    hAD1937->WCodecInterface.IsUsbMode = pI2sCodecInt->IsUsbMode;
    hAD1937->WCodecInterface.I2sCodecDataCommFormat = pI2sCodecInt->I2sCodecDataCommFormat;
    hAD1937->WCodecInterface.I2sCodecLRLineControl = pI2sCodecInt->I2sCodecLRLineControl;


    // Codec control parameter
    hAD1937->WCodecControl.LineInVolLeft = 0;
    hAD1937->WCodecControl.LineInVolRight = 0;
    hAD1937->WCodecControl.HPOutVolLeft = 0;
    hAD1937->WCodecControl.HPOutVolRight = 0;
    hAD1937->WCodecControl.AdcSamplingRate = 0;
    hAD1937->WCodecControl.DacSamplingRate = 0;

    // Opening the I2C ODM Service
    hAD1937->hOdmService = NvOdmI2cOpen(NvOdmIoModule_I2c,I2CInstance);


    if (!hAD1937->hOdmService) {
        goto ErrorExit;
    }

    if (OpenAD1937Codec(hAD1937))
        return NV_TRUE;

ErrorExit:
    NvOdmI2cClose(hAD1937->hOdmService);
    hAD1937->hOdmService = NULL;
    return NV_FALSE;
}

static void ACAD1937Close(NvOdmPrivAudioCodecHandle hOdmAudioCodec)
{
    AD1937AudioCodecHandle hAudioCodec = (AD1937AudioCodecHandle)hOdmAudioCodec;
    if (hOdmAudioCodec != NULL)
    {
        ShutDownCodec(hOdmAudioCodec);
        NvOdmI2cClose(hAudioCodec->hOdmService);
    }
}


static const NvOdmAudioWave *
ACAD1937GetPortPcmCaps(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvU32 *pValidListCount)
{
    static const NvOdmAudioWave s_AudioPcmProps[] =
    {
        // NumberOfChannels;
        // IsSignedData;
        // IsLittleEndian;
        // IsInterleaved;
        // NumberOfBitsPerSample;
        // SamplingRateInHz;
        // DatabitsPerLRCLK
        // DataFormat
        // NvOdmAudioWaveFormat FormatType;
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 16, 8000, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 20, 32000, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 24, 44100, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 32, 48000, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 32, 88200, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
        {2, NV_FALSE, NV_FALSE, NV_FALSE, 32, 96000, 32, NvOdmQueryI2sDataCommFormat_I2S, NvOdmAudioWaveFormat_Pcm},
    };

    *pValidListCount = sizeof(s_AudioPcmProps)/sizeof(s_AudioPcmProps[0]);
    return &s_AudioPcmProps[0];
}

static NvBool
ACAD1937SetPortPcmProps(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioWave *pWaveParams)
{
    NvBool IsSuccess;
    NvOdmAudioPortType PortType;
    NvU32 PortId;

    PortType = GET_PORT_TYPE(PortName);
    PortId = GET_PORT_ID(PortName);
    if (PortId != 0)
        return NV_FALSE;

    if ((PortType == NvOdmAudioPortType_Input) ||
        (PortType == NvOdmAudioPortType_Output))
    {

        IsSuccess = SetPcmSize(hOdmAudioCodec, PortType, pWaveParams->NumberOfBitsPerSample);
        if (IsSuccess)
            IsSuccess = SetSamplingRate(hOdmAudioCodec, PortType, pWaveParams->SamplingRateInHz);
        return IsSuccess;
    }

    return NV_FALSE;
}

static void
ACAD1937SetVolume(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioVolume *pVolume,
    NvU32 ListCount)
{
    NvU32 PortId;
    NvU32 Index;

    PortId = GET_PORT_ID(PortName);
    if (PortId != 0)
        return;

    for (Index = 0; Index < ListCount; ++Index)
    {
        if (pVolume[Index].SignalType ==  NvOdmAudioSignalType_HeadphoneOut)
        {
            (void)SetHeadphoneOutVolume(hOdmAudioCodec, pVolume[Index].SignalId,
                        pVolume[Index].VolumeLevel, pVolume[Index].VolumeLevel);
        }
        else if (pVolume[Index].SignalType ==  NvOdmAudioSignalType_LineIn)
        {
            (void)SetLineInVolume(hOdmAudioCodec, pVolume[Index].SignalId,
                        pVolume[Index].VolumeLevel, pVolume[Index].VolumeLevel);
        }
        else if (pVolume[Index].SignalType ==  NvOdmAudioSignalType_MicIn)
        {
            (void)SetMicrophoneInVolume(hOdmAudioCodec, pVolume[Index].VolumeLevel);
        }
    }
}


static void
ACAD1937SetMuteControl(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioMuteData *pMuteParam,
    NvU32 ListCount)
{
    NvU32 PortId;
    NvU32 Index;

    PortId = GET_PORT_ID(PortName);
    if (PortId != 0)
        return;

    for (Index = 0; Index < ListCount; ++Index)
    {
        if (pMuteParam[Index].SignalType ==  NvOdmAudioSignalType_HeadphoneOut)
        {
            (void)SetHeadphoneOutMute(hOdmAudioCodec, pMuteParam[Index].SignalId,
                        pMuteParam[Index].IsMute);
        }
        else if (pMuteParam[Index].SignalType ==  NvOdmAudioSignalType_LineIn)
        {
            (void)SetLineInMute(hOdmAudioCodec, pMuteParam[Index].SignalId,
                        pMuteParam[Index].IsMute);
        }
        else if (pMuteParam[Index].SignalType ==  NvOdmAudioSignalType_MicIn)
        {
            (void)SetMicrophoneInMute(hOdmAudioCodec, pMuteParam[Index].IsMute);
        }
    }
}


static NvBool
ACAD1937SetConfiguration(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioConfigure ConfigType,
    void *pConfigData)
{
    NvBool IsSuccess = NV_TRUE;
    NvOdmAudioPortType PortType;
    NvU32 PortId;
    NvOdmAudioConfigBypass *pBypassConfig = pConfigData;
    NvOdmAudioConfigSideToneAttn *pSideToneAttn = pConfigData;

    PortType = GET_PORT_TYPE(PortName);
    PortId = GET_PORT_ID(PortName);
    if (PortId != 0)
        return NV_FALSE;

    if (ConfigType == NvOdmAudioConfigure_Pcm)
    {
        return ACAD1937SetPortPcmProps(hOdmAudioCodec, PortName, pConfigData);
    }

    if (ConfigType == NvOdmAudioConfigure_InOutSignal)
    {
        if (PortType & NvOdmAudioPortType_Input)
        {
            IsSuccess = SetInputPortIO(hOdmAudioCodec, PortId, pConfigData);
            return IsSuccess;
        }

        if (PortType & NvOdmAudioPortType_Output)
        {
            IsSuccess = SetOutputPortIO(hOdmAudioCodec, PortId, pConfigData);
            return IsSuccess;
        }
        return NV_TRUE;
    }

    if (ConfigType == NvOdmAudioConfigure_ByPass)
    {
        IsSuccess = SetBypass(hOdmAudioCodec, pBypassConfig->InSignalType,
            pBypassConfig->InSignalId, pBypassConfig->OutSignalType,
            pBypassConfig->OutSignalId, pBypassConfig->IsEnable);
        return IsSuccess;
    }

    if (ConfigType == NvOdmAudioConfigure_SideToneAtt)
    {
        IsSuccess = SetSideToneAttenuation(hOdmAudioCodec, pSideToneAttn->SideToneAttnValue,
                            pSideToneAttn->IsEnable);
        return IsSuccess;
    }
    return NV_TRUE;
}

static void
ACAD1937GetConfiguration(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioConfigure ConfigType,
    void *pConfigData)
{
}

static NvBool EnableInput(
        AD1937AudioCodecHandle hOdmAudioCodec,
        NvU32  IsMic,  /* 0 for LineIn, 1 for Mic */
        NvBool IsPowerOn)
{
    NvBool IsSuccess = NV_TRUE;
    NvU32  CtrlReg = 0;
    /* What else needs to be done to enable?, Figureout and add when necessary*/
    return IsSuccess;
}

static void
ACAD1937SetPowerState(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioPortName PortName,
    NvOdmAudioSignalType SignalType,
    NvU32 SignalId,
    NvBool IsPowerOn)
{
    //NvOsDebugPrintf("Set Power State %d \n",IsPowerOn);
}

static void
ACAD1937SetPowerMode(
    NvOdmPrivAudioCodecHandle hOdmAudioCodec,
    NvOdmAudioCodecPowerMode PowerMode)
{
    //NvOsDebugPrintf("Set Power Mode %d \n",PowerMode);
}

void AD1937InitCodecInterface(NvOdmAudioCodec *pCodecInterface)
{
    pCodecInterface->pfnGetPortCaps = ACAD1937GetPortCaps;
    pCodecInterface->pfnGetOutputPortCaps = ACAD1937GetOutputPortCaps;
    pCodecInterface->pfnGetInputPortCaps = ACAD1937GetInputPortCaps;
    pCodecInterface->pfnGetPortPcmCaps = ACAD1937GetPortPcmCaps;
    pCodecInterface->pfnOpen = ACAD1937Open;
    pCodecInterface->pfnClose = ACAD1937Close;
    pCodecInterface->pfnSetVolume = ACAD1937SetVolume;
    pCodecInterface->pfnSetMuteControl = ACAD1937SetMuteControl;
    pCodecInterface->pfnSetConfiguration = ACAD1937SetConfiguration;
    pCodecInterface->pfnGetConfiguration = ACAD1937GetConfiguration;
    pCodecInterface->pfnSetPowerState = ACAD1937SetPowerState;
    pCodecInterface->pfnSetPowerMode = ACAD1937SetPowerMode;

    pCodecInterface->hCodecPrivate = &s_AD1937;
    pCodecInterface->IsInited = NV_TRUE;
}

