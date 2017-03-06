/*
 * Copyright (c) 2012-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#include "nvcommon.h"
#include "nvos.h"
#include "nverror.h"
#include "nvodm_services.h"
#include "nvaboot_private.h"

#include "nvaboot_tf.h"
#include "tf_include.h"
#include "eks_index_offset.h"

/* Part of the available Secure RAM is allocated for the Trusted Foundations
 * Software workspace memory.  Note that this range does not include all
 * the available Secure RAM on the platform, since some Secure RAM is
 * required to store the code of the Trusted Foundations.
 */
#define TF_WORKSPACE_OFFSET      0x100000
#define TF_WORKSPACE_SIZE        0x100000
#define TF_WORKSPACE_ATTRIBUTES  0x6

/* Tegra Crypto driver */
#define TF_CRYPTO_DRIVER_UID    "[770CB4F8-2E9B-4C6F-A538-744E462E150C]"

/* Tegra L2 Cache Controller driver*/
#define TF_L2CC_DRIVER_UID      "[cb632260-32c6-11e0-bc8e-0800200c9a66]"

/* Tegra Trace driver*/
#define TF_TRACE_DRIVER_UID     "[e85b1505-67ac-4d14-8680-4d97c019942d]"

#define ADD_TF_BOOT_STRING_ARG(sptr, rem, vptr) \
    do {                                             \
        NvU32 idx;                                   \
        idx = NvOsSnprintf(sptr, rem, "%s\n", vptr); \
        rem  -= idx;                                 \
        sptr += idx;                                 \
    } while (0)

#define ADD_TF_BOOT_INT_ARG(sptr, rem, aptr, val) \
    do {                                                              \
        NvU32 idx;                                                    \
        idx = NvOsSnprintf(sptr, rem, aptr ": 0x%x\n", (NvU32)(val)); \
        rem  -= idx;                                                  \
        sptr += idx;                                                  \
    } while (0)

/*
 * Copy TF image (and keys) to what will become secure memory (called
 * from NvAbootPrepareBoot_TF just prior to jumping to the monitor).
 */
void NvAbootCopyTFImageAndKeys(TFSW_PARAMS *pTFSWParams)
{
    extern NvU32 SOSKeyIndexOffset;

    /* Copy TF image to secure memory */
    NvOsMemcpy((void *)pTFSWParams->TFStartAddr,
               (const void *)pTFSWParams->pTFAddr, pTFSWParams->TFSize);

    /* Copy keys buffer to secure memory */
    NvOsMemcpy((void *)pTFSWParams->pTFKeysAddr,
               (const void *)pTFSWParams->pTFKeys, pTFSWParams->pTFKeysSize);

    if (SOSKeyIndexOffset == 0)
    {
        NvUPtr encryptedKeys;
        NvU32 i, *destKeyPtr;

        /* Copy encrypted keys to secure memory */
        encryptedKeys = pTFSWParams->pEncryptedKeys;
        destKeyPtr    = (NvU32 *)(pTFSWParams->TFStartAddr + EKS_INDEX_OFFSET);

        for (i = 0; i < pTFSWParams->nNumOfEncryptedKeys; i++)
        {
            NvU32 slotKeyIndex, keyLength;

            keyLength = *((NvU32 *)encryptedKeys);
            encryptedKeys += sizeof(NvU32);

            slotKeyIndex = *destKeyPtr++;
            if (slotKeyIndex)
            {
                NvOsMemcpy((void *)(pTFSWParams->TFStartAddr + slotKeyIndex),
                           (const void *)encryptedKeys, keyLength);
            }
            encryptedKeys += keyLength;
        }
    }
}

/* Initialize TF_BOOT_PARAMS with data computed by the bootloader and stored in TFSW_PARAM */
static void NvAbootTFGenerateBootArgs(TFSW_PARAMS *pTFSWParams,
                                      NvU32 *pKernelRegisters)
{
    NvU32 Remain = sizeof(pTFSWParams->sTFBootParams.pParamString);
    char *ptr = (char *)pTFSWParams->sTFBootParams.pParamString;

    /* Update TF boot args struct with data computed in bootloader */
    pTFSWParams->sTFBootParams.nBootParamsHeader = 'TFBP';

    pTFSWParams->sTFBootParams.nDeviceVersion = pKernelRegisters[1];
    pTFSWParams->sTFBootParams.nNormalOSArg =   pKernelRegisters[2];

    pTFSWParams->sTFBootParams.nWorkspaceAddress =
            pTFSWParams->TFStartAddr + TF_WORKSPACE_OFFSET;
    pTFSWParams->sTFBootParams.nWorkspaceSize = TF_WORKSPACE_SIZE;
    pTFSWParams->sTFBootParams.nWorkspaceAttributes = TF_WORKSPACE_ATTRIBUTES;

    ADD_TF_BOOT_STRING_ARG(ptr, Remain, "[Global]");
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "normalOS.RAM.1.address", pTFSWParams->RamBase);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "normalOS.RAM.1.size", pTFSWParams->RamSize);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "normalOS.ColdBoot.PA", pTFSWParams->ColdBootAddr);

    /* Init Crypto driver section */
    ADD_TF_BOOT_STRING_ARG(ptr, Remain, TF_CRYPTO_DRIVER_UID);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "config.s.mem.1.address", pTFSWParams->pTFKeysAddr);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "config.s.mem.1.size", pTFSWParams->pTFKeysSize);

    /* Init L2CC driver section */
    ADD_TF_BOOT_STRING_ARG(ptr, Remain, TF_L2CC_DRIVER_UID);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "config.register.aux.value", 0x7E080001);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "config.register.lp.tag_ram_latency", pTFSWParams->L2TagLatencyLP);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "config.register.lp.data_ram_latency", pTFSWParams->L2DataLatencyLP);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "config.register.g.tag_ram_latency", pTFSWParams->L2TagLatencyG);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "config.register.g.data_ram_latency", pTFSWParams->L2DataLatencyG);

    /* Init Trace driver section */
    ADD_TF_BOOT_STRING_ARG(ptr, Remain, TF_TRACE_DRIVER_UID);
    ADD_TF_BOOT_INT_ARG(ptr, Remain,
            "config.register.uart.id", pTFSWParams->ConsoleUartId);

    pTFSWParams->sTFBootParams.nParamStringSize =
            sizeof(pTFSWParams->sTFBootParams.pParamString) - Remain;
}

