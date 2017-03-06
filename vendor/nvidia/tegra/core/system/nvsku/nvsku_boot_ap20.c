/*
 * Copyright (c) 2011-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nvbct.h"
#include "nvsku.h"
#include "nvbct_nvinternal_ap20.h"
#include "nvmachtypes.h"
#include "nvodm_query.h"
#include "nvsku_private.h"
#include "nvfuse.h"
#include "ap20/nvboot_bct.h"

#define NVSKU(x) x

static NvError NvBctGetSkuInfo(NvSkuInfo *SkuInfo)
{
    NvError e = NvSuccess;
    NvInternalOneTimeRaw InternalInfoOneTimeRaw;
    NvBctHandle hBct = NULL;
    NvU32 Size = 0, Instance = 0;

    NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &hBct));

    NV_CHECK_ERROR_CLEANUP(NvBctGetData(hBct, NvBctDataType_InternalInfoOneTimeRaw,
                &Size, &Instance, NULL));
    NV_CHECK_ERROR_CLEANUP(NvBctGetData(hBct, NvBctDataType_InternalInfoOneTimeRaw,
                &Size, &Instance, (void *)&InternalInfoOneTimeRaw));

    NvOsMemset(SkuInfo, 0, sizeof(NvSkuInfo));
    NvOsMemcpy(&SkuInfo->Version, &(InternalInfoOneTimeRaw.SkuInfoRaw.Version),
            sizeof(SkuInfo->Version));
    NvOsMemcpy(&SkuInfo->TestConfig, &(InternalInfoOneTimeRaw.SkuInfoRaw.TestConfig),
            sizeof(SkuInfo->TestConfig));
    NvOsMemcpy(&SkuInfo->BomPrefix, &(InternalInfoOneTimeRaw.SkuInfoRaw.BomPrefix[0]),
            sizeof(SkuInfo->BomPrefix));
    NvOsMemcpy(&SkuInfo->Project, &(InternalInfoOneTimeRaw.SkuInfoRaw.Project[0]),
            sizeof(SkuInfo->Project));
    NvOsMemcpy(&SkuInfo->SkuId, &(InternalInfoOneTimeRaw.SkuInfoRaw.SkuId[0]),
            sizeof(SkuInfo->SkuId));
    NvOsMemcpy(&SkuInfo->Revision, &(InternalInfoOneTimeRaw.SkuInfoRaw.Revision[0]),
            sizeof(SkuInfo->Revision));
    NvOsMemcpy(&SkuInfo->SkuVersion[0], &(InternalInfoOneTimeRaw.SkuInfoRaw.SkuVersion[0]),
            sizeof(SkuInfo->SkuVersion));

fail:
    NvBctDeinit(hBct);
    return e;
}

static NvError GuessSku (NvU32 *pSku)
{
    NvError e = NvSuccess;
    NvBctHandle hBct = NULL;
    NvBitHandle hBit = NULL;
    NvBitSecondaryBootDevice NvBitBootDev;
    NvBootDevType  BootDev = NvBootDevType_None;
    NvU32 BootDevice;
    NvU32 CustomerOption = 0;
    NvU32 Size = 0, Instance;

    NvFuseGetSecondaryBootDevice(&BootDevice, &Instance);
    NvBitBootDev = NvBitGetBootDevTypeFromBitData(BootDevice);

    NV_CHECK_ERROR_CLEANUP(
            NvBitInit(&hBit)
            );

    NV_CHECK_ERROR_CLEANUP(
            NvBitGetData(hBit, NvBitDataType_SecondaryDevice, &Size,
                &Instance, NULL)
            );
    NV_CHECK_ERROR_CLEANUP(
            NvBitGetData(hBit, NvBitDataType_SecondaryDevice, &Size,
                &Instance, &BootDev)
            );

    if ( NvBitBootDev == NvBitBootDevice_Nand8 ||
            NvBitBootDev == NvBitBootDevice_Nand16 ||
                BootDev == NvBootDevType_Nand ||
                    BootDev == NvBootDevType_Nand_x16)
    {
        NvU32 RamSize;

        NV_CHECK_ERROR_CLEANUP(NvBctInit(&Size, NULL, &hBct));

        Size = 0;
        Instance = 0;
        NV_CHECK_ERROR_CLEANUP(
                NvBctGetData(hBct, NvBctDataType_OdmOption,
                    &Size, &Instance, NULL)
                );

        if ( (Size * Instance) != sizeof(CustomerOption) )
        {
            e = NvError_InvalidSize;
            goto fail;
        }

        NV_CHECK_ERROR_CLEANUP(
                NvBctGetData(hBct, NvBctDataType_OdmOption,
                    &Size, &Instance, &CustomerOption)
                );

        RamSize = NvOdmQueryMemSize(NvOdmMemoryType_Sdram);
        switch ( RamSize )
        {
            case 0x20000000:
                // 512MB SKU3
                *pSku =  TEGRA_P852_SKU3_A00;
                break;
            case 0x40000000:
                // 1GB SKU13
                *pSku =  TEGRA_P852_SKU13_A00;
                break;

            default:
                // Fallback to generic
                *pSku = MACHINE_TYPE_TEGRA_GENERIC;
                break;
        }
    }
    else
    {
        // Default is SKU11 since it has 18 bit display
        // This works for 24 bit too (SKU1). hw stuffs bits.
        *pSku = TEGRA_P852_SKU11_A00;
    }

fail:
    NvBctDeinit(hBct);
    NvBitDeinit(hBit);
    return e;
}

NvError NvBootGetSkuNumber(NvU32 *pSku)
{
    NvError e = NvSuccess;
    NvSkuInfo SkuInfo;

    NV_CHECK_ERROR_CLEANUP(NvBctGetSkuInfo(&SkuInfo));

    if ( SkuInfo.Version == 0 )
    {
        NV_CHECK_ERROR_CLEANUP(GuessSku(pSku));
    }
    else
    {
        if ( SkuInfo.Project != EMBEDDED_P852_PROJECT )
        {
            e = NvError_BadValue;
            goto fail;
        }

        switch (SkuInfo.SkuId)
        {
            case NVSKU(3):

                if (SkuInfo.Revision >= 100)
                {
                    *pSku = TEGRA_P852_SKU3_A00;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;

            case NVSKU(13):

                if (SkuInfo.Revision >= 100 && SkuInfo.Revision < 200)
                {
                    *pSku = TEGRA_P852_SKU13_A00;
                }
                else if(SkuInfo.Revision >= 200 && SkuInfo.Revision < 400)
                {
                    *pSku = TEGRA_P852_SKU13_B00;
                }
                else if(SkuInfo.Revision >= 400)
                {
                    *pSku = TEGRA_P852_SKU13_C01;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;

            case NVSKU(5):

                if (SkuInfo.Revision >= 201 && SkuInfo.Revision < 400)
                {
                    *pSku = TEGRA_P852_SKU5_B00;
                }
                else if(SkuInfo.Revision >= 400)
                {
                    *pSku = TEGRA_P852_SKU5_C01;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;

            case NVSKU(23):

                if (SkuInfo.Revision >= 100 && SkuInfo.Revision < 200)
                {
                    *pSku = TEGRA_P852_SKU23_A00;
                }
                else if(SkuInfo.Revision >= 200 && SkuInfo.Revision < 400)
                {
                    *pSku = TEGRA_P852_SKU23_B00;
                }
                else if(SkuInfo.Revision >= 400)
                {
                    *pSku = TEGRA_P852_SKU23_C01;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;

            case NVSKU(24):

                if(SkuInfo.Revision >= 400)
                {
                    *pSku = TEGRA_P852_SKU24_C01;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;

            case NVSKU(25):

                if(SkuInfo.Revision >= 400)
                {
                    *pSku = TEGRA_P852_SKU25_C01;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;

            case NVSKU(1):
                if (SkuInfo.Revision == 100)
                {
                    *pSku = TEGRA_P852_SKU1_A00;
                }
                else if (SkuInfo.Revision >= 200 && SkuInfo.Revision <= 209)
                {
                    *pSku = TEGRA_P852_SKU1_B00;
                }
                else if (SkuInfo.Revision >= 300)
                {
                    *pSku = TEGRA_P852_SKU1_C0x;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;
            case NVSKU(11):
                  switch (SkuInfo.Revision) {
                    case 100:
                        *pSku = TEGRA_P852_SKU11_A00;
                        break;
                    default:
                        e = NvError_BadValue;
                        goto fail;
                }
                break;
            case NVSKU(8):
                if (SkuInfo.Revision >= 200 && SkuInfo.Revision <= 209)
                {
                    *pSku = TEGRA_P852_SKU8_B00;
                }
                else if (SkuInfo.Revision >= 400)
                {
                    *pSku = TEGRA_P852_SKU8_C01;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;
            case NVSKU(9):
                if (SkuInfo.Revision >= 200 && SkuInfo.Revision <= 209)
                {
                    *pSku = TEGRA_P852_SKU9_B00;
                }
                else if (SkuInfo.Revision >= 400)
                {
                    *pSku = TEGRA_P852_SKU9_C01;
                }
                else
                {
                    e = NvError_BadValue;
                    goto fail;
                }
                break;

            default:
                e = NvError_BadValue;
                goto fail;
        }
    }

fail:
    return e;
}

NvError NvBootGetSkuInfo(NvU8 *pInfo, NvU32 Stringify, NvU32 strlength)
{
    NvError e = NvSuccess;

    NvSkuInfo SkuInfo;

    NV_CHECK_ERROR_CLEANUP(NvBctGetSkuInfo(&SkuInfo));
    if (Stringify)
    {
        /* Use NvBootGetBaseSkuInfoVersion if needed */
        NvOsSnprintf((char*)pInfo, strlength, "nvsku=%03d-%05d-%04d-%03d SkuVer=%c%c",
                SkuInfo.BomPrefix, SkuInfo.Project,
                SkuInfo.SkuId, SkuInfo.Revision, SkuInfo.SkuVersion[0],
                SkuInfo.SkuVersion[1]);
    }
    else
    {
        NvOsMemcpy(pInfo, &SkuInfo, sizeof(NvSkuInfo));
    }

fail:
    return e;
}
