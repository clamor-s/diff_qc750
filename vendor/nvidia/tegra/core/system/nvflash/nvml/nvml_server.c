/*
 * Copyright (c) 2008 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvml_utils.h"
#include "nvml.h"
#include "nvml_device.h"
#include "nvbct.h"
#include "nvml_aes.h"
#include "nvuart.h"

NvBootInfoTable *pBitTable;
NvBctHandle BctHandle = NULL;
NvBootFuseOperatingMode opMode;
NvBootInfoTable BootInfoTable;

#define IRAM_START_ADDRESS 0x40000000
#define USB_MAX_TRANSFER_SIZE 2048
#define NVBOOT_MAX_BOOTLOADERS 4 // number of bl info in bct

#ifndef NVBOOT_SSK_ENGINE
#define NVBOOT_SSK_ENGINE NvBootAesEngine_B
#endif

#ifndef NVBOOT_SBK_ENGINE
#define NVBOOT_SBK_ENGINE NvBootAesEngine_A
#endif

#define MISC_PA_BASE    0x70000000
#define MISC_PA_LEN    4096

#define VERIFY_DATA1 0xaa55aa55
#define VERIFY_DATA2 0x55aa55aa

#define SKIP_EVT_SDRAM_BYTES  64

#define VERIFY( exp, code ) \
    if ( !(exp) ) { \
        code; \
    }

static NvU32 NoBRBinary;

/**
 * Process 3P command
 *
 * Execution of received 3P commands are carried out by the COMMAND_HANDLER's
 *
 * The command handler must handle all errors related to the request internally,
 * except for those that arise from low-level 3P communication and protocol
 * errors.  Only in this latter case is the command handler allowed to report a
 * return value other than NvSuccess.
 *
 * Thus, the return value does not indicate whether the command itself was
 * carried out successfully, but rather that the 3P communication link is still
 * operating properly.  The success status of the command is generally returned
 * to the 3P client by sending a Nv3pCommand_Status message.
 *
 * @param hSock pointer to the handle for the 3P socket state
 * @param command 3P command to execute
 * @param args pointer to command arguments
 *
 * @retval NvSuccess 3P communication link/protocol still valid
 * @retval NvError_* 3P communication link/protocol failure
 */

#define COMMAND_HANDLER(name)                   \
    NvBool                                     \
    name(                                       \
        Nv3pSocketHandle hSock,                 \
        Nv3pCommand command,                    \
        void *arg                               \
        )

#if !__GNUC__
__asm void
ExecuteBootLoader( NvU32 entry )
{
    CODE32
    PRESERVE8

    IMPORT NvOsDataCacheWritebackInvalidate

    //First, turn off interrupts
    msr cpsr_fsxc, #0xdf

    //Clear TIMER1
    ldr r1, =TIMER1
    mov r2, #0
    str r2, [r1]

    //Clear IRQ handler/SWI handler
    ldr r1, =VECTOR_IRQ
    str r2, [r1]
    ldr r1, =VECTOR_SWI
    str r2, [r1]

    stmfd sp!, {r0}

    //Flush the cache
    bl NvOsDataCacheWritebackInvalidate

    ldmfd sp!, {r0}

    //Jump to BL
    bx r0
}
#else
NV_NAKED void
ExecuteBootLoader( NvU32 entry )
{
    //First, turn off interrupts
    asm volatile("msr cpsr_fsxc, #0xdf");

    asm volatile(
    //Clear TIMER1
    "ldr r1, =0x60005008    \n"
    "mov r2, #0         \n"
    "str r2, [r1]       \n"

    //Clear IRQ handler/SWI handler
    "ldr r1, =0x6000F218\n"
    "str r2, [r1]       \n"
    "ldr r1, =0x6000F208\n"
    "str r2, [r1]       \n"

    "stmfd sp!, {r0}    \n"

    //Flush the cache
    "bl NvOsDataCacheWritebackInvalidate\n"

    "ldmfd sp!, {r0}    \n"

    //Jump to BL
    "bx r0              \n"
    );
}
#endif

NvU32 NvMlGetChipId(void)
{
    NvU8 *pMisc;
    NvU32 RegData;
    NvU32 ChipId = 0;
    NvError e;

    NV_CHECK_ERROR_CLEANUP(
      NvOsPhysicalMemMap(MISC_PA_BASE, MISC_PA_LEN,
      NvOsMemAttribute_Uncached, NVOS_MEM_READ_WRITE, (void **)&pMisc)
      );

    RegData = NV_READ32(pMisc + APB_MISC_GP_HIDREV_0);

    NvOsPhysicalMemUnmap(pMisc, MISC_PA_LEN);
    ChipId = NV_DRF_VAL(APB_MISC_GP, HIDREV, CHIPID, RegData);

fail:
    return ChipId;

}

