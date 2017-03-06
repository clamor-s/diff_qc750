/*
 * Copyright (c) 2007 - 2012 NVIDIA Corporation.  All rights reserved.
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
 *           Private functions for the spdif hw access</b>
 *
 * @b Description:  Implements  the private interfacing functions for the spdif
 * hw interface.
 *
 */

#include "nvrm_drf.h"
#include "ddkspdif_hw_private.h"
#include "ap20/arspdif_ppi.h"
#include "nvrm_hardware_access.h"
#include "nvddk_spdif.h"
#include "nvassert.h"

/********* Fifo1 is transmitter **************/

/**
 * Initialize the spdif Struct.
 */
NvError
NvDdkPrivSpdifHwInitialize(
        NvU32 SpdifChannelId,
        SpdifHwRegisters *pSpdifHwRegs)
{
    NV_ASSERT(pSpdifHwRegs);

    // Initialize the member of Spdif hw register structure.
    pSpdifHwRegs->ChannelId     = SpdifChannelId;
    pSpdifHwRegs->RmDmaModuleId = NvRmDmaModuleID_Spdif;
    pSpdifHwRegs->TxFifoAddress = pSpdifHwRegs->RegsPhyBaseAdd
                                  + SPDIF_DATA_OUT_0;
    pSpdifHwRegs->RxFifoAddress = pSpdifHwRegs->RegsPhyBaseAdd
                                  + SPDIF_DATA_IN_0;
    return NvSuccess;
}

#if !NV_IS_AVP
/**
 * Set the sampling rate.
 */
NvError
NvDdkPrivSpdifSetSamplingFreq(
    SpdifHwRegisters *pSpdifHwRegs,
    NvU32 SamplingRate,
    NvU32 *pClockSourceFreq)
{
    NvError Status = NvSuccess;

    static NvU32 spdif_channel_status[] = {
        0x0, // 44.1, default values
        0xf << 4, // bits 36-39, original sample freq -- 44.1
        0x0,
        0x0,
        0x0,
        0x0,
    };

    NV_ASSERT(pSpdifHwRegs);
    NV_ASSERT(pClockSourceFreq);

    *pClockSourceFreq = 0;

    switch (SamplingRate)
    {
        case 32000:
            *pClockSourceFreq = 4096; // 4.0960 MHz
            spdif_channel_status[0] = 0x3 << 24;
            spdif_channel_status[1] = 0xC << 4;
           break;
        case 44100:
            *pClockSourceFreq = 5644; // 5.6448 MHz
            spdif_channel_status[0] = 0x0;
            spdif_channel_status[1] = 0xF << 4;
           break;
        case 48000:
            *pClockSourceFreq = 6144; // 6.1440MHz
            spdif_channel_status[0] = 0x2 << 24;
            spdif_channel_status[1] = 0xD << 4;
            break;
        case 88200:
            *pClockSourceFreq = 11289; // 11.2896 MHz
            break;
        case 96000:
            *pClockSourceFreq = 12288; // 12.288 MHz
            break;
        case 176400:
            *pClockSourceFreq = 22579; // 22.5792 MHz
            break;
        case 192000:
            *pClockSourceFreq = 24576; // 24.5760 MHz
            break;
        default:
            NV_ASSERT(!"Bad sampling Rate set");
            Status = NvError_NotSupported;
    }
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CH_STA_TX_A, spdif_channel_status[0]);
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CH_STA_TX_B, spdif_channel_status[1]);
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CH_STA_TX_C, spdif_channel_status[2]);
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CH_STA_TX_D, spdif_channel_status[3]);
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CH_STA_TX_E, spdif_channel_status[4]);
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CH_STA_TX_F, spdif_channel_status[5]);



    return Status;
}

#endif //!NV_IS_AVP

/**
 * Enable/Disable the transmit.
 */
NvError NvDdkPrivSpdifSetTransmit(SpdifHwRegisters *pSpdifHwRegs, NvBool IsEnable)
{
    NvU32 SpdifControl;

    // Get the fifo status register value.
    SpdifControl = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);
    if(IsEnable)
    {
        SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, TX_EN,
                            ENABLE, SpdifControl);
        SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, TC_EN,
                            ENABLE, SpdifControl);

    }
    else
    {
        SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, TX_EN,
                            DISABLE, SpdifControl);
        SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, TC_EN,
                            DISABLE, SpdifControl);
    }
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifControl);
    return NvSuccess;
}

/**
 * Enable/Disable the receive.
 */
NvError NvDdkPrivSpdifSetReceive(SpdifHwRegisters *pSpdifHwRegs, NvBool IsEnable)
{
    NvU32 SpdifControl;

    // Get the fifo status register value.
    SpdifControl = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);
    if(IsEnable)
    {
        SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, RX_EN,
                            ENABLE, SpdifControl);
    }
    else
    {
        SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, RX_EN,
                            DISABLE, SpdifControl);
    }
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifControl);
    return NvSuccess;
}

