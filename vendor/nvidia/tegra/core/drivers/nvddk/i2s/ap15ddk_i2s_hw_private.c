/*
 * Copyright (c) 2007-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>nVIDIA Driver Development Kit:
 *           Private functions for the i2s hw access</b>
 *
 * @b Description:  Implements  the private interfacing functions for the i2s
 * hw interface.
 *
 */

#include "nvrm_drf.h"
#include "nvddk_i2s_ac97_hw_private.h"
#include "ap20/ari2s_ppi.h"
#include "nvrm_hardware_access.h"
#include "nvddk_i2s.h"
#include "nvassert.h"

/********* Fifo1 is transmitter **************/

/**
 * Initialize the i2s register.
 */
void NvDdkPrivI2sHwInitialize(NvU32 I2sChannelId, I2sAc97HwRegisters *pI2sHwRegs)
{
    // Initialize the member of I2s hw register structure.
    pI2sHwRegs->IsI2sChannel = NV_TRUE;
    pI2sHwRegs->pRegsVirtBaseAdd = NULL;
    pI2sHwRegs->ChannelId = I2sChannelId;
    pI2sHwRegs->RegsPhyBaseAdd = 0;
    pI2sHwRegs->BankSize = 0;
    pI2sHwRegs->TxFifoAddress = I2S_I2S_FIFO1_0;
    pI2sHwRegs->RxFifoAddress = I2S_I2S_FIFO2_0;

#if !NV_IS_AVP
    pI2sHwRegs->TxFifo2Address = 0;
    pI2sHwRegs->RxFifo2Address = 0;
#endif //!NV_IS_AVP
}

/**
 * Enable/Disable the data flow.
 */
void
NvDdkPrivI2sSetDataFlow(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction Direction,
    NvBool IsEnable)
{
    NvU32 I2sControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL);

    NVDDK_I2S_CONTLOG(("NvDdkPrivI2sSetDataFlow I2sControl 0x%x \n", I2sControl));

    if (Direction & I2sAc97Direction_Transmit)
    {
        if (IsEnable)
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO1_TRANSACTION_EN,
                            ENABLE, I2sControl);
        else
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO1_TRANSACTION_EN,
                            DISABLE, I2sControl);
    }
    if (Direction & I2sAc97Direction_Receive)
    {
        if (IsEnable)
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO2_TRANSACTION_EN,
                                ENABLE, I2sControl);
        else
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO2_TRANSACTION_EN,
                                DISABLE, I2sControl);
    }
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL, I2sControl);
}

/**
 * Set the the trigger level for receive/transmit fifo.
 */
void
NvDdkPrivI2sSetTriggerLevel(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction FifoType,
    NvU32 TriggerLevel)
{
    NvU32 I2sFifoRegVal;

    // Get the fifo control register value.
    I2sFifoRegVal = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_FIFO_SCR);

    // If receive fifo or both fifo then configure the receive fifo.
    if (FifoType & I2sAc97Direction_Receive)
    {
        switch (TriggerLevel)
        {
            case I2sTriggerlvl_1:
                // One slots
                I2sFifoRegVal = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR,
                                        FIFO2_ATN_LVL, ONE_SLOT, I2sFifoRegVal);
                break;

            case I2sTriggerlvl_4:
                // four slots
                I2sFifoRegVal = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR,
                                        FIFO2_ATN_LVL, FOUR_SLOTS, I2sFifoRegVal);
                break;

            case I2sTriggerlvl_8:
                // 8 slots
                I2sFifoRegVal = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR,
                                        FIFO2_ATN_LVL, EIGHT_SLOTS, I2sFifoRegVal);
                break;

            case I2sTriggerlvl_12:
                // 12 slots
                I2sFifoRegVal = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR,
                                        FIFO2_ATN_LVL, TWELVE_SLOTS, I2sFifoRegVal);
                break;

            default:
                NV_ASSERT(!"Invalid TriggerLevel");
        }
    }

    if (FifoType == I2sAc97Direction_Transmit)
    {
        switch (TriggerLevel)
        {
            case I2sTriggerlvl_1:
                // One slots
                I2sFifoRegVal = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR,
                                        FIFO1_ATN_LVL, ONE_SLOT, I2sFifoRegVal);
                break;

            case I2sTriggerlvl_4:
                // four slots
                I2sFifoRegVal = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR,
                                        FIFO1_ATN_LVL, FOUR_SLOTS, I2sFifoRegVal);
                break;

            case I2sTriggerlvl_8:
                // 8 slots
                I2sFifoRegVal = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR,
                                        FIFO1_ATN_LVL, EIGHT_SLOTS, I2sFifoRegVal);
                break;

            case I2sTriggerlvl_12:
                // 12 slots
                I2sFifoRegVal = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR,
                                        FIFO1_ATN_LVL, TWELVE_SLOTS, I2sFifoRegVal);
                break;

            default:
                NV_ASSERT(!"Invalid TriggerLevel");
        }
    }

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_FIFO_SCR, I2sFifoRegVal);
    NVDDK_I2S_CONTLOG(("NvDdkPrivI2sSetTriggerLevel FIFO SCR 0x%x \n", I2sFifoRegVal));
}

