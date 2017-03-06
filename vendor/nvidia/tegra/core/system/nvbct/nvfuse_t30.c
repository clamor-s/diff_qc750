/*
 * Copyright (c) 2010-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "t30/nvboot_bit.h"
#include "t30/nvboot_fuse.h"
#include "nvassert.h"
#include "nvbl_operatingmodes.h"
#include "t30/arfuse.h"
#include "nvos.h"
#include "t30/arapb_misc.h"
#include "nvrm_drf.h"
#include "nvrm_hardware_access.h"
#include "nvfuse_private.h"
#if NV_USE_FUSE_CLOCK_ENABLE
#include "t30/t30rm_clocks.h"
#include "nvbl_rm.h"
#endif

#if NVODM_BOARD_IS_SIMULATION
    NvU32 nvflash_get_devid(void);

static NvBool T30IsOdmProductionMode(void)
{
    return NV_FALSE;
}

static NvBool T30IsNvProductionMode(void)
{
    return NV_TRUE;
}

static void T30GetSecBootDevice(NvU32* DeviceId, NvU32* Instance)
{
    *DeviceId = NvBootDevType_Sdmmc;
    //for ap20 device instace is hardcoded to 3 if boot device is sdmmc
    if(*DeviceId == NvBootDevType_Sdmmc)
        *Instance = 3;
}

NvBool NvFuseGetT30Hal(NvFuseHal *pHal)
{
    NvU32 ChipId;
    ChipId =nvflash_get_devid();
    if (!ChipId)
        goto fail;
    if (ChipId == 0x30)
    {
        pHal->IsOdmProductionMode = T30IsOdmProductionMode;
        pHal->IsNvProductionMode = T30IsNvProductionMode;
        pHal->GetSecBootDevice = T30GetSecBootDevice;
        return NV_TRUE;
    }

fail:
    return NV_FALSE;
}

#else

#define MISC_PA_BASE 0x70000000UL
#define MISC_PA_LEN  4096
#define NV_ADDRESS_MAP_FUSE_BASE 0x7000F800UL
#define BOOT_DEVICE_CONFIG_INSTANCE_SHIFT 7

#define FUSE_BOOT_DEVICE_INFO_0_BOOT_DEVICE_CONFIG_RANGE 13:0
#define FUSE_RESERVED_SW_0_BOOT_DEVICE_SELECT_RANGE       2:0
#define FUSE_RESERVED_SW_0_SKIP_DEV_SEL_STRAPS_RANGE      3:3
#define FUSE_RESERVED_SW_0_SW_RESERVED_RANGE              7:4

/*
 * NvStrapDevSel: Enumerated list of devices selectable via straps.
 */
typedef enum
{
    NvStrapDevSel_Emmc_Primary_x4 = 0, /* eMMC primary (x4)          */
    NvStrapDevSel_Emmc_Primary_x8,     /* eMMC primary (x8)          */
    NvStrapDevSel_Emmc_Secondary_x4,   /* eMMC secondary (x4)        */
    NvStrapDevSel_Nand,                /* NAND (x8 or x16)           */
    NvStrapDevSel_Nand_42nm_x8,           /* NAND_42nm (x8)           */
    NvStrapDevSel_MobileLbaNand,       /* mobileLBA NAND             */
    NvStrapDevSel_MuxOneNand,          /* MuxOneNAND                 */
    NvStrapDevSel_Esd_x4,              /* eSD (x4)                   */
    NvStrapDevSel_SpiFlash,            /* SPI Flash                  */
    NvStrapDevSel_Snor_Muxed_x16,      /* Sync NOR (Muxed, x16)      */
    NvStrapDevSel_Snor_Muxed_x32,      /* Sync NOR (Muxed, x32)      */
    NvStrapDevSel_Snor_NonMuxed_x16,   /* Sync NOR (NonMuxed, x16)   */
    NvStrapDevSel_FlexMuxOneNand,      /* FlexMuxOneNAND             */
    NvStrapDevSel_Sata,                /* Sata                       */
    NvStrapDevSel_Emmc_Secondary_x8,   /* eMMC secondary (x8)        */
    NvStrapDevSel_UseFuses,            /* Use fuses instead          */

    /* The following definitions must be last. */
    NvStrapDevSel_Num, /* Must appear after the last legal item */
    NvStrapDevSel_Force32 = 0x7fffffff
} NvStrapDevSel;

static NvBootDevType MapFuseDevToBitDevType[] =
{
    NvBootDevType_Sdmmc,         // NvBootFuseBootDevice_Sdmmc
    NvBootDevType_Snor,          // NvBootFuseBootDevice_SnorFlash
    NvBootDevType_Spi,           // NvBootFuseBootDevice_SpiFlash
    NvBootDevType_Nand,          // NvBootFuseBootDevice_NandFlash
    NvBootDevType_MobileLbaNand, // NvBootFuseBootDevice_MobileLbaNand
    NvBootDevType_MuxOneNand,    // NvBootFuseBootDevice_MuxOneNand
    NvBootDevType_Sata,           // NvBootFuseBootDevice_Sata
    NvBootDevType_Sdmmc3,        // NvBootFuseBootDevice_BootRom_Reserved_Sdmmc3
};