Nv3pDkStatus CheckIfDkBurned()
{
    NvU8 *plainText = "*this is a test*";
    NvU8 cipherTextA[NVBOOT_AES_BLOCK_LENGTH_BYTES], cipherTextB[NVBOOT_AES_BLOCK_LENGTH_BYTES];
    NvBootAesKey key, sskKey;
    NvBootAesIv zeroIv;
    NvU8 ssk[NVBOOT_AES_BLOCK_LENGTH_BYTES];
    NvU32 i, j;
    Nv3pDkStatus dkBurned;


    //Step I: We take a random plaintext string and encrypt it with the SSK.
    //We store this in cipherTextA.
    NvMlAesInitializeEngine();
    NvMlAesSelectKeyIvSlot(NVBOOT_SSK_ENGINE, NVBOOT_SSK_ENCRYPT_SLOT);
    NvMlAesStartEngine(NVBOOT_SSK_ENGINE, 1, plainText, cipherTextA, NV_TRUE);

    while (NvMlAesIsEngineBusy(NVBOOT_SSK_ENGINE));

    NvOsMemset(&key, 0, sizeof(key));
    NvOsMemset(&zeroIv, 0, sizeof(zeroIv));

    //Step II: We assume that SBK=DK=0. Then we generate
    //a new SSK value. SSK is a function of UID, SBK, DK:
    //SSK = f(UID, SBK, DK).
    NvMlAesSetKeyAndIv(NvBootAesEngine_A, NvBootAesKeySlot_2, &key, &zeroIv, NV_TRUE);
    NvMlUtilsSskGenerate(NvBootAesEngine_A, NvBootAesKeySlot_2, ssk);

    j = 0;
    //Convert the ssk key from a byte array to an NvU32 array
    for (i=0;i<NVBOOT_AES_BLOCK_LENGTH_BYTES;i+=4)
    {
        sskKey.key[j] = ssk[i+3] << 24 | ssk[i+2] << 16 | ssk[i+1] << 8 | ssk[i];
        j++;
    }

    //Step III: We use the new SSK to encrypt the same plaintext
    //string and store the result in cipherTextB.
    NvMlAesSetKeyAndIv(NvBootAesEngine_A, NvBootAesKeySlot_2, &sskKey, &zeroIv, NV_TRUE);
    NvMlAesSelectKeyIvSlot(NvBootAesEngine_A, NvBootAesKeySlot_2);
    NvMlAesStartEngine(NvBootAesEngine_A, 1, plainText, cipherTextB, NV_TRUE);

    while (NvMlAesIsEngineBusy(NvBootAesEngine_A));

    //Step IV: If cipherTextA == cipherTextB, then that
    //implies that the original SSK in step I and the
    //the new generated SSK in step II are the same.
    //This further implies that the DK is zero
    //since we know that the SBK is zero (We execute
    //this function only when SBK is zero) and the
    //UID is a constant. So the value of DK=0
    //that we plugged into SSK = f(UID, SBK, DK) is
    //correct.
    dkBurned = Nv3pDkStatus_NotBurned;
    for (i=0;i<NVBOOT_AES_BLOCK_LENGTH_BYTES;i++)
    {
        if (cipherTextA[i] != cipherTextB[i])
        {
            dkBurned = Nv3pDkStatus_Burned;
            break;
        }
    }

    return dkBurned;
}

NvBool CheckIfSbkBurned()
{
    NvU8 *plainText = "*this is a test*";
    NvU8 cipherTextA[NVBOOT_AES_BLOCK_LENGTH_BYTES], cipherTextB[NVBOOT_AES_BLOCK_LENGTH_BYTES];
    NvBootAesKey key;
    NvBootAesIv zeroIv;
    NvU32 i;
    NvBool sbkBurned = NV_FALSE;

    //Now we need to figure out of the DK and SBK are burned
    NvOsMemset(&key, 0, sizeof(key));
    NvOsMemset(&zeroIv, 0, sizeof(zeroIv));

    //Step I: We take a random plaintext string, and encrypt it with the SBK.
    //We store this in cipherTextA
    NvMlAesInitializeEngine();
    NvMlAesSelectKeyIvSlot(NVBOOT_SBK_ENGINE, NVBOOT_SBK_ENCRYPT_SLOT);

    NvMlAesStartEngine(NVBOOT_SBK_ENGINE, 1, plainText, cipherTextA, NV_TRUE);

    while (NvMlAesIsEngineBusy(NVBOOT_SBK_ENGINE));

    //Step II: We take the same plaintext string and encrypt it
    //with a key and IV of zero. We store this in cipherTextB
    NvMlAesSetKeyAndIv(NvBootAesEngine_A, NvBootAesKeySlot_2, &key, &zeroIv, NV_TRUE);

    NvMlAesSelectKeyIvSlot(NvBootAesEngine_A, NvBootAesKeySlot_2);
    NvMlAesStartEngine(NvBootAesEngine_A, 1, plainText, cipherTextB, NV_TRUE);

    while (NvMlAesIsEngineBusy(NvBootAesEngine_A));

    //Step III: If cipherTextA == cipherTextB, it implies
    //that the SBK is zero (ie, not burned).
    sbkBurned = NV_FALSE;

    for (i=0;i<NVBOOT_AES_BLOCK_LENGTH_BYTES;i++)
    {
        if (cipherTextA[i] != cipherTextB[i])
        {
            sbkBurned = NV_TRUE;
            break;
        }
    }

    return sbkBurned;
}

static NvBool CheckIfHdmiEnabled(void)
{
    NvU32 RegValue = NV_READ32 (NV_ADDRESS_MAP_FUSE_BASE +  FUSE_RESERVED_PRODUCTION_0 );
    NvBool hdmiEnabled = NV_FALSE;

    if (RegValue & 0x2)
        hdmiEnabled = NV_TRUE;

    return hdmiEnabled;
}

static NvBool CheckIfMacrovisionEnabled(void)
{
    NvU32 RegValue = NV_READ32 (NV_ADDRESS_MAP_FUSE_BASE +  FUSE_RESERVED_PRODUCTION_0 );
    NvBool macrovisionEnabled = NV_FALSE;

    if (RegValue & 0x1)
        macrovisionEnabled = NV_TRUE;

    return macrovisionEnabled;
}

static NvBool CheckIfJtagEnabled(void)
{
    NvU32 RegData;

    RegData = NV_READ32(NV_ADDRESS_MAP_FUSE_BASE + FUSE_SECURITY_MODE_0);
    if (RegData)
        return NV_FALSE;
    else
        return NV_TRUE;
}

static NvBool AllZero(NvU8 *p, NvU32 size)
{
    NvU32 i;
    for (i = 0; i < size; i++)
    {
        if (*p++)
            return NV_FALSE;
    }
    return NV_TRUE;
}

