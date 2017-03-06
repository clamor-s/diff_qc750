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
#include "t30/arapbpm.h"
#include "t30/nvboot_bit.h"
#include "t30/nvboot_version.h"
#include "nvbct.h"
#include "nvrm_hardware_access.h"
#include "aos_ap.h"

#define SUPPORTED_VERSION_INT(x) \
    (((x)==NVBOOT_VERSION(3,1)) || ((x)==NVBOOT_VERSION(3,2)))

#define SUPPORTED_VERSION(x)                            \
    (SUPPORTED_VERSION_INT((x)->BootRomVersion) &&      \
     (x)->DataVersion == NVBOOT_VERSION(3,1) &&         \
     (x)->RcmVersion == NVBOOT_VERSION(3,1))

static NvBool nvaosT30IsValidBit(const NvBootInfoTable *pBootInfo)
{
    if (SUPPORTED_VERSION(pBootInfo) &&
        (pBootInfo->BootType == NvBootType_Cold ||
         pBootInfo->BootType == NvBootType_Recovery) &&
        (pBootInfo->PrimaryDevice == NvBootDevType_Irom))
    {
        return NV_TRUE;
    }
    return NV_FALSE;
}

void nvaosT30InitPmcScratch(void)
{
    NvBctHandle Bct = NULL;
    NvU32 CustomerOption = 0;
    NvU32 Size = 0, Instance = 0;
    volatile NvU8 *pPmc;
    const NvBootInfoTable *pBootInfo;
    NvError e;

    pBootInfo = (const NvBootInfoTable *)(NV_BIT_ADDRESS);
    if (!nvaosT30IsValidBit(pBootInfo))
    {
        // The customer option should already have been set in SCRATCH20
        // by the initialization script.
        return;
    }

    NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &Bct));
    if (pBootInfo->BctPtr)
    {
        Size = sizeof(CustomerOption);
        NV_CHECK_ERROR_CLEANUP(
             NvBctGetData(Bct, NvBctDataType_OdmOption,
                 &Size, &Instance, &CustomerOption)
        );
    }
    if (Bct)
    {
        NvBctDeinit(Bct);
    }
fail:
    pPmc = (volatile NvU8 *)(PMC_PA_BASE);
    NV_WRITE32(pPmc + APBDEV_PMC_SCRATCH20_0, CustomerOption);
}

NvU32 nvaosT30GetOdmData(void)
{
    const NvBootInfoTable *pBootInfo = NULL;
    NvBctHandle Bct = NULL;
    NvU32 CustomerOption  = 0;
    NvU32 Size = 0;
    NvU32 Instance = 0;
    volatile NvU8 *pPmc;
    NvError e = NvError_BadValue;

    pPmc = (volatile NvU8 *)(PMC_PA_BASE);
    pBootInfo = (const NvBootInfoTable *)NV_BIT_ADDRESS;
    if (NvBlIsMmuEnabled())
    {
        CustomerOption = NV_READ32(pPmc + APBDEV_PMC_SCRATCH20_0);
        return CustomerOption;
    }
    if (!nvaosT30IsValidBit(pBootInfo))
    {
        // The customer option should already have been set in SCRATCH20
        // by the initialization script.
        CustomerOption = NV_READ32(pPmc + APBDEV_PMC_SCRATCH20_0);
        return CustomerOption;
    }

    NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &Bct));
    if (pBootInfo->BctPtr != NULL)
    {
        Size = sizeof( CustomerOption );
        NV_CHECK_ERROR_CLEANUP(
            NvBctGetData(Bct, NvBctDataType_OdmOption,
                 &Size, &Instance, &CustomerOption));
    }
fail:
    if (Bct)
    {
        NvBctDeinit(Bct);
    }
    return CustomerOption;
}