// set Data Flow for dsp mode
void
NvDdkPrivI2sSetDspDataFlow
(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction Direction,
    NvBool IsEnable
)
{
    NvU32 DspControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL);
    NvU32 I2sControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL);
    // Set to DSP mode to I2s Ctrl
    if (IsEnable)
        I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT, DSP, I2sControl);
    else
        I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT, I2S, I2sControl);

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL, I2sControl);

    // Program bit 4 & 0 only at end.
    if (Direction & I2sAc97Direction_Transmit)
    {
        if (IsEnable)
             DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, PCM_TRM_MODE,
                               ENABLE, DspControl);
        else
             DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, PCM_TRM_MODE,
                                 DISABLE, DspControl);
    }
    if (Direction & I2sAc97Direction_Receive)
    {
        if (IsEnable)
            DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, PCM_RCV_MODE,
                                ENABLE, DspControl);
        else
            DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, PCM_RCV_MODE,
                                DISABLE, DspControl);
    }

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL, DspControl);
    NVDDK_I2S_CONTLOG(("DspControl 0x%x \n", DspControl));
}

// set dsp mode mask bits
void
NvDdkPrivI2sSetDspMaskBits
(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvU32 TxMaskBits,
    NvU32 RxMaskBits
)
{
    NvU32 DspControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL);

    DspControl = NV_FLD_SET_DRF_NUM(I2S, I2S_PCM_CTRL, TRM_MASK_BITS,
                                TxMaskBits, DspControl);
    DspControl = NV_FLD_SET_DRF_NUM(I2S, I2S_PCM_CTRL, RCV_MASK_BITS,
                                RxMaskBits, DspControl);

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL, DspControl);
}

// set dsp mode Fsync control
void
NvDdkPrivI2sSetDspFsyncWidth
(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvU32 FsyncWidth
)
{
    NvU32 DspControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL);

    if (FsyncWidth == 0)
    {
        DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, FSYNC_PCM_CTRL,
                            SHORT, DspControl);
    }
    else if (FsyncWidth == 1)
    {
        DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, FSYNC_PCM_CTRL,
                            LONG, DspControl);
    }

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL, DspControl);
}

// set dsp mode Highz control
void
NvDdkPrivI2sSetDspEdgeMode
(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvDdkI2SHighzMode HighzEdgeMode
)
{
    NvU32 DspControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL);

    switch (HighzEdgeMode)
    {
        case NvDdkI2SHighzMode_No_PosEdge:
        default:
            DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, PCM_TRM_EDGE_CNTRL,
                            POS_EDGE_NO_HIGHZ, DspControl);
            break;
        case NvDdkI2SHighzMode_PosEdge:
            DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, PCM_TRM_EDGE_CNTRL,
                           POS_EDGE_HIGHZ, DspControl);
            break;
        case NvDdkI2SHighzMode_No_NegEdge:
            DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, PCM_TRM_EDGE_CNTRL,
                          NEG_EDGE_NO_HIGHZ, DspControl);
            break;
        case NvDdkI2SHighzMode_NegEdge:
            DspControl = NV_FLD_SET_DRF_DEF(I2S, I2S_PCM_CTRL, PCM_TRM_EDGE_CNTRL,
                          NEG_EDGE_HIGHZ, DspControl);
            break;
    }

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL, DspControl);
}