static NvError WriteBctToBit(NvU8 *bct, NvU32 bctLength)
{
    NvU32 SdramIndex;
    NvU32 Size = sizeof(NvU32);
    NvU32 Instance = 0;
    NvError e =  NvSuccess;
    NvU32 NumSdramSets = 0;
    NvBootSdramParams SdramParams;

    SdramIndex = NvBootStrapSdramConfigurationIndex();

    //Copy the BCT to the location specified by the bootrom
    pBitTable->BctPtr = (NvBootConfigTable*)pBitTable->SafeStartAddr;
    NvOsMemcpy(pBitTable->BctPtr, bct, bctLength);

    //Intialize bcthandle with Bctptr
    NV_CHECK_ERROR_CLEANUP(NvBctInit(&bctLength, pBitTable->BctPtr, &BctHandle));

    //The sdram index we obtained from the straps
    //must be less that the max indicated in the bct
    Instance = SdramIndex;
    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(BctHandle, NvBctDataType_SdramConfigInfo,
                     &Size, &Instance, &SdramParams)
    );

    Instance = 0;
    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(BctHandle, NvBctDataType_NumValidSdramConfigs,
                     &Size, &Instance, &NumSdramSets)
    );

#if NO_BOOTROM
    //checking if bootrom binary is not present and
    //sdram params in bct are dummy
    if (NoBRBinary &&
        AllZero((NvU8*)&SdramParams,
              sizeof(NvBootSdramParams))
        )
    {
        // bypassing the below functionality
        //till the valid BCT file is available
        //use scripts files to intiliaze sdram
        //Move up the safe ptr
        pBitTable->SdramInitialized = NV_TRUE;
        pBitTable->SafeStartAddr +=
            ((pBitTable->BctSize + BOOTROM_BCT_ALIGNMENT)
             & ~BOOTROM_BCT_ALIGNMENT);
        pBitTable->BctValid = NV_TRUE;

        return e;
    }
#endif

    //The sdram index we obtained from the straps
    //must be less that the max indicated in the bct
    if (SdramIndex < NumSdramSets ||
        // Workaround to use nvflash --download, --read, and --format
        // when Microboot is on the ROM.
        // When Microboot is written on the ROM, NumSdramSets is overwritten
        // by zero so BootROM won't initialize DRAM. Because of this hack,
        // once Microboot is on the ROM, nvflash --download, --read,
        // and --format stops working.
        // With this workaround, even in case of NumSdramSets == 0,
        // NvML still uses SdramParams[SdramIndex].
        // See bug 682227 for more info.
        !AllZero((NvU8*)&SdramParams,
                  sizeof(NvBootSdramParams)))
    {
        NvBootSdramParams *pData;
        NvU32 StableTime = 0;
        pData = &SdramParams;

        //Start PLLM for EMC/MC
        NvMlUtilsClocksStartPll(NvBootClocksPllId_PllM,
            pData,
            StableTime);

        //Now, Initialize SDRAM
        NvBootSdramInit(&SdramParams);

        //If QueryTotalSize fails, it means the sdram
        //isn't initialized properly
        if ( NvBootSdramQueryTotalSize() )
            pBitTable->SdramInitialized = NV_TRUE;
        else
            goto fail;

        //Move up the safe ptr
        pBitTable->SafeStartAddr += ((pBitTable->BctSize + BOOTROM_BCT_ALIGNMENT) & ~BOOTROM_BCT_ALIGNMENT);
        pBitTable->BctValid = NV_TRUE;
    }
    else
    {
        //Sdram index is wrong. Return an error.
        goto fail;
    }

    return NvError_Success;
fail:
    return NvError_NotInitialized;
}

static NvBool
VerifySdram(NvU32 val,NvU32 *Size)
{
    NvU32 SdRamBaseAddress = 0 ;
    NvU32 Count = 0;
    NvU32 Data = 0;
    NvU32 Iterations = 0;
    NvU32 ChipId;
    NvU32 IterSize;

    if (!*Size)
        return NV_FALSE;

    ChipId = NvMlGetChipId();
    if (ChipId == 0x20)
    {
        SdRamBaseAddress = SKIP_EVT_SDRAM_BYTES;
        // Memory location 0x0-0x40 i.e. 64 bytes of SDRAM is not
        // accessible in Ap20 chips. This might be the result of EVT
        // redirection enabled for AVP in Ap20, which diverts the
        // access of first 64 bytes to EVT mmio
        *Size = 1 << *Size;
        IterSize = *Size - SKIP_EVT_SDRAM_BYTES;
    }
    else
    {
        SdRamBaseAddress = 0x80000000;
        IterSize = *Size;
    }

    //sdram verification :soft test
    //known pattern  write and read back per MB
    if (val == 0)
    {
        Iterations = IterSize / (1024 * 1024);

        for(Count = 0; Count < Iterations; Count ++)
            *(NvU32 *)(SdRamBaseAddress + Count * 1024 * 1024) = VERIFY_DATA1;

        for(Count = 0; Count < Iterations; Count ++)
        {
            Data = *(NvU32 *)(SdRamBaseAddress + Count * 1024 * 1024);
            if(Data != VERIFY_DATA1)
                return NV_FALSE;
        }

        for(Count = 0; Count < Iterations; Count ++)
            *(NvU32 *)(SdRamBaseAddress + Count * 1024 * 1024) = VERIFY_DATA2;

        for(Count = 0; Count < Iterations; Count ++)
        {
            Data = *(NvU32 *)(SdRamBaseAddress + Count * 1024 * 1024);
            if(Data != VERIFY_DATA2)
                return NV_FALSE;
        }
    }
    else
    {
        //sdram stress test
        //known pattern  write and read back of entire sdram area
        NvU32 offset = sizeof(NvU32);
        Iterations = IterSize;

        for(Count = 0; Count < Iterations; Count += offset)
            *(NvU32*)(SdRamBaseAddress + Count) = VERIFY_DATA1;

        for(Count = 0; Count < Iterations; Count += offset)
        {
            Data = *(NvU32*)(SdRamBaseAddress + Count);
            if(Data != VERIFY_DATA1)
                return NV_FALSE;
        }

        for(Count = 0; Count < Iterations; Count += offset)
            *(NvU32*)(SdRamBaseAddress + Count) = VERIFY_DATA2;

        for(Count = 0; Count < Iterations; Count += offset)
        {
            Data = *(NvU32*)(SdRamBaseAddress + Count);
            if(Data != VERIFY_DATA2)
                return NV_FALSE;
        }
    }
    return NV_TRUE;
}

