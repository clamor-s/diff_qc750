/*
 * Copyright (c) 2006 - 2007 NVIDIA Corporation.  All rights reserved.
 * 
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file nvboot_fuse_int.h
 *
 * Fuse interface for NvBoot
 *
 * NvBootFuse is NVIDIA's interface for fuse query and programming.
 *
 * Note that fuses have a value of zero in the unburned state; burned 
 * fuses have a value of one.
 *
 */

#ifndef INCLUDED_NVBOOT_FUSE_INT_H
#define INCLUDED_NVBOOT_FUSE_INT_H

#include "ap15/nvboot_fuse.h"
#include "nvboot_error.h"
#include "nvboot_sku_int.h"
#include "nvcommon.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/*
 * NvBootFuseOperatingMode -- The chip's current operating mode
 */
typedef enum
{
    NvBootFuseOperatingMode_None = 0,
    NvBootFuseOperatingMode_Preproduction,
    NvBootFuseOperatingMode_FailureAnalysis,
    NvBootFuseOperatingMode_NvProduction,
    NvBootFuseOperatingMode_OdmProductionSecure,
    NvBootFuseOperatingMode_OdmProductionNonsecure,
    NvBootFuseOperatingMode_Max, /* Must appear after the last legal item */
    NvBootFuseOperatingMode_Force32 = 0x7fffffff
} NvBootFuseOperatingMode;

/**
 * Reports chip's operating mode
 *
 * The Decision Tree is as follows --
 * 1. if Failure Analysis (FA) Fuse burned, then Failure Analysis Mode
 * 2. if ODM Production Fuse burned and ...
 *    a. if SBK Fuses are NOT all zeroes, then ODM Production Mode Secure
 *    b. if SBK Fuses are all zeroes, then ODM Production Mode Non-Secure
 * 3. if NV Production Fuse burned, then NV Production Mode
 * 4. else, Preproduction Mode
 *
 *                                  Fuse Value*
 *                             ----------------------
 * Operating Mode              FA    NV    ODM    SBK
 * --------------              --    --    ---    ---
 * Failure Analysis            1     x     x      x
 * ODM Production Secure       0     x     1      <>0
 * ODM Production Non-Secure   0     x     1      ==0
 * NV Production               0     1     0      x
 * Preproduction               0     0     0      x
 *
 * * where 1 = burned, 0 = unburned, x = don't care
 *
 * @param pMode pointer to buffer where operating mode is to be stored
 *
 * @return void, table is complete decode and so cannot fail
 */
void
NvBootFuseGetOperatingMode(NvBootFuseOperatingMode *pMode);
    
/**
 * Reports whether chip is in Failure Analysis (FA) Mode
 *
 * Failure Analysis Mode means all of the following are true --
 * 1. Failure Analysis Fuse is burned
 *
 * @return NvTrue if chip is in Failure Analysis Mode; else NvFalse
 */
NvBool
NvBootFuseIsFailureAnalysisMode(void);
    
/**
 * Reports whether chip is in ODM Production Mode
 *
 * Production Mode means all of the following are true --
 * 1. FA Fuse is not burned
 * 2. ODM Production Fuse is burned
 *
 * @return NvTrue if chip is in ODM Production Mode; else NvFalse
 */
NvBool
NvBootFuseIsOdmProductionMode(void);

/**
 * Reports whether chip is in ODM Production Mode Secure
 *
 * Production Mode means all of the following are true --
 * 1. chip is in ODM Production Mode
 * 2. Secure Boot Key is not all zeroes
 *
 * @return NvTrue if chip is in ODM Production Mode Secure; else NvFalse
 */
NvBool
NvBootFuseIsOdmProductionModeSecure(void);

/**
 * Reports whether chip is in ODM Production Mode Non-Secure
 *
 * Production Mode means all of the following are true --
 * 1. chip is in ODM Production Mode
 * 2. Secure Boot Key is all zeroes
 *
 * @return NvTrue if chip is in ODM Production Mode Non-Secure; else NvFalse
 */
NvBool
NvBootFuseIsOdmProductionModeNonsecure(void);

/**
 * Reports whether chip is in NV Production Mode
 *
 * Production Mode means all of the following are true --
 * 1. FA Fuse is not burned
 * 2. NV Production Fuse is burned 
 * 3. ODM Production Fuse is not burned
 *
 * @return NvTrue if chip is in NV Production Mode; else NvFalse
 */
NvBool
NvBootFuseIsNvProductionMode(void);

/**
 * Reports whether chip is in Preproduction Mode
 *
 * Pre-production Mode means all of the following are true --
 * 1. FA Fuse is not burned
 * 2. ODM Production Fuse is not burned
 * 3. NV Production Fuse is not burned 
 *
 * @return NvTrue if chip is in Preproduction Mode; else NvFalse
 */
NvBool
NvBootFuseIsPreproductionMode(void);
    
/**
 * Reads Secure Boot Key (SBK) from fuses
 *
 * User must guarantee that the buffer can hold 16 bytes
 *
 * @param pKey pointer to buffer where SBK value will be placed
 *
 * @return void (assume trusted caller, no parameter validation)
 */
