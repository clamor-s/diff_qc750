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
 *           Private functions for the spdif Ddk driver</b>
 *
 * @b Description:  Defines the private interfacing functions for the spdif
 * hw interface.
 *
 */

#ifndef INCLUDED_DDKSPDIF_HW_PRIVATE_H
#define INCLUDED_DDKSPDIF_HW_PRIVATE_H

/**
 * @defgroup nvddk_spdif Integrated Inter Sound (SPDIF) Controller hw interface API
 *
 * This is the SPDIF hw interface controller api.
 *
 * @ingroup nvddk_modules
 * @{
 *
 */

#include "nvcommon.h"
#include "nvddk_spdif.h"
#include "nvodm_query.h"
#include "nvrm_dma.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define SPDIF_REG_READ32(pSpdifHwRegsVirtBaseAdd, reg) \
    NV_READ32((pSpdifHwRegsVirtBaseAdd) + ((SPDIF_##reg##_0)/sizeof(NvU32)))

#define SPDIF_REG_WRITE32(pSpdifHwRegsVirtBaseAdd, reg, val) \
    do { \
        NV_WRITE32((((pSpdifHwRegsVirtBaseAdd) + ((SPDIF_##reg##_0)/sizeof(NvU32)))), (val)); \
    } while(0)


//#define NVDDK_SPDIF_POWER_PRINT
//#define NVDDK_SPDIF_CONTROLLER_PRINT
//#define NVDDK_SPDIF_REG_PRINT

#if defined(NVDDK_SPDIF_POWER_PRINT)
    #define NVDDK_SPDIF_POWERLOG(x)       NvOsDebugPrintf x
#else
    #define NVDDK_SPDIF_POWERLOG(x)       do {} while(0)
#endif
#if defined(NVDDK_SPDIF_CONTROLLER_PRINT)
    #define NVDDK_SPDIF_CONTLOG(x)        NvOsDebugPrintf x
#else
    #define NVDDK_SPDIF_CONTLOG(x)        do {} while(0)
#endif
#if defined(NVDDK_SPDIF_REG_PRINT)
    #define NVDDK_SPDIF_REGLOG(x)         NvOsDebugPrintf x
#else
    #define NVDDK_SPDIF_REGLOG(x)         do {} while(0)
#endif

/**
 * Combines the definition of the spdif register and modem signals.
 */
typedef struct SpdifHwRegistersRec
{
    // Spdif channel Id.
    NvU32 ChannelId;

    // Virtual base address of the spdif hw register.
    NvU32 *pRegsVirtBaseAdd;

    // Physical base address of the spdif hw register.
    NvRmPhysAddr RegsPhyBaseAdd;

    // Register bank size.
    NvU32 BankSize;

    // Tx Fifo address.
    NvRmPhysAddr TxFifoAddress;

    // Rx Fifo address.
    NvRmPhysAddr RxFifoAddress;

    // Dma module Id
    NvRmDmaModuleID RmDmaModuleId;

} SpdifHwRegisters;

/**
 * Combines the spdif interrupt sources.
 */
typedef enum
{
    // No Spdif interrupt source.
    SpdifHwInterruptSource_None = 0x0,

    // Receive error interrupt.
    SpdifHwInterruptSource_ReceiveError,

    // Transmit error interrupt.
    SpdifHwInterruptSource_TransmitError,

    // Channel info valid interrupt
    SpdifHwInterruptSource_ChannelInoReceive,

    // User data transmit interrupt
    SpdifHwInterruptSource_UserDataTransmit,

    // User data receive interrupt
    SpdifHwInterruptSource_UserDataReceive,

    SpdifHwInterruptSource_Force32 = 0x7FFFFFFF
} SpdifHwInterruptSource;

typedef enum
{
    SpdifHwDirection_Transmit,
    SpdifHwDirection_Receive,
    SpdifHwDirection_Force32 = 0x7FFFFFFF
} SpdifHwDirection;

/**
 * Combines the spdif hw fifo type.
 */
typedef enum
{
    // Invalid fifo type.
    SpdifHwFifo_Invalid = 0x0,

    // Receive fifo type.
    SpdifHwFifo_Receive,

    // Transmit fifo type.
    SpdifHwFifo_Transmit,

    // Both, receive and transmit fifo type.
    SpdifHwFifo_Both,

    SpdifHwFifo_Force32 = 0x7FFFFFFF
} SpdifHwFifo;

#if !NV_IS_AVP
/*
 * Spdif Registers saved before standby entry
 * and are restored after exit from stanbby mode.
 */
typedef struct SpdifStandbyRegistersRec
{
    NvU32 SpdifHwFifoConfigReg;
    NvU32 SpdifHwDataStrobeCtrlReg;
    NvU32 SpdifHwCtrlReg;
} SpdifStandbyRegisters;

#endif // !NV_IS_AVP

/**
 * Initialize the spdif register.
 */
NvError
NvDdkPrivSpdifHwInitialize(
        NvU32 SpdifChannelId,
        SpdifHwRegisters *pSpdifHwRegs);

#if !NV_IS_AVP

NvError NvDdkPrivSpdifHwFifoInit(SpdifHwRegisters *pSpdifHwRegs);

NvError
NvDdkPrivSpdifSetSamplingFreq(
    SpdifHwRegisters *pSpdifHwRegs,
    NvU32 SamplingRate,
    NvU32 *pClockSourceFreq);
#endif // !NV_IS_AVP

/**
 * Enable/Disable the transmit.
 */
NvError
NvDdkPrivSpdifSetTransmit(SpdifHwRegisters *pSpdifHwRegs, NvBool IsEnable);

/**
 * Enable/Disable the receive.
 */
NvError
NvDdkPrivSpdifSetReceive(SpdifHwRegisters *pSpdifHwRegs, NvBool IsEnable);

/**
 * Set the the trigger level for receive/transmit fifo.
 */
NvError
NvDdkPrivSpdifSetTriggerLevel(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwFifo FifoType,
    NvU32 TriggerLevel);

#if !NV_IS_AVP
/**
 * Reset the rx/tx fifo.
 */
NvError
NvDdkPrivSpdifResetFifo(
        SpdifHwRegisters *pSpdifHwRegs,
        SpdifHwFifo FifoType);

/**
 * Set the fifo format.
 */
NvError NvDdkPrivSpdifSetFifoFormat(SpdifHwRegisters *pSpdifHwRegs,
    NvDdkSpdifDataWordValidBits DataFifoFormat,
    NvU32 DataSize);

/**
 * Set Packed mode or not
 */
NvError
NvDdkPrivSpdifSetPackedDataMode(
    SpdifHwRegisters *pSpdifHwRegs,
    NvDdkSpdifDataWordValidBits DataFifoFormat,
    NvU32 DataSize);

/**
 * Set the data bit size
 */
NvError NvDdkPrivSpdifSetDataBitSize(SpdifHwRegisters *pSpdifHwRegs, NvU32 DataBitSize);

/**
 * Set the Spdif left right capture control.
 */
NvError NvDdkPrivSpdifSetDataCaptureControl(SpdifHwRegisters *pSpdifHwRegs,
    NvOdmQuerySpdifDataCaptureControl SpdifLrControl);

/**
 * Enable/Disable the interrupt source.
 */
NvError
NvDdkPrivSpdifSetInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource IntSource,
    NvBool IsEnable);

/**
 * Get the interrupt source occured from spdif channels.
 */
NvError
NvDdkPrivSpdifGetInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource *pIntSource);

/**
 * Ack the interrupt source occured from spdif channels.
 */
NvError
NvDdkPrivSpdifAckInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource IntSource);

/**
 * Enable the loop back in the spdif channels.
 */
NvError
NvDdkPrivSpdifSetLoopbackTest(SpdifHwRegisters *pSpdifHwRegs, NvBool IsEnable);

NvError
NvDdkPrivSpdifSetUserData(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwDirection operation,
    NvBool IsEnable);

NvError
NvDdkPrivSpdifSetChannelStatus(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwDirection operation,
    NvBool IsEnable);

/*
 * Return the SPDIF registers to be saved before standby entry.
 */
void
NvDdkPrivSpdifGetStandbyRegisters(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifStandbyRegisters *pStandbyRegs);

/*
 * Return the SPDIF registers saved  on standby entry.
 */
void
NvDdkPrivSpdifSetStandbyRegisters(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifStandbyRegisters *pStandbyRegs);

#endif // !NV_IS_AVP
/** @}*/

#if defined(__cplusplus)
}
#endif

#endif  // INCLUDED_DDKSPDIF_HW_PRIVATE_H
