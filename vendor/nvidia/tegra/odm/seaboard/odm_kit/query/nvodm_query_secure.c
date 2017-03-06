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
#include "nvrm_drf.h"

/* --- Function Implementations ---*/

NvU32 NvOdmQueryOsMemSize(NvOdmMemoryType MemType, const NvOdmOsOsInfo *pOsInfo)
{
    if (pOsInfo == NULL)
    {
        return 0;
    }

    switch (MemType)
    {
    case NvOdmMemoryType_Sdram:
        return 0x40000000;
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
    return 0;
}

NvU32 NvOdmQueryDataPartnEncryptFooterSize(const char *name)
{
    return 0;
}
