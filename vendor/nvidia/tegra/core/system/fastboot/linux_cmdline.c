/*
 * Copyright (c) 2009-2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/* linux_cmdline.c -
 *
 *  This file implements all of the Linux-specific command line and ATAG
 *  structure generation, for passing data to the operating system.
 *
 *  AOS weakly-links its own no-op implementation of NvOsBootArgSet, so all
 *  uses of this function throughout the bootloader will use the
 *  implementation defined in this file.
 */

#include "nvcommon.h"
#include "nvos.h"
#include "nverror.h"
#include "fastboot.h"
#include "nvaboot.h"
#include "nvodm_query.h"
#include "nvodm_services.h"
#include "nvboot_bit.h"
#include "nvddk_blockdevmgr.h"
#include "nvassert.h"
#include "nvpartmgr.h"
#include "nvbootargs.h"
#include "nvsku.h"
#include "nvodm_query_discovery.h"
#include "nvbct.h"
#include "nvrm_structure.h"
#include "libfdt/libfdt.h"

#define ATAG_CMDLINE      0x54410009
#define ATAG_SERIAL       0x54410006
#define ATAG_NVIDIA_TEGRA 0x41000801
#define COMMAND_LINE_SIZE 1024
#define GPT_BLOCK_SIZE    512
//  up to 2K of ATAG data, including the command line will be passed to the OS
NvU32 s_BootTags[BOOT_TAG_SIZE];
char  s_CmdLine[COMMAND_LINE_SIZE];

int nOrigCmdLen, nIgnoreFastBoot = 0;
static const char * const szIgnFBCmd = IGNORE_FASTBOOT_CMD;

extern NvU32 g_wb0_address;
extern NvU32 surf_size;
extern NvU32 surf_address;
extern struct fdt_header *fdt_hdr;

extern NvUPtr nvaos_GetMemoryStart( void );

static NvU32 GetPartitionByType(NvU32 PartitionType)
{
    NvU32 PartitionId = 0;
    NvError e=NvSuccess;
    NvPartInfo PartitionInfo;
    while(1)
    {
        PartitionId = NvPartMgrGetNextId(PartitionId);
        if(PartitionId == 0)
            return PartitionId;
        NV_CHECK_ERROR_CLEANUP(NvPartMgrGetPartInfo(PartitionId, &PartitionInfo));
        if(PartitionInfo.PartitionType == PartitionType)
            break;
    }
    return PartitionId;
fail:
    return 0;
}

/*
 * NvOsBootArgSet passes an NvOsBootArg structure and key to the operating
 * system as a Linux ATAG
 */
NvError NvOsBootArgSet(NvU32 key, void *arg, NvU32 size)
{
    NvU32* BootArg;
    NvBool b;

    BootArg = NvOsAlloc(size + 2*sizeof(NvU32));
    if (!BootArg)
        return NvError_InsufficientMemory;

    BootArg[0] = key;
    BootArg[1] = size;
    if (arg && size)
        NvOsMemcpy(&BootArg[2], arg, size);

    b = FastbootAddAtag(ATAG_NVIDIA_TEGRA, size+2*sizeof(NvU32), BootArg);
    NvOsFree(BootArg);

    return (b) ? NvSuccess : NvError_InsufficientMemory;
}

NvU32 IsSpace(NvU32 c) {
    return ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'));
}

int CheckForIgnoreFastBootCmd(const char * szCmd, int nLen, int nStart, int nEnd)
{
    if ('"' == s_CmdLine[nStart]) /* quoted */
    {
        if (nLen == nEnd - nStart - 2 &&
            '"' == s_CmdLine[nEnd - 1])
            nIgnoreFastBoot = !NvOsMemcmp(&s_CmdLine[nStart + 1], szCmd, nLen);
    }
    else /* or not */
    {
        if (nLen == nEnd - nStart)
            nIgnoreFastBoot = !NvOsMemcmp(&s_CmdLine[nStart], szCmd, nLen);
    }

    return nIgnoreFastBoot;
}

