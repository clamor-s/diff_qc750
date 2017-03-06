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

#ifndef INCLUDED_T30DDKSPDIF_HW_PRIVATE_H
#define INCLUDED_T30DDKSPDIF_HW_PRIVATE_H

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

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Initialize the spdif register.
 */
NvError
NvDdkPrivT30SpdifHwInitialize(
        NvU32 SpdifChannelId,
        SpdifHwRegisters *pSpdifHwRegs);

/**
 * Set the the trigger level for receive/transmit fifo.
 */
NvError
NvDdkPrivT30SpdifSetTriggerLevel(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwFifo FifoType,
    NvU32 TriggerLevel);

#if !NV_IS_AVP
/**
 * Reset the rx/tx fifo.
 */

NvError
    NvDdkPrivT30SpdifResetFifo(
        SpdifHwRegisters *pSpdifHwRegs,
        SpdifHwFifo FifoType);

/**
 * Enable/Disable the interrupt source.
 */
NvError
NvDdkPrivT30SpdifSetInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource IntSource,
    NvBool IsEnable);

/**
 * Get the interrupt source occured from spdif channels.
 */
NvError
NvDdkPrivT30SpdifGetInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource *pIntSource);

/**
 * Ack the interrupt source occured from spdif channels.
 */
NvError
NvDdkPrivT30SpdifAckInteruptSource(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifHwInterruptSource IntSource);

/*
 * Return the SPDIF registers to be saved before standby entry.]
 */
void
NvDdkPrivT30SpdifGetStandbyRegisters(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifStandbyRegisters *pStandbyRegs);

/*
 * Return the SPDIF registers to be saved before standby entry.]
 */
void
NvDdkPrivT30SpdifSetStandbyRegisters(
    SpdifHwRegisters *pSpdifHwRegs,
    SpdifStandbyRegisters *pStandbyRegs);

#endif // !NV_IS_AVP
/** @}*/

#if defined(__cplusplus)
}
#endif

#endif  // INCLUDED_T30DDKSPDIF_HW_PRIVATE_H
