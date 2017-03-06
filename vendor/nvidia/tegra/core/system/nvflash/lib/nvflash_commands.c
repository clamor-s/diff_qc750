/*
 * Copyright (c) 2008-2012, NVIDIA CORPORATION. All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "nvflash_commands.h"
#include "nvutil.h"
#include "nvassert.h"
#include "nvflash_verifylist.h"
#include <nv3p.h>

#define NV_FLASH_MAX_COMMANDS (32)
#define MAX_SUPPORTED_DEVICES 127

#ifdef NV_EMBEDDED_BUILD
#if NVOS_IS_LINUX || NVOS_IS_UNIX
#define PATH_DELIMITER '/'
#else
#define PATH_DELIMITER '\\'
#endif
#endif

typedef struct CommandRec
{
    NvFlashCommand cmd;
    void *arg;
} Command;

#ifdef NV_EMBEDDED_BUILD
static char *s_NvFlash_BaseDir;
#endif
static NvU32 s_NumCommands;
static Command s_Commands[NV_FLASH_MAX_COMMANDS];
static NvFlashBctFiles s_OptionBct;
static const char *s_OptionConfigFile;
static const char *s_OptionBlob;
static NvFlashCmdRcm s_OptionRcm;
static const char *s_OptionBlHash;
static const char *s_OptionBootLoader;
static NvFlashCmdDiskImageOptions s_DiskImg_options;
static NvU32 s_OptionOdmData;
static const NvU8 *s_OptionBootDevType;
static NvU32 s_OptionBootDevConfig;
static NvBool s_OptionQuiet;
static NvBool s_OptionWait;
static NvBool s_FormatAll;
static NvBool s_OptionVerifyPartitionsEnabled;
static NvBool s_OptionSetBct;
static NvBool s_OptionSetVerifySdram;
static NvU32 s_OptionVerifySdramvalue;
static NvU32 s_OptionTransportMode = 0;
static NvU32 s_OptionEmulationDevice = 0;
static NvFlashCmdDevParamOptions s_DevParams;
static NvU32 s_OptionDevInstance = 0;
static NvFlashSkipPart s_OptionSkipPart;
static const char *s_LastError;
static NvU32 s_CurrentCommand;
static NvBool s_CreateCommandOccurred;
static char s_Buffer[256];
static NvBool AddPartitionToVerify(const char *Partition);

#define NVFLASH_INVALID_ENTRY	0xffffffff
static Nv3pCmdDownloadBootloader s_OptionEntryAndAddress;

NvError
NvFlashCommandParse( NvU32 argc, const char *argv[] )
{
    NvU32 i;
    NvU32 idx = 0;
    const char *arg;
    NvBool bOpt;

    s_LastError = 0;
    s_NumCommands = 0;
    s_CurrentCommand = 0;
    s_OptionBct.BctOrCfgFile = 0;
    s_OptionBct.OutputBctFile = 0;
    s_OptionBlHash = 0;
    s_OptionRcm.RcmFile1 = 0;
    s_OptionRcm.RcmFile2 = 0;
    s_OptionConfigFile = 0;
    s_OptionBlob = 0;
    s_OptionBootLoader = 0;
    s_OptionVerifyPartitionsEnabled = NV_FALSE;
    s_CreateCommandOccurred = NV_FALSE;
    s_OptionSetBct = NV_FALSE;
    s_OptionSetVerifySdram = NV_FALSE;
    s_OptionVerifySdramvalue = 0;
    s_OptionSkipPart.number = 0;
    NvOsMemset( s_Commands, 0, sizeof(s_Commands) );

    s_OptionEntryAndAddress.EntryPoint = NVFLASH_INVALID_ENTRY;
#ifdef NV_EMBEDDED_BUILD
    {
        NvU32 len;
        s_NvFlash_BaseDir = NvOsAlloc(NvOsStrlen(argv[0]) + 1);
        if(!s_NvFlash_BaseDir)
        {
            s_LastError = "--NvFlash_BaseDir allocation failure";
            goto fail;
        }

        NvOsStrncpy(s_NvFlash_BaseDir, argv[0], NvOsStrlen(argv[0]) + 1);
        len = NvOsStrlen(s_NvFlash_BaseDir);
        while (len > 0)
        {
            if (s_NvFlash_BaseDir[len] == PATH_DELIMITER)
                break;
            len--;
        }

        s_NvFlash_BaseDir[len] = '\0';
    }
#endif
    for( i = 1; i < argc; i++ )
    {
        arg = argv[i];
        bOpt = NV_FALSE;

        if(( NvOsStrcmp( arg, "--help" ) == 0 )|| (NvOsStrcmp( arg, "-h" ) == 0 ))
        {
            // really should just skip this command, but this is an amusing
            // easter egg.
            s_Commands[idx].cmd = NvFlashCommand_Help;
        }
        else if(( NvOsStrcmp( arg, "--cmdhelp" ) == 0 ) || ( NvOsStrcmp( arg, "-ch" ) == 0 ))
        {
            NvFlashCmdHelp *a;
            s_Commands[idx].cmd = NvFlashCommand_CmdHelp;
            if( i + 1 >= argc )
            {
                s_LastError = "--cmdhelp argument missing";
                goto fail;
            }
            a = NvOsAlloc( sizeof(NvFlashCmdHelp) );
            if( !a )
            {
                s_LastError = "--getbit allocation failure";
                goto fail;
            }
            i++;
            a->cmdname = argv[i];
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--resume" ) == 0 ||
            NvOsStrcmp( arg, "-r" ) == 0 )
        {
            if( i == 1 )
            {
                continue;
            }
            else
            {
                s_LastError = "resume must be specified first in the "
                    "command line";
                goto fail;
            }
        }
        else if( NvOsStrcmp( arg, "--create" ) == 0 )
        {
            s_CreateCommandOccurred = NV_TRUE;
            s_Commands[idx].cmd = NvFlashCommand_Create;
        }
        else if( NvOsStrcmp( arg, "--setboot" ) == 0 )
        {
            NvFlashCmdSetBoot *a;

            if( i + 1 >= argc )
            {
                s_LastError = "--setboot argument missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_SetBoot;
            a = NvOsAlloc( sizeof(NvFlashCmdSetBoot) );
            if( !a )
            {
                s_LastError = "--setboot allocation failure";
                goto fail;
            }

            i++;
            a->PartitionID = NvUStrtoul( argv[i], 0, 0 );
            if (!a->PartitionID)
                a->PartitionName = argv[i];
            else
                a->PartitionName = NULL;
            s_Commands[idx].arg = a;
        }
        else if(( NvOsStrcmp( arg, "--format_partition" ) == 0 ) ||
            ( NvOsStrcmp( arg, "--delete_partition" ) == 0 ))
        {
            NvFlashCmdFormatPartition *a;

            if( i + 1 >= argc )
            {
                s_LastError = "--format_partition argument missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_FormatPartition;
            a = NvOsAlloc( sizeof(NvFlashCmdFormatPartition) );
            if( !a )
            {
                s_LastError = "--format_partition allocation failure";
                goto fail;
            }

            i++;
            a->PartitionID = NvUStrtoul( argv[i], 0, 0 );
            if (!a->PartitionID)
                a->PartitionName = argv[i];
            else
                a->PartitionName = NULL;
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--verifypart" ) == 0 )
        {
            NvBool PartitionAdded = NV_FALSE;

            if( i + 1 >= argc )
            {
                s_LastError = "--verifypart argument missing";
                goto fail;
            }

            i++;
            PartitionAdded = AddPartitionToVerify(argv[i]);
            if (!PartitionAdded)
            {
                goto fail;
            }

            s_OptionVerifyPartitionsEnabled = NV_TRUE;
            bOpt = NV_TRUE;

        }
        else if( NvOsStrcmp( arg, "--download" ) == 0 )
        {
            NvFlashCmdDownload *a;

            if( i + 2 >= argc )
            {
                s_LastError = "--download arguments missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_Download;
            a = NvOsAlloc( sizeof(NvFlashCmdDownload) );
            if( !a )
            {
                s_LastError = "--download allocation failure";
                goto fail;
            }

            i++;
            a->PartitionID = NvUStrtoul( argv[i], 0, 0 );
            if (!a->PartitionID)
                a->PartitionName = argv[i];
            else
                a->PartitionName = NULL;
            i++;
            a->filename = argv[i];
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--read" ) == 0 )
        {
            NvFlashCmdRead *a;

            if( i + 2 >= argc )
            {
                s_LastError = "--read arguments missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_Read;
            a = NvOsAlloc( sizeof(NvFlashCmdRead) );
            if( !a )
            {
                s_LastError = "--read allocation failure";
                goto fail;
            }

            i++;
            a->PartitionID = NvUStrtoul( argv[i], 0, 0 );
            if (!a->PartitionID)
                a->PartitionName = argv[i];
            else
                a->PartitionName = NULL;
            i++;
            a->filename = argv[i];
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--updatebct" ) == 0 )
        {
            NvFlashCmdUpdateBct *a;

            if( i + 1 >= argc )
            {
                s_LastError = "--updatebct arguments missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_UpdateBct;
            a = NvOsAlloc( sizeof(NvFlashCmdUpdateBct) );
            if( !a )
            {
                s_LastError = "--updatebct allocation failure";
                goto fail;
            }

            i++;
            a->bctsection= argv[i];
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--rawdeviceread" ) == 0 )
        {
            NvFlashCmdRawDeviceReadWrite *a;

            if( i + 3 >= argc )
            {
                s_LastError = "--rawdeviceread arguments missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_RawDeviceRead;
            a = NvOsAlloc( sizeof(NvFlashCmdRawDeviceReadWrite) );
            if( !a )
            {
                s_LastError = "--rawdeviceread allocation failure";
                goto fail;
            }

            i++;
            a->StartSector = NvUStrtoul( argv[i], 0, 0 );
            i++;
            a->NoOfSectors = NvUStrtoul( argv[i], 0, 0 );
            i++;
            a->filename = argv[i];
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--rawdevicewrite" ) == 0 )
        {
            NvFlashCmdRawDeviceReadWrite *a;

            if( i + 3 >= argc )
            {
                s_LastError = "--rawdevicewrite arguments missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_RawDeviceWrite;
            a = NvOsAlloc( sizeof(NvFlashCmdRawDeviceReadWrite) );
            if( !a )
            {
                s_LastError = "--rawdevicewrite allocation failure";
                goto fail;
            }

            i++;
            a->StartSector = NvUStrtoul( argv[i], 0, 0 );
            i++;
            a->NoOfSectors = NvUStrtoul( argv[i], 0, 0 );
            i++;
            a->filename = argv[i];
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--getpartitiontable" ) == 0 )
        {
            NvFlashCmdGetPartitionTable *a;

            if( i + 1 >= argc )
            {
                s_LastError = "--getpartitiontable argument missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_GetPartitionTable;
            a = NvOsAlloc( sizeof(NvFlashCmdGetPartitionTable) );
            if( !a )
            {
                s_LastError = "--getpartitiontable allocation failure";
                goto fail;
            }

            i++;
            a->filename = argv[i];
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--getbct" ) == 0 )
        {
            s_Commands[idx].cmd = NvFlashCommand_GetBct;
        }
        else if( NvOsStrcmp( arg, "--getbit" ) == 0 )
        {
            NvFlashCmdGetBit *a;

            if( i + 1 >= argc )
            {
                s_LastError = "--getbit argument missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_GetBit;
            a = NvOsAlloc( sizeof(NvFlashCmdGetBit) );
            if( !a )
            {
                s_LastError = "--getbit allocation failure";
                goto fail;
            }

            i++;
            a->filename = argv[i];
            s_Commands[idx].arg = a;
        }
        else if ( NvOsStrcmp( arg, "--dumpbit" ) == 0 )
        {
            NvFlashCmdDumpBit *a;
            char DefaultOption[] = "anything";

            s_Commands[idx].cmd = NvFlashCommand_DumpBit;
            a = NvOsAlloc( sizeof(NvFlashCmdDumpBit) );
            if( !a )
            {
                s_LastError = "--dumpbit allocation failure";
                goto fail;
            }

            a->DumpbitOption = DefaultOption;
            if (argv[i + 1][0] != '-')
            {
                i++;
                a->DumpbitOption = argv[i];
            }
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--odm" ) == 0 )
        {
            NvFlashCmdSetOdmCmd *a;

            if( i + 1 >= argc )
            {
                s_LastError = "--odm argument missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_SetOdmCmd;

            a = NvOsAlloc( sizeof(NvFlashCmdSetOdmCmd) );
            if( !a )
            {
                s_LastError = "--odm allocation failure";
                goto fail;
            }
            i++;
            if( NvOsStrcmp(argv[i], "fuelgaugefwupgrade" ) == 0)
            {
                a->odmExtCmd = NvFlashOdmExtCmd_FuelGaugeFwUpgrade;
                if ( i + 1 >= argc )
                {
                    s_LastError = "fuelgaugefwupgrade argument missing";
                    goto fail;
                }
                i++;
                a->odmExtCmdParam.fuelGaugeFwUpgrade.filename1 = argv[i];
                if( i + 1 < argc && argv[i+1][0] != '-' )
                {
                    i++;
                    a->odmExtCmdParam.fuelGaugeFwUpgrade.filename2 = argv[i];
                }
                else
                {
                    a->odmExtCmdParam.fuelGaugeFwUpgrade.filename2 = NULL;
                }
            }
            else if( NvOsStrcmp(argv[i], "runsddiag") == 0 )
            {
                a->odmExtCmd = NvFlashOdmExtCmd_RunSdDiag;
                if( i + 2 >= argc )
                {
                    s_LastError = "RunSdDiag argument missing";
                    goto fail;
                }
                i++;
                a->odmExtCmdParam.sdDiag.Value = NvUStrtoul(argv[i], 0, 0);
                i++;
                a->odmExtCmdParam.sdDiag.TestType = NvUStrtoul(argv[i], 0, 0);
            }
            else if( NvOsStrcmp(argv[i], "runsediag") == 0 )
            {
                a->odmExtCmd = NvFlashOdmExtCmd_RunSeDiag;
                if( i + 1 >= argc )
                {
                    s_LastError = "RunSeDiag argument missing";
                    goto fail;
                }
                i++;
                a->odmExtCmdParam.seDiag.Value = NvUStrtoul(argv[i], 0, 0);
            }
            else if( NvOsStrcmp(argv[i], "runpwmdiag") == 0 )
            {
                a->odmExtCmd = NvFlashOdmExtCmd_RunPwmDiag;
            }
            else if( NvOsStrcmp(argv[i], "rundsidiag") == 0 )
            {
                a->odmExtCmd = NvFlashOdmExtCmd_RunDsiDiag;
            }
            else if (NvOsStrcmp(argv[i], "verifysdram") == 0)
            {
                s_OptionSetVerifySdram = NV_TRUE;
                if (i + 1 >= argc)
                {
                    s_LastError = "verifysdram argument missing";
                    goto fail;
                }
                i++;
                s_OptionVerifySdramvalue = NvUStrtoul(argv[i], 0, 0);
                bOpt = NV_TRUE;
            }
            else
            {
                s_LastError = "--odm invalid command";
                goto fail;
            }
            s_Commands[idx].arg = a;
        }
        else if( NvOsStrcmp( arg, "--go" ) == 0 )
        {
            s_Commands[idx].cmd = NvFlashCommand_Go;
        }
        else if( NvOsStrcmp( arg, "--sync" ) == 0 )
        {
            s_Commands[idx].cmd = NvFlashCommand_Sync;
        }
        else if( NvOsStrcmp( arg, "--obliterate" ) == 0 )
        {
            s_Commands[idx].cmd = NvFlashCommand_Obliterate;
        }
        else if( NvOsStrcmp( arg, "--configfile" ) == 0 )
        {
            if( i + 1 >= argc )
            {
                s_LastError = "--configfile argument missing";
                goto fail;
            }

            i++;
            s_OptionConfigFile = argv[i];
            bOpt = NV_TRUE;
        }
        else if(NvOsStrcmp(arg, "--blob" ) == 0)
        {
            if(i + 1 >= argc)
            {
                s_LastError = "--blob argument missing";
                goto fail;
            }

            i++;
            s_OptionBlob = argv[i];
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--bct" ) == 0 )
        {
            if( i + 1 >= argc )
            {
                s_LastError = "--bct argument missing";
                goto fail;
            }

            i++;
            s_OptionBct.BctOrCfgFile = argv[i];
            if (argv[i + 1][0] != '-')
            {
                i++;
                s_OptionBct.OutputBctFile = argv[i];
            }
            bOpt = NV_TRUE;
        }
        else if (NvOsStrcmp(arg, "--rcm") == 0)
        {
            if ((i + 2) >= argc)
            {
                s_LastError = "--rcm argument missing";
                goto fail;
            }
            i++;
            s_OptionRcm.RcmFile1 = argv[i];
            i++;
            if (argv[i][0] == '-')
            {
                s_LastError = "--rcm second argument missing";
                goto fail;
            }
            s_OptionRcm.RcmFile2 = argv[i];
            bOpt = NV_TRUE;
        }
        else if (NvOsStrcmp(arg, "--setblhash") == 0)
        {
            if ((i + 1) >= argc)
            {
                s_LastError = "--setblhash argument missing";
                goto fail;
            }
            i++;
            s_OptionBlHash = argv[i];
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--bl" ) == 0 )
        {
            if( i + 1 >= argc )
            {
                s_LastError = "--bl argument missing";
                goto fail;
            }

            i++;
            s_OptionBootLoader = argv[i];
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--odmdata" ) == 0)
        {
            if( i + 1 >= argc )
            {
                s_LastError = "--odmdata argument missing";
                goto fail;
            }

            i++;
            s_OptionOdmData = NvUStrtoul( argv[i], 0, 0 );
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--deviceid" ) == 0)
        {
            if( i + 1 >= argc )
            {
                s_LastError = "deviceid argument missing";
                goto fail;
            }

            i++;
            s_OptionEmulationDevice = NvUStrtoul( argv[i], 0, 0 );
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--transport" ) == 0)
        {
            if( i + 1 >= argc )
            {
                s_LastError = "transport mode argument missing";
                goto fail;
            }

            i++;
            if(NvOsStrcmp(argv[i], (const char *)"simulation") == 0)
                s_OptionTransportMode = NvFlashTransportMode_Simulation;
#if NVODM_BOARD_IS_FPGA
            else if(NvOsStrcmp(argv[i], (const char *)"jtag") == 0)
                s_OptionTransportMode = NvFlashTransportMode_Jtag;
#endif
            else if(NvOsStrcmp(argv[i], (const char *)"usb") == 0)
                s_OptionTransportMode = NvFlashTransportMode_Usb;
            else
                s_OptionTransportMode = NvFlashTransportMode_default;

            bOpt = NV_TRUE;
        }
        else if (NvOsStrcmp(arg, "--setbootdevtype") == 0)
        {
            if (i + 1 >= argc)
            {
                s_LastError = "--setbootdevtype argument missing";
                goto fail;
            }

            i++;
            s_OptionBootDevType = (const NvU8 *)argv[i];
            bOpt = NV_TRUE;
        }
        else if (NvOsStrcmp(arg, "--setbootdevconfig") == 0)
        {
            if (i + 1 >= argc)
            {
                s_LastError = "--setbootdevconfig argument missing";
                goto fail;
            }

            i++;
            s_OptionBootDevConfig = NvUStrtoul(argv[i],0,0);
            bOpt = NV_TRUE;
        }
        else if ( NvOsStrcmp( arg, "--diskimgopt" ) == 0 )
        {
            if ( i + 1 >= argc )
            {
                s_LastError = "--diskimgopt argument missing";
                goto fail;
            }

            i++;
            s_DiskImg_options.BlockSize = NvUStrtoul( argv[i], 0, 0 );
            bOpt = NV_TRUE;
        }
        else if ( NvOsStrcmp( arg, "--devparams" ) == 0 )
        {
            if ( i + 3 >= argc )
            {
                s_LastError = "--devparams argument missing";
                goto fail;
            }

            i++;
            s_DevParams.PageSize = NvUStrtoul( argv[i], 0, 0 );
            i++;
            s_DevParams.BlockSize = NvUStrtoul( argv[i], 0, 0 );
            i++;
            s_DevParams.TotalBlocks = NvUStrtoul( argv[i], 0, 0 );
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--quiet" ) == 0 ||
            NvOsStrcmp( arg, "-q" ) == 0 )
        {
            s_OptionQuiet = NV_TRUE;
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--wait" ) == 0 ||
            NvOsStrcmp( arg, "-w" ) == 0 )
        {
            s_OptionWait = NV_TRUE;
            bOpt = NV_TRUE;
        }
        else if(( NvOsStrcmp( arg, "--delete_all" ) == 0 ) ||
            (NvOsStrcmp( arg, "--format_all" ) == 0 ))
        {
            s_FormatAll = NV_TRUE;
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--setbct" ) == 0 )
        {
            s_OptionSetBct= NV_TRUE;
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--settime" ) == 0 )
        {
            s_Commands[idx].cmd = NvFlashCommand_SetTime;
        }
        else if( NvOsStrcmp( arg, "--instance" ) == 0 )
        {
            NvU32 argval = 0;

            if( i + 1 >= argc )
            {
                s_LastError = "instance argument missing";
                goto fail;
            }

            i++;

            // Check input for usb device path string or instance number
            if (argv[i][0] >= '0' && argv[i][0] <= '9')
            {
                argval = NvUStrtoul(argv[i], 0, 0);
                //validate if device instance is crossing max supported devices
                if(argval > MAX_SUPPORTED_DEVICES)
                {
                    s_LastError = "--instance wrong parameter";
                    goto fail;
                }
            }
            else
            {
                argval = NvFlashEncodeUsbPath(argv[i]);
                if (argval == 0xFFFFFFFF)
                    goto fail;
            }

            s_OptionDevInstance = argval;
            bOpt = NV_TRUE;
        }
        // added to set entry & address thru command line
        else if (NvOsStrcmp( arg, "--setentry" ) == 0)
        {
            if( i + 2 >= argc )
            {
                s_LastError = "--set entry argument missing";
                goto fail;
            }

            i++;
            s_OptionEntryAndAddress.EntryPoint = NvUStrtoul(argv[i], 0, 0);
            i++;
            s_OptionEntryAndAddress.Address = NvUStrtoul(argv[i], 0, 0);
            bOpt = NV_TRUE;
        }
        else if (NvOsStrcmp(arg, "--skip_part") == 0)
        {
            if (i + 1 >= argc)
            {
                s_LastError = "--skip_part argument missing";
                goto fail;
            }

            i++;
            s_OptionSkipPart.pt_name[s_OptionSkipPart.number] = argv[i];
            s_OptionSkipPart.number++;
            bOpt = NV_TRUE;
        }
        else if( NvOsStrcmp( arg, "--internaldata" ) == 0 )
        {
            NvFlashCmdNvPrivData *a;
            if( i + 1 >= argc )
            {
                s_LastError = "--NvInternal-data argument missing";
                goto fail;
            }

            s_Commands[idx].cmd = NvFlashCommand_NvPrivData;
            a = NvOsAlloc( sizeof(NvFlashCmdNvPrivData));
            if( !a )
            {
                s_LastError = "--NvInternal-data allocation failure";
                goto fail;
            }

            i++;
            a->filename = argv[i];
            s_Commands[idx].arg = a;
        }
        else
        {
            NvOsSnprintf( s_Buffer, sizeof(s_Buffer), "unknown command: %s",
                arg );
            s_LastError = s_Buffer;
            goto fail;
        }

        if( !bOpt )
        {
            idx++;
        }

        if( idx == NV_FLASH_MAX_COMMANDS )
        {
            s_LastError = "too many commands";
            goto fail;
        }
    }

    s_NumCommands = idx;

    // Constraint: verify_partition option is to be used in conjunction with
    // --create command.
    if (s_OptionVerifyPartitionsEnabled && !s_CreateCommandOccurred)
    {
        s_LastError = "--verify_partition used without --create";
        goto fail;
    }

    return NvSuccess;

fail:
    return NvError_BadParameter;
}

NvU32
NvFlashGetNumCommands( void )
{
    return s_NumCommands;
}

NvError
NvFlashCommandGetCommand( NvFlashCommand *cmd, void **arg )
{
    NV_ASSERT( cmd );
    NV_ASSERT( arg );

    s_LastError = 0;

    if( s_CurrentCommand == s_NumCommands )
    {
        s_LastError = "invalid command requested";
        // FIXME: better error code
        return NvError_BadParameter;
    }

    *cmd = s_Commands[s_CurrentCommand].cmd;
    *arg = s_Commands[s_CurrentCommand].arg;
    s_CurrentCommand++;
    return NvSuccess;
}

NvError
NvFlashCommandGetOption( NvFlashOption opt, void **data )
{
    NV_ASSERT( data );

    s_LastError = 0;

    switch( opt ) {
    case NvFlashOption_Bct:
        *data = (void *)&s_OptionBct;
        break;
    case NvFlashOption_ConfigFile:
        *data = (void *)s_OptionConfigFile;
        break;
    case NvFlashOption_Blob:
        *data = (void *)s_OptionBlob;
        break;
    case NvFlashOption_BlHash:
        *data = (void *)s_OptionBlHash;
        break;
    case NvFlashOption_Rcm:
        *data = (void *)&s_OptionRcm;
        break;
    case NvFlashOption_Bootloader:
        *data = (void *)s_OptionBootLoader;
        break;
    case NvFlashOption_DevParams:
        *data = (void *)&s_DevParams;
        break;
    case NvFlashOption_DiskImgOpt:
        *data = (void *)&s_DiskImg_options;
        break;
    case NvFlashOption_OdmData:
        *data = (void *)s_OptionOdmData;
        break;
    case NvFlashOption_EmulationDevId:
        *data = (void *)s_OptionEmulationDevice;
        break;
    case NvFlashOption_TransportMode:
        *data = (void *)s_OptionTransportMode;
        break;
    case NvFlashOption_SetBootDevType:
        *data = (void *)s_OptionBootDevType;
        break;
    case NvFlashOption_SetBootDevConfig:
        *data = (void *)s_OptionBootDevConfig;
        break;
    case NvFlashOption_FormatAll:
        *(NvBool *)data = s_FormatAll;
        break;
    case NvFlashOption_Quiet:
        *(NvBool *)data = s_OptionQuiet;
        break;
    case NvFlashOption_Wait:
        *(NvBool *)data = s_OptionWait;
        break;
    case NvFlashOption_VerifyEnabled:
        *(NvBool *)data = s_OptionVerifyPartitionsEnabled;
        break;
    case NvFlashOption_SetBct:
        *(NvBool *)data = s_OptionSetBct;
        break;
    case NvFlashOption_DevInstance:
        *(NvU32 *)data = s_OptionDevInstance;
        break;
    case NvFlashOption_EntryAndAddress:
        if (s_OptionEntryAndAddress.EntryPoint == NVFLASH_INVALID_ENTRY)
            return NvError_BadParameter;

        ((Nv3pCmdDownloadBootloader *)data)->EntryPoint = s_OptionEntryAndAddress.EntryPoint;
        ((Nv3pCmdDownloadBootloader *)data)->Address = s_OptionEntryAndAddress.Address;
        break;
    case NvFlashOption_SetOdmCmdVerifySdram:
        *(NvBool *)data = s_OptionSetVerifySdram;
        break;
    case NvFlashOption_OdmCmdVerifySdramVal:
        *(NvU32 *)data = s_OptionVerifySdramvalue;
        break;
#ifdef NV_EMBEDDED_BUILD
    case NvFlashOption_NvFlash_BaseDir:
        *data = (void *)s_NvFlash_BaseDir;
        break;
#endif
    case NvFlashOption_SkipPartition:
        // CHECK 1: Skip cannot be called without create.
        if (!s_CreateCommandOccurred)
        {
            s_LastError = "--skip_part used without --create";
            return NvError_BadParameter;
        }
        *data = (void *)&s_OptionSkipPart;
        break;
    default:
        s_LastError = "invalid option requested";
        return NvError_BadParameter;
    }

    return NvSuccess;
}

const char *
NvFlashCommandGetLastError( void )
{
    const char *ret;

    if( !s_LastError )
    {
        return "success";
    }

    ret = s_LastError;
    s_LastError = 0;
    return ret;
}

NvBool
AddPartitionToVerify(const char *Partition)
{

    NvError e = NvSuccess;
    static NvBool s_VerifyAllPartitions = NV_FALSE;

   if (s_VerifyAllPartitions)
   {
       // Do nothing. Checking at this stage is better than going through
       // the list of partitions to be verified once.
       s_LastError = " WARNING: Verify All Partitions already Enabled...\n";
       return NV_TRUE;
   }

   if (!NvOsStrcmp(Partition, "-1"))
   {
        // Delete any nodes already existing in the list since all partitions
        // is enough to ensure all specific cases.
        NvFlashVerifyListDeInitLstPartitions();

        s_VerifyAllPartitions  = NV_TRUE;
        // Add node with Partition Id = 0xFFFFFFFF
   }

    // Mark partition for verification
    NV_CHECK_ERROR_CLEANUP(NvFlashVerifyListAddPartition(Partition));
    return NV_TRUE;

fail:
    return NV_FALSE;
}

void
NvFlashStrrev(char *pstr)
{
    NvU32 i, j, length = 0;
    char temp;

    length = NvOsStrlen(pstr);
    for (i = 0, j = length - 1; i < j; i++, j--)
    {
        temp = pstr[i];
        pstr[i] = pstr[j];
        pstr[j] = temp;
    }
}

NvU32
NvFlashEncodeUsbPath(const char *usbpath)
{
    char *pusb = NULL;
    NvU32 count = 0, index = 0, size = 0, retval = 0;

    // Note: This type of encoding technique is strictly based on the
    // assumption that the input path has the usbdirno and usbdevno in
    // the title i.e. : /xxx/yyy
    // If this format is not followed , error is thrown.
    if (!usbpath)
    {
        s_LastError = "Null instance device path string";
        return -1;
    }
    index = NvOsStrlen(usbpath);
    if (index < 8 || usbpath[index - 4] != '/' || usbpath[index - 8] != '/')
    {
        s_LastError = "Invalid instance device path string (Correct format:../xxx/yyy)";
        return -1;
    }

    while (1)
    {
        index--;
        if (usbpath[index] == '/')
        {
            count++;
            // Break extraction when 2nd '/' is encountered
            if (count == 2)
                break;
            continue;
        }
        else
        {
            size++;
            pusb = NvOsRealloc(pusb, size);
            pusb[size - 1] = usbpath[index];
        }
    }
    // The encoded number is prefixed by 1 to indicate that input is usb path string.
    size++;
    pusb = NvOsRealloc(pusb, size);
    pusb[size - 1] = '0' + 1;

    // String reverse to keep the format 1xxxyyy
    NvFlashStrrev(pusb);
    size++;
    pusb[size] = '\0';

    retval = NvUStrtoul(pusb, 0, 0);
    if (pusb)
        NvOsFree(pusb);
    return retval;
}