// Currently following are enabled only for cpu side
#if !NV_IS_AVP

/**
 * Get fifo Count.
 */
NvBool
NvDdkPrivI2sGetFifoCount(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction Direction,
    NvU32* pFifoCount)
{
    NvBool IsSuccess = NV_FALSE;
    NvU32 FifoScr = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_FIFO_SCR);
    NvU32 I2sControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL);

    if (Direction & I2sAc97Direction_Receive)
    {
        if (I2sControl & NV_DRF_DEF(I2S, I2S_CTRL, FIFO2_TRANSACTION_EN, ENABLE))
        {
            FifoScr = FifoScr & NV_DRF_DEF(I2S, I2S_FIFO_SCR, FIFO2_FULL_EMPTY_COUNT, DEFAULT_MASK);
            IsSuccess = NV_TRUE;
        }
    }

    if (Direction & I2sAc97Direction_Transmit)
    {
        if (I2sControl & NV_DRF_DEF(I2S, I2S_CTRL, FIFO1_TRANSACTION_EN, ENABLE))
        {
            FifoScr = FifoScr & NV_DRF_DEF(I2S, I2S_FIFO_SCR, FIFO1_FULL_EMPTY_COUNT, DEFAULT_MASK);

            // To make sure nothing is left in FIFO for next run
            if (FifoScr == NV_DRF_DEF(I2S, I2S_FIFO_SCR, FIFO1_FULL_EMPTY_COUNT, DEFAULT))
            {
                NvDdkPrivI2sResetFifo(pI2sHwRegs, Direction);
            }
            IsSuccess = NV_TRUE;
        }
    }

    *pFifoCount = FifoScr;

    return IsSuccess;
}

/**
 * Reset the fifo.
 */
void
NvDdkPrivI2sResetFifo(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction Direction)
{
    NvU32 FifoScr = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_FIFO_SCR);

    if (Direction & I2sAc97Direction_Receive)
        FifoScr = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR, FIFO2_CLR, CLEAR, FifoScr);

    if (Direction & I2sAc97Direction_Transmit)
        FifoScr = NV_FLD_SET_DRF_DEF(I2S, I2S_FIFO_SCR, FIFO1_CLR, CLEAR, FifoScr);

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_FIFO_SCR, FifoScr);

    // For safety purpose - clean up the I2S_FIFO with Zeros as well
     // this will help to reduce the pop on next run
     if (Direction & I2sAc97Direction_Transmit)
     {
         NvU32 i = 0;
         for (i = 0; i < 16 ; i++)
         {
             I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_FIFO1, 0);
         }
     }
}

/**
 * Enable the loop back in the i2s channels.
 */
void NvDdkPrivI2sSetLoopbackTest(I2sAc97HwRegisters *pI2sHwRegs, NvBool IsEnable)
{
    NvU32 I2sControlVal = 0;

    I2sControlVal = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL);
    if (IsEnable)
    {
        I2sControlVal = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, LPBK, ENABLE, I2sControlVal);
    }
    else
    {
        I2sControlVal = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, LPBK, DISABLE, I2sControlVal);
    }

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL, I2sControlVal);
}

/**
* Verify whether to enable the Auto Adjust flag for Clock function
* for PLLA to program the correct frequency based on SamplingRate
*/
NvBool NvDdkPrivI2sGetAutoAdjustFlag(NvU32 SamplingRate)
{
    NvBool IsFound  = NV_FALSE;
    NvU32  MaxIndex = sizeof(s_I2sSupportedFreq)/sizeof(NvU32);
    NvU32  Index    = 0;
    for (Index = 0; Index < MaxIndex; Index++)
    {
      if (SamplingRate == s_I2sSupportedFreq[Index])
          IsFound = NV_TRUE;

      if (IsFound) break;
    }
    return IsFound;
}

/**
 * Get the clock source frequency for desired sampling rate.
 */
