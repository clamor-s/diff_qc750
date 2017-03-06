/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
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
 *           Private functions for the i2s Ddk driver</b>
 *
 * @b Description:  Defines the private interfacing functions for the i2s
 * hw interface.
 *
 */

#ifndef INCLUDED_AP15DDKI2S_HW_PRIVATE_H
#define INCLUDED_AP15DDKI2S_HW_PRIVATE_H

/**
 * @defgroup nvddk_i2s Integrated Inter Sound (I2S) Controller hw interface API
 *
 * This is the I2S hw interface controller api.
 *
 * @ingroup nvddk_modules
 * @{
 *
 */

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Initialize the i2s register.
 */
void
NvDdkPrivI2sHwInitialize(
    NvU32 InstanceId,
    I2sAc97HwRegisters *pI2sHwRegs);

/**
 * Enable/Disable the data flow.
 */
void
NvDdkPrivI2sSetDataFlow(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction Direction,
    NvBool IsEnable);


/**
 * Set the the trigger level for receive/transmit fifo.
 */
void
NvDdkPrivI2sSetTriggerLevel(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction FifoType,
    NvU32 TriggerLevel);

/**
* Set the data flow for dsp mode
*/
void
NvDdkPrivI2sSetDspDataFlow
(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction Direction,
    NvBool IsEnable
);

/**
* set dsp mode mask bits
*/
void
NvDdkPrivI2sSetDspMaskBits
(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvU32 TxMaskBits,
    NvU32 RxMaskBits
);

/**
* set Data communication format
*/
void NvDdkPrivSetDataCommFormat(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvDdkI2SMode I2sMode
);

/**
* set Fsync Width
*/
void
NvDdkPrivI2sSetDspFsyncWidth
(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvU32 FsyncWidth
);
/**
* Set the edge higzh control
*/
void
NvDdkPrivI2sSetDspEdgeMode
(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvDdkI2SHighzMode HighzEdgeMode
);

#if !NV_IS_AVP

/**
 * Get the fifo count.
 */
NvBool
NvDdkPrivI2sGetFifoCount(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction Direction,
    NvU32* pFifoCount);

/**
 * Enable the loop back in the i2s channels.
 */
void NvDdkPrivI2sSetLoopbackTest(I2sAc97HwRegisters *pI2sHwRegs, NvBool IsEnable);

/**
 * Reset the fifo.
 */
void
NvDdkPrivI2sResetFifo(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97Direction Direction);

/*
 * Return the I2s registers to be saved before standby entry.
 */
void
NvDdkPrivI2sGetStandbyRegisters(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97StandbyRegisters *pStandbyRegs);

/*
 * Return the I2s registers to be saved before PowerOn entry.
 */
void
NvDdkPrivI2sSetStandbyRegisters(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sAc97StandbyRegisters *pStandbyRegs);

/**
 * Set the timing ragister based on the clock source frequency.
 */
void
NvDdkPrivI2sSetSamplingFreq(
    I2sAc97HwRegisters *pI2sHwRegs,
    NvU32 SamplingRate,
    NvU32 DatabitsPerLRCLK,
    NvU32 ClockSourceFreq);

void
NvDdkPrivI2sSetInterfaceProperty(
    I2sAc97HwRegisters *pI2sHwRegs,
    void *pInterface);

/**
 * Set the fifo format.
 */
void NvDdkPrivI2sSetFifoFormat(I2sAc97HwRegisters *pI2sHwRegs,
    NvDdkI2SDataWordValidBits DataFifoFormat,
    NvU32 DataSize);

/**
 * Get the interrupt source occured from i2s channels.
 */
I2sHwInterruptSource
NvDdkPrivI2sGetInterruptSource(
    I2sAc97HwRegisters *pI2sHwRegs);

/**
 * Ack the interrupt source occured from i2s channels.
 */
void
NvDdkPrivI2sAckInterruptSource(
    I2sAc97HwRegisters *pI2sHwRegs,
    I2sHwInterruptSource IntSource);

/**
 * Sets the Slot selection mask for Tx or Rx in the Network mode
 * based on IsRecieve flag
 */
void
NvDdkPrivI2sNwModeSetSlotSelectionMask(
            I2sAc97HwRegisters *pHwRegs,
            I2sAc97Direction Direction,
            NvU8 ActiveRxSlotSelectionMask,
            NvBool IsEnable);

/**
 * Sets the number of active slots in TDM mode
 */
void
NvDdkPrivTdmModeSetNumberOfSlotsFxn(
        I2sAc97HwRegisters *pHwRegs,
        NvU32 NumberOfSlotsPerFsync);

/**
 * Sets the Tx or Rx Data word format (LSB first or MSB first)
 */
void
NvDdkPrivTdmModeSetDataWordFormatFxn(
        I2sAc97HwRegisters *pHwRegs,
        NvBool IsRecieve,
        NvDdkI2SDataWordValidBits I2sDataWordFormat);

/**
 * Sets the number of bits per slot in TDM mode
 */
void
NvDdkPrivTdmModeSetSlotSizeFxn(
        I2sAc97HwRegisters *pHwRegs,
        NvU32 SlotSize);

#endif  // !NV_IS_AVP
/** @} */

#if defined(__cplusplus)
}
#endif

#endif  // INCLUDED_AP15DDKI2S_HW_PRIVATE_H