int InitCmdList(const unsigned char * CommandLine) {
    int i, nNext, nMood = 0, nQuote = 0, nStart;
    unsigned char CurCh;
    int IgnFBCmdLen = NvOsStrlen(szIgnFBCmd);

    for (i = nNext = 0; i < ANDROID_BOOT_CMDLINE_SIZE; i++)
    {
        CurCh = CommandLine[i];
        if (0 == nMood) /* skip while space */
        {
            if (!IsSpace(CurCh))
            {
                if (CurCh)
                {
                    s_CmdLine[nStart = nNext++] = CurCh;
                    if ('"' == CurCh) /* quote */
                        nQuote = !0;
                    nMood = 1;
                    continue;
                }
                else
                    break;
            }
        }
        else if (1 == nMood) /* a command option */
        {
            if (IsSpace(CurCh))
            {
                if (nQuote) /* quoted -> space does not count as a separator */
                    s_CmdLine[nNext++] = CurCh;
                else /* otherwise, end of a command */
                {
                    if (!nIgnoreFastBoot) /* check for fastboot ignore command */
                    {
                        if (CheckForIgnoreFastBootCmd(szIgnFBCmd, IgnFBCmdLen, nStart, nNext))
                            nNext = nStart; /* and do not include it in final command line */
                    }
                    if (!nIgnoreFastBoot)
                        s_CmdLine[nNext++] = ' ';
                    nMood = 0;
                }
                continue;
            }
            else if ('"' == CurCh) /* quote character */
                s_CmdLine[nNext++] = CurCh, nQuote = !nQuote;
            else
            {
                if (CurCh) /* non-space, non-quote, non-null */
                {
                    s_CmdLine[nNext++] = CurCh;
                    continue;
                }
                else /* null */
                {
                    if (!nIgnoreFastBoot) /* check for fastboot ignore command */
                    {
                        if (CheckForIgnoreFastBootCmd(szIgnFBCmd, IgnFBCmdLen, nStart, nNext))
                            nNext = nStart; /* and do not include it in final command line */
                    }
                    break;
                }
            }
        }
    }

    if (nNext)
    {
        /* if quoted, complete it */
        if (nQuote)
            s_CmdLine[nNext++] = '"';

        /* check for fastboot ignore command */
        if (!nIgnoreFastBoot)
        {
            if (CheckForIgnoreFastBootCmd(szIgnFBCmd, IgnFBCmdLen, nStart, nNext))
                nNext = nStart; /* and do not include it in final command line */
        }

        /* add an space at the end, if necessary */
        if (' ' != s_CmdLine[nNext - 1])
            s_CmdLine[nNext++] = ' ';
    }

    s_CmdLine[nNext] = 0; /* The END */

    return nOrigCmdLen = nNext;
}

int DoesCommandExist(const unsigned char * Cmd)
{
    int i, nMood = 0, nQuote = 0;
    unsigned char CurCh;
    const unsigned char *p = Cmd;

    /* ignore fastboot */
    if (nIgnoreFastBoot) return !0;
    /* nothing to find */
    if (!p || !*p) return 0;

    for (i = 0; i < nOrigCmdLen; i++)
    {
        CurCh = s_CmdLine[i];
        if (0 == nMood) /* skip space */
        {
            if ('"' == CurCh) /* quote at the begining of a cmd */
                nMood = 4, nQuote = !0;
            else if (*p == CurCh)
                p++, nMood = 1;
            else
                nMood = 2;
            continue;
        }
        else if (1 == nMood) /* try matching cmd */
        {
            if (*p == CurCh) /* match character */
            {
                if ('"' == CurCh) /* keep the quote logic happy */
                    nQuote = !nQuote;
                p++;
                continue;
            }
            else if (*p) /* match failed */
            {
                if (' ' == CurCh) /* separator ? */
                {
                    if (nQuote) /* not end of cmd in quote */
                        nMood=2; /* wait until the next cmd */
                    else /* end of cmd, try next */
                        p = Cmd, nMood = 0;
                }
                else /* not a seperator */
                {
                    if ('"' == CurCh) /* keep the quote logic happy */
                        nQuote = !nQuote;
                    nMood = 2; /* wait until the next cmd */
                }
                continue;
            }
            else /* *p == 0 -> end of cmd, we are searching, cmd match? */
            {
                if (nQuote) /* quoted */
                {
                    if ('=' == CurCh)
                        return !0; /* a real match */
                    if ('"' == CurCh) /* and an end quote */
                        nQuote = !nQuote, nMood = 3; /* not yet, but there is a chance */
                    else
                        nMood = 2; /* alas, wait until the next cmd */
                }
                else if (' ' == CurCh || '=' == CurCh) /* end of cmd in cmdline */
                    return !0; /* a real match */
                else /* no match */
                {
                    if ('"' == CurCh) /* keep the quote logic happy */
                        nQuote = !nQuote;
                    nMood = 2; /* wait until the next cmd */
                }
            }
        }
        else if (2 == nMood) /* wait until the next cmd */
        {
            if ('"' == CurCh) /* quote */
                nQuote = !nQuote;
            else if (!nQuote && ' ' == CurCh) /* end of a cmd */
                p = Cmd, nMood = 0; /* try the next cmd */
            continue;
        }
        else if (3 == nMood) /* matched cmd string while quoted */
        {
            if ('=' == CurCh || ' ' == CurCh) /* match, as quote ends right */
                return !0; /* a real match */
            else  /* no match, wait until the next cmd */
                nMood = 2;
            continue;
        }
        else if (4 == nMood)
        {
            if (*p == CurCh) /* match, try rest */
                p++, nMood = 1;
            else /* no match, wait until the next cmd */
                nMood = 2;
            continue;
        }
    }

    return 0;
}

