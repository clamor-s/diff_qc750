/*
 * Copyright (c) 2009-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "bootloader.h"
#include "ap20/arapbpm.h"
#include "ap20/nvboot_bit.h"
#include "ap20/nvboot_version.h"
#include "nvbct.h"
#include "aos_ap.h"

#define SUPPORTED_VERSION_INT(x) \
    (((x)==NVBOOT_VERSION(2,3)) || \
     ((x)==NVBOOT_VERSION(2,2)) || ((x)==NVBOOT_VERSION(2,1)))

#define SUPPORTED_VERSION(x)                            \
    (SUPPORTED_VERSION_INT((x)->BootRomVersion) &&      \
     (x)->DataVersion == NVBOOT_VERSION(2,1) &&         \
     (x)->RcmVersion == NVBOOT_VERSION(2,1))

static NvBool nvaosAp20IsValidBit(const NvBootInfoTable *pBootInfo)
{
    if (SUPPORTED_VERSION(pBootInfo) &&
        (pBootInfo->BootType == NvBootType_Cold ||
         pBootInfo->BootType == NvBootType_Recovery) &&
        (pBootInfo->PrimaryDevice == NvBootDevType_Irom))
        return NV_TRUE;
    return NV_FALSE;
}

void nvaosAp20InitPmcScratch(void)
{
    NvBctHandle Bct = NULL;
    NvU32 CustomerOption = 0;
    NvU32 Size = 0, Instance = 0;
    volatile NvU8 *pPmc;
    const NvBootInfoTable *pBootInfo;
    NvError e;
    NvU32 i;

    pBootInfo = (const NvBootInfoTable *)(NV_BIT_ADDRESS);
    if (!nvaosAp20IsValidBit(pBootInfo))
        goto fail;

    NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &Bct));
    if (pBootInfo->BctPtr)
    {
        Size = sizeof(CustomerOption);
        NV_CHECK_ERROR_CLEANUP(
             NvBctGetData(Bct, NvBctDataType_OdmOption,
                 &Size, &Instance, &CustomerOption)
        );
    }
 fail:
    if (Bct)
        NvBctDeinit(Bct);
    pPmc = (volatile NvU8 *)(PMC_PA_BASE);

    //  SCRATCH0 is initialized by the boot ROM and shouldn't be cleared
    for (i=APBDEV_PMC_SCRATCH1_0; i<=APBDEV_PMC_SCRATCH23_0; i+=4)
    {
        if (i==APBDEV_PMC_SCRATCH20_0)
            NV_WRITE32(pPmc+i, CustomerOption);
        else
            NV_WRITE32(pPmc+i, 0);
    }
}

NvU32 nvaosAp20GetOdmData(void)
{
    const NvBootInfoTable *pBootInfo = NULL;
    NvBctHandle Bct = NULL;
    NvU32 CustomerOption = 0;
    NvU32 Size = 0;
    NvU32 Instance = 0;
    volatile NvU8 *pPmc;
    NvError e = NvError_BadValue;

    pBootInfo = (const NvBootInfoTable *)NV_BIT_ADDRESS;
    pPmc = (volatile NvU8 *)(PMC_PA_BASE);
    if (NvBlIsMmuEnabled())
    {
        CustomerOption = NV_READ32(pPmc + APBDEV_PMC_SCRATCH20_0);
        return CustomerOption;
    }
    if (nvaosAp20IsValidBit(pBootInfo))
    {
        NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &Bct));
        if (pBootInfo->BctPtr != NULL)
        {
            Size = sizeof( CustomerOption );
            NV_CHECK_ERROR_CLEANUP(
                NvBctGetData(Bct, NvBctDataType_OdmOption,
                     &Size, &Instance, &CustomerOption));
        }
    }
fail:
    if (Bct)
    {
        NvBctDeinit(Bct);
    }
    return CustomerOption;
}

