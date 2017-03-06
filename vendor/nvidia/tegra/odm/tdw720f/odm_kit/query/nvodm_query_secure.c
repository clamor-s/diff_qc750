/*
 * Copyright (c) 2009-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_query.h"
#include "nvodm_services.h"
#include "nvodm_keylist_reserved.h"
#include "nvrm_drf.h"
#include "tegra_devkit_custopt.h"

/* --- Function Implementations ---*/
static NvU32 GetBctCustOptKeyValue(void)
{
    NvOdmServicesKeyListHandle hKeyList = NULL;
    NvU32 BctCustOpt = 0;

    hKeyList = NvOdmServicesKeyListOpen();
    if (hKeyList)
    {
        BctCustOpt = NvOdmServicesGetKeyValue(hKeyList,
                         NvOdmKeyListId_ReservedBctCustomerOption);
        NvOdmServicesKeyListClose(hKeyList);
    }

    return BctCustOpt;
}

NvU32 NvOdmQueryOsMemSize(NvOdmMemoryType MemType, const NvOdmOsOsInfo *pOsInfo)
{
    NvU32 SdramBctCustOpt;
    NvU32 SdramSize;

    if (pOsInfo == NULL)
    {
        return 0;
    }

    switch (MemType)
    {
    case NvOdmMemoryType_Sdram:
        SdramBctCustOpt = GetBctCustOptKeyValue();
        switch (NV_DRF_VAL(TEGRA_DEVKIT, BCT_SYSTEM, MEMORY, SdramBctCustOpt))
        {
            case TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_1:
                SdramSize = 0x10000000; //256 MB
                break;
            case TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_3:
                SdramSize = 0x40000000; //1GB
                break;
            case TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_2:
            case TEGRA_DEVKIT_BCT_SYSTEM_0_MEMORY_DEFAULT:
            default:
                SdramSize = 0x20000000; //512 MB
                break;
        }
        return SdramSize;
    case NvOdmMemoryType_Nor:
    case NvOdmMemoryType_Nand:
    case NvOdmMemoryType_I2CEeprom:
    case NvOdmMemoryType_Hsmmc:
    case NvOdmMemoryType_Mio:
    default:
        return 0;
    }
}

NvU32 NvOdmQueryMemSize(NvOdmMemoryType MemType)
{
    NvOdmOsOsInfo Info;

    // Get the information about the calling OS.
    (void)NvOdmOsGetOsInformation(&Info);

    return NvOdmQueryOsMemSize(MemType, &Info);
}

NvU32 NvOdmQueryCarveoutSize(void)
{
    return 0x08000000; // 128 MB
}

NvU32 NvOdmQuerySecureRegionSize(void)
{
#ifdef CONFIG_TRUSTED_FOUNDATIONS
    return 0x200000; // 2 MB
#else
    return 0x0;
#endif
}

NvU32 NvOdmQueryDataPartnEncryptFooterSize(const char *name)
{
    /*
     * the size of the crypto footer for data partitions on android
     * is 16KB. we double it, to make it future-proof.
     */
    if (NvOdmOsStrcmp(name, "UDA") == 0)
        return (16384 * 2);
    else
        return 0;
}