NvU32 NvDdkPrivI2sGetClockSourceFreq(NvU32 SamplingRate, NvU32 DatabitsPerLRCLK)
{
    NvU32 ClockFreq=0;
    NvBool IsSupportedFrequency =  NV_FALSE;

    IsSupportedFrequency = NvDdkPrivI2sGetAutoAdjustFlag(SamplingRate);

    if (IsSupportedFrequency)
    {
        // samplerate * 2 * x - maintaines BCLK at 'x' data bits for each LRCLK
        ClockFreq = SamplingRate * DatabitsPerLRCLK * 2;
    }
    else
    {
        NvU32  MaxClockSource;
        NvU32  DivIndex;
        NvBool IsGotClockSource = NV_FALSE;
        NvU32 SamplingRateConstant;

        // Calculated sampling rate constant for 5% baundary.
        NvU32 CalcSamplingRateL;
        NvU32 CalcSamplingRateH;

        // 2% boundary of the sampling rate.
        NvU32 SamplingRate_deviation_L;
        NvU32 SamplingRate_deviation_H;
        NvU32 ClockSourceIndex;

        MaxClockSource = sizeof(s_I2sClockSourceFreq)/sizeof(NvU32);

        // Get the  over and lesser sampling rate constant calculation.
        SamplingRate_deviation_L =
                (SamplingRate - (SamplingRate * SAMPLING_RATE_ACCURACY)/100);
        SamplingRate_deviation_H =
                (SamplingRate + (SamplingRate * SAMPLING_RATE_ACCURACY)/100);

        // Search the clock source which gives the proper sampling rate constant
        // for given SamplingRate.
        for (ClockSourceIndex =0; ClockSourceIndex < MaxClockSource;
                                                                ++ClockSourceIndex)
        {
            for (DivIndex = 1; DivIndex < 64; ++DivIndex)
            {
                // Get the Clock source setting for given clock frequency
                ClockFreq = (s_I2sClockSourceFreq[ClockSourceIndex]*1000)/DivIndex;

                SamplingRateConstant = (ClockFreq /(SamplingRate *2));

                if (!SamplingRateConstant) break;

                // Get the sampling rate with calculated sampling rate constant and +1.
                CalcSamplingRateH= (ClockFreq)/(SamplingRateConstant * 2);
                CalcSamplingRateL= (ClockFreq)/((SamplingRateConstant+1) *2);

                // If calculated low SamplingRate is in up/down 5 percent SamplingRates then
                // we have got the correct clock source.
                if ((CalcSamplingRateL > SamplingRate_deviation_L) &&
                     (CalcSamplingRateL < SamplingRate_deviation_H))
                {
                    IsGotClockSource = NV_TRUE;
                    break;
                }

                // If calculated High SamplingRate is in up/down 5 percent SamplingRates then
                // we have got the correct clock source.
                if ((CalcSamplingRateH > SamplingRate_deviation_L) &&
                     (CalcSamplingRateH < SamplingRate_deviation_H))
                {
                    IsGotClockSource = NV_TRUE;
                    break;
                }
             }
             if (IsGotClockSource)
             {
                break;
             }
        }
    }

    // Now if we dont get the clock source then this sampling rate can not be
    // possible.
    NV_ASSERT(ClockFreq);

    // Return the clock freq in KHz
    return (ClockFreq/1000);
}

/**
 * Calculate the timing register value based on the clock source frequency.
 */
