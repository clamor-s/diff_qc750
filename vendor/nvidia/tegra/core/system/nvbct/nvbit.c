/*
 * Copyright (c) 2008 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvbit.h"
#include "nvassert.h"
#include "nvos.h"
#include "nvbit_private.h"
#include "nvrm_drf.h"
#include "arapb_misc.h"
#include "nvboot_bct.h"  // needed for the NvBootDevType enumerant
#include "nvboot_bit.h"  // needed for the NvBootRdrStatus enumerant

#if NVODM_BOARD_IS_SIMULATION
    NvU32 nvflash_get_devid( void );
#endif

#define NV_BIT_APB_MISC_BASE 0x70000000UL

typedef struct NvRmChipIdRec
{
    NvU16 Id;
    NvU8  Major;
    NvU8  Minor;

#if 0
    /* the following only apply for emulation -- Major will be 0 and
     * Minor is either 0 for quickturn or 1 for fpga
     */
    NvU16 Netlist;
    NvU16 Patch;
#endif
} NvChipId;

//Forward declarations

static NvError NvBitPrivGetFullChipId(NvChipId* id);
static NvError GetDataSize(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *NumInstances);


static NvError NvBitPrivGetFullChipId(NvChipId* id)
{
#if NVODM_BOARD_IS_SIMULATION
    id->Id = (NvU16) nvflash_get_devid();
#else
    volatile NvU8*  pApbMisc = NULL;
    NvU32           reg;
    NvError         e;

    NV_ASSERT(id != NULL);
    NV_CHECK_ERROR(
        NvOsPhysicalMemMap(NV_BIT_APB_MISC_BASE, 4096,
            NvOsMemAttribute_Uncached, NVOS_MEM_READ_WRITE, (void**)&pApbMisc)
    );

    reg = *(volatile NvU32*)(pApbMisc + APB_MISC_GP_HIDREV_0);
    id->Id = (NvU16)NV_DRF_VAL( APB_MISC_GP, HIDREV, CHIPID, reg );
    id->Major = (NvU8)NV_DRF_VAL( APB_MISC_GP, HIDREV, MAJORREV, reg );
    id->Minor = (NvU8)NV_DRF_VAL( APB_MISC_GP, HIDREV, MINORREV, reg );
#if 0
    reg = *(volatile NvU32*)(pApbMisc + APB_MISC_GP_EMU_REVID_0);
    id->Netlist = (NvU16)NV_DRF_VAL( APB_MISC_GP, EMU_REVID, NETLIST, reg );
    id->Patch = (NvU16)NV_DRF_VAL( APB_MISC_GP, EMU_REVID, PATCH, reg );
#endif
    NvOsPhysicalMemUnmap((void*)pApbMisc, 4096);
#endif

    return NvError_Success;
}


NvU32 NvBitPrivGetChipId(void);
NvU32 NvBitPrivGetChipId(void)
{
    NvU32 ChipId = 0;
#if NVODM_BOARD_IS_SIMULATION
    ChipId = nvflash_get_devid();
#else
    volatile NvU8 *pApbMisc = NULL;
    NvError e;
    NV_CHECK_ERROR_CLEANUP(
        NvOsPhysicalMemMap(NV_BIT_APB_MISC_BASE, 4096,
            NvOsMemAttribute_Uncached, NVOS_MEM_READ_WRITE, (void**)&pApbMisc)
    );
    ChipId = *(volatile NvU32*)(pApbMisc + APB_MISC_GP_HIDREV_0);
    ChipId = NV_DRF_VAL(APB_MISC_GP, HIDREV, CHIPID, ChipId);

 fail:
#endif
    return ChipId;
}

NvError NvBitInit(NvBitHandle *phBit)
{
    NvChipId    ChipId;
    NvError     e;

    if (!phBit)
        return NvError_InvalidAddress;

    NV_CHECK_ERROR_CLEANUP(
        NvBitPrivGetFullChipId(&ChipId));

    if (ChipId.Id == 0x20)
        e = NvBitInitAp20(phBit);
    else if (ChipId.Id == 0x30)
        e = NvBitInitT30(phBit);
    else
        return NvError_NotSupported;

    // If there is no BIT ...
    if (e == NvError_BadParameter)
    {
        // ... we may be using the backdoor loader perhaps.
        e = NvError_NotImplemented;
    }

fail:
    return e;
}