static void NvAbootTFGetMemoryParams(NvAbootHandle hAboot,
                                     TFSW_PARAMS *pTFSWParams)
{
    NvAbootMemLayoutBaseSize(hAboot, NvAbootMemLayout_Primary,
                             &pTFSWParams->RamBase, &pTFSWParams->RamSize);
    NvAbootMemLayoutBaseSize(hAboot, NvAbootMemLayout_SecureOs,
                             &pTFSWParams->TFStartAddr, NULL);
}

static void NvAbootTFGetL2Latencies(NvAbootHandle hAboot,
                                    TFSW_PARAMS *pTFSWParams)
{
    NvAbootChipSku sku;

    sku = NvAbootPrivGetChipSku(hAboot);
    if (sku == NvAbootChipSku_T20)
    {
        pTFSWParams->L2TagLatencyLP = 0x331;
        pTFSWParams->L2DataLatencyLP = 0x441;

        /* not expecting G values to be used */
        pTFSWParams->L2TagLatencyG = pTFSWParams->L2TagLatencyLP;
        pTFSWParams->L2DataLatencyG = pTFSWParams->L2DataLatencyLP;
        return;
    }

    /* T30 family uses same LP latencies */
    pTFSWParams->L2TagLatencyLP = 0x221;
    pTFSWParams->L2DataLatencyLP = 0x221;

    if (((sku == NvAbootChipSku_T33) || (sku == NvAbootChipSku_AP33)) ||
        ((sku == NvAbootChipSku_T37) || (sku == NvAbootChipSku_AP37)))
    {
        pTFSWParams->L2TagLatencyG = 0x442;
        pTFSWParams->L2DataLatencyG = 0x552;
    }
    else
    {
        /* AP30, T30 */
        pTFSWParams->L2TagLatencyG = 0x441;
        pTFSWParams->L2DataLatencyG = 0x551;
    }
    return;
}

static void NvAbootTFGetUartConsoleId(NvAbootHandle hAboot,
                                      TFSW_PARAMS *pTFSWParams)
{
    NvOdmDebugConsole DbgConsole;

    DbgConsole = NvOdmQueryDebugConsole();
    if (DbgConsole == NvOdmDebugConsole_None)
    {
        /* currently trace driver only supports lsport */
        NvOsDebugPrintf("Warning: console set to hsport                        \
                                          (secure world tracing won't work)\n");
        pTFSWParams->ConsoleUartId = 0;
    }
    else if (DbgConsole & NvOdmDebugConsole_Automation)
        pTFSWParams->ConsoleUartId =
                                   (DbgConsole & 0xF) - NvOdmDebugConsole_UartA;
    else
        pTFSWParams->ConsoleUartId = DbgConsole - NvOdmDebugConsole_UartA;

    /* trace driver expects 1-based indexes (not 0-based) */
    pTFSWParams->ConsoleUartId++;
}

void NvAbootTFPrepareBootParams(NvAbootHandle hAboot,
                                TFSW_PARAMS *pTFSWParams,
                                NvU32 *pKernelRegisters)
{
    extern NvU32 a1dff673f0bb86260b5e7fc18c0460ad859d66dc3;
    extern void *TFSW_Params_BootParams, *TFSW_Params_StartAddr;
    NvU32 *pLP0BootParams;

    NvAbootTFGetMemoryParams(hAboot, pTFSWParams);
    NvAbootTFGetL2Latencies(hAboot, pTFSWParams);
    NvAbootTFGetUartConsoleId(hAboot, pTFSWParams);

    pTFSWParams->pTFAddr        = (NvUPtr)trusted_foundations;
    pTFSWParams->TFSize         = trusted_foundations_size;
    pTFSWParams->pTFKeysAddr    = pTFSWParams->TFStartAddr +
                                      (NvOdmQuerySecureRegionSize() - TF_KEYS_BUFFER_SIZE);
    pTFSWParams->pTFKeysSize    = TF_KEYS_BUFFER_SIZE;

    NvAbootTFGenerateBootArgs(pTFSWParams, pKernelRegisters);

    /* copy params used to locate bootparams and entry point */
    TFSW_Params_BootParams = (void *)&pTFSWParams->sTFBootParams;
    TFSW_Params_StartAddr  = (void *)pTFSWParams->TFStartAddr;

    /* now, patch LP0 warmboot code with params */
    pLP0BootParams = &a1dff673f0bb86260b5e7fc18c0460ad859d66dc3;
    pLP0BootParams[0] = pTFSWParams->TFStartAddr + 0x8;
    pLP0BootParams[1] = pTFSWParams->sTFBootParams.nBootParamsHeader;
    pLP0BootParams[2] = pTFSWParams->sTFBootParams.nDeviceVersion;
    pLP0BootParams[3] = pTFSWParams->sTFBootParams.nNormalOSArg;
    pLP0BootParams[4] = pTFSWParams->sTFBootParams.nWorkspaceAddress;
    pLP0BootParams[5] = pTFSWParams->sTFBootParams.nWorkspaceSize;
    pLP0BootParams[6] = pTFSWParams->sTFBootParams.nWorkspaceAttributes;
}