static NvBootFuseBootDevice StrapDevSelMap[] =
{
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Emmc_Primary_x4
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Emmc_Primary_x8
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Emmc_Secondary_x4
    NvBootFuseBootDevice_NandFlash,     // NvStrapDevSel_Nand
    NvBootFuseBootDevice_NandFlash,     // NvStrapDevSel_Nand_42nm_x8
    NvBootFuseBootDevice_MobileLbaNand, // NvStrapDevSel_MobileLbaNand
    NvBootFuseBootDevice_MuxOneNand,    // NvStrapDevSel_MuxOneNand
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Esd_x4
    NvBootFuseBootDevice_SpiFlash,      // NvStrapDevSel_SpiFlash
    NvBootFuseBootDevice_SnorFlash,     // NvStrapDevSel_Snor_Muxed_x16
    NvBootFuseBootDevice_SnorFlash,     // NvStrapDevSel_Snor_Muxed_x32
    NvBootFuseBootDevice_SnorFlash,     // NvStrapDevSel_Snor_NonMuxed_x16
    NvBootFuseBootDevice_MuxOneNand,    // NvStrapDevSel_FlexMuxOneNand
    NvBootFuseBootDevice_Sata,          // NvStrapDevSel_Sata
    NvBootFuseBootDevice_Sdmmc,         // NvStrapDevSel_Reserved_0xe
    NvBootFuseBootDevice_Sdmmc          // NvStrapDevSel_UseFuses
};


static NvU32 GetMajorVersion(void);
static NvBool IsNvProductionModeFuseSet(void);
static NvBool IsOdmProductionModeFuseSet(void);
static NvBool T30IsFailureAnalysisMode(void);
static NvBool T30IsOdmProductionMode(void);
static NvBool T30IsNvProductionMode(void);
static void T30GetSecBootDevice(NvU32* DeviceId, NvU32* Instance);
static NvBool NvFuseSkipDevSelStraps(void);
static NvStrapDevSel NvStrapDeviceSelection(void);

static NvU32 GetMajorVersion(void)
{
    NvU32 regVal = 0;
    NvU32 majorId = 0;
    #define NV_ADDRESS_MAP_MISC_BASE            1879048192
    regVal = NV_READ32(NV_ADDRESS_MAP_MISC_BASE + APB_MISC_GP_HIDREV_0) ;
    majorId = NV_DRF_VAL(APB_MISC, GP_HIDREV, MAJORREV, regVal);
    #undef NV_ADDRESS_MAP_MISC_BASE
    return majorId;
}

static NvBool IsNvProductionModeFuseSet(void)
{
    NvU32 RegData;

// Enable fuse clock
#if NV_USE_FUSE_CLOCK_ENABLE
    T30EnableModuleClock(gs_RmDeviceHandle,
                          NvRmModuleID_Fuse, NV_TRUE);
#endif
    RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_PRODUCTION_MODE_0);
// Disable fuse clock
#if NV_USE_FUSE_CLOCK_ENABLE
    T30EnableModuleClock(gs_RmDeviceHandle,
                          NvRmModuleID_Fuse, NV_FALSE);
#endif
    if (RegData)
        return NV_TRUE;
    else
        return NV_FALSE;
}


static NvBool IsOdmProductionModeFuseSet(void)
{
    NvU32 RegData;

// Enable fuse clock
#if NV_USE_FUSE_CLOCK_ENABLE
    T30EnableModuleClock(gs_RmDeviceHandle,
                          NvRmModuleID_Fuse, NV_TRUE);
#endif
    RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_SECURITY_MODE_0);
// Disable fuse clock
#if NV_USE_FUSE_CLOCK_ENABLE
    T30EnableModuleClock(gs_RmDeviceHandle,
                          NvRmModuleID_Fuse, NV_FALSE);
#endif
    if (RegData)
        return NV_TRUE;
    else
        return NV_FALSE;
}


static NvBool T30IsFailureAnalysisMode(void)
{
    volatile NvU32 RegData;

// Enable fuse clock
#if NV_USE_FUSE_CLOCK_ENABLE
    T30EnableModuleClock(gs_RmDeviceHandle,
                          NvRmModuleID_Fuse, NV_TRUE);
#endif
    RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_FA_0);
// Disable fuse clock
#if NV_USE_FUSE_CLOCK_ENABLE
    T30EnableModuleClock(gs_RmDeviceHandle,
                          NvRmModuleID_Fuse, NV_FALSE);
#endif
    if (RegData)
        return NV_TRUE;
    else
        return NV_FALSE;
}


static NvBool T30IsOdmProductionMode(void)
{
    if (!T30IsFailureAnalysisMode()
         && IsOdmProductionModeFuseSet())
    {
        return NV_TRUE;
    }
    else
    {
        return NV_FALSE;
    }
}