static NvBool ReportError(Nv3pSocketHandle hSock, Nv3pCmdStatus* pStatus)
{
    NvError e;
    Nv3pNack(hSock, Nv3pNackCode_BadData);
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(hSock, Nv3pCommand_Status, pStatus, 0)
    );
fail:
    return NV_FALSE;
}

COMMAND_HANDLER(GetPlatformInfo)
{
    NvError e;
    Nv3pCmdStatus status;
    Nv3pCmdGetPlatformInfo a;
    NvU32 regVal;
    NvBootFuseBootDevice FuseDev;
    NvMlContext  Context;

    status.Code = Nv3pStatus_Ok;
    regVal = NV_READ32(NV_ADDRESS_MAP_MISC_BASE + APB_MISC_GP_HIDREV_0) ;

    a.ChipId.Id = NV_DRF_VAL(APB_MISC, GP_HIDREV, CHIPID, regVal);
    a.ChipId.Major = NV_DRF_VAL(APB_MISC, GP_HIDREV, MAJORREV, regVal);
    a.ChipId.Minor = NV_DRF_VAL(APB_MISC, GP_HIDREV, MINORREV, regVal);

    NvOsMemset((void *)&a.ChipUid, 0, sizeof(Nv3pChipUniqueId));
    NvMlUtilsFuseGetUniqueId(&a.ChipUid);
    NvBootFuseGetSkuRaw(&a.ChipSku);

    a.BootRomVersion = 1;
    // Get Boot Device Id from fuses
    NvMlGetBootDeviceType(&FuseDev);
    a.DeviceConfigStrap = NvBootStrapDeviceConfigurationIndex();
    NvBootFuseGetBootDeviceConfiguration(&a.DeviceConfigFuse);

    NvMlUtilsConvertBootFuseDeviceTo3p(FuseDev,&a.SecondaryBootDevice);

    a.SbkBurned = CheckIfSbkBurned();

    //If the SBK isn't burned, we can figure out the DK.
    //If the SBK is burned, then we check to see if we're
    //not in ODM secure mode. If we're not in this mode,
    //we can read the dk fuse directly.
    if (a.SbkBurned == NV_FALSE)
    {
        // Now we check for the DK burn
        a.DkBurned = CheckIfDkBurned();
    }
    else if (opMode != NvBootFuseOperatingMode_OdmProductionSecure)
    {
        NvU32 dk;

        NvBootFuseGetDeviceKey((NvU8*)&dk);

        if (dk)
            a.DkBurned = Nv3pDkStatus_Burned;
        else
            a.DkBurned = Nv3pDkStatus_NotBurned;
    }
    else
    {
        //We can't figure the DK out
        a.DkBurned = Nv3pDkStatus_Unknown;
    }

    a.SdramConfigStrap = NvBootStrapSdramConfigurationIndex();
    a.HdmiEnable = CheckIfHdmiEnabled();
    a.MacrovisionEnable = CheckIfMacrovisionEnabled();
    a.JtagEnable= CheckIfJtagEnabled();

    NvBootFuseGetOpMode(&a.OperatingMode, a.SbkBurned);

    opMode = (NvBootFuseOperatingMode)a.OperatingMode;

    // Make sure that the boot device gets initialized.
    NvMlUtilsSetupBootDevice(&Context);

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, &a, 0 )
    );

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(hSock, Nv3pCommand_Status, &status, 0)
    );
    return NV_TRUE;
fail:
    return ReportError(hSock, &status);
}

COMMAND_HANDLER(GetBct)
{
    NvError e;
    Nv3pCmdStatus status;
    NvU8 *bct = NULL;
    Nv3pCmdGetBct a;

    status.Code = Nv3pStatus_Ok;
    a.Length = NvMlUtilsGetBctSize();

    //For testing purposes, we create
    // different BCTs to differentiate between
    // valid and invalid BCTs.
    if (pBitTable->BctValid == NV_TRUE)
    {
        //Valid BCT implies it's in in the BIT
        bct = (NvU8*) pBitTable->BctPtr;
        VERIFY( bct, goto clean );
    }
    else
    {
        NvBootFuseBootDevice BootDev;
        // Get Boot Device Id from fuses
        // if boot device is undefined get the user defined value
        NvMlGetBootDeviceType(&BootDev);
        // Return error if boot device type is undefined
        if (BootDev >= NvBootFuseBootDevice_Max)
        {
            status.Code = Nv3pStatus_NotBootDevice;
            goto fail;
        }
        //Go get it from storage if possible
        e = (NvError)NvMlBootGetBct(&bct, opMode);
        if (e != NvSuccess)
        {
            status.Code = Nv3pStatus_BctNotFound;
            goto fail;
        }
        pBitTable->BctSize = NvMlUtilsGetBctSize();

        VERIFY( bct, goto clean );

        e = WriteBctToBit(bct, a.Length);
        if (e != NvSuccess)
        {
            status.Code = Nv3pStatus_InvalidBCT;
            goto fail;
        }
    }

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, &a, 0 )
    );

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(hSock, Nv3pCommand_Status, &status, 0)
    );

    NV_CHECK_ERROR_CLEANUP(
        Nv3pDataSend( hSock, bct, a.Length, 0 )
    );
    return NV_TRUE;

clean:
fail:
    return ReportError(hSock, &status);
}

COMMAND_HANDLER(GetBit)
{
    NvError e;
    Nv3pCmdGetBit a;

    a.Length = NvMlUtilsGetBctSize();
    pBitTable->BctSize = NvMlUtilsGetBctSize();

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, &a, 0 )
    );

    NV_CHECK_ERROR_CLEANUP(
        Nv3pDataSend( hSock, (NvU8*)pBitTable, a.Length, 0 )
    );
    return NV_TRUE;

fail:
    return ReportError(hSock, NULL);
}