/**
 * Set the the trigger level for receive/transmit fifo.
 */
NvError
NvDdkPrivSpdifSetTriggerLevel(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwFifo FifoType,
    NvU32 TriggerLevel)
{
    NvU32 SpdifFifoRegVal;
    NvError Error = NvSuccess;

    // Get the fifo control register value.
    SpdifFifoRegVal = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, DATA_FIFO_CSR);

    // If receive fifo or both fifo then configure the receive fifo.
    if ( (FifoType == SpdifHwFifo_Receive) || (FifoType == SpdifHwFifo_Both) )
    {
        switch (TriggerLevel)
        {
            case 4:
                // One slots
                SpdifFifoRegVal = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR,
                                        RX_ATN_LVL, RX1_WORD_FULL, SpdifFifoRegVal);
                break;

            case 16:
                // four slots
                SpdifFifoRegVal = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR,
                                        RX_ATN_LVL, RX4_WORD_FULL, SpdifFifoRegVal);
                break;

            case 32:
                // 8 slots
                SpdifFifoRegVal = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR,
                                        RX_ATN_LVL, RX8_WORD_FULL, SpdifFifoRegVal);
                break;

            case 48:
                // 12 slots
                SpdifFifoRegVal = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR,
                                        RX_ATN_LVL, RX12_WORD_FULL, SpdifFifoRegVal);
                break;

            default:
                NV_ASSERT(!"Invalid TriggerLevel");
                break;
        }
    }

    if ( (FifoType == SpdifHwFifo_Transmit) || (FifoType == SpdifHwFifo_Both))
    {
        switch (TriggerLevel)
        {
            case 4:
                // One slots
                SpdifFifoRegVal = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR,
                                        TX_ATN_LVL, TX1_WORD_EMPTY, SpdifFifoRegVal);
                break;

            case 16:
                // four slots
                SpdifFifoRegVal = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR,
                                        TX_ATN_LVL, TX4_WORD_EMPTY, SpdifFifoRegVal);
                break;

            case 32:
                // 8 slots
                SpdifFifoRegVal = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR,
                                        TX_ATN_LVL, TX8_WORD_EMPTY, SpdifFifoRegVal);
                break;

            case 48:
                // 12 slots
                SpdifFifoRegVal = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR,
                                        TX_ATN_LVL, TX12_WOR_DEMPTY, SpdifFifoRegVal);
                break;

            default:
                NV_ASSERT(!"Invalid TriggerLevel");
                break;
        }
    }

    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, DATA_FIFO_CSR, SpdifFifoRegVal);
    return Error;
}

#if !NV_IS_AVP
/**
 * Reset the rx/tx fifo.
 */
NvError NvDdkPrivSpdifResetFifo(SpdifHwRegisters *pSpdifHwRegs, SpdifHwFifo FifoType)
{
    NvU32 SpdifFifoScr;

    // Get the fifo status register value.
    SpdifFifoScr = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, DATA_FIFO_CSR);
    if ((FifoType == SpdifHwFifo_Receive) || (FifoType == SpdifHwFifo_Both))
    {
        SpdifFifoScr = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR, RX_CLR,
                        DEFAULT_MASK, SpdifFifoScr);
        SpdifFifoScr = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR, RU_CLR,
                        DEFAULT_MASK, SpdifFifoScr);
    }

    if ((FifoType == SpdifHwFifo_Transmit) || (FifoType == SpdifHwFifo_Both))
    {
        SpdifFifoScr = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR, TX_CLR,
                        DEFAULT_MASK, SpdifFifoScr);
        SpdifFifoScr = NV_FLD_SET_DRF_DEF(SPDIF, DATA_FIFO_CSR, TU_CLR,
                        DEFAULT_MASK, SpdifFifoScr);
    }
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, DATA_FIFO_CSR, SpdifFifoScr);
    return NvSuccess;

}

/**
 * Set Packed mode or not
 */
NvError
NvDdkPrivSpdifSetPackedDataMode(
    SpdifHwRegisters *pSpdifHwRegs,
    NvDdkSpdifDataWordValidBits DataFifoFormat,
    NvU32 DataSize)
{
    NvU32 SpdifControl;

    // Get the fifo status register value.
    SpdifControl = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);

    if (DataFifoFormat ==
            NvDdkSpdifDataWordValidBits_InPackedFormat)
    {
        if (DataSize == 16)
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, PACK,
                                ENABLE, SpdifControl);
    }
    else
    {
        SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, PACK,
                                DISABLE, SpdifControl);
    }

    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifControl);
    return NvSuccess;
}

/**
 * Set the data bit size
 */