static NvBool T30IsNvProductionMode(void)
{
    if (GetMajorVersion() == 0)
        return NV_TRUE;

    if (!T30IsFailureAnalysisMode()     &&
         IsNvProductionModeFuseSet() &&
        !IsOdmProductionModeFuseSet())
    {
        return NV_TRUE;
    }
    else
    {
        return NV_FALSE;
    }
}

NvU32 T30GetBootDeviceConfig(void)
{
    NvU32 RegData;

    RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_BOOT_DEVICE_INFO_0);
    RegData = NV_DRF_VAL(FUSE, BOOT_DEVICE_INFO, BOOT_DEVICE_CONFIG, RegData);

    return RegData;
}

static void T30GetSecBootDevice(NvU32* DeviceId, NvU32* Instance)
{

    NvU32 RegData;
    NvStrapDevSel StrapDevSel;

    *Instance = 0;
    // Read device selcted using the strap
    StrapDevSel = NvStrapDeviceSelection();

    /* Determine if the fuse data should be overwritten by strap data. */
    if (!NvFuseSkipDevSelStraps() && StrapDevSel != NvStrapDevSel_UseFuses)
    {
        /* Handle out-of-bound values */
        if (StrapDevSel >= NvStrapDevSel_Num)
            StrapDevSel = NvStrapDevSel_Emmc_Primary_x4;

        RegData     = StrapDevSelMap[StrapDevSel];
    }
    else
    {
        // Enable fuse clock
#if NV_USE_FUSE_CLOCK_ENABLE
        T30EnableModuleClock(gs_RmDeviceHandle,
                  NvRmModuleID_Fuse, NV_TRUE);
#endif

        RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_RESERVED_SW_0);
        RegData = NV_DRF_VAL(FUSE, RESERVED_SW, BOOT_DEVICE_SELECT, RegData);

        // Disable fuse clock
#if NV_USE_FUSE_CLOCK_ENABLE
        T30EnableModuleClock(gs_RmDeviceHandle,
                  NvRmModuleID_Fuse, NV_FALSE);
#endif
    }

    if (RegData >= (int) NvBootFuseBootDevice_Max)
    {
        *DeviceId = (NvU32)NvBootDevType_None;
    }
    else
    {
        *DeviceId = (NvU32)MapFuseDevToBitDevType[RegData];
    }

    if (*DeviceId == NvBootDevType_Sdmmc)
    {
        // Instance of sdmmc is stored in boot deivce config fuse.
        // Read this fuse to find the instance.
        RegData = T30GetBootDeviceConfig();
        if (RegData & (1 << BOOT_DEVICE_CONFIG_INSTANCE_SHIFT))
            *Instance = 2;
        else
            *Instance = 3;
    }
    if (*DeviceId == NvBootDevType_Spi)
    {
        *Instance = 3;
    }
}


NvBool NvFuseGetT30Hal(NvFuseHal *pHal)
{
    NvU8 *pMisc;
    NvU32 RegData;
    NvError e;

    NV_CHECK_ERROR_CLEANUP(
        NvOsPhysicalMemMap(MISC_PA_BASE, MISC_PA_LEN,
            NvOsMemAttribute_Uncached, NVOS_MEM_READ_WRITE, (void **)&pMisc)
    );

    RegData = NV_READ32(pMisc + APB_MISC_GP_HIDREV_0);

    NvOsPhysicalMemUnmap(pMisc, MISC_PA_LEN);

    if (NV_DRF_VAL(APB_MISC_GP, HIDREV, CHIPID, RegData)==0x30)
    {
        pHal->IsOdmProductionMode = T30IsOdmProductionMode;
        pHal->IsNvProductionMode = T30IsNvProductionMode;
        pHal->GetSecBootDevice = T30GetSecBootDevice;
        return NV_TRUE;
    }

 fail:
    return NV_FALSE;
}

static NvBool NvFuseSkipDevSelStraps(void)
{
    NvU32 RegData;

    RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_RESERVED_SW_0);
    RegData = NV_DRF_VAL(FUSE, RESERVED_SW, SKIP_DEV_SEL_STRAPS, RegData);

    if (RegData)
        return NV_TRUE;
    else
        return NV_FALSE;
}

static NvStrapDevSel NvStrapDeviceSelection(void)
{
    NvU32 RegData;
    NvU8 *pMisc;
    NvError e;

    NV_CHECK_ERROR(
        NvOsPhysicalMemMap(MISC_PA_BASE, MISC_PA_LEN,
            NvOsMemAttribute_Uncached, NVOS_MEM_READ_WRITE, (void **)&pMisc));

    RegData = NV_READ32(pMisc + APB_MISC_PP_STRAPPING_OPT_A_0);

    NvOsPhysicalMemUnmap(pMisc, MISC_PA_LEN);

    return (NvStrapDevSel)(NV_DRF_VAL(APB_MISC_PP,
                                          STRAPPING_OPT_A,
                                          BOOT_SELECT,
                                          RegData));
}
#endif