void
NvBootFuseGetSecureBootKey(NvU8 *pKey);
    
/**
 * Reads Device Key (DK) from fuses
 *
 * User must guarantee that the buffer can hold 4 bytes
 *
 * @param pKey pointer to buffer where DK value will be placed
 *
 * @return void (assume trusted caller, no parameter validation)
 */
void
NvBootFuseGetDeviceKey(NvU8 *pKey);

/**
 * Hide SBK and DK
 *
 */
void
NvBootFuseHideKeys(void);

    
/**
 * Reads Chip's Unique 64-bit Id (UID) from fuses
 *
 * @param pId pointer to NvU64 where chip's Unique Id number is to be stored
 *
 * @return void (assume trusted caller, no parameter validation)
 */
void
NvBootFuseGetUniqueId(NvU64 *pId);

/**
 * Get Boot Device Id from fuses
 *
 * @param pDev pointer to buffer where ID of boot device is to be stored
 *
 * @return NvBootError_InvalidBootDeviceEncoding if the encoding is not valid
 *         NvBootError_Success otherwise
 */
NvBootError
NvBootFuseGetBootDevice(NvBootFuseBootDevice *pDev);

/**
 * Get Boot Device Configuration Index from fuses
 *
 * @param pConfigIndex pointer to NvU32 where boot device configuration 
 *        index is to be stored
 *
 * @return void ((assume trusted caller, no parameter validation) 
 */
void
NvBootFuseGetBootDeviceConfiguration(NvU32 *pConfigIndex);

/**
 * Get SW reserved field from fuses
 *
 * @param pSwReserved pointer to NvU32 where SwReserved field to be stored 
 *        note this is only 8 bits at this time, but could grow
 */
void
NvBootFuseGetSwReserved(NvU32 *pSwReserved); 

/**
 * Get Marketing-defined SKU ID information from fuses
 *
 * SKU ID is a subset of the full SKU field
 *
 * @param pSkuId pointer to SkuId variable
 *
 * @return void ((assume trusted caller, no parameter validation) 
 */
void
NvBootFuseGetSku(NvBootSku_SkuId *pSkuId); 

/**
 * Get SKU field from fuses
 *
 * @param pSku pointer to NvU32 variable
 *
 * @return void ((assume trusted caller, no parameter validation) 
 */
void
NvBootFuseGetSkuRaw(NvU32 *pSku); 

/**
 * Program (burn) fuses as specified by caller
 *
 * Fuses cannot be burned if the chip is in ODM Production Mode;
 * the NvBootError_DeviceUnsupported error code is returned in 
 * this case.
 *
 * @param pSecureBootKey buffer containing Secure Boot Key (SBK) value
 * @param pDeviceKey buffer containing Device Key (DK) value
 * @param JtagDisableFuse controls the use of ARM JTAG
 * @param BootDeviceSelection controls the boot device (enum)
 * @param BootDeviceConfig define boot device parameters (6 bits)
 * @param SwReserved unallocated field reserved for sw use (6 bits)
 * @param ODMProductionFuse NvTrue to set ODM Production Mode
 * @param SpareBits only 16, using 32 for future etxension)
 * @param TProgramCycles number of clock cycles spent on each fuse (0 means no change)
 *
 * @return NvBootError_Success after fuse burning done (call is blocking)
 *         NvBootError_DeviceUnsupported if fuse burning is not allowed
 *
 */

NvBootError
NvBootFuseProgramFuses(      NvU8 *pSecureBootKey,
                             NvU8 *pDeviceKey,
                       const NvBool JtagDisableFuse,
                       const NvBootFuseBootDevice   BootDeviceSelection,
                       const NvU32  BootDeviceConfig, /* right aligned */
                       const NvU32  SwReserved,
                       const NvBool ODMProductionFuse,
                       const NvU32  SpareBits,
                       const NvU32  TProgramCycles);

/**
 * Verify fuses against expected values (normally after programming)
 *
 * @param pSecureBootKey buffer containing Secure Boot Key (SBK) value
 * @param pDeviceKey buffer containing Device Key (DK) value
 * @param JtagDisableFuse controls the use of ARM JTAG
 * @param BootDeviceSelection controls the boot device (enum)
 * @param BootDeviceConfig define boot device parameters (6 bits)
 * @param SwReserved unallocated field reserved for sw use (6 bits)
 * @param ODMProductionFuse NvTrue to set ODM Production Mode
 * @param SpareBits only 16, using 32 for future etxension)
 *
 * @return NvBootError_ValidationFailure if check fails, i.e.,
 * the post-burning fuse values don't match the requested values
 *         NvBootError_Success otherwise
 */
NvBootError
NvBootFuseVerifyFuses(const NvU8 *pSecureBootKey,
                      const NvU8 *pDeviceKey,
                      const NvBool JtagDisableFuse,
                      const NvBootFuseBootDevice   BootDeviceSelection,
                      const NvU32  BootDeviceConfig, /* right aligned */
                      const NvU32  SwReserved,
                      const NvBool ODMProductionFuse,
                      const NvU32  SpareBits);
    
  
#if defined(__cplusplus)
}
#endif

#endif // INCLUDED_NVBOOT_FUSE_INT_H