/*  Creates the command line, and adds it to the TAG list */
NvError FastbootCommandLine(
    NvAbootHandle hAboot,
    const PartitionNameMap *PartitionList,
    NvU32 NumPartitions,
    NvU32 SecondaryBootDevice,
    const unsigned char *CommandLine,
    NvBool HasInitRamDisk,
    const unsigned char *ProductName,
    NvBool RecoveryKernel)
{
    NvRmDeviceHandle hRm;
    NvAbootDramPart DramPart;
    int Remain;
    NvU32 Idx, i;
    NvOdmBoardInfo BoardInfo;
    char *ptr;
    NvUPtr MemBase;
    NvU64 MemSize;
    NvBool Ret = NV_FALSE;
    NvOdmDebugConsole DbgConsole;
    NvOdmPmuBoardInfo PmuBoardInfo;
    NvOdmDisplayBoardInfo DisplayBoardInfo;
    NvOdmPowerSupplyInfo PowerSupplyInfo;
    NvOdmAudioCodecBoardInfo AudioCodecInfo;
    NvOdmMaxCpuCurrentInfo CpuCurrInfo;
    NvOdmCameraBoardInfo CameraBoardInfo;
    NvU8 ModemId;
	NvU32 HardwareVersion;//added by jimmy 
    NvU8 SkuOverride;
    NvU32 CommChipVal;
    int node, err;
    NvError NvErr = NvError_Success;
    const unsigned char * CmdStr;

    NV_ASSERT_SUCCESS(NvRmOpen(&hRm, 0));

    NvOsMemset(s_CmdLine, 0, sizeof(s_CmdLine));

    i = InitCmdList(CommandLine);
    Remain = sizeof(s_CmdLine) / sizeof(char) - i;
    ptr = &s_CmdLine[i];

    CmdStr = "tegraid";
    if (!DoesCommandExist(CmdStr))
    {
        NvOsSnprintf(ptr, Remain, "%s=%x.%x.%x.%x.%x",
                        CmdStr,
                        hRm->ChipId.Id,
                        hRm->ChipId.Major,
                        hRm->ChipId.Minor,
                        hRm->ChipId.Netlist,
                        hRm->ChipId.Patch);
        Idx = NvOsStrlen(ptr);
        Remain -= Idx;
        ptr += Idx;

        if (hRm->ChipId.Private)
            NvOsSnprintf(ptr, Remain, ".%s ", hRm->ChipId.Private);
        else
            NvOsStrncpy(ptr, " ", Remain - 1);

        Idx = NvOsStrlen(ptr);
        Remain -= Idx;
        ptr += Idx;
    }

    /* Find base/size of Primary partition */
    CmdStr = "mem";
    if (!DoesCommandExist(CmdStr))
    {
        NvAbootMemLayoutBaseSize(hAboot, NvAbootMemLayout_Primary, &MemBase, &MemSize);

        MemSize >>= 20;
        MemBase >>= 20;

        NvOsSnprintf(ptr, Remain, "%s=%uM@%uM ", CmdStr, (NvU32)MemSize, MemBase);
        Idx = NvOsStrlen(ptr);
        Remain -= Idx;
        ptr += Idx;
    }
    NvOsSnprintf(ptr, Remain, "androidboot.bootloader=%s ", "1.2");
    Idx = NvOsStrlen(ptr);
    Remain -= Idx;
    ptr += Idx;

    HardwareVersion = NvOdmQueryGetHardwareVersion();
    NvOsSnprintf(ptr, Remain, "androidboot.hw_ver=%u ", HardwareVersion);
    Idx = NvOsStrlen(ptr);
    Remain -= Idx;
    ptr += Idx;
    // Setting the commchip kernel parameter based on odmdata
    CmdStr = "commchip_id";
    if (!DoesCommandExist(CmdStr)) {
        CommChipVal = NvOdmQueryGetWifiOdmData();
        NvOsSnprintf(ptr, Remain, "%s=%u ", CmdStr, CommChipVal);
        Idx = NvOsStrlen(ptr);
        Remain -= Idx;
        ptr += Idx;
    }

#if defined(ANDROID)
    {
        NvU64 uid = 0;
        NvRmDeviceHandle hRm = NULL;
        NV_ASSERT_SUCCESS(NvRmOpen(&hRm, 0));
        NvRmQueryChipUniqueId(hRm, sizeof(uid), &uid);
        NvRmClose(hRm);

        if (uid != 0)
        {
            CmdStr = "androidboot.cpuid";
            if (!DoesCommandExist(CmdStr))
            {
                NvOsSnprintf(ptr, Remain, "%s=%08x%08x ", CmdStr, (NvU32)(uid >> 32), (NvU32)(uid));
                Idx = NvOsStrlen(ptr);
                Remain -= Idx;
                ptr += Idx;
            }
        }

        CmdStr = "androidboot.commchip_id";
        if (!DoesCommandExist(CmdStr)) {
            CommChipVal = NvOdmQueryGetWifiOdmData();
            NvOsSnprintf(ptr, Remain, "%s=%u ", CmdStr, CommChipVal);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }
#ifdef MODULE_3G_CONF
        CmdStr = "androidboot.module_3g_config";
        if (!DoesCommandExist(CmdStr)) {
            CommChipVal = NvOdmQueryGetModule3gConfig();
            NvOsSnprintf(ptr, Remain, "%s=%u ", CmdStr, CommChipVal);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }
#endif
    }
#endif

    CmdStr = "video";
    if (!DoesCommandExist(CmdStr)) {
        NvOsSnprintf(ptr, Remain, "%s=tegrafb ", CmdStr);
        Idx = NvOsStrlen(ptr);
        Remain -= Idx;
        ptr += Idx;
    }

    CmdStr = "no_console_suspend";
    if (!DoesCommandExist(CmdStr)) {
        NvOsSnprintf(ptr, Remain, "%s=1 ", CmdStr);
        Idx = NvOsStrlen(ptr);
        Remain -= Idx;
        ptr += Idx;
    }

    DbgConsole = NvOdmQueryDebugConsole();
    if (DbgConsole == NvOdmDebugConsole_None)
    {
        CmdStr = "console";
        if (!DoesCommandExist(CmdStr)) {
            NvOsSnprintf(ptr, Remain, "%s=none ", CmdStr);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }

        CmdStr = "debug_uartport";
        if (!DoesCommandExist(CmdStr)) {
            NvOsSnprintf(ptr, Remain, "%s=hsport ", CmdStr);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }
    }
    else if (DbgConsole & NvOdmDebugConsole_Automation)
    {
        CmdStr = "console";
        if (!DoesCommandExist(CmdStr)) {
            NvOsSnprintf(ptr, Remain, "%s=none ", CmdStr);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }

        CmdStr = "debug_uartport";
        if (!DoesCommandExist(CmdStr)) {
            NvOsSnprintf(ptr, Remain, "%s=lsport,%d ", CmdStr,
                          (DbgConsole & 0xF) - NvOdmDebugConsole_UartA);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }
    }
    else
    {
        CmdStr = "console";
        if (!DoesCommandExist(CmdStr))
        {
            NvOsSnprintf(ptr, Remain, "%s=ttyS0,115200n8 ", CmdStr);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;

        }

        CmdStr = "debug_uartport";
        if (!DoesCommandExist(CmdStr))
        {
            NvOsSnprintf(ptr, Remain, "%s=lsport,%d ", CmdStr,
                DbgConsole - NvOdmDebugConsole_UartA);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }
    }

    if (!HasInitRamDisk)
    {
        CmdStr = "console";
        if (!DoesCommandExist(CmdStr))
        {
            NvOsSnprintf(ptr, Remain, "%s=tty1 ", CmdStr);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }
    }

    if (NvOdmQueryGetSkuOverride(&SkuOverride))
    {
        CmdStr = "sku_override";
        if (!DoesCommandExist(CmdStr))
        {
            NvOsSnprintf(ptr, Remain, "%s=%d ", CmdStr, SkuOverride);
            Idx = NvOsStrlen(ptr);
            ptr += Idx;
            Remain -= Idx;
        }
    }

    CmdStr = "usbcore.old_scheme_first";
    if (!DoesCommandExist(CmdStr))
    {
        NvOsSnprintf(ptr, Remain, "%s=1 ", CmdStr);
        Idx = NvOsStrlen(ptr);
        Remain -= Idx;
        ptr += Idx;
    }

    CmdStr = "lp0_vec";
    if (!DoesCommandExist(CmdStr))
    {
        NvOsSnprintf(ptr, Remain, "%s=%d@0x%x ", CmdStr, WB0_SIZE, g_wb0_address);
        Idx = NvOsStrlen(ptr);
        ptr += Idx;
        Remain -= Idx;
    }

    CmdStr = "tegra_fbmem";
    if (!DoesCommandExist(CmdStr))
    {
        NvOsSnprintf(ptr, Remain, "%s=%d@0x%x ", CmdStr, surf_size, surf_address);
        Idx = NvOsStrlen(ptr);
        ptr += Idx;
        Remain -= Idx;
    }

    if (NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_CpuCurrent,
                      &CpuCurrInfo, sizeof(CpuCurrInfo))) {
        CmdStr = "max_cpu_cur_ma";
        if (!DoesCommandExist(CmdStr))
        {
            NvOsSnprintf(ptr, Remain, "%s=%d ",CmdStr,
                         CpuCurrInfo.MaxCpuCurrentmA);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }
    }

    if (NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_PmuBoard,
                      &PmuBoardInfo, sizeof(PmuBoardInfo))) {
        CmdStr = "core_edp_mv";
        if (!DoesCommandExist(CmdStr))
        {
            NvOsSnprintf(ptr, Remain, "%s=%d ", CmdStr,
                         PmuBoardInfo.core_edp_mv);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }

        // Pass the pmu board info if it is require
        if (PmuBoardInfo.isPassBoardInfoToKernel) {
            CmdStr = "pmuboard";
            if (!DoesCommandExist(CmdStr))
            {
                NvOsSnprintf(ptr, Remain,
                    "%s=0x%04x:0x%04x:0x%02x:0x%02x:0x%02x ", CmdStr,
                    PmuBoardInfo.BoardInfo.BoardID, PmuBoardInfo.BoardInfo.SKU,
                    PmuBoardInfo.BoardInfo.Fab, PmuBoardInfo.BoardInfo.Revision,
                    PmuBoardInfo.BoardInfo.MinorRevision);
                 Idx = NvOsStrlen(ptr);
                 Remain -= Idx;
                 ptr += Idx;
            }
        }
    }

    if (NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_DisplayBoard,
                      &DisplayBoardInfo, sizeof(DisplayBoardInfo))) {
        if (DisplayBoardInfo.IsPassBoardInfoToKernel)
        {
            CmdStr = "displayboard";
            if (!DoesCommandExist(CmdStr))
            {
                NvOsSnprintf(ptr, Remain,
                    "%s=0x%04x:0x%04x:0x%02x:0x%02x:0x%02x ", CmdStr,
                    DisplayBoardInfo.BoardInfo.BoardID,
                    DisplayBoardInfo.BoardInfo.SKU,
                    DisplayBoardInfo.BoardInfo.Fab,
                    DisplayBoardInfo.BoardInfo.Revision,
                    DisplayBoardInfo.BoardInfo.MinorRevision);
                Idx = NvOsStrlen(ptr);
                Remain -= Idx;
                ptr += Idx;
            }
        }
    }

    if (NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_PowerSupply,
                      &PowerSupplyInfo, sizeof(PowerSupplyInfo))) {
        CmdStr = "power_supply";
        if (!DoesCommandExist(CmdStr))
        {
            if (PowerSupplyInfo.SupplyType == NvOdmBoardPowerSupply_Adapter)
                NvOsSnprintf(ptr, Remain, "%s=Adapter ", CmdStr);
            else
                NvOsSnprintf(ptr, Remain, "%s=Battery ", CmdStr);
            Idx = NvOsStrlen(ptr);
            Remain -= Idx;
            ptr += Idx;
        }
    }
