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
 *           Private functions for the spdif hw access</b>
 *
 * @b Description:  Implements  the private interfacing functions for the spdif
 * hw interface.
 *
 */

#include "nvrm_drf.h"
#include "ddkspdif_hw_private.h"
#include "t30/arspdif.h"
#include "nvrm_hardware_access.h"
#include "nvddk_spdif.h"
#include "nvassert.h"
#include "t30ddkspdif_hw_private.h"

/**
 * Initialize the spdif struct.
 */
NvError
NvDdkPrivT30SpdifHwInitialize(
    NvU32 SpdifChannelId,
    SpdifHwRegisters *pSpdifHwRegs)
{
    NV_ASSERT(pSpdifHwRegs);

    // Initialize the member of Spdif hw register structure.
    // FIXME : Apbif channel 4 is set to Spdif
    // as default for current testing.
    // Will fix it properly with appropriate
    // channel which is free to use per use case
    pSpdifHwRegs->RmDmaModuleId = NvRmDmaModuleID_AudioApbIf;
    pSpdifHwRegs->TxFifoAddress = 0x7008006C;
    pSpdifHwRegs->RxFifoAddress = 0x70080070;
    pSpdifHwRegs->ChannelId     = 3;
    return NvSuccess;
}

/**
 * Set the the trigger level for receive/transmit fifo.
 */
NvError
NvDdkPrivT30SpdifSetTriggerLevel(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwFifo FifoType,
    NvU32 TriggerLevel)
{
    // FIXME : Set the TX_Threshold and Rx threshold in the apbif
    // channel based on trigger level
    return NvSuccess;
}

#if !NV_IS_AVP
/**
 * Reset the rx/tx fifo.
 */
NvError NvDdkPrivT30SpdifResetFifo(SpdifHwRegisters *pSpdifHwRegs, SpdifHwFifo FifoType)
{
    // FIXME: do apdif channel reset over here - if needed
    return NvSuccess;

}

/**
 * Enable/Disable the interrupt source.
 */
NvError
NvDdkPrivT30SpdifSetInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource IntSource,
    NvBool IsEnable)
{
    /* NvU32 SpdifControl;

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
    */
    return NvSuccess;

}


/**
 * Get the interrupt source occured from spdif channels.
 */
NvError
NvDdkPrivT30SpdifGetInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource *pIntSource)
{
    // Get the apbif fifo status mask value.
    *pIntSource = SpdifHwInterruptSource_None;
    return NvSuccess;
}

/**
 * Ack the interrupt source occured from spdif channels.
 */
NvError
NvDdkPrivT30SpdifAckInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource IntSource)
{
    // Get the abpbif fifo status register value.
    return NvSuccess;
}

void
NvDdkPrivT30SpdifGetStandbyRegisters(
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
}

void
NvDdkPrivT30SpdifSetStandbyRegisters(
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
}
#endif //!NV_IS_AVP