NvError NvDdkPrivSpdifSetDataBitSize(SpdifHwRegisters *pSpdifHwRegs, NvU32 DataBitSize)
{
    NvU32 SpdifControl;

    // Get the fifo status register value.
    SpdifControl = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);
    switch(DataBitSize)
    {
        case 16:
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, BIT_MODE,
                                MODE16BIT, SpdifControl);
            break;

        case 20:
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, BIT_MODE,
                                MODE20BIT, SpdifControl);
            break;

        case 24:
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, BIT_MODE,
                                MODE24BIT, SpdifControl);
            break;

        case 32:
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, BIT_MODE,
                                MODERAW, SpdifControl);
            break;

        default:
            NV_ASSERT(!"Invalid Data Bit Size");
    }

    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifControl);
    return NvSuccess;
}

/**
 * Set the Spdif left right control polarity.
 */
NvError NvDdkPrivSpdifSetDataCaptureControl(SpdifHwRegisters *pSpdifHwRegs,
    NvOdmQuerySpdifDataCaptureControl SpdifLrControl)
{
    NvU32 SpdifControl;

    // Get the fifo status register value.
    SpdifControl = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);

    switch(SpdifLrControl)
    {
        case NvOdmQuerySpdifDataCaptureControl_FromLeft:
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, CAP_LC, DEFAULT_MASK, SpdifControl);
            break;

        case NvOdmQuerySpdifDataCaptureControl_FromRight:
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, CAP_LC, DEFAULT, SpdifControl);
            break;

        default:
            NV_ASSERT(!"Invalid Data Capture Control");
    }

    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifControl);
    return NvSuccess;
}

/**
 * Enable/Disable the interrupt source.
 */
NvError
NvDdkPrivSpdifSetInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource IntSource,
    NvBool IsEnable)
{
    NvU32 SpdifControl;

    // Get the fifo status register value.
    SpdifControl = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);

    if (IntSource ==  SpdifHwInterruptSource_ReceiveError)
    {
        if(IsEnable)
        {
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, IE_RXE,
                                ENABLE, SpdifControl);
        }
        else
        {
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, IE_RXE,
                                DISABLE, SpdifControl);
        }
    }

    if (IntSource ==  SpdifHwInterruptSource_TransmitError)
    {
        if(IsEnable)
        {
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, IE_TXE,
                                ENABLE, SpdifControl);
        }
        else
        {
            SpdifControl = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, IE_TXE,
                                DISABLE, SpdifControl);
        }
    }

    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifControl);
    return NvSuccess;

}


/**
 * Get the interrupt source occured from spdif channels.
 */
NvError
NvDdkPrivSpdifGetInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource *pIntSource)
{
    NvU32 SpdifFifoStatus;
    // Get the fifo status register value.
    SpdifFifoStatus = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, STATUS);

    if (SpdifFifoStatus & NV_DRF_DEF(SPDIF, STATUS, RX_ERR, DEFAULT_MASK))
    {
        *pIntSource = SpdifHwInterruptSource_ReceiveError;
        return NvSuccess;
    }

    if (SpdifFifoStatus & NV_DRF_DEF(SPDIF, STATUS, TX_ERR, DEFAULT_MASK))
    {
        *pIntSource = SpdifHwInterruptSource_TransmitError;
        return NvSuccess;
    }

    if (SpdifFifoStatus & NV_DRF_DEF(SPDIF, STATUS, IS_C, DEFAULT_MASK))
    {
        *pIntSource = SpdifHwInterruptSource_ChannelInoReceive;
        return NvSuccess;
    }

    if (SpdifFifoStatus & NV_DRF_DEF(SPDIF, STATUS, QS_TU, DEFAULT_MASK))
    {
        *pIntSource = SpdifHwInterruptSource_UserDataTransmit;
        return NvSuccess;
    }

    if (SpdifFifoStatus & NV_DRF_DEF(SPDIF, STATUS, QS_RU, DEFAULT_MASK))
    {
        *pIntSource = SpdifHwInterruptSource_UserDataReceive;
        return NvSuccess;
    }

    *pIntSource = SpdifHwInterruptSource_None;
    return NvSuccess;
}

/**
 * Ack the interrupt source occured from spdif channels.
 */
NvError
NvDdkPrivSpdifAckInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource IntSource)
{
    NvU32 SpdifFifoStatus;
    NvU32 RxErrorClear;
    NvU32 TxErrorClear;

    // Get the fifo status register value.
    SpdifFifoStatus = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, STATUS);

    if (IntSource ==  SpdifHwInterruptSource_ReceiveError)
    {
        RxErrorClear = NV_DRF_DEF(SPDIF, STATUS, RX_ERR, DEFAULT_MASK);
        if(SpdifFifoStatus & RxErrorClear)
        {
            SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, STATUS, RxErrorClear);
        }
    }

    if (IntSource ==  SpdifHwInterruptSource_TransmitError)
    {
        TxErrorClear = NV_DRF_DEF(SPDIF, STATUS, TX_ERR, DEFAULT_MASK);
        if(SpdifFifoStatus & TxErrorClear)
        {
            SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, STATUS, TxErrorClear);
        }
    }

    return NvSuccess;
}