#ifdef NV_EMBEDDED_BUILD
    {
        NvError e;
        NvU8 *InfoArg;
        NvU32 Size = CMDLINE_BUF_SIZE;

        InfoArg =  NvOsAlloc(Size);
        if(InfoArg != NULL)
        {
            e = NvBootGetSkuInfo(InfoArg, 1, Size);
            if(e == NvSuccess)
            {
                NvOsSnprintf(ptr, Remain, "%s ", InfoArg);
                Idx = NvOsStrlen(ptr);
                Remain -= Idx;
                ptr += Idx;
            }
        }
    }
#endif
    if (NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_AudioCodec,
                      &AudioCodecInfo, sizeof(AudioCodecInfo))) {
        if (NvOsStrlen(AudioCodecInfo.AudioCodecName)) {
            CmdStr = "audio_codec";
            if (!DoesCommandExist(CmdStr))
            {
                NvOsSnprintf(ptr, Remain, "%s=%s ", CmdStr, AudioCodecInfo.AudioCodecName);
                Idx = NvOsStrlen(ptr);
                Remain -= Idx;
                ptr += Idx;
            }
        }
    }

    if (NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_CameraBoard,
                      &CameraBoardInfo, sizeof(CameraBoardInfo))) {
        if (CameraBoardInfo.IsPassBoardInfoToKernel)
        {
            CmdStr = "cameraboard";
            if (!DoesCommandExist(CmdStr))
            {
                NvOsSnprintf(ptr, Remain, "%s=0x%04x:0x%04x:0x%02x:0x%02x:0x%02x ", CmdStr,
                    CameraBoardInfo.BoardInfo.BoardID, CameraBoardInfo.BoardInfo.SKU,
                    CameraBoardInfo.BoardInfo.Fab, CameraBoardInfo.BoardInfo.Revision,
                    CameraBoardInfo.BoardInfo.MinorRevision);
                Idx = NvOsStrlen(ptr);
                Remain -= Idx;
                ptr += Idx;
            }
        }
    }

    if (NvOdmPeripheralGetBoardInfo(0x0, &BoardInfo)) {
        CmdStr = "board_info";
        if (!DoesCommandExist(CmdStr))
        {
            NvOsSnprintf(ptr, Remain, "%s=%x:%x:%x:%x:%x ", CmdStr,
                BoardInfo.BoardID, BoardInfo.SKU, BoardInfo.Fab,
                BoardInfo.Revision, BoardInfo.MinorRevision);

            Idx = NvOsStrlen(ptr);
            ptr += Idx;
            Remain -= Idx;
        }
    }

    if (ProductName[0])
    {
        if (!NvOsMemcmp(ProductName, "up", NvOsStrlen("up")))
        {
            ProductName += NvOsStrlen("up");
            CmdStr = "nosmp";
            if (!DoesCommandExist(CmdStr))
            {
                NvOsSnprintf(ptr, Remain, "%s ", CmdStr);
                Idx = NvOsStrlen(ptr);
                Remain -= Idx;
                ptr += Idx;
            }
            if (ProductName[0]=='_')
                ProductName++;
        }

        if (!HasInitRamDisk && ProductName[0])
        {
            CmdStr = "root";
            if (!DoesCommandExist(CmdStr))
            {
                if (!NvOsMemcmp(ProductName, "usb", NvOsStrlen("usb")))
                {
                    ProductName += NvOsStrlen("usb");
                    NvOsSnprintf(ptr, Remain, "%s=/dev/nfs rw netdevwait ip=:::::usb%c:on ", CmdStr, ProductName[0]);
                    ProductName++;
                }
                else if (!NvOsMemcmp(ProductName, "eth", NvOsStrlen("eth")))
                {
                    ProductName += NvOsStrlen("eth");
                    NvOsSnprintf(ptr, Remain, "%s=/dev/nfs rw netdevwait ip=:::::eth%c:on ", CmdStr, ProductName[0]);
                    ProductName++;
                }
                else if (!NvOsMemcmp(ProductName, "sd", NvOsStrlen("sd")))
                {
                    ProductName += NvOsStrlen("sd");
                    NvOsSnprintf(ptr, Remain,
                        "%s=/dev/sd%c%c rw rootwait ", CmdStr,
                        ProductName[0], ProductName[1]);
                    ProductName += 2;
                }
                else if (!NvOsMemcmp(ProductName, "mmchd", NvOsStrlen("mmchd")))
                {
                    ProductName += NvOsStrlen("mmchd");
                    NvOsSnprintf(ptr, Remain,
                        "%s=/dev/mmchd%c%c%c rw rootwait ", CmdStr,
                        ProductName[0], ProductName[1], ProductName[2]);
                    ProductName += 3;
                }
                else if (!NvOsMemcmp(ProductName, "mtdblock", NvOsStrlen("mtdblock")))
                {
                    ProductName += NvOsStrlen("mtdblock");
                    NvOsSnprintf(ptr, Remain,
                        "%s=/dev/mtdblock%c rw rootwait ", CmdStr, ProductName[0]);
                    ProductName += 1;
                }
                else if (!NvOsMemcmp(ProductName, "mmcblk", NvOsStrlen("mmcblk")))
                {
                    ProductName += NvOsStrlen("mmcblk");
                    NvOsSnprintf(ptr, Remain,
                        "%s=/dev/mmcblk%c%c%c rw rootwait ", CmdStr,
                        ProductName[0], ProductName[1], ProductName[2]);
                    ProductName += 3;
                }
                else
                    FastbootError("Unrecognized root device: %s\n", ProductName);

                Idx = NvOsStrlen(ptr);
                Remain -= Idx;
                ptr += Idx;
            }
        }
    }