COMMAND_HANDLER(DownloadBct)
{
    NvError e;
    Nv3pCmdStatus status;
    NvU8 *data, *bct = NULL;
    NvU32 bytesLeftToTransfer = 0;
    NvU32 transferSize = 0;
    NvU32 BctOffset = 0;
    NvBool IsFirstChunk = NV_TRUE;
    NvBool IsLastChunk = NV_FALSE;
    Nv3pCmdDownloadBct *a = (Nv3pCmdDownloadBct *)arg;

    status.Code = Nv3pStatus_Ok;
    VERIFY( a->Length, goto clean );

    //If a valid BCT is already present, just overwrite it
    if (pBitTable->BctValid == NV_TRUE)
    {
        bct = (NvU8*) pBitTable->BctPtr;
    }
    else
    {
        bct = (NvU8*)pBitTable->SafeStartAddr;
    }

    pBitTable->BctSize = a->Length;

    VERIFY( bct, goto clean );
    VERIFY( a->Length == NvMlUtilsGetBctSize(), goto clean );

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, a, 0 )
    );

    bytesLeftToTransfer = a->Length;
    data = bct;
    do
    {
        transferSize = (bytesLeftToTransfer > USB_MAX_TRANSFER_SIZE) ?
                        USB_MAX_TRANSFER_SIZE :
                        bytesLeftToTransfer;

        NV_CHECK_ERROR_CLEANUP(
            Nv3pDataReceive( hSock, data, transferSize, 0, 0 )
        );
        if (opMode == NvBootFuseOperatingMode_OdmProductionSecure)
        {
            // exclude crypto hash of bct for decryption and signing.
            if (IsFirstChunk)
                BctOffset = NvMlUtilsGetDataOffset();
            else
                BctOffset = 0;
            // need to decrypt the encrypted bct got from nvsbktool
            if ((bytesLeftToTransfer - transferSize) == 0)
                IsLastChunk = NV_TRUE;
            if (!NvMlDecryptValidateImage(data + BctOffset, transferSize - BctOffset,
                IsFirstChunk, IsLastChunk, NV_TRUE, NV_TRUE, NULL))
            {
                status.Code = Nv3pStatus_BLValidationFailure;
                goto fail;
            }
            // first chunk has been processed.
            IsFirstChunk = NV_FALSE;
        }
        data += transferSize;
        bytesLeftToTransfer -= transferSize;
    } while (bytesLeftToTransfer);

    e = WriteBctToBit(bct, NvMlUtilsGetBctSize());
    NvOsMemset( &status, 0, sizeof(status) );
    if (e != NvSuccess)
    {
        status.Code = Nv3pStatus_DeviceFailure;
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( hSock, Nv3pCommand_Status, &status, 0 )
    );
    return NV_TRUE;

clean:
fail:
    return ReportError(hSock, &status);
}

COMMAND_HANDLER(SetBlHash)
{
    NvError e;
    Nv3pCmdStatus status;
    NvU8 *data, *bct = NULL;
    NvU32 bytesLeftToTransfer = 0;
    NvU32 transferSize = 0;
    NvU32 BctOffset = 0;
    NvBool IsFirstChunk = NV_TRUE;
    NvBool IsLastChunk = NV_FALSE;
    // store bl hash and index in odm secure mode
    NvBootHash BlHash[NVBOOT_MAX_BOOTLOADERS];
    NvU32 BlLoadAddr[NVBOOT_MAX_BOOTLOADERS];
    Nv3pCmdBlHash *a = (Nv3pCmdBlHash *)arg;
    NvU32 i, j;
    NvU32 Size;
    NvU32 Instance;
    NvBctHandle LocalBctHandle;

    status.Code = Nv3pStatus_Ok;
    VERIFY(a->Length, goto clean);

    if (pBitTable->BctValid == NV_TRUE)
    {
        // If a valid BCT is already present, return error.like getbct
        status.Code = Nv3pStatus_NotSupported;
        goto fail;
    }
    else
    {
        bct = (NvU8*)pBitTable->SafeStartAddr;
    }
    VERIFY(bct, goto clean);
    VERIFY(a->Length == NvMlUtilsGetBctSize(), goto clean);

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete(hSock, command, a, 0)
    );

    bytesLeftToTransfer = a->Length;
    data = bct;
    do
    {
        transferSize = (bytesLeftToTransfer > USB_MAX_TRANSFER_SIZE) ?
                        USB_MAX_TRANSFER_SIZE :
                        bytesLeftToTransfer;

        NV_CHECK_ERROR_CLEANUP(
            Nv3pDataReceive( hSock, data, transferSize, 0, 0 )
        );

        if (IsFirstChunk)
            BctOffset = NvMlUtilsGetDataOffset();
        else
            BctOffset = 0;
        // need to decrypt the encrypted bct got from nvsbktool
        if ((bytesLeftToTransfer - transferSize) == 0)
            IsLastChunk = NV_TRUE;
        if (!NvMlDecryptValidateImage(data + BctOffset, transferSize - BctOffset,
            IsFirstChunk, IsLastChunk, NV_TRUE, NV_TRUE, NULL))
        {
            status.Code = Nv3pStatus_BLValidationFailure;
            goto fail;
        }
        // first chunk has been processed.
        IsFirstChunk = NV_FALSE;
        data += transferSize;
        bytesLeftToTransfer -= transferSize;
    } while (bytesLeftToTransfer);

    NV_CHECK_ERROR_CLEANUP(NvBctInit(&a->Length, bct, &LocalBctHandle));
    // save bl hash and index to be saved in system bct.
    for (i = 0; i < NVBOOT_MAX_BOOTLOADERS; i++)
    {
        NvU32 LoadAddress;
        NvBootHash HashSrc;

        Size = sizeof(NvU32);
        Instance = i;
        NV_CHECK_ERROR_CLEANUP(
            NvBctGetData(LocalBctHandle, NvBctDataType_BootLoaderLoadAddress,
                         &Size, &Instance, &LoadAddress)
        );
        BlLoadAddr[i] = LoadAddress;
        NvBctGetData(LocalBctHandle, NvBctDataType_BootLoaderCryptoHash,
                     &Size, &Instance, &HashSrc);
        NvOsMemcpy(&BlHash[i], &HashSrc, sizeof(NvBootHash));
    }

    //If we don't have a valid BCT,
    //try to get it from the boot device
    if (pBitTable->BctValid == NV_FALSE)
    {
        e = (NvError)NvMlBootGetBct(&bct, opMode);
        if (e != NvSuccess)
        {
            status.Code = Nv3pStatus_BctNotFound;
            goto fail;
        }
        pBitTable->BctSize = NvMlUtilsGetBctSize();
        VERIFY( bct, goto clean );
        e = WriteBctToBit(bct, NvMlUtilsGetBctSize());
        if (e != NvSuccess)
        {
            status.Code = Nv3pStatus_InvalidBCT;
            goto fail;
        }
    }

    // update hash value of bootloader whose attribute matches with it's partition id
    // given with --download option, also find out which hash needs to be updated
    // from secure bct using load address for this bootloader.
    for (i = 0 ; i < NVBOOT_MAX_BOOTLOADERS; i++)
    {
        NvU32 BlAttr;

        Size = sizeof(NvU32);
        Instance = i;
        NV_CHECK_ERROR_CLEANUP(
            NvBctGetData(BctHandle, NvBctDataType_BootLoaderAttribute,
                         &Size, &Instance, &BlAttr)
        );
        // copy bl hash in system bct if Bl is downloaded in secure mode
        if (BlAttr == a->BlIndex)
        {
            NvU32 LoadAddress;
            for (j = 0; j < NVBOOT_MAX_BOOTLOADERS; j++)
            {
                // find index in secure bct from where bl hash needs to be copied
                NV_CHECK_ERROR_CLEANUP(
                    NvBctGetData(BctHandle, NvBctDataType_BootLoaderLoadAddress,
                                 &Size, &Instance, &LoadAddress)
                );
                if (BlLoadAddr[j] == LoadAddress)
                {
                    NvBctSetData(BctHandle, NvBctDataType_BootLoaderCryptoHash,
                                 &Size, &Instance, &BlHash[j]);
                    break;
                }
            }
            if (j == NVBOOT_MAX_BOOTLOADERS)
            {
                status.Code = Nv3pStatus_InvalidBCT;
                goto fail;
            }
            break;
        }
    }

    NvOsMemset(&status, 0, sizeof(status));
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(hSock, Nv3pCommand_Status, &status, 0)
    );
    return NV_TRUE;