NvU32
NvDdkPrivI2sCalculateSamplingFreq(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvU32 SamplingRate,
    NvU32 DatabitsPerLRCLK,
    NvU32 ClockSourceFreq)
{
    NvU32 FinalSamplingRateConstant = 0;
    NvBool IsSupportedFrequency =  NV_FALSE;

    IsSupportedFrequency = NvDdkPrivI2sGetAutoAdjustFlag(SamplingRate);

    if (IsSupportedFrequency)
    {
        FinalSamplingRateConstant = DatabitsPerLRCLK - 1;
    }
    else
    {
        NvU32 SamplingRateConstant;

        // Calculated sampling rate constant for 5% baundary.
        NvU32 CalcSamplingRateL;
        NvU32 CalcSamplingRateH;

        // 2% boundary for the sampling rate.
        NvU32 SamplingRate_deviation_L;
        NvU32 SamplingRate_deviation_H;

        NvU32 ClockFreq;
        // Get the  over and lesser sampling rate constant calculation.
        SamplingRate_deviation_L =
                (SamplingRate - (SamplingRate * SAMPLING_RATE_ACCURACY)/100);
        SamplingRate_deviation_H =
                (SamplingRate + (SamplingRate * SAMPLING_RATE_ACCURACY)/100);

        // Get the Clock source setting for given clock frequency
        ClockFreq = (ClockSourceFreq*1000);

        SamplingRateConstant = (ClockFreq /(SamplingRate *2));

        // Get the sampling rate with calculated sampling rate constant and +1.
        CalcSamplingRateH= (ClockFreq)/(SamplingRateConstant * 2);
        CalcSamplingRateL= (ClockFreq)/((SamplingRateConstant+1) *2);

        // If calculated low SamplingRate is in up/down 5 percent SamplingRates then
        // we have got the correct clock source.
        if ((CalcSamplingRateL > SamplingRate_deviation_L) &&
             (CalcSamplingRateL < SamplingRate_deviation_H))
        {
            FinalSamplingRateConstant = SamplingRateConstant;
        }
        // If calculated High SamplingRate is in up/down 5 percent SamplingRates then
        // we have got the correct clock source.
        else if ((CalcSamplingRateH > SamplingRate_deviation_L) &&
                 (CalcSamplingRateH < SamplingRate_deviation_H))
        {
            FinalSamplingRateConstant = SamplingRateConstant;
        }
        // Now if we dont get the clock source then this sampling rate can not be
        // possible.
        else
        {
            NV_ASSERT(!"Sampling rate not supported");
            return 0;
        }
    }

    return FinalSamplingRateConstant;
}

/**
 * Set the timing register based on the clock source frequency.
 */
void
NvDdkPrivI2sSetSamplingFreq(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvU32 SamplingRate,
    NvU32 DatabitsPerLRCLK,
    NvU32 ClockSourceFreq)
{
    NvU32 FinalSamplingRateConstant;
    NvU32 TimingRegisterVal;

    FinalSamplingRateConstant = NvDdkPrivI2sCalculateSamplingFreq (pI2sHwRegs,
                                SamplingRate, DatabitsPerLRCLK, ClockSourceFreq);

    TimingRegisterVal = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_TIMING);
    TimingRegisterVal = NV_FLD_SET_DRF_NUM(I2S, I2S_TIMING, CHANNEL_BIT_CNT,
                        FinalSamplingRateConstant, TimingRegisterVal);
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_TIMING, TimingRegisterVal);
}

void NvDdkPrivSetDataCommFormat(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvDdkI2SMode I2sMode
)
{
    NvU32 I2sControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL);

    switch (I2sMode)
    {
        case NvDdkI2SMode_PCM:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT,
                            I2S, I2sControl);
            break;
        case NvDdkI2SMode_LeftJustified:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT,
                            LJM, I2sControl);
            break;
        case NvDdkI2SMode_RightJustified:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT,
                            RJM, I2sControl);
            break;
        case NvDdkI2SMode_DSP:
        case NvDdkI2SMode_Network:
        case NvDdkI2SMode_TDM:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT,
                            DSP, I2sControl);
            break;
        default:
            NV_ASSERT(!"Invalid DataCommFormat");
    }
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL, I2sControl);
    NVDDK_I2S_CONTLOG((" Bitformat Reg 0x%x \n", I2sControl));
}