/**
 * Enable the loop back in the spdif channels.
 */
NvError NvDdkPrivSpdifSetLoopbackTest(SpdifHwRegisters *pSpdifHwRegs, NvBool IsEnable)
{
    NvU32 SpdifRegVal = 0;

    SpdifRegVal = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);
    if (IsEnable)
    {
        SpdifRegVal = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, LBK_EN, ENABLE,
            SpdifRegVal);
    }
    else
    {
        SpdifRegVal = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, LBK_EN, DISABLE,
            SpdifRegVal);
    }

    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifRegVal);

    return NvSuccess;
}

NvError
NvDdkPrivSpdifSetUserData(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwDirection operation,
    NvBool IsEnable)
{
    NvU32 SpdifRegVal = 0;

    if (operation == SpdifHwDirection_Transmit)
    {
        SpdifRegVal = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);
        if (IsEnable)
        {
            SpdifRegVal = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, TU_EN, ENABLE,
                SpdifRegVal);
        }
        else
        {
            SpdifRegVal = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, TU_EN, DISABLE,
                SpdifRegVal);
        }
        SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifRegVal);
    }
    return NvSuccess;
}

NvError
NvDdkPrivSpdifSetChannelStatus(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwDirection operation,
    NvBool IsEnable)
{
    NvU32 SpdifRegVal = 0;

    if (operation == SpdifHwDirection_Transmit)
    {
        SpdifRegVal = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);
        if (IsEnable)
        {
            SpdifRegVal = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, TC_EN, ENABLE,
                SpdifRegVal);
        }
        else
        {
            SpdifRegVal = NV_FLD_SET_DRF_DEF(SPDIF, CTRL, TC_EN, DISABLE,
                SpdifRegVal);
        }
        SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, SpdifRegVal);
    }
    return NvSuccess;
}

void
NvDdkPrivSpdifGetStandbyRegisters(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifStandbyRegisters *pStandbyRegs)
{
    NvU32 Reg32Rd;

    /*
     * Save the registers :
     *  1) Spdif Control Register - entire register saved and restored
     *  2) Spdif Data strobe control register - data strobe mode
            only saved and restored
     *  3) Spdif Fifo Config and status register - fields saved and restored
                a) Rx Fifo attention level
                b) Tx Fifo attention level
     */

    pStandbyRegs->SpdifHwCtrlReg = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL);

    Reg32Rd = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, STROBE_CTRL);
    pStandbyRegs->SpdifHwDataStrobeCtrlReg = (Reg32Rd & NV_DRF_DEF(SPDIF, STROBE_CTRL, STROBE, DEFAULT_MASK));

    Reg32Rd = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, DATA_FIFO_CSR);
    pStandbyRegs->SpdifHwFifoConfigReg = (Reg32Rd &
        (NV_DRF_DEF(SPDIF, DATA_FIFO_CSR, RX_ATN_LVL, DEFAULT_MASK) |
        NV_DRF_DEF(SPDIF, DATA_FIFO_CSR, TX_ATN_LVL, DEFAULT_MASK)));
}

void
NvDdkPrivSpdifSetStandbyRegisters(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifStandbyRegisters *pStandbyRegs)
{
    NvU32 Reg32Rd;

    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, CTRL, pStandbyRegs->SpdifHwCtrlReg);

    Reg32Rd = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, STROBE_CTRL);
    // Clear the saved bits first
    Reg32Rd = (Reg32Rd & (~(NV_DRF_DEF(SPDIF, STROBE_CTRL, STROBE, DEFAULT_MASK))));
    // Restore the saved bits
    Reg32Rd |= pStandbyRegs->SpdifHwDataStrobeCtrlReg;
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, STROBE_CTRL, Reg32Rd);

    Reg32Rd = SPDIF_REG_READ32(pSpdifHwRegs->pRegsVirtBaseAdd, DATA_FIFO_CSR);
    // Clear the saved bits first
    Reg32Rd = (Reg32Rd &
        (~(NV_DRF_DEF(SPDIF, DATA_FIFO_CSR, RX_ATN_LVL, DEFAULT_MASK) |
            NV_DRF_DEF(SPDIF, DATA_FIFO_CSR, TX_ATN_LVL, DEFAULT_MASK))));
    // Restore the saved bits
    Reg32Rd |= pStandbyRegs->SpdifHwFifoConfigReg;
    SPDIF_REG_WRITE32(pSpdifHwRegs->pRegsVirtBaseAdd, DATA_FIFO_CSR, Reg32Rd);
}
#endif //!NV_IS_AVP