clean:
fail:
    return ReportError(hSock, &status);
}

COMMAND_HANDLER(DownloadBootloader)
{
    NvError e;
    Nv3pCmdStatus status;
    NvU8 *data = NULL;
    NvU8 *bct = NULL;
    NvU32 transferSize, bytesLeftToTransfer;
    Nv3pCmdDownloadBootloader *a = (Nv3pCmdDownloadBootloader *)arg;
    NvBootFuseBootDevice BootDev;
    NvBool IsFirstChunk = NV_TRUE;
    NvBool IsLastChunk = NV_FALSE;
    NvU32 BlEntryPoint = 0;
    NvU8 StoredBlhash[1 << NVBOOT_AES_BLOCK_LENGTH_LOG2];
    NvU32 Size;
    NvU32 Instance;

    status.Code = Nv3pStatus_Ok;
    VERIFY( a->Length, goto clean );
    VERIFY( a->Address, goto clean );
    VERIFY( a->EntryPoint, goto clean );

    // Get Boot Device Id from fuses
    // if boot device is undefined get the user defined value
    NvMlGetBootDeviceType(&BootDev);
    // Return error if boot device type is undefined
    if (BootDev >= NvBootFuseBootDevice_Max)
    {
        status.Code = Nv3pStatus_NotBootDevice;
        goto fail;
    }
    //If we don't have a valid BCT,
    //try to get it from the boot device
    if (pBitTable->BctValid == NV_FALSE)
    {
        e = (NvError)NvMlBootGetBct(&bct, opMode);
        if (e != NvSuccess)
        {
            status.Code = Nv3pStatus_BctNotFound;
            goto fail;
        }
        pBitTable->BctSize = NvMlUtilsGetBctSize();

        VERIFY( bct, goto clean );

        e = WriteBctToBit(bct, NvMlUtilsGetBctSize());
        if (e != NvSuccess)
        {
            status.Code = Nv3pStatus_InvalidBCT;
            goto fail;
        }
    }
    if (opMode == NvBootFuseOperatingMode_OdmProductionSecure)
    {
        NvU32 i;
        NvU32 LoadAddress;
        // this is required for eboot case
        for (i = 0; i < NVBOOT_MAX_BOOTLOADERS; i++)
        {
            // iterate over blinfo in bct till bootloader is found
            Size = sizeof(NvU32);
            Instance = i;
            NV_CHECK_ERROR_CLEANUP(
                NvBctGetData(BctHandle, NvBctDataType_BootLoaderEntryPoint,
                             &Size, &Instance, &BlEntryPoint)
            );
            if ((BlEntryPoint & IRAM_START_ADDRESS) == IRAM_START_ADDRESS)
                continue; // found microboot since address starts with 4G,continue
            NV_CHECK_ERROR_CLEANUP(
                NvBctGetData(BctHandle, NvBctDataType_BootLoaderLoadAddress,
                             &Size, &Instance, &LoadAddress)
            );
            data = (NvU8*)LoadAddress;
            break; //bootloader found
        }
    }

    else
    {
        data = (NvU8*)a->Address;
        BlEntryPoint = a->EntryPoint;
    }
    VERIFY(data, goto clean);

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, a, 0 )
    );
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(hSock, Nv3pCommand_Status, &status, 0)
    );
    // store --bl bootloader hash info to be validated
    if (opMode == NvBootFuseOperatingMode_OdmProductionSecure)
    {
        NvU8 NullString[1 << NVBOOT_AES_BLOCK_LENGTH_LOG2] = {0};

        NV_CHECK_ERROR_CLEANUP(
            Nv3pDataReceive(hSock, StoredBlhash, 1 << NVBOOT_AES_BLOCK_LENGTH_LOG2, 0, 0)
        );
        //if --blob option is used in secure mode
        if (NvOsMemcmp(StoredBlhash, NullString, 1 << NVBOOT_AES_BLOCK_LENGTH_LOG2))
        {
            if (!NvMlDecryptValidateImage(StoredBlhash, 1 << NVBOOT_AES_BLOCK_LENGTH_LOG2,
                                     NV_TRUE, NV_TRUE, NV_FALSE, NV_FALSE, NULL))
            {
                status.Code = Nv3pStatus_BLValidationFailure;
                goto fail;
            }
        }
        // if --rcm option is used in secure mode
        else
        {
            NvBootHash HashSrc;

            Size = sizeof(NvU32);
            Instance = 0;
            NvBctGetData(BctHandle, NvBctDataType_BootLoaderCryptoHash,
                         &Size, &Instance, &HashSrc);
            NvOsMemcpy(StoredBlhash, &HashSrc, 1 << NVBOOT_AES_BLOCK_LENGTH_LOG2);
        }
    }
    bytesLeftToTransfer = a->Length;
    do
    {
        transferSize = (bytesLeftToTransfer > USB_MAX_TRANSFER_SIZE) ?
                        USB_MAX_TRANSFER_SIZE :
                        bytesLeftToTransfer;
        NV_CHECK_ERROR_CLEANUP(
            Nv3pDataReceive(hSock, data, transferSize, 0, 0)
        );
        if (opMode == NvBootFuseOperatingMode_OdmProductionSecure)
        {
            // need to decrypt and validate sign of the encrypted bootloader got from nvsbktool
            if ((bytesLeftToTransfer - transferSize) == 0)
                IsLastChunk = NV_TRUE;
            if (!NvMlDecryptValidateImage(data, transferSize, IsFirstChunk,
                                    IsLastChunk, NV_TRUE, NV_FALSE, StoredBlhash))
            {
                status.Code = Nv3pStatus_BLValidationFailure;
                goto fail;
            }
            IsFirstChunk = NV_FALSE;
        }
        data += transferSize;
        bytesLeftToTransfer -= transferSize;
    } while (bytesLeftToTransfer);

    //Write to a specific memory address indicating
    //that we are entering the bootloader from the
    //miniloader. The bootloader will examine
    //bitTable->SafeStartAddr - sizeof(NvU32) and
    //determine whether to turn on the nv3p server or not
    *(NvU32*)pBitTable->SafeStartAddr = MINILOADER_SIGNATURE;
    pBitTable->SafeStartAddr += sizeof(NvU32);
    //Claim that we booted off the first bootloader
    pBitTable->BlState[0].Status = NvBootRdrStatus_Success;

    // Enables Pllm and and sets PLLM in recovery path which is normally
    //done by BOOT ROM in all paths except recovery
    SetPllmInRecovery();