#ifndef NV_EMBEDDED_BUILD
    else if (!HasInitRamDisk)
    {
        NvOsSnprintf(ptr, Remain,
           "root=/dev/sda1 rw rootwait ");
        Idx = NvOsStrlen(ptr);
        Remain -= Idx;
        ptr += Idx;
    }
#endif
    CmdStr = "tegraboot";
    if (!DoesCommandExist(CmdStr))
    {
        switch ((NvDdkBlockDevMgrDeviceId)SecondaryBootDevice)
        {
        case NvDdkBlockDevMgrDeviceId_Nand:
            NvOsSnprintf(ptr, Remain, "%s=nand ", CmdStr);
            break;
        case NvDdkBlockDevMgrDeviceId_Nor:
            NvOsSnprintf(ptr, Remain, "%s=nor ", CmdStr);
            break;
        case NvDdkBlockDevMgrDeviceId_SDMMC:
            NvOsSnprintf(ptr, Remain, "%s=sdmmc ", CmdStr);
            break;
        default:
            break;
        }

        Idx = NvOsStrlen(ptr);
        ptr += Idx;
        Remain -= Idx;
    }

    if ((SecondaryBootDevice == NvDdkBlockDevMgrDeviceId_Nand)
            || (SecondaryBootDevice == NvDdkBlockDevMgrDeviceId_Nor))
    {
        CmdStr = "mtdparts";
        if (!DoesCommandExist(CmdStr))
        {
            NvBool first = NV_TRUE;
            for (i=0; i<NumPartitions; i++)
            {
                NvU64 Start, Num;
                NvU32 SectorSize;
                if (NvAbootGetPartitionParameters(hAboot,
                    PartitionList[i].NvPartName, &Start, &Num,
                    &SectorSize) == NvSuccess)
                {
                    if (first)
                    {
                        if (SecondaryBootDevice == NvDdkBlockDevMgrDeviceId_Nand)
                            NvOsSnprintf(ptr, Remain, "%s=tegra_nand:", CmdStr);
                        else
                            NvOsSnprintf(ptr, Remain, "%s=tegra-nor:", CmdStr);
                        Idx = NvOsStrlen(ptr);
                        ptr += Idx;
                        Remain -= Idx;
                        first = NV_FALSE;
                    }

                    NvOsSnprintf(ptr, Remain, "%uK@%uK(%s),",
                        (NvU32)(Num * SectorSize >> 10),
                        (NvU32)(Start * SectorSize >> 10),
                        PartitionList[i].LinuxPartName);
                    Idx = NvOsStrlen(ptr);
                    ptr += Idx;
                    Remain -= Idx;
                }
            }
            if (!first)
            {
                ptr--;
                *ptr = ' ';
                ptr++;
            }
        }
    }
    else
    {
        NvU32 PartitionIdGpt2;
        if ((PartitionIdGpt2 = GetPartitionByType(NvPartMgrPartitionType_GPT)) != 0)
        {
            char PartName[NVPARTMGR_PARTITION_NAME_LENGTH];

            CmdStr = "gpt";
            if (!DoesCommandExist(CmdStr))
            {
                NvOsSnprintf(ptr, Remain, "%s ", CmdStr);
                Idx = NvOsStrlen(ptr);
                ptr += Idx;
                Remain -= Idx;
            }

            CmdStr = "gpt_sector";
            if (!DoesCommandExist(CmdStr))
            {
                if (NvPartMgrGetNameById(PartitionIdGpt2, PartName) == NvSuccess)
                {
                    NvU64 PartitionStart, PartitionSize;
                    NvU32 TempSectorSize;
                    NvU32 GptSector;
                    if (NvAbootGetPartitionParameters(hAboot,
                        PartName, &PartitionStart, &PartitionSize,
                        &TempSectorSize) == NvSuccess)
                    {
                        GptSector = (PartitionStart + PartitionSize)
                                    * (TempSectorSize / GPT_BLOCK_SIZE) - 1;

                        NvOsSnprintf(ptr, Remain, "%s=%d ", CmdStr, GptSector);
                        Idx = NvOsStrlen(ptr);
                        ptr += Idx;
                        Remain -= Idx;
                    }
                    else
                    {
                        FastbootError("Unable to query partition %s\n", PartName);
                    }
                }
            }
        }
        else
        {
            CmdStr = "tegrapart";
            if (!DoesCommandExist(CmdStr))
            {
                NvOsSnprintf(ptr, Remain, "%s=", CmdStr);
                Idx = NvOsStrlen(ptr);
                ptr += Idx;
                Remain -= Idx;

                for (i=0; i<NumPartitions; i++)
                {
                    NvU64 Start, Num;
                    NvU32 SectorSize;
                    if (NvAbootGetPartitionParameters(hAboot,
                        PartitionList[i].NvPartName, &Start, &Num,
                        &SectorSize) == NvSuccess)
                    {
                        NvOsSnprintf(ptr, Remain, "%s:%x:%x:%x%c",
                            PartitionList[i].LinuxPartName, (NvU32)Start, (NvU32)Num,
                            SectorSize, (i==(NumPartitions-1))?' ':',');
                        Idx = NvOsStrlen(ptr);
                        ptr += Idx;
                        Remain -= Idx;
                    }
                    else
                    {
                        FastbootError("Unable to query partition %s\n", PartitionList[i].NvPartName);
                    }
                }
            }
        }
    }

    if (NvOdmQueryGetModemId(&ModemId))
    {
        CmdStr = "modem_id";
        if (!DoesCommandExist(CmdStr))
        {
            NvOsSnprintf(ptr, Remain, "%s=%d ", CmdStr, ModemId);
            Idx = NvOsStrlen(ptr);
            ptr += Idx;
            Remain -= Idx;
        }
    }

    if (RecoveryKernel)
        NvOsSnprintf(ptr, Remain, "android.kerneltype=recovery");
    else
        NvOsSnprintf(ptr, Remain, "android.kerneltype=normal");
    Idx = NvOsStrlen(ptr);
    ptr += Idx;
    Remain -= Idx;
	
    NvOsSnprintf(ptr, Remain, " vmalloc=512MB");
    Idx = NvOsStrlen(ptr);
    ptr += Idx;
    Remain -= Idx;
    
    if (fdt_hdr)
    {
        node = fdt_path_offset((void *)fdt_hdr, "/chosen");
        if (node < 0)
        {
            NvOsDebugPrintf("%s: Unable to open dtb image (%s)\n", __func__, fdt_strerror(node));
            NvErr = NvError_ResourceError;
            goto fail;
        }

        err = fdt_setprop((void *)fdt_hdr, node, "bootargs", s_CmdLine, strlen(s_CmdLine));
        if (err < 0)
        {
             NvOsDebugPrintf("%s: Unable to set bootargs (%s)\n", __func__, fdt_strerror(err));
             NvErr = NvError_ResourceError;
             goto fail;
        }
    }
    else
    {
        FastbootAddAtag(ATAG_CMDLINE, sizeof(s_CmdLine), s_CmdLine);
    }