void
NvDdkPrivI2sSetInterfaceProperty(
    I2sAc97HwRegisters *pI2sHwRegs,
    void *pInterface)
{
    NvOdmQueryI2sInterfaceProperty *pIntProps = NULL;
    NvU32 I2sControl;

    pIntProps  = (NvOdmQueryI2sInterfaceProperty *)pInterface;

    // Get the fifo status register value.
    I2sControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL);

    if (pIntProps->Mode == NvOdmQueryI2sMode_Master)
        I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, M_S, MASTER, I2sControl);
    else
        I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, M_S, SLAVE, I2sControl);


    switch (pIntProps->I2sLRLineControl)
    {
        case NvOdmQueryI2sLRLineControl_LeftOnLow:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, L_R, LRCK_LOW, I2sControl);
            break;

        case NvOdmQueryI2sLRLineControl_RightOnLow:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, L_R, LRCK_HIGH, I2sControl);
            break;

        default:
            NV_ASSERT(!"Invalid I2sLrControl");
    }

    switch (pIntProps->I2sDataCommunicationFormat)
    {
        case NvOdmQueryI2sDataCommFormat_I2S:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT,
                                I2S, I2sControl);
            break;

        case NvOdmQueryI2sDataCommFormat_LeftJustified:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT,
                                LJM, I2sControl);
            break;

        case NvOdmQueryI2sDataCommFormat_RightJustified:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT,
                                RJM, I2sControl);
            break;

        case NvOdmQueryI2sDataCommFormat_Dsp:
            I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_FORMAT,
                                DSP, I2sControl);
            break;

        default:
            NV_ASSERT(!"Invalid DataCommFormat");
    }
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL, I2sControl);
    NVDDK_I2S_REGLOG(("NvDdkPrivI2sSetInterfaceProperty i2sControl 0x%x \n", I2sControl));
}

/**
 * Set the fifo format.
 */
void
NvDdkPrivI2sSetFifoFormat(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvDdkI2SDataWordValidBits DataFifoFormat,
    NvU32 DataSize)
{
    NvU32 I2sControl;

    // Get the fifo status register value.
    I2sControl = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL);
    switch (DataFifoFormat)
    {
        case NvDdkI2SDataWordValidBits_StartFromLsb:
            if (DataSize == 16)
            {
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO_FORMAT,
                                    BIT_SIZE_16_LSB, I2sControl);
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_SIZE,
                                    BIT_SIZE_16, I2sControl);
            }
            else if (DataSize == 20)
            {
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO_FORMAT,
                                    BIT_SIZE_20_LSB, I2sControl);
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_SIZE,
                                    BIT_SIZE_20, I2sControl);
            }
            else if (DataSize == 24)
            {
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO_FORMAT,
                                    BIT_SIZE_24_LSB, I2sControl);
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_SIZE,
                                    BIT_SIZE_24, I2sControl);
            }
            else if (DataSize == 32)
            {
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO_FORMAT,
                                    BIT_SIZE_32, I2sControl);
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_SIZE,
                                    BIT_SIZE_32, I2sControl);
            }
            else
            {
                NV_ASSERT(!"Data size not supported");
            }
            break;

        case NvDdkI2SDataWordValidBits_StartFromMsb:
            NV_ASSERT(!"DataFifoFormat not supported");
            break;

        case NvDdkI2SDataWordValidBits_InPackedFormat:
            if (DataSize == 16)
            {
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, BIT_SIZE,
                                    BIT_SIZE_16, I2sControl);
                I2sControl = NV_FLD_SET_DRF_DEF(I2S, I2S_CTRL, FIFO_FORMAT,
                                    PACKED, I2sControl);
            }
            else
            {
                NV_ASSERT(!"Data size not supported");
            }
            break;

        default:
            NV_ASSERT(!"Invalid DataFifoFormat");
    }

    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL, I2sControl);
    NVDDK_I2S_CONTLOG((" Fifoformat Reg 0x%x \n", I2sControl));
}


/**
 * Get the interrupt source occured from i2s channels.
 */
I2sHwInterruptSource
NvDdkPrivI2sGetInterruptSource(
    I2sAc97HwRegisters *pI2sHwRegs)
{
    NvU32 I2sFifoStatus;
    // Get the fifo status register value.
    I2sFifoStatus = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_STATUS);

    if (I2sFifoStatus & NV_DRF_DEF(I2S, I2S_STATUS, IS_FIFO2_ERR, ERR))
    {
        return I2sHwInterruptSource_Receive;
    }

    if (I2sFifoStatus & NV_DRF_DEF(I2S, I2S_STATUS, IS_FIFO1_ERR, ERR))
    {
        return I2sHwInterruptSource_Transmit;
    }

    return I2sHwInterruptSource_None;
}