#if __GNUC__ && !NVML_BYPASS_PRINT
    NvOsAvpDebugPrintf("Transferring control to Bootloader\n");
#endif
    ExecuteBootLoader(BlEntryPoint);
    return NV_TRUE;

clean:
fail:
    if (BctHandle)
        NvBctDeinit(BctHandle);
    return ReportError(hSock, &status);
}

COMMAND_HANDLER(Go)
{
    NvError e;

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, 0, 0 )
    );
    return NV_TRUE;

fail:
    return ReportError(hSock, NULL);
}

COMMAND_HANDLER(OdmOptions)
{
    NvError e;
    Nv3pCmdStatus status;
    Nv3pCmdOdmOptions *a = (Nv3pCmdOdmOptions *)arg;
    NvU32 Size = 0, Instance = 0;
    NvU8 *bct = NULL;

    status.Code = Nv3pStatus_Ok;
    //If we don't have a valid BCT,
    //try to get it from the boot device
    if (pBitTable->BctValid == NV_FALSE)
    {
        NvBootFuseBootDevice BootDev;
        // Get Boot Device Id from fuses
        // if boot device is undefined get the user defined value
        NvMlGetBootDeviceType(&BootDev);
        // Return error if boot device type is undefined
        if (BootDev >= NvBootFuseBootDevice_Max)
        {
            status.Code = Nv3pStatus_NotBootDevice;
            goto fail;
        }
        e = (NvError)NvMlBootGetBct(&bct, opMode);
        if (e != NvSuccess)
        {
            status.Code = Nv3pStatus_BctNotFound;
            goto fail;
        }
        pBitTable->BctSize = NvMlUtilsGetBctSize();
        VERIFY( bct, goto clean );
        e = WriteBctToBit(bct, NvMlUtilsGetBctSize());
        if (e != NvSuccess)
        {
            status.Code = Nv3pStatus_InvalidBCT;
            goto fail;
        }
    }

    NV_CHECK_ERROR_CLEANUP(
        NvBctGetData(BctHandle, NvBctDataType_OdmOption,
                     &Size, &Instance, NULL));

    NV_CHECK_ERROR_CLEANUP(
        NvBctSetData(BctHandle, NvBctDataType_OdmOption,
                 &Size, &Instance, (void *)&a->Options));

#if __GNUC__ && !NVML_BYPASS_PRINT
        // reinit uart since debug port info is available now
        NvAvpUartInit(NvMlUtilsGetPLLPFreq());
        NvOsAvpDebugPrintf("Starting Miniloader\n");
#endif

    NvOsMemset( &status, 0, sizeof(status) );

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, a, 0 )
    );

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(hSock, Nv3pCommand_Status, &status, 0)
    );
    return NV_TRUE;

clean:
fail:
    return ReportError(hSock, &status);
}