void NvBitDeinit(
    NvBitHandle hBit)
{
    if (!hBit)
        return;
#if NVODM_BOARD_IS_SIMULATION
    NvOsFree(hBit);
#endif
    hBit = NULL;
}

NvError NvBitGetData(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *Instance,
    void *Data)
{
    NvU32 ChipId;
    NvU32 MinDataSize = 0;
    NvU32 NumInstances = 0;
    NvError e;

    NV_ASSERT(hBit);

    if (!hBit)
        return NvError_BadParameter;

    if (!Size || !Instance)
        return NvError_InvalidAddress;


    NV_CHECK_ERROR(GetDataSize(hBit, DataType, &MinDataSize, &NumInstances));

    if (*Size == 0)
    {
        *Size = MinDataSize;
        *Instance = NumInstances;

        if (Data)
            return NvError_InsufficientMemory;
        else
            return NvSuccess;
    }

    *Size = MinDataSize;

    if (*Instance > NumInstances)
        return NvError_BadParameter;

    if (!Data)
        return NvError_InvalidAddress;

    ChipId = NvBitPrivGetChipId();

    if (ChipId == 0x20)
        return NvBitGetDataAp20(hBit, DataType, Size, Instance, Data);
    else if (ChipId == 0x30)
        return NvBitGetDataT30(hBit, DataType, Size, Instance, Data);

    return NvError_NotSupported;

}

NvBitSecondaryBootDevice NvBitGetBootDevTypeFromBitData(NvU32 BitDataDevice)
{
    NvBootDevType Boot = (NvBootDevType)BitDataDevice;

    switch (Boot)
    {
    case NvBootDevType_Nand_x8:
        return NvBitBootDevice_Nand8;
    case NvBootDevType_Nand_x16:
        return NvBitBootDevice_Nand16;
    case NvBootDevType_Sdmmc:
        return NvBitBootDevice_Emmc;
    case NvBootDevType_Spi:
        return NvBitBootDevice_SpiNor;
    case NvBootDevType_Nor:
        return NvBitBootDevice_Nor;
    case NvBootDevType_MuxOneNand:
        return NvBitBootDevice_MuxOneNand;
    case NvBootDevType_MobileLbaNand:
        return NvBitBootDevice_MobileLbaNand;
    default:
        return NvBitBootDevice_None;
    }
}

NvBitRdrStatus NvBitGetBitStatusFromBootRdrStatus(NvU32 BootRdrStatus)
{
    NvBootRdrStatus BootStatus = (NvBootRdrStatus)BootRdrStatus;

    switch (BootStatus)
    {
    case NvBootRdrStatus_Success:
        return NvBitStatus_Success;
    case NvBootRdrStatus_ValidationFailure:
        return NvBitStatus_ValidationFailure;
    case NvBootRdrStatus_DeviceReadError:
        return NvBitStatus_DeviceReadError;
    default:
        return NvBitStatus_None;
    }
}
static NvError GetDataSize(
    NvBitHandle hBit,
    NvBitDataType DataType,
    NvU32 *Size,
    NvU32 *NumInstances)
{
    NV_ASSERT(hBit);
    NV_ASSERT(Size);
    NV_ASSERT(NumInstances);

    switch(DataType)
    {
        case NvBitDataType_BootRomVersion:
        case NvBitDataType_BootDataVersion:
        case NvBitDataType_RcmVersion:
        case NvBitDataType_BootType:
        case NvBitDataType_PrimaryDevice:
        case NvBitDataType_SecondaryDevice:
        case NvBitDataType_OscFrequency:
        case NvBitDataType_ActiveBctBlock:
        case NvBitDataType_ActiveBctSector:
        case NvBitDataType_BctSize:
        case NvBitDataType_ActiveBctPtr:
        case NvBitDataType_SafeStartAddr:
            *Size = sizeof(NvU32);
            *NumInstances = 1;
            break;

        case NvBitDataType_IsValidBct:
            *Size = sizeof(NvBool);
            *NumInstances = 1;
            break;


        default:
        {
            NvU32 ChipId = NvBitPrivGetChipId();
            NvError e;

            if (ChipId == 0x20)
                e = NvBitPrivGetDataSizeAp20(hBit, DataType, Size,
                        NumInstances);
            else if (ChipId == 0x30)
                e = NvBitPrivGetDataSizeT30(hBit, DataType, Size,
                        NumInstances);
            else
                e = NvError_NotSupported;

            if (e!=NvSuccess)
                goto fail;
        }
    }
fail:
    return NvSuccess;
}