fail:
    NvRmClose(hRm);
    return NvErr;
}

NvError AddSerialBoardInfoTag(void)
{
    NvU32 BoardInfoData[2];
    NvOdmBoardInfo BoardInfo;
    NvBool Ret = NV_FALSE;
    int node, newnode, err;
    NvError NvErr = NvError_Success;

    BoardInfoData[0] = 0;
    BoardInfoData[1] = 0;

    Ret = NvOdmQueryGetBoardModuleInfo(NvOdmBoardModuleType_ProcessorBoard,
                 &BoardInfo, sizeof(NvOdmBoardInfo));
    if (Ret) {
        // BoardId:SKU:Fab:Revision:MinorRevision
        BoardInfoData[1] = (BoardInfo.BoardID << 16) | BoardInfo.SKU;
        BoardInfoData[0] = (BoardInfo.Fab << 24) | (BoardInfo.Revision << 16) |
                                     (BoardInfo.MinorRevision << 8);
    }

    if (fdt_hdr)
    {
        node = fdt_path_offset((void *)fdt_hdr, "/chosen/board_info");
        if (node < 0)
        {
            node = fdt_path_offset((void *)fdt_hdr, "/chosen");
            if (node >= 0)
                goto skip_chosen;

            node = fdt_add_subnode((void *)fdt_hdr, 0, "chosen");
            if (node < 0)
            {
                NvOsDebugPrintf("%s: Unable to create /chosen (%s)\n", __func__,
                    fdt_strerror(node));
                NvErr = NvError_ResourceError;
                goto fail;
            }

skip_chosen:
            newnode = fdt_add_subnode((void *)fdt_hdr, node, "board_info");
            if (newnode < 0)
            {
                NvOsDebugPrintf("%s: Unable to create board_info (%s)\n", __func__,
                    fdt_strerror(node));
                NvErr = NvError_ResourceError;
                goto fail;
            }

            node = newnode;
        }

        err = fdt_setprop_cell((void *)fdt_hdr, node, "id", BoardInfo.BoardID);
        if (err)
        {
            NvOsDebugPrintf("%s: Unable to set id (%s)\n", __func__,
                fdt_strerror(err));
            NvErr = NvError_ResourceError;
            goto fail;
        }

        err = fdt_setprop_cell((void *)fdt_hdr, node, "sku", BoardInfo.SKU);
        if (err)
        {
            NvOsDebugPrintf("%s: Unable to set sku (%s)\n", __func__,
                fdt_strerror(err));
            NvErr = NvError_ResourceError;
            goto fail;
        }

        err = fdt_setprop_cell((void *)fdt_hdr, node, "fab", BoardInfo.Fab);
        if (err)
        {
            NvOsDebugPrintf("%s: Unable to set fab (%s)\n", __func__,
                fdt_strerror(err));
            NvErr = NvError_ResourceError;
            goto fail;
        }

        err = fdt_setprop_cell((void *)fdt_hdr, node, "major_revision",
                BoardInfo.Revision);
        if (err)
        {
            NvOsDebugPrintf("%s: Unable to set major_revision (%s)\n", __func__,
                fdt_strerror(err));
            NvErr = NvError_ResourceError;
            goto fail;
        }

        err = fdt_setprop_cell((void *)fdt_hdr, node, "minor_revision",
                BoardInfo.MinorRevision);
        if (err)
        {
            NvOsDebugPrintf("%s: Unable to set minor_revision (%s)\n", __func__,
                fdt_strerror(err));
            NvErr = NvError_ResourceError;
            goto fail;
        }
    }
    else
    {
        FastbootAddAtag(ATAG_SERIAL, sizeof(BoardInfoData), BoardInfoData);
    }

fail:
    return NvErr;
}

/*  Adds an ATAG to the list */
NvBool FastbootAddAtag(
    NvU32        Atag,
    NvU32        DataSize,
    const void  *Data)
{
    static NvU32 *tag = (NvU32 *)s_BootTags;
    NvU32 TagWords = ((DataSize+3)>>2) + 2;

    if (fdt_hdr)
        NvOsDebugPrintf("%s: setting ATAG (%d)\n", __func__, Atag);

    if (!Atag)
    {
        *tag++ = 0;
        *tag++ = 0;
        tag = NULL;
        return NV_TRUE;
    }
    else if (tag)
    {
        tag[0] = TagWords;
        tag[1] = Atag;
        if (Data)
            NvOsMemcpy(&tag[2], Data, DataSize);
        tag += TagWords;
        return NV_TRUE;
    }
    return NV_FALSE;
}