COMMAND_HANDLER(SetBootDevType)
{
    NvError e;
    Nv3pCmdStatus status;
    Nv3pCmdSetBootDevType *a = (Nv3pCmdSetBootDevType *)arg;
    NvBootFuseBootDevice FuseDev;
    NvBootFuseBootDevice OdmFuseDev;

    status.Code = Nv3pStatus_Ok;
    // Get Boot Device Id fuses
    NvMlUtilsGetBootDeviceType(&FuseDev);
    NvOsMemset(&status, 0, sizeof(status));
    if (NvMlUtilsConvert3pToBootFuseDevice(a->DevType,&OdmFuseDev) != NvSuccess)
    {
        status.Code = Nv3pStatus_NotBootDevice;
        goto fail;
    }
    // handle this command only for the system for which no customer
    // fuses have been burned and if customer fuses are burned then odm
    //set fuses should not differ with them.
    if ((FuseDev != NvBootFuseBootDevice_Max) &&
        (FuseDev != OdmFuseDev))
    {
        status.Code = Nv3pStatus_NotSupported;
        goto fail;
    }

    NvMlSetBootDeviceType(OdmFuseDev);

#if NAND_SUPPORT
    // update BIT with the user specified boot device type
    if (OdmFuseDev == NvBootFuseBootDevice_NandFlash_x16)
        pBitTable->SecondaryDevice = NvBootDevType_Nand_x16;
    else
        pBitTable->SecondaryDevice = (NvBootDevType)OdmFuseDev;
#endif

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete(hSock, command, a, 0)
    );
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(hSock, Nv3pCommand_Status, &status, 0)
    );
    return NV_TRUE;

fail:
    return ReportError(hSock, &status);
}

COMMAND_HANDLER(SetBootDevConfig)
{
    NvError e;
    Nv3pCmdStatus status;
    Nv3pCmdSetBootDevConfig *a = (Nv3pCmdSetBootDevConfig *)arg;
    NvBootFuseBootDevice FuseDev;
    NvU32 DevConfig;

    status.Code = Nv3pStatus_Ok;
    // Get Boot Device Id fuses
    NvMlUtilsGetBootDeviceType(&FuseDev);
    NvBootFuseGetBootDeviceConfiguration(&DevConfig);
    NvOsMemset(&status, 0, sizeof(status));
    NvMlSetBootDeviceConfig(a->DevConfig);

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete(hSock, command, a, 0)
    );
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(hSock, Nv3pCommand_Status, &status, 0)
    );
    return NV_TRUE;

fail:
    return ReportError(hSock, &status);
}

COMMAND_HANDLER(SdramVerification)
{
    NvError e;
    Nv3pCmdStatus status;
    Nv3pCmdOdmCommand *a = (Nv3pCmdOdmCommand *)arg;

    status.Code = Nv3pStatus_Ok;
    switch(a->odmExtCmd)
    {
        case Nv3pOdmExtCmd_VerifySdram:
        {
            NvBool res = NV_TRUE;
            Nv3pCmdOdmCommand b;
            Nv3pOdmExtCmdVerifySdram *c = (Nv3pOdmExtCmdVerifySdram*)
                                               &a->odmExtCmdParam.verifySdram;

            b.odmExtCmd = Nv3pOdmExtCmd_VerifySdram;
            b.Data = NvBootSdramQueryTotalSize();

            res = VerifySdram(c->Value, &b.Data);
            if (res != NV_TRUE)
            {
                status.Code = Nv3pStatus_VerifySdramFailure;
                goto fail;
            }

            NV_CHECK_ERROR_CLEANUP(
                Nv3pCommandComplete(hSock, command, &b, 0)
            );

            NV_CHECK_ERROR_CLEANUP(
                Nv3pCommandSend(hSock, Nv3pCommand_Status, &status, 0)
            );
            break;
        }
        default:
            break;
    }
    return NV_TRUE;

fail:
    return ReportError(hSock, &status);
}

void
server(void)
{
    NvError e;
    Nv3pSocketHandle hSock = 0;
    Nv3pCommand command;
    void *arg;
    NvBool b;

#if __GNUC__ && !NVML_BYPASS_PRINT
    // this init will no work for boards which does not use uartA as debug port
    // for those board, init is gain done in odmoption command
    NvAvpUartInit(NvMlUtilsGetPLLPFreq());
    NvOsAvpDebugPrintf("Starting Miniloader\n");
#endif
    ReadBootInformationTable(&pBitTable);

#if NO_BOOTROM
    //check if bootrom binary is available
    if (pBitTable->BootRomVersion == 0x0)
        NoBRBinary = 1;

    if (NoBRBinary)
    {
        //BootROM does the usb enumeration
        //since BR is not present,we are doing
        //enumeration here using nvboot drivers
        b = NvMlUtilsStartUsbEnumeration();
        VERIFY(b == NV_TRUE,goto clean);

        NvMlUtilsPreInitBit(pBitTable);
    }
#endif

#if AES_KEYSCHED_LOCK_WAR_BUG_598910
    NvMlUtilsLockAesKeySchedule();
#endif
#if SE_AES_KEYSCHED_READ_LOCK
    NvMlUtilsLockSeAesKeySchedule();
#endif
    NvOsMemcpy(&BootInfoTable, pBitTable, sizeof(NvBootInfoTable));
    e = Nv3pOpen( &hSock, Nv3pTransportMode_default, 0);
    VERIFY( e == NvSuccess, goto clean );

    for( ;; )
    {
        e = Nv3pCommandReceive( hSock, &command, &arg, 0 );
        VERIFY( e == NvSuccess, goto clean );

        switch( command ) {
        case Nv3pCommand_GetPlatformInfo:
            b = GetPlatformInfo(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_GetBct:
            b = GetBct(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_GetBit:
            b = GetBit(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_DownloadBct:
            b = DownloadBct(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_SetBlHash:
            b = SetBlHash(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_DownloadBootloader:
            b = DownloadBootloader(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_Go:
            b = Go(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_OdmOptions:
            b = OdmOptions(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_SetBootDevType:
            b = SetBootDevType(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_SetBootDevConfig:
            b = SetBootDevConfig(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        case Nv3pCommand_OdmCommand:
            b = SdramVerification(hSock, command, arg);
            VERIFY(b == NV_TRUE, goto clean);
            break;
        default:
            Nv3pNack(hSock, Nv3pNackCode_BadCommand);
            break;
        }
    }

clean:
    Nv3pClose(hSock);
}

void NvOsAvpShellThread()
{
    server();
}


