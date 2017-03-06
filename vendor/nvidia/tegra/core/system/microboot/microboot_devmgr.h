/*
 * Copyright (c) 2010 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_MICROBOOT_DEVMGR_H
#define INCLUDED_MICROBOOT_DEVMGR_H
#include "microboot_devmgr_headers.h"
#include "nvboot_bit.h"
#include "nvboot_nand_context.h"
#include "nvboot_spi_flash_context.h"

typedef union
{
    NvBootNandContext      NandContext;
    NvBootSpiFlashContext  SpiFlashContext;
} NvMicrobootDevContext;

typedef struct NvMicrobootDevMgrCallbacksRec
{
    NvBootDeviceGetParams      GetParams;
    NvBootDeviceValidateParams ValidateParams;
    NvBootDeviceGetBlockSizes  GetBlockSizes;
    NvBootDeviceInit           Init;
    NvBootDeviceReadPage       ReadPage;
    NvBootDeviceQueryStatus    QueryStatus;
    NvBootDeviceShutdown       Shutdown;
    NvBootDeviceReadMultiPage  ReadMulti;
    NvBootDeviceGetPagesPerChunkLog2 PagesPerChunkLog2;
} NvMicrobootDevMgrCallbacks;

/*
 * NvMicrobootDevMgr: State & data used by the device manager.
 */
typedef struct NvMicrobootDevMgrRec
{
    /* Device Info */
    NvU32                   BlockSizeLog2;
    NvU32                   PageSizeLog2;
    NvMicrobootDevMgrCallbacks     *Callbacks;    /* Callbacks to the chosen driver. */
} NvMicrobootDevMgr;

/**
 * NvMicrobootDevMgrInit(): Initialize the device manager, select the boot
 * device, and initialize it.  Upon completion, the device is ready for
 * access.
 *
 * @param[in] DevMgr Pointer to the device manager
 * @param[in] DevType The type of device from the fuses.
 * @param[in] ConfigIndex The device configuration data from the fuses
 *
 * @retval NvBootErrorSuccess The device was successfully initialized.
 * @retval NvBootInvalidParameter The device type was not valid.
 * @retval TODO Error codes from InitDevice()
 */
NvBootError NvMicrobootDevMgrInit(NvMicrobootDevMgr   *DevMgr,
                             NvBootDevType  DevType,
                             NvU32             ConfigIndex);

/**
 * NvMicrobootDevMgrReinit(): Reinitialize the device manager, select the boot
 * device, and initialize it.  Upon completion, the device is ready for
 * access.
 *
 * @param[in] DevMgr Pointer to the device manager
 * @param[in] BctParams Pointer to BCT params
 * @param[in] ParamCount The number of BCT params
 * @param[in] DeviceStraps The device boot strap index, read from fuses
 *
 * @retval NvBootErrorSuccess The device was successfully initialized.
 * @retval NvBootInvalidParameter The device type was not valid.
 */
NvBootError
NvMicrobootDevMgrReinit(NvMicrobootDevMgr    *DevMgr,
             NvBootDevParams *BctParams,
             const NvU8       ParamCount,
             const NvU8       DeviceStraps);

#endif//INCLUDED_MICROBOOT_DEVMGR_H