/**
 * Ack the interrupt source occured from i2s channels.
 */
void
NvDdkPrivI2sAckInterruptSource(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sHwInterruptSource IntSource)
{
    NvU32 I2sFifoStatus;
    NvU32 RxErrorClear;
    NvU32 TxErrorClear;

    // Get the fifo status register value.
    I2sFifoStatus = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_STATUS);

    if (IntSource ==  I2sHwInterruptSource_Receive)
    {
        RxErrorClear = NV_DRF_DEF(I2S, I2S_STATUS, IS_FIFO2_ERR, ERR);
        if (I2sFifoStatus & RxErrorClear)
        {
            I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_STATUS, RxErrorClear);
        }
    }

    if (IntSource ==  I2sHwInterruptSource_Transmit)
    {
        TxErrorClear = NV_DRF_DEF(I2S, I2S_STATUS, IS_FIFO1_ERR, ERR);
        if (I2sFifoStatus & TxErrorClear)
        {
            I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_STATUS, TxErrorClear);
        }
    }
}

void
NvDdkPrivI2sNwModeSetSlotSelectionMask(
            I2sAc97HwRegisters *pHwRegs,
            I2sAc97Direction Direction,
            NvU8 ActiveSlotSelectionMask,
            NvBool IsEnable)
{
    NvU32 NwControlReg = 0;
    NwControlReg = I2S_REG_READ32(pHwRegs->pRegsVirtBaseAdd, I2S_NW_CTRL);
    if (Direction & I2sAc97Direction_Receive) // Rx mode
    {
        NwControlReg |= NV_DRF_VAL(I2S, I2S_NW_CTRL, RCV_TLPHY_SLOT_SEL,ActiveSlotSelectionMask);
        if (IsEnable)
            NwControlReg |= NV_DRF_DEF(I2S, I2S_NW_CTRL, RCV_TLPHY_MODE, ENABLE);
        else
            NwControlReg |= NV_DRF_DEF(I2S, I2S_NW_CTRL, RCV_TLPHY_MODE, DISABLE);
    }
    else if (Direction & I2sAc97Direction_Transmit)// Tx mode
    {
        NwControlReg |= NV_DRF_VAL(I2S, I2S_NW_CTRL, TRM_TLPHY_SLOT_SEL,ActiveSlotSelectionMask);
        if (IsEnable)
            NwControlReg |= NV_DRF_DEF(I2S, I2S_NW_CTRL, TRM_TLPHY_MODE, ENABLE);
        else
            NwControlReg |= NV_DRF_DEF(I2S, I2S_NW_CTRL, TRM_TLPHY_MODE, DISABLE);
    }

    I2S_REG_WRITE32(pHwRegs->pRegsVirtBaseAdd, I2S_NW_CTRL, NwControlReg);

    NVDDK_I2S_CONTLOG((" NW mode selected 0x%x \n",
                     I2S_REG_READ32(pHwRegs->pRegsVirtBaseAdd, I2S_NW_CTRL)));
}

void
NvDdkPrivTdmModeSetNumberOfSlotsFxn(
        I2sAc97HwRegisters *pHwRegs,
        NvU32 NumberOfSlotsPerFsync)
{
    NvU32 TdmControlReg =0;
    TdmControlReg = I2S_REG_READ32(pHwRegs->pRegsVirtBaseAdd, I2S_TDM_CTRL);
    TdmControlReg |= NV_DRF_VAL(I2S, I2S_TDM_CTRL, TOTAL_SLOTS, NumberOfSlotsPerFsync);
    I2S_REG_WRITE32(pHwRegs->pRegsVirtBaseAdd, I2S_TDM_CTRL, TdmControlReg);

}

void
NvDdkPrivTdmModeSetDataWordFormatFxn(
        I2sAc97HwRegisters *pHwRegs,
        NvBool IsRecieve,
        NvDdkI2SDataWordValidBits I2sDataWordFormat)
{
    NvU32 TdmControlReg =0;
    TdmControlReg = I2S_REG_READ32(pHwRegs->pRegsVirtBaseAdd, I2S_TDM_CTRL);
    if (IsRecieve)  // Rx mode
    {
        if (I2sDataWordFormat == NvDdkI2SDataWordValidBits_StartFromLsb)
            TdmControlReg |= NV_DRF_DEF(I2S, I2S_TDM_CTRL, RX_MSB_LSB, LSB_FIRST);
        else if (I2sDataWordFormat == NvDdkI2SDataWordValidBits_StartFromMsb)
            TdmControlReg |= NV_DRF_DEF(I2S, I2S_TDM_CTRL, RX_MSB_LSB, MSB_FIRST);
        else
            NV_ASSERT(!"I2s Data format not supported\n");
    }
    else            // Tx mode
    {
        if (I2sDataWordFormat == NvDdkI2SDataWordValidBits_StartFromLsb)
            TdmControlReg |= NV_DRF_DEF(I2S, I2S_TDM_CTRL, TX_MSB_LSB, LSB_FIRST);
        else if (I2sDataWordFormat == NvDdkI2SDataWordValidBits_StartFromMsb)
            TdmControlReg |= NV_DRF_DEF(I2S, I2S_TDM_CTRL, TX_MSB_LSB, MSB_FIRST);
        else
            NV_ASSERT(!"I2s Data format not supported\n");
    }
    I2S_REG_WRITE32(pHwRegs->pRegsVirtBaseAdd, I2S_TDM_CTRL, TdmControlReg);
}

void
NvDdkPrivTdmModeSetSlotSizeFxn(
    I2sAc97HwRegisters * pHwRegs,
    NvU32 SlotSize)
{
    NvU32 TdmControlReg =0;
    TdmControlReg = I2S_REG_READ32(pHwRegs->pRegsVirtBaseAdd, I2S_TDM_CTRL);
    TdmControlReg |= NV_DRF_VAL(I2S, I2S_TDM_CTRL, TDM_BIT_SIZE, SlotSize);
    I2S_REG_WRITE32(pHwRegs->pRegsVirtBaseAdd, I2S_TDM_CTRL, TdmControlReg);
}

void
NvDdkPrivI2sGetStandbyRegisters(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97StandbyRegisters *pStandbyRegs)
{
    NvU32 Reg32Rd;

     //I2s Control Register - entire register saved and restored
     Reg32Rd  = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL);
     // Retain only the needed regbits
     pStandbyRegs->I2sAc97CtrlReg = Reg32Rd & 0x03FFFFF0;

     // I2s Timing Registers
     pStandbyRegs->I2sAc97TimingReg = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_TIMING);

     // I2s Fifo Config and status register - Attn Lvl
     pStandbyRegs->I2sAc97FifoConfigReg  = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_FIFO_SCR);

     // Pcm ctrl registers
     pStandbyRegs->I2sAc97PcmCtrlReg = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL);

     // Nw Ctrl registers
     pStandbyRegs->I2sAc97NwCtrlReg = I2S_REG_READ32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_NW_CTRL);

     NVDDK_I2S_POWERLOG(("NvDdkPrivI2sGetStandbyRegisters Dspcontrol 0x%x \n", pStandbyRegs->I2sAc97PcmCtrlReg));
}

void
NvDdkPrivI2sSetStandbyRegisters(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97StandbyRegisters *pStandbyRegs)
{
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_CTRL, pStandbyRegs->I2sAc97CtrlReg);
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_TIMING, pStandbyRegs->I2sAc97TimingReg);
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_FIFO_SCR, pStandbyRegs->I2sAc97FifoConfigReg);
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_PCM_CTRL, pStandbyRegs->I2sAc97PcmCtrlReg);
    I2S_REG_WRITE32(pI2sHwRegs->pRegsVirtBaseAdd, I2S_NW_CTRL, pStandbyRegs->I2sAc97NwCtrlReg);
    NVDDK_I2S_POWERLOG(("NvDdkPrivI2sSetStandbyRegisters Dspcontrol 0x%x \n", pStandbyRegs->I2sAc97PcmCtrlReg));
}

#endif


