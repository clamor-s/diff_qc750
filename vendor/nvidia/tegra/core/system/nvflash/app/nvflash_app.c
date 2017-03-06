/*
 * Copyright (c) 2008-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "nvos.h"
#include "nvutil.h"
#include "nvcommon.h"
#include "nvapputil.h"
#include "nv3p.h"
#include "nvflash_usage.h"
#include "nvflash_version.h"
#include "nvflash_configfile.h"
#include "nvflash_commands.h"
#include "nvflash_verifylist.h"
#include "nvflash_util.h"
#include "nvusbhost.h"
#include "nvboothost.h"

#ifdef NV_EMBEDDED_BUILD
#include "nvimager.h"
#endif

/* don't connect to the bootrom if this is set. */
#define NV_FLASH_STAND_ALONE        (0)

/* dangerous command, must edit source code to enable it */
#define NV_FLASH_HAS_OBLITERATE     (0)

#define NV_FLASH_PARTITION_NAME_LENGTH 4

#ifndef DEBUG_VERIFICATION
#define DEBUG_VERIFICATION 0
#define DEBUG_VERIFICATION_PRINT(x) \
    do { \
        if (DEBUG_VERIFICATION) \
        { \
            NvAuPrintf x; \
        } \
    } while(0)
#endif

#define VERIFY( exp, code ) \
    if( !(exp) ) { \
        code; \
    }

NvU8 s_MiniLoader_ap20[] =
{
    #include "nvflash_miniloader_ap20.h"
};

NvU8 s_MiniLoader_t30[] =
{
    #include "nvflash_miniloader_t30.h"
};

Nv3pSocketHandle s_hSock;
NvFlashDevice *s_Devices;
NvU32 s_nDevices;
NvBool s_Resume;
NvBool s_Quiet;
NvOsSemaphoreHandle s_FormatSema = 0;
NvBool s_FormatSuccess = NV_TRUE;
NvBool s_UpdateBctReveived = NV_FALSE;
static Nv3pPartitionInfo *s_pPartitionInfo = NULL;
NvBool s_bIsFormatPartitionsNeeded;


NvError nvflash_start_pseudoserver(void);
void nvflash_exit_pseudoserver(void);
void PseudoServer(void* args);
void nvflash_rcm_open( NvUsbDeviceHandle* hUsb, NvU32 DevInstance );
static NvBool
nvflash_rcm_read_uid( NvUsbDeviceHandle hUsb, void *uniqueId, NvU32 DeviceId);
NvBool nvflash_rcm_send( NvUsbDeviceHandle hUsb, NvU8 *msg, NvU32* response );
void nvflash_rcm_close(NvUsbDeviceHandle hUsb);
NvBool nvflash_connect(void);
NvBool nvflash_configfile( const char *filename );
NvBool nvflash_dispatch( void );
NvBool nvflash_sendfile( const char *filename, NvU64 NoOfBytes);
NvBool nvflash_recvfile( const char *filename, NvU64 total );
NvBool nvflash_wait_status( void );

static NvBool nvflash_cmd_create( void );
static NvBool nvflash_cmd_settime( void );
static NvBool nvflash_cmd_formatall( void );
static NvBool nvflash_rcm_buff(const char *RcmFile, NvU8 **pMsgBuffer);
static NvBool nvflash_parse_blob(const char *RcmFile, NvU8 **pMsgBuffer, NvU32 HeaderType);
static NvBool nvflash_cmd_updatebct( const char *bct_file, const char *bctsection );
static NvBool nvflash_query_partition_type(NvU32 PartId, NvU32 *PartitionType);
static NvBool nvflash_cmd_set_blhash(NvFlashCmdDownload *a, char *bct_file);
static NvBool nvflash_cmd_setbct(const NvFlashBctFiles *bct_files);
static NvBool nvflash_cmd_getbct( const char *bct_file );
static NvBool nvflash_cmd_getbit( NvFlashCmdGetBit *a);
static NvBool nvflash_cmd_dumpbit( NvFlashCmdDumpBit * a );
static NvBool nvflash_cmd_setboot( NvFlashCmdSetBoot *a );
static NvBool nvflash_cmd_download( NvFlashCmdDownload *a, NvU64 max_size );
static NvBool nvflash_odmcmd_VerifySdram(NvFlashCmdSetOdmCmd *a);
static NvBool nvflash_cmd_bootloader( const char *filename );
static NvBool nvflash_cmd_go( void );
static NvBool nvflash_cmd_read( NvFlashCmdRead *a );
static NvBool nvflash_cmd_read_raw( NvFlashCmdRawDeviceReadWrite *a );
static NvBool nvflash_cmd_write_raw( NvFlashCmdRawDeviceReadWrite *a );
static NvBool nvflash_cmd_getpartitiontable(NvFlashCmdGetPartitionTable *a,
                                            Nv3pPartitionInfo **pPartitionInfo);
static NvBool nvflash_cmd_setodmcmd( NvFlashCmdSetOdmCmd*a );
static NvBool nvflash_cmd_sync( void );
static NvBool nvflash_cmd_odmdata( NvU32 odm_data );
static NvBool nvflash_cmd_setbootdevicetype( const NvU8 *devType );
static NvBool nvflash_cmd_setbootdeviceconfig( const NvU32 devConfig );
static NvBool nvflash_cmd_formatpartition( Nv3pCommand cmd, NvU32 PartitionNumber);

static NvBool nvflash_enable_verify_partition(NvU32 PartitionId );
static NvBool nvflash_verify_partitions(void);
static NvBool nvflash_send_verify_command(NvU32 PartitionId);
static NvBool nvflash_check_compatibility(void);
static NvBool nvflash_check_verify_option_args(void);
static NvBool nvflash_check_skippart(char *name);
static NvBool nvflash_check_skiperror(void);
static NvBool nvflash_match_file_ext(const char *filename, const char *match);
static NvBool nvflash_get_partlayout(void);
static NvU32 nvflash_get_partIdByName(const char *pStr);
static char* nvflash_get_partNameById(NvU32 PartitionId);

static NvU32 nvflash_read_devid(NvUsbDeviceHandle hUsb);
#if NV_FLASH_HAS_OBLITERATE
static NvBool nvflash_cmd_obliterate( void );
#endif
static void nvflash_thread_execute_command(void *args);
NvError Nv3pServer(void);

typedef struct Nv3pPseudoServerThreadArgsRec
{
    NvBool IsThreadRunning;
    NvBool ExitThread;
    NvOsThreadHandle hThread;
} Nv3pPseudoServerThreadArgs;

static Nv3pPseudoServerThreadArgs s_gPseudoServerThread;


void PseudoServer(void* args)
{
#if NVODM_ENABLE_SIMULATION_CODE==1
    Nv3pServer();
#endif
}

NvError nvflash_start_pseudoserver(void)
{
#if NVODM_ENABLE_SIMULATION_CODE
    return NvOsThreadCreate(
        PseudoServer,
        (void *)&s_gPseudoServerThread,
        &s_gPseudoServerThread.hThread);
#else
    return NvError_NotImplemented;
#endif
}

void nvflash_exit_pseudoserver(void)
{
    NvOsThreadJoin(s_gPseudoServerThread.hThread);
}


static NvBool nvflash_odmcmd_fuelgaugefwupgrade(NvFlashCmdSetOdmCmd *a);

/**
 * This sends a download-execute message to the bootrom. This may only
 * be called once.
 */
void
nvflash_rcm_open( NvUsbDeviceHandle* hUsb,  NvU32 DevInstance )
{
    NvError e = NvSuccess;
    NvBool bWait;

    /* might want to wait for a usb device to be plugged in */
    e = NvFlashCommandGetOption( NvFlashOption_Wait, (void *)&bWait );
    VERIFY( e == NvSuccess, return );

    do {
        e = NvUsbDeviceOpen( DevInstance, hUsb );
        if( e == NvSuccess )
        {
            break;
        }
        if( bWait )
        {
            NvOsSleepMS( 1000 );
        }
    } while( bWait );

    if(e != NvSuccess)
    {
        if(e == NvError_AccessDenied)
            NvAuPrintf("Permission Denied\n");
        else
            NvAuPrintf("USB device not found\n");
        }
}

static NvBool
nvflash_rcm_read_uid( NvUsbDeviceHandle hUsb, void *uniqueId, NvU32 DeviceId)
{
    NvU32 len;
    NvError e;
    Nv3pChipUniqueId uid;
    NvU32 Size = 0;
    const char *err_str = 0;

    NvOsMemset((void *)&uid, 0, sizeof(uid));

    /* the bootrom sends back the chip's uid, read it. */
    if (DeviceId == 0x20 || DeviceId == 0x30)
        Size = sizeof(NvU64);
    else
        Size = sizeof(uid);

    e = NvUsbDeviceRead( hUsb, &uid, Size, &len );
    VERIFY( e == NvSuccess, err_str = "uid read failed"; goto fail );
    VERIFY( len == Size,
        err_str = "uid read failed (truncated)"; goto fail );
    *(Nv3pChipUniqueId *)uniqueId = uid;
    return NV_TRUE;
fail:
    if( err_str )
    {
        NvAuPrintf( "%s\n", err_str );
    }
    return NV_FALSE;
}


NvBool
nvflash_rcm_send( NvUsbDeviceHandle hUsb, NvU8 *msg, NvU32* response )
{
    NvBool b;
    NvError e;
    NvU32 resp = 0;
    NvU32 len;
    const char *err_str = 0;

    len = NvBootHostRcmGetMsgLength( msg );
    e = NvUsbDeviceWrite( hUsb, msg, len );
    VERIFY( e == NvSuccess,
        err_str = "Command send failed (usb write failed)";
        goto fail );

    /* get the bootrom response */
    e = NvUsbDeviceRead( hUsb, &resp, sizeof(resp), &len );
    VERIFY( e == NvSuccess,
        err_str = "miniloader download failed (response read failed)";
        goto fail );

    VERIFY( len == sizeof(resp),
        err_str = "miniloader download failed (response truncated)";
        goto fail );

    b = NV_TRUE;
    *response = resp;
    goto clean;

fail:
    if (err_str)
    {
        NvAuPrintf( "%s\n", err_str );
    }

    b = NV_FALSE;

clean:
    if (msg)
        NvOsFree(msg);
    return b;
}

NvU32
nvflash_read_devid(NvUsbDeviceHandle hUsb)
{
    NvU32 DevId = 0;
    NvU32 NvDeviceId = 0x00;

    if (NvUsbDeviceReadDevId(hUsb,&DevId) ==  NvSuccess)
    {
        NvDeviceId = DevId & 0x00FF;
    }
    return NvDeviceId;
}

void nvflash_rcm_close(NvUsbDeviceHandle hUsb)
{
    NvUsbDeviceClose( hUsb );
}

NvBool
nvflash_rcm_buff(const char *RcmFile, NvU8 **pMsgBuffer)
{
    NvBool b;
    NvError e;
    NvOsFileHandle hFile = 0;
    NvOsStatType stat;
    NvU32 bytes;
    char *err_str = 0;

    // read the RCM data files in a buffer
    e = NvOsFopen(RcmFile, NVOS_OPEN_READ, &hFile);
    VERIFY(e == NvSuccess, err_str = "file open failed"; goto fail);
    e = NvOsFstat(hFile, &stat);
    VERIFY(e == NvSuccess, err_str = "file stat failed"; goto fail);

    *pMsgBuffer = NvOsAlloc((NvU32)stat.size);
    if (!*pMsgBuffer) {
        e = NvError_InsufficientMemory;
        goto fail;
    }
    e = NvOsFread(hFile, *pMsgBuffer, (NvU32)stat.size, &bytes);
    VERIFY(e == NvSuccess, err_str = "file read failed"; goto fail);

    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;
    if (err_str)
        NvAuPrintf("%s NvError 0x%x\n", err_str , e);
    if (*pMsgBuffer)
        NvOsFree(*pMsgBuffer);
clean:
    NvOsFclose(hFile);
    return b;
}

NvBool
nvflash_parse_blob(const char *BlobFile, NvU8 **pMsgBuffer, NvU32 HeaderType)
{
    NvBool b;
    NvError e;
    NvOsFileHandle hFile = 0;
    NvFlashBlobHeader header;
    NvU32 bytes;
    char *err_str = 0;

    // read the blob data to extract info like RCM messages, bl hash etc
    e = NvOsFopen(BlobFile, NVOS_OPEN_READ, &hFile);
    VERIFY(e == NvSuccess, err_str = "file open failed"; goto fail);

    for(;;)
    {
        e = NvOsFread(hFile, &header, sizeof(header), &bytes);
        VERIFY(e == NvSuccess, err_str = "file read failed"; goto fail);
        if ((NvU32)header.BlobHeaderType == HeaderType)
            break;
        NvOsFseek(hFile, header.length, NvOsSeek_Cur);
    }
    *pMsgBuffer = NvOsAlloc(header.length);
    if (!*pMsgBuffer) {
        e = NvError_InsufficientMemory;
        goto fail;
    }
    e = NvOsFread(hFile, *pMsgBuffer, header.length, &bytes);
    VERIFY(e == NvSuccess, err_str = "file read failed"; goto fail);
    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;
    if (err_str)
        NvAuPrintf("%s NvError 0x%x\n", err_str , e);
    if (*pMsgBuffer)
        NvOsFree(*pMsgBuffer);
clean:
    NvOsFclose(hFile);
    return b;
}

NvBool
nvflash_connect(void)
{
    NvBool b = NV_TRUE;
    NvError e = NvSuccess;
    NvU8 *msg_buff = 0;
    NvBootRcmDownloadData download_arg;
    const char *err_str = 0;
    Nv3pCommand cmd;
    Nv3pCmdGetPlatformInfo info;
    NvUsbDeviceHandle hUsb = 0;
    Nv3pChipUniqueId uniqueId;
    NvU32 response = 0;
    NvU32 DeviceId = 0;
    NvU8* pMiniLoader = 0;
    NvU32 MiniLoaderSize = 0;
    const char* pBootDevStr = 0;
    const char* pChipNameStr = 0;
    const char* str = 0;
    Nv3pTransportMode TransportMode = Nv3pTransportMode_Usb;
    NvU32 devinstance = 0;
    NvFlashCmdRcm *pRcmFiles;
    const char* blob_file;

    NvFlashCommandGetOption(NvFlashOption_TransportMode,(void*)&TransportMode);

#if NVODM_BOARD_IS_FPGA
    if(TransportMode == Nv3pTransportMode_Jtag)
    {
        NvU32 EmulDevId = 0;
        NvFlashCommandGetOption(NvFlashOption_EmulationDevId,(void*)&EmulDevId);
        nvflash_set_devid(EmulDevId);
    }
    else
    {
        TransportMode = Nv3pTransportMode_Usb;
    }
#endif

    //default transport mode is USB
    if(TransportMode == Nv3pTransportMode_default)
        TransportMode = Nv3pTransportMode_Usb;

    /* don't try to connect to the bootrom in resume mode */
    if (!s_Resume && (TransportMode == Nv3pTransportMode_Usb))
    {
        NvBlOperatingMode op_mode;

        op_mode = NvBlOperatingMode_NvProduction;

        NvFlashCommandGetOption(NvFlashOption_DevInstance,(void*)&devinstance);
        nvflash_rcm_open(&hUsb, devinstance);
        VERIFY( hUsb, goto fail );

        // obtain the product id
        DeviceId = nvflash_read_devid(hUsb);
        nvflash_set_devid(DeviceId);

        b = nvflash_rcm_read_uid(hUsb, &uniqueId, DeviceId);
        VERIFY(b, goto fail);
        NvAuPrintf("chip uid from BR is: 0x%.8x%.8x%.8x%.8x\n",
                                uniqueId.ecid_3,uniqueId.ecid_2,
                                uniqueId.ecid_1,uniqueId.ecid_0);

        // find whether blob is used or rcm messages in secure mode
        // --blob option need to be used following nvflash v1.1.51822
        NvFlashCommandGetOption(NvFlashOption_Rcm, (void*)&pRcmFiles);
        VERIFY(e == NvSuccess, goto fail);
        NvFlashCommandGetOption(NvFlashOption_Blob, (void *)&blob_file);
        VERIFY(e == NvSuccess, goto fail);

        if (pRcmFiles->RcmFile1)
        {
            // odm secure mode, RCM message comes from sbktool utility
            op_mode = NvBlOperatingMode_OdmProductionSecure;
            b = nvflash_rcm_buff(pRcmFiles->RcmFile1, &msg_buff);
            VERIFY(b, err_str = "miniloader message copy from file into buffer failed";
                goto fail);
        }
        else if (blob_file)
        {
            // odm secure mode, RCM message comes from sbktool utility in a blob
            op_mode = NvBlOperatingMode_OdmProductionSecure;
            b = nvflash_parse_blob(blob_file, &msg_buff, NvFlash_RCM1);
            VERIFY(b, err_str = "query rcm version parse from blob failed"; goto fail);
        }
        else
        {
            // nvprod mode, RCM message is prepared
            e = NvBootHostRcmCreateMsgFromBuffer(NvBootRcmOpcode_QueryRcmVersion,
                NULL, (NvU8 *)NULL, 0,
                0, NULL,
                op_mode, NULL, &msg_buff);
            VERIFY(e == NvSuccess, err_str = "miniloader message create failed";
                goto fail);
        }

#if NV_FLASH_STAND_ALONE == 0
        b = nvflash_rcm_send(hUsb, msg_buff, &response);
        NvAuPrintf("rcm version 0X%x\n", response);
        VERIFY(b, goto fail);
        //e = NvBootHostRcmSetVersionInfo(response);
        //VERIFY( e == NvSuccess, err_str = "Unsupported RCM version";
            //goto fail );
#endif
        msg_buff = 0;
        if (pRcmFiles->RcmFile2)
        {
            // odm secure mode, RCM message comes from sbktool utility
            b = nvflash_rcm_buff(pRcmFiles->RcmFile2, &msg_buff);
            VERIFY(b, err_str = "miniloader message copy from file into buffer failed";
                goto fail);
        }
        else if (blob_file)
        {
            // odm secure mode, RCM message comes from sbktool utility in a blob
            b = nvflash_parse_blob(blob_file, &msg_buff, NvFlash_RCM2);
            VERIFY(b, err_str = "download execute command parse from blob failed"; goto fail);
        }
        else
        {
            // nvprod mode, RCM message is prepared
            // FIXME: figure this out at runtime
            #define NVFLASH_MINILOADER_ENTRY_32K 0x40008000
            #define NVFLASH_MINILOADER_ENTRY_40K 0x4000A000

            if (DeviceId == 0x20)
            {
                pMiniLoader = s_MiniLoader_ap20;
                MiniLoaderSize = sizeof(s_MiniLoader_ap20);
                /* download the miniloader to the bootrom */
                download_arg.EntryPoint = NVFLASH_MINILOADER_ENTRY_32K;
            }
            else if (DeviceId == 0x30)
            {
                pMiniLoader = s_MiniLoader_t30;
                MiniLoaderSize = sizeof(s_MiniLoader_t30);
                /* download the miniloader to the bootrom */
                download_arg.EntryPoint = NVFLASH_MINILOADER_ENTRY_40K;
            }
            else
            {
                b = NV_FALSE;
                NvAuPrintf("UnKnown device found\n");
                goto fail;
            }
            e = NvBootHostRcmCreateMsgFromBuffer(NvBootRcmOpcode_DownloadExecute,
                NULL, (NvU8 *)&download_arg, sizeof(download_arg),
                MiniLoaderSize, pMiniLoader,
                op_mode, NULL, &msg_buff);
            VERIFY(e == NvSuccess, err_str = "miniloader message create failed";
                goto fail);

            #undef NVFLASH_MINILOADER_ENTRY_32K
            #undef NVFLASH_MINILOADER_ENTRY_40K
        }

#if NV_FLASH_STAND_ALONE == 0
        b = nvflash_rcm_send(hUsb, msg_buff, &response);
        VERIFY(b, goto fail);
#endif
        nvflash_rcm_close(hUsb);

    }

    e = Nv3pOpen( &s_hSock, TransportMode, devinstance);
    VERIFY( e == NvSuccess, err_str = "connection failed"; goto fail );

    // FIXME: add support for platform info into bootloader?
    if( s_Resume || (TransportMode == Nv3pTransportMode_Sema))
    {
        b = NV_TRUE;
        goto clean;
    }

    /* get the platform info */
    cmd = Nv3pCommand_GetPlatformInfo;
    e = Nv3pCommandSend( s_hSock, cmd, (NvU8 *)&info, 0 );
    VERIFY( e == NvSuccess, err_str = "unable to retrieve platform info";
        goto fail );

    VERIFY( info.BootRomVersion == 1, err_str = "unknown bootrom version";
        goto fail );

    b = nvflash_wait_status();
    VERIFY( b, err_str = "unable to retrieve platform info ";
        goto fail );

// Convert chip sku to chip name as per chip id
    if (info.ChipId.Id == 0x15 || info.ChipId.Id == 0x16)
    {
        switch (info.ChipSku)
        {
            case Nv3pChipName_Development: pChipNameStr = "development"; break;
            case Nv3pChipName_Apx2600: pChipNameStr = "apx 2600"; break;
            case Nv3pChipName_Tegra600: pChipNameStr = "tegra 600"; break;
            case Nv3pChipName_Tegra650: pChipNameStr = "tegra 650"; break;
            default: pChipNameStr = "unknown"; break;
        }
    }
    else if (info.ChipId.Id == 0x20)
    {
        switch (info.ChipSku)
        {
            case Nv3pChipName_Development: pChipNameStr = "development"; break;
            case Nv3pChipName_Ap20: pChipNameStr = "ap20"; break;
            case Nv3pChipName_T20: pChipNameStr = "t20"; break;
            default: pChipNameStr = "unknown"; break;
        }
    }
    else if (info.ChipId.Id == 0x30)
    {
        switch (info.ChipSku)
        {
            case Nv3pChipName_Development: pChipNameStr = "development"; break;
            default: pChipNameStr = "unknown"; break;
        }
    }
    else
    {
        pChipNameStr = "unknown";
    }

    NvAuPrintf(
    "System Information:\n"
    "   chip name: %s\n"
    "   chip id: 0x%x major: %d minor: %d\n"
    "   chip sku: 0x%x\n"
    "   chip uid: 0x%.8x%.8x%.8x%.8x\n"
    "   macrovision: %s\n"
    "   hdcp: %s\n"
    "   jtag: %s\n"
    "   sbk burned: %s\n",
    pChipNameStr,
    info.ChipId.Id, info.ChipId.Major, info.ChipId.Minor,
    info.ChipSku,
    (NvU32)(info.ChipUid.ecid_3),(NvU32)(info.ChipUid.ecid_2),
    (NvU32)(info.ChipUid.ecid_1),(NvU32)(info.ChipUid.ecid_0),
    (info.MacrovisionEnable) ? "enabled" : "disabled",
    (info.HdmiEnable) ? "enabled" : "disabled",
    (info.JtagEnable) ? "enabled" : "disabled",
    (info.SbkBurned) ? "true" : "false");

    if( info.DkBurned == Nv3pDkStatus_NotBurned )
        str = "false";
    else if ( info.DkBurned == Nv3pDkStatus_Burned )
        str = "true";
    else
        str = "unknown";
    NvAuPrintf( "   dk burned: %s\n", str );

    switch( info.SecondaryBootDevice ) {
    case Nv3pDeviceType_Nand: pBootDevStr = "nand"; break;
    case Nv3pDeviceType_Emmc: pBootDevStr = "emmc"; break;
    case Nv3pDeviceType_Spi: pBootDevStr = "spi"; break;
    case Nv3pDeviceType_Ide: pBootDevStr = "ide"; break;
    case Nv3pDeviceType_Nand_x16: pBootDevStr = "nand_x16"; break;
    default: pBootDevStr = "unknown"; break;
    }
    NvAuPrintf( "   boot device: %s\n", pBootDevStr );

    NvAuPrintf(
"   operating mode: %d\n"
"   device config strap: %d\n"
"   device config fuse: %d\n"
"   sdram config strap: %d\n",
    info.OperatingMode, info.DeviceConfigStrap,
    info.DeviceConfigFuse, info.SdramConfigStrap );

    NvAuPrintf( "\n" );

    b = NV_TRUE;
    goto clean;

fail:
    if( err_str )
    {
        NvAuPrintf( "%s NvError 0x%x\n", err_str , e);
    }

    b = NV_FALSE;

clean:
    return b;
}

NvBool
nvflash_configfile( const char *filename )
{
    NvBool b;
    NvError e;
    NvFlashConfigFileHandle hCfg = 0;
    NvFlashDevice *dev = 0;
    NvU32 nDev = 0;
    const char *err_str = 0;

    e = NvFlashConfigFileParse( filename, &hCfg );
    VERIFY( e == NvSuccess,
        err_str = NvFlashConfigGetLastError(); goto fail );

    e = NvFlashConfigListDevices( hCfg, &nDev, &dev );
    VERIFY( e == NvSuccess,
        err_str = NvFlashConfigGetLastError(); goto fail );

    VERIFY( nDev, goto fail );
    VERIFY( dev, goto fail );

    s_Devices = dev;
    s_nDevices = nDev;

    b = NV_TRUE;
    goto clean;

fail:
    if( err_str )
    {
        NvAuPrintf( "nvflash configuration file error: %s\n", err_str );
    }

    b = NV_FALSE;

clean:
    NvFlashConfigFileClose( hCfg );
    return b;
}

/*
* nvflash_sendfile: send data present in file "filename" to nv3p server.
* If NoOfBytes is valid (>0 && <file_size) then only those many number
* of bytes will be sent.
*/
NvBool
nvflash_sendfile( const char *filename, NvU64 NoOfBytes)
{
    NvBool b;
    NvError e;
    NvOsFileHandle hFile = 0;
    NvOsStatType stat;
    NvU8 *buf = 0;
    NvU32 size;
    NvU64 total;
    NvU32 bytes;
    NvU64 count;
    char *spinner = "-\\|/";
    NvU32 spin_idx = 0;
    char *err_str = 0;

    #define NVFLASH_DOWNLOAD_CHUNK (1024 * 64)

    NvAuPrintf( "sending file: %s\n", filename );

    e = NvOsFopen( filename, NVOS_OPEN_READ, &hFile );
    VERIFY( e == NvSuccess, err_str = "file open failed"; goto fail );

    e = NvOsFstat( hFile, &stat );
    VERIFY( e == NvSuccess, err_str = "file stat failed"; goto fail );

    total = stat.size;
    if(NoOfBytes && (NoOfBytes < stat.size))
    {
        total = NoOfBytes;
    }
    buf = NvOsAlloc( NVFLASH_DOWNLOAD_CHUNK );
    VERIFY( buf, err_str = "buffer allocation failed"; goto fail );

    count = 0;
    while( count != total )
    {
        size = (NvU32) NV_MIN( total - count, NVFLASH_DOWNLOAD_CHUNK );

        e = NvOsFread( hFile, buf, size, &bytes );
        VERIFY( e == NvSuccess, err_str = "file read failed"; goto fail );
        VERIFY( bytes == size, goto fail );

        e = Nv3pDataSend( s_hSock, buf, size, 0 );
        VERIFY( e == NvSuccess, err_str = "data send failed"; goto fail );

        count += size;

        if( !s_Quiet )
        {
            NvAuPrintf( "\r%c %llu/%llu bytes sent", spinner[spin_idx],
                count, total );
            spin_idx = (spin_idx + 1) % 4;
        }
    }
    NvAuPrintf( "\n%s sent successfully\n", filename );

    #undef NVFLASH_DOWNLOAD_CHUNK

    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;
    if( err_str )
    {
        NvAuPrintf( "%s NvError 0x%x\n", err_str , e);
    }

clean:
    NvOsFclose( hFile );
    NvOsFree( buf );
    return b;
}

NvBool
nvflash_recvfile( const char *filename, NvU64 total )
{
    NvBool b;
    NvError e;
    NvOsFileHandle hFile = 0;
    NvU8 *buf = 0;
    NvU32 bytes;
    NvU64 count;
    char *spinner = "-\\|/";
    NvU32 spin_idx = 0;
    char *err_str = 0;

    #define NVFLASH_UPLOAD_CHUNK (1024 * 2)

    NvAuPrintf( "receiving file: %s, expected size: %Lu bytes\n",
        filename, total );

    e = NvOsFopen( filename, NVOS_OPEN_WRITE | NVOS_OPEN_CREATE, &hFile );
    VERIFY( e == NvSuccess, err_str = "file create failed"; goto fail );

    buf = NvOsAlloc( NVFLASH_UPLOAD_CHUNK );
    VERIFY( buf, err_str = "buffer allocation failure"; goto fail );

    count = 0;
    while( count != total )
    {
        e = Nv3pDataReceive( s_hSock, buf, NVFLASH_UPLOAD_CHUNK, &bytes, 0 );
        VERIFY( e == NvSuccess,
            err_str = "data receive failure"; goto fail );

        e = NvOsFwrite( hFile, buf, bytes );
        VERIFY( e == NvSuccess, err_str = "file write failed"; goto fail );

        count += (NvU64)bytes;

        if( !s_Quiet )
        {
            NvAuPrintf( "\r%c %Lu/%Lu bytes received", spinner[spin_idx],
                count, total );
            spin_idx = (spin_idx + 1) % 4;
        }
    }
    NvAuPrintf( "\nfile received successfully\n" );

    #undef NVFLASH_UPLOAD_CHUNK

    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;
    if( err_str )
    {
        NvAuPrintf( "%s NvError 0x%x\n", err_str , e);
    }

clean:
    NvOsFclose( hFile );
    NvOsFree( buf );
    return b;
}

static char *
nvflash_status_string( Nv3pStatus s )
{
    switch( s ) {
#define NV3P_STATUS(_name_, _value_, _desc_) \
    case Nv3pStatus_##_name_ : return _desc_;
    #include "nv3p_status.h"
#undef NV3P_STATUS
    default:
        return "unknown";
    }
}

NvBool
nvflash_wait_status( void )
{
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdStatus *status_arg = 0;

    e = Nv3pCommandReceive( s_hSock, &cmd, (void **)&status_arg, 0 );
    VERIFY( e == NvSuccess, goto fail );
    VERIFY( cmd == Nv3pCommand_Status, goto fail );

    e = Nv3pCommandComplete( s_hSock, cmd, status_arg, 0 );
    VERIFY( e == NvSuccess, goto fail );

    VERIFY( status_arg->Code == Nv3pStatus_Ok, goto fail );

    return NV_TRUE;

fail:
    if( status_arg )
    {
        char *str;
        str = nvflash_status_string( status_arg->Code );
        NvAuPrintf(
            "bootloader status: %s (code: %d) message: %s flags: %d\n",
            str, status_arg->Code, status_arg->Message, status_arg->Flags );
    }
    return NV_FALSE;
}

void nvflash_thread_execute_command(void *args)
{
        NvError e = NvSuccess;
        NvFlashDevice *d;
        NvFlashPartition *p;
        NvU32 i,j;
        NvBool CheckStatus = NV_TRUE;
        const char *PartitionName = NULL;
        Nv3pNonBlockingCmdsArgs* pThreadArgs = (Nv3pNonBlockingCmdsArgs*)args;
        Nv3pCmdFormatPartition* pFormatArgs =
                                    (Nv3pCmdFormatPartition*)pThreadArgs->CmdArgs;
        if(pThreadArgs->cmd == Nv3pCommand_FormatAll)
        {
            NvAuPrintf( "Formatting all partitions please wait.. " );
            e = Nv3pCommandSend( s_hSock,
                pThreadArgs->cmd, pThreadArgs->CmdArgs, 0 );
        }
        else if(pFormatArgs->PartitionId == (NvU32)-1)
        {
            CheckStatus = NV_FALSE;
            d = s_Devices;
            /* format all the partitions */
            for( i = 0; i < s_nDevices; i++, d = d->next )
            {
                p = d->Partitions;
                for( j = 0; j < d->nPartitions; j++, p = p->next )
                {
                    if (nvflash_check_skippart(p->Name))
                    {
                        /* This partition must be skipped from formatting procedure */
                        NvAuPrintf("Skipping Formatting partition %d %s\n",p->Id, p->Name);
                        continue;
                    }
                    NvAuPrintf( "\bFormatting partition %d %s please wait.. ", p->Id, p->Name );
                    pFormatArgs->PartitionId = p->Id;
                    e = Nv3pCommandSend( s_hSock,
                        pThreadArgs->cmd, pThreadArgs->CmdArgs, 0 );
                    if( e != NvSuccess)
                        break;
                    s_FormatSuccess = nvflash_wait_status();
                    if( !s_FormatSuccess)
                        break;
                    NvAuPrintf( "%s\n", s_FormatSuccess ? "done!" : "FAILED!" );
                }
                if( (e != NvSuccess) || !s_FormatSuccess)
                    break;
            }
        }
        else
        {
            PartitionName = nvflash_get_partNameById(pFormatArgs->PartitionId);
            NvAuPrintf( "Formatting partition %d %s please wait.. ",
                       pFormatArgs->PartitionId, PartitionName);
            e = Nv3pCommandSend( s_hSock,
                pThreadArgs->cmd, pThreadArgs->CmdArgs, 0 );
        }

        if(CheckStatus)
        {
            if( e == NvSuccess )
            {
                s_FormatSuccess = nvflash_wait_status();
            }
            else
            {
                s_FormatSuccess = NV_FALSE;
                NvAuPrintf("Command Execution failed cmd %d, error 0x%x \n",pThreadArgs->cmd,e);
            }
        }
        NvOsSemaphoreSignal(s_FormatSema);
}

NvBool
nvflash_cmd_formatpartition( Nv3pCommand cmd, NvU32 PartitionNumber)
{
    Nv3pCmdFormatPartition args;
    NvError e;
    NvBool b = NV_TRUE;
    char *spinner = "-\\|/-";
    NvU32 spin_idx = 0;
    NvOsThreadHandle thread = 0;
    Nv3pNonBlockingCmdsArgs ThreadArgs;

    if((cmd != Nv3pCommand_FormatAll) && (cmd != Nv3pCommand_FormatPartition))
        return NV_FALSE;

    e = NvOsSemaphoreCreate(&s_FormatSema, 0);
    if (e != NvSuccess)
        return NV_FALSE;

    s_FormatSuccess = NV_FALSE;
    args.PartitionId = PartitionNumber;
    ThreadArgs.cmd = cmd;
    ThreadArgs.CmdArgs = (void *)&args;
    e = NvOsThreadCreate(
            nvflash_thread_execute_command,
            (void *)&ThreadArgs,
            &thread);
    if( e == NvSuccess )
    {
        do
        {
            if(!s_Quiet)
            {
                NvAuPrintf( "%c\b", spinner[spin_idx]);
                spin_idx = (spin_idx + 1) % 5;
            }
            e = NvOsSemaphoreWaitTimeout(s_FormatSema, 100);
        } while (e != NvSuccess);

        NvAuPrintf( "%s\n", s_FormatSuccess ? "done!" : "FAILED!" );
        NvOsThreadJoin(thread);
    }

    NvOsSemaphoreDestroy(s_FormatSema);
    s_FormatSema = 0;

    if(!s_FormatSuccess)
        b = NV_FALSE;

    return b;
}

/** per-command implementation */
static NvBool
nvflash_cmd_settime( void )
{
    Nv3pCommand cmd;
    Nv3pCmdSetTime arg;
    NvBool b= NV_FALSE;
    NvError e = NvSuccess;
    char *err_str = 0;
    NvOsMemset(&arg, 0, sizeof(Nv3pCmdSetTime));
    NvOsGetSystemTime((NvOsSystemTime*)&arg);
    cmd = Nv3pCommand_SetTime;
    NvAuPrintf("Setting current time %d seconds with epoch time as base\r\n",arg.Seconds);
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );

    b = nvflash_wait_status();
    VERIFY( b, err_str="unable to do set time"; goto fail );

    return NV_TRUE;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

NvBool
nvflash_check_skippart(char *name)
{
    NvBool res = NV_FALSE;    /* partition never skipped by default */
    NvError e;
    NvFlashSkipPart *skipped=NULL;
    NvU32 i = 0;

    do
    {
        e = NvFlashCommandGetOption(NvFlashOption_SkipPartition, (void *)&skipped);
        if (e != NvSuccess)
        {
            NvAuPrintf("%s: error getting skip part info.\n", __FUNCTION__);
            break;
        }

        if (skipped)
        {
            for (i = 0; i < skipped->number; i++)
            {
                if (!NvOsStrcmp(skipped->pt_name[i], (const char *)name))
                {
                    /* Found partition name in skip list, exit loop and return info */
                    res = NV_TRUE;
                    break;
                }
            }
        }
        else
        {
            NvAuPrintf("%s: error invalid skip part info\n", __FUNCTION__);
        }

    } while(0);

    return res;
}

static NvBool nvflash_check_skiperror(void)
{
    NvError e = NvSuccess;
    NvBool b = NV_TRUE;
    const char *err_str = 0;
    NvU32 i, j;
    NvU32 SizeMultiple = 0, TotalCardSectors = 0, TotalPartitions = 0;
    NvU32 PartIndex = 0, nSkipPart = 0;
    NvU64 TotalSize = 0;

    Nv3pCommand cmd = 0;
    Nv3pCmdGetDevInfo DevInfo;
    NvFlashSkipPart *pSkipped = NULL;
    NvFlashDevice *pDevices = NULL;
    NvFlashPartition *pPartitions = NULL;
    NvFlashCmdGetPartitionTable *a = NULL;
    Nv3pPartitionTableLayout *pPartitionInfoLayout = NULL;
    Nv3pPartitionInfo *pPartitionInfo = NULL;

    // Retrieving skip list
    e = NvFlashCommandGetOption(NvFlashOption_SkipPartition, (void *)&pSkipped);
    // If --skip is not present , then return true & don't proceed with error check
    if (pSkipped->number == 0)
    {
        return NV_TRUE;
    }

    // Set this flag, if skip is present, to not to delete entire device. Needs
    // to do format partitions individually
    s_bIsFormatPartitionsNeeded = NV_TRUE;

    // CHECK 2: If there is a typo in skip list,
    // give a warning for the unknown partition in skip list
    pDevices = s_Devices;
    // Searching for skip partition name in partition table
    for (i = 0; i < s_nDevices; i++, pDevices = pDevices->next)
    {
        pPartitions = pDevices->Partitions;
        for (j = 0; j < pDevices->nPartitions; j++, pPartitions = pPartitions->next)
        {
            if (nvflash_check_skippart(pPartitions->Name))
                nSkipPart++;
        }
    }
    if (nSkipPart != pSkipped->number)
        NvAuPrintf("Warning: Unknown partition(s) in the skip list.\n");

    // Get the BlockDevInfo info
    NvOsMemset(&DevInfo, 0, sizeof(Nv3pCmdGetDevInfo));
    cmd = Nv3pCommand_GetDevInfo;
    e = Nv3pCommandSend(s_hSock, cmd, (NvU8 *)&DevInfo, 0);
    VERIFY(e == NvSuccess, err_str = "Failed to complete GetDevInfo command";b = NV_FALSE;
        goto fail);

    b = nvflash_wait_status();
    VERIFY(b, err_str = "Unable to retrieve device info"; b = NV_FALSE; goto fail);

    SizeMultiple = DevInfo.BytesPerSector * DevInfo.SectorsPerBlock;
    TotalCardSectors = DevInfo.SectorsPerBlock * DevInfo.TotalBlocks;

    // Calculating the total number of partitions and allocating memory to pPartitionInfoLayout
    // which will store the new partition layout
    pDevices = s_Devices;
    for (i = 0 ; i < s_nDevices ; i++, pDevices = pDevices->next)
    {
        pPartitions = pDevices->Partitions;
        for (j = 0 ; j < pDevices->nPartitions ; j++, pPartitions = pPartitions->next)
            TotalPartitions++;
    }
    pPartitionInfoLayout = NvOsAlloc(TotalPartitions * sizeof(Nv3pPartitionTableLayout));
    if (!pPartitionInfoLayout)
    {
        err_str = "Insufficient system memory";
        b = NV_FALSE;
        goto fail;
    }
    NvOsMemset(pPartitionInfoLayout, 0, TotalPartitions * sizeof(Nv3pPartitionTableLayout));

    a = NvOsAlloc(sizeof(NvFlashCmdGetPartitionTable));
    if (!a)
    {
        err_str = "Insufficient system memory";
        b = NV_FALSE;
        goto fail;
    }

    NvOsMemset(a, 0, sizeof(NvFlashCmdGetPartitionTable));

    // Plotting the new partition table layout & filling pPartitonTableLayout
    pDevices = s_Devices;
    for (i = 0 ; i < s_nDevices ; i++, pDevices = pDevices->next)
    {
        pPartitions = pDevices->Partitions;
        for (j = 0 ; j < pDevices->nPartitions ; j++, pPartitions = pPartitions->next)
        {
            NvU64 PartitionSize = 0;
            if (pPartitions->AllocationAttribute == 0x808)
            {
                // Temporary storing UDA info
                PartitionSize = nvflash_util_roundsize(pPartitions->Size, SizeMultiple);
                TotalSize += PartitionSize;

                pPartitions = pPartitions->next;j++;
                // GPT info with size calculated using TotalCardSize
                pPartitionInfoLayout[PartIndex].StartLogicalAddress  =
                    pPartitionInfoLayout[PartIndex-1].StartLogicalAddress
                    + pPartitionInfoLayout[PartIndex-1].NumLogicalSectors;
                pPartitionInfoLayout[PartIndex].NumLogicalSectors =
                    (NvU32)(TotalCardSectors -(TotalSize / DevInfo.BytesPerSector));
                PartIndex++;

                // Filling UDA info
                pPartitionInfoLayout[PartIndex].StartLogicalAddress =
                    pPartitionInfoLayout[PartIndex-1].StartLogicalAddress
                    + pPartitionInfoLayout[PartIndex-1].NumLogicalSectors;
                pPartitionInfoLayout[PartIndex].NumLogicalSectors =
                    (NvU32)(PartitionSize / DevInfo.BytesPerSector);

            }
            else
            {
                if (PartIndex == 0)
                    pPartitionInfoLayout[PartIndex].StartLogicalAddress = 0;
                else
                {
                    pPartitionInfoLayout[PartIndex].StartLogicalAddress =
                        pPartitionInfoLayout[PartIndex-1].StartLogicalAddress
                        + pPartitionInfoLayout[PartIndex-1].NumLogicalSectors;
                }
                PartitionSize = pPartitions->Size;
                PartitionSize = nvflash_util_roundsize(PartitionSize, SizeMultiple);
                TotalSize += PartitionSize;
                pPartitionInfoLayout[PartIndex].NumLogicalSectors =
                    (NvU32)(PartitionSize / DevInfo.BytesPerSector);
            }
            // Store PT partition attributes for loading partition table in case of skip.
            if (!NvOsStrcmp(pPartitions->Name, "PT"))
            {
                a->NumLogicalSectors = pPartitionInfoLayout[PartIndex].NumLogicalSectors;
                a->StartLogicalSector = pPartitionInfoLayout[PartIndex].StartLogicalAddress;
                a->filename = NULL;
            }
            PartIndex++;
        }
    }

    if (!a->NumLogicalSectors)
    {
        err_str = "PT partition absent in the cfg file";
        b = NV_FALSE;
        goto fail;
    }

    // CHECK 3: If skip is being used on a formatted device
    // Retrieving stored partition table in the device.

    // Note: Partiontable attributes(NumLogicalSectors & StartLogicalSector) are calculated
    // from the host config file which are used/sent to load partition table present on the device.
    // If these attributes mismatch, then it will error out.
    b = nvflash_cmd_getpartitiontable(a, &pPartitionInfo);
    VERIFY(b, err_str = "Unable to retrieve Partition table from device. Partition\
                         table attributes mismatch"; goto fail);
    b = nvflash_wait_status();
    VERIFY(b, goto fail);
    // CHECK 4: If there is change in partition layout.
    for (i = 0 ; i < TotalPartitions ; i++)
    {
        if (nvflash_check_skippart((char*)pPartitionInfo[i].PartName))
        {
            if ((pPartitionInfo[i].StartLogicalAddress !=
                pPartitionInfoLayout[i].StartLogicalAddress) ||
                (pPartitionInfo[i].NumLogicalSectors !=
                pPartitionInfoLayout[i].NumLogicalSectors))
            {
                NvAuPrintf("Partition boundaries mismatch for %s partition\n",
                           pPartitionInfo[i].PartName);
                b = NV_FALSE;
                goto fail;
            }
        }
        else
            continue;
    }

fail:
    if (a)
        NvOsFree(a);
    if (pPartitionInfo)
        NvOsFree(pPartitionInfo);
    if (pPartitionInfoLayout)
        NvOsFree(pPartitionInfoLayout);
    if (e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    if (err_str)
        NvAuPrintf("%s\n", err_str);
    return b;
}

static NvBool
nvflash_cmd_deleteall(void)
{
    Nv3pCmdStatus *status_arg = 0;
    Nv3pCommand cmd = 0;
    NvError e;
    NvAuPrintf("deleting device partitions\n");

    /* delete everything on the device before creating the partitions */
    cmd = Nv3pCommand_DeleteAll;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(s_hSock, cmd, 0, 0)
    );

    e = Nv3pCommandReceive(s_hSock, &cmd, (void **)&status_arg, 0);
    VERIFY(e == NvSuccess, goto fail);
    VERIFY(cmd == Nv3pCommand_Status, goto fail);

    e = Nv3pCommandComplete(s_hSock, cmd, status_arg, 0);
    VERIFY(e == NvSuccess, goto fail);

    VERIFY((status_arg->Code == Nv3pStatus_Ok ||
               status_arg->Code == Nv3pStatus_NotSupported), goto fail);

    if (status_arg->Code == Nv3pStatus_NotSupported)
        s_bIsFormatPartitionsNeeded = NV_TRUE;
    return NV_TRUE;
fail:
    return NV_FALSE;
}

static NvBool
nvflash_cmd_create(void)
{
    NvError e = NvSuccess;
    NvBool b;
    NvU32 i, j, len;
    NvFlashDevice *d;
    NvFlashPartition *p;
    Nv3pCommand cmd = 0;
    Nv3pCmdSetDevice dev_arg;
    Nv3pCmdCreatePartition part_arg;
    Nv3pCmdStartPartitionConfiguration part_cfg;
    NvU32 nPartitions;
    NvU32 odm_data;
    NvBool verifyPartitionEnabled;
    const NvFlashBctFiles *bct_files;
    char *err_str = 0;

    s_bIsFormatPartitionsNeeded = NV_FALSE;

    if (s_Resume)
    {
        // need to send bct and odmdata for --create in resume mode
        e = NvFlashCommandGetOption(NvFlashOption_Bct, (void *)&bct_files);
        VERIFY(e == NvSuccess, goto fail);
        VERIFY(bct_files->BctOrCfgFile, err_str = "bct file required for this command"; goto fail);
        b = nvflash_cmd_setbct(bct_files);
        VERIFY(b, err_str = "setbct failed"; goto fail);
        b = nvflash_wait_status();
        VERIFY(b, goto fail);    /* count all the partitions for all devices */
        e = NvFlashCommandGetOption(NvFlashOption_OdmData, (void *)&odm_data);
        VERIFY(e == NvSuccess, goto fail);
        if(odm_data)
        {
            b = nvflash_cmd_odmdata(odm_data);
            VERIFY( b, err_str = "odm data failure"; goto fail );
        }
    }

    // Check for skip and do the errorcheck using this function.
    b = nvflash_check_skiperror();
    VERIFY(b, err_str = "Skip error check handler failed"; goto fail);

    d = s_Devices;
    nPartitions = 0;
    for (i = 0; i < s_nDevices; i++, d = d->next)
    {
        nPartitions += d->nPartitions;
    }

    part_cfg.nPartitions = nPartitions;
    cmd = Nv3pCommand_StartPartitionConfiguration;
    e = Nv3pCommandSend( s_hSock,
        cmd, &part_cfg, 0 );
    VERIFY( e == NvSuccess, goto fail );

    b = nvflash_wait_status();
    VERIFY( b, err_str="unable to do start partition"; goto fail );

    d = s_Devices;
    for( i = 0; i < s_nDevices; i++, d = d->next )
    {
        NvAuPrintf( "setting device: %d %d\n", d->Type, d->Instance );

        cmd = Nv3pCommand_SetDevice;
        dev_arg.Type = d->Type;
        dev_arg.Instance = d->Instance;
        e = Nv3pCommandSend( s_hSock, cmd, (NvU8 *)&dev_arg, 0 );
        VERIFY( e == NvSuccess, goto fail );

        b = nvflash_wait_status();
        VERIFY( b, err_str="unable to do set device"; goto fail );

        // delete all the partitions by erasing the whole device contents
        if (s_bIsFormatPartitionsNeeded != NV_TRUE)
        {
            b = nvflash_cmd_deleteall();
            VERIFY(b == NV_TRUE, err_str = "deleteall failed"; goto fail);
        }

        p = d->Partitions;
        for( j = 0; j < d->nPartitions; j++, p = p->next )
        {
            cmd = Nv3pCommand_CreatePartition;

            NvAuPrintf( "creating partition: %s\n", p->Name );

            len = NV_MIN( NV3P_STRING_MAX - 1, NvOsStrlen( p->Name ) );
            NvOsMemcpy( part_arg.Name, p->Name, len );
            part_arg.Name[len] = 0;
            part_arg.Size = p->Size;
            part_arg.Address = p->StartLocation;
            part_arg.Id = p->Id;
            part_arg.Type = p->Type;
            part_arg.FileSystem = p->FileSystemType;
            part_arg.AllocationPolicy = p->AllocationPolicy;
            part_arg.FileSystemAttribute = p->FileSystemAttribute;
            part_arg.PartitionAttribute = p->PartitionAttribute;
            part_arg.AllocationAttribute = p->AllocationAttribute;
            part_arg.PercentReserved = p->PercentReserved;
#ifdef NV_EMBEDDED_BUILD
            part_arg.IsWriteProtected = p->IsWriteProtected;
#endif
            e = Nv3pCommandSend( s_hSock, cmd, (NvU8 *)&part_arg, 0 );
            VERIFY( e == NvSuccess, goto fail );

            b = nvflash_wait_status();
            VERIFY( b, err_str="unable to do create partition"; goto fail );
        }
    }

    cmd = Nv3pCommand_EndPartitionConfiguration;
    e = Nv3pCommandSend( s_hSock,
        cmd, 0, 0 );
    VERIFY( e == NvSuccess, goto fail );

    b = nvflash_wait_status();
    VERIFY( b, err_str="unable to do end partition"; goto fail );

    if (s_bIsFormatPartitionsNeeded == NV_TRUE)
    {
        /* Format partitions */
        b = nvflash_cmd_formatpartition(Nv3pCommand_FormatPartition, -1);
        VERIFY(b, goto fail);
    }
    d = s_Devices;
    /* download all of the partitions with file names specified */
    for( i = 0; i < s_nDevices; i++, d = d->next )
    {
        p = d->Partitions;
        for( j = 0; j < d->nPartitions; j++, p = p->next )
        {
            /* hack up a command to re-use the helper function */
            NvFlashCmdDownload dp;
            if( !p->Filename ||
                p->Type == Nv3pPartitionType_Bct ||
                p->Type == Nv3pPartitionType_PartitionTable ||
                nvflash_check_skippart(p->Name))
            {
                /* not allowed to download bcts or partition tables
                 * here.
                 */
                continue;
            }

            e = NvFlashCommandGetOption(NvFlashOption_VerifyEnabled,
                (void *)&verifyPartitionEnabled);
            VERIFY( e == NvSuccess, goto fail );
            if (verifyPartitionEnabled)
            {
                // Check if partition data needs to be verified. If yes, send
                // an auxiliary command to indicate this to the device side
                // code.
                b = nvflash_enable_verify_partition(p->Id);
                VERIFY( b, goto fail );
            }
            dp.PartitionID = p->Id;
            dp.filename = p->Filename; // FIXME: consistent naming

            if (p->Type == Nv3pPartitionType_Bootloader ||
                    p->Type == Nv3pPartitionType_BootloaderStage2)
            {
                b = nvflash_align_bootloader_length(p->Filename);
                VERIFY( b, err_str = "failed to pad bootloader"; goto fail);
            }

            b = nvflash_cmd_download( &dp, p->Size );
            VERIFY( b, goto fail );

            b = nvflash_wait_status();
            VERIFY( b, err_str="failed to verify partition"; goto fail );
        }
    }

    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;
    if (err_str)
    {
        NvAuPrintf( "%s\n", err_str);
    }
    if (e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);

clean:
    return b;
}

#if NV_FLASH_HAS_OBLITERATE
static NvBool
nvflash_cmd_obliterate( void )
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd;
    NvFlashDevice *d;
    Nv3pCmdSetDevice dev_arg;
    NvU32 i;

    /* for each device in the config file, send a set-device, then an
     * obliterate
     */

    d = s_Devices;
    for( i = 0; i < s_nDevices; i++, d = d->next )
    {
        NvAuPrintf( "setting device: %d %d\n", d->Type, d->Instance );

        cmd = Nv3pCommand_SetDevice;
        dev_arg.Type = d->Type;
        dev_arg.Instance = d->Instance;
        e = Nv3pCommandSend( s_hSock, cmd, (NvU8 *)&dev_arg, 0 );
        VERIFY( e == NvSuccess, goto fail );

        b = nvflash_wait_status();
        VERIFY( b, goto fail );

        NvAuPrintf( "issuing obliterate\n" );

        cmd = Nv3pCommand_Obliterate;
        NV_CHECK_ERROR_CLEANUP(
            Nv3pCommandSend( s_hSock, cmd, 0, 0 )
        );

        b = nvflash_wait_status();
        VERIFY( b, goto fail );
    }

    return NV_TRUE;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}
#endif

static NvBool
nvflash_cmd_formatall( void )
{
    NvBool b;
    b = nvflash_cmd_formatpartition(Nv3pCommand_FormatAll, -1);
    VERIFY( b, goto fail );

    return NV_TRUE;
fail:
    NvAuPrintf( "\r\nformatting failed\n");
    return NV_FALSE;
}

static NvBool
nvflash_cmd_setnvinternal( NvFlashCmdNvPrivData *a )
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd = Nv3pCommand_NvPrivData;
    Nv3pCmdNvPrivData arg;
    NvOsStatType stat;

    e = NvOsStat( a->filename, &stat );
    if( e != NvSuccess )
    {
        NvAuPrintf( "file not found: %s\n", a->filename );
        goto fail;
    }

    arg.Length = (NvU32)stat.size;

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );

    b = nvflash_sendfile( a->filename, 0 );
    return b;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x \n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_query_partition_type(NvU32 PartId, NvU32 *PartitionType)
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd = Nv3pCommand_Force32;
    Nv3pCmdQueryPartition query_part;
    const char *err_str = 0;

    query_part.Id = PartId;
    cmd = Nv3pCommand_QueryPartition;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(s_hSock, cmd, &query_part, 0)
    );

    /* wait for status */
    b = nvflash_wait_status();
    VERIFY(b, err_str = "failed to query partition"; goto fail);
    *PartitionType = (NvU32)query_part.PartType;
    return b;

fail:
    return NV_FALSE;
}

static NvBool
nvflash_cmd_set_blhash(NvFlashCmdDownload *a, char *bct_file)
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd = Nv3pCommand_SetBlHash;
    Nv3pCmdBlHash arg;
    NvOsStatType stat;
    NvFlashDevice *d;
    NvFlashPartition *p;
    NvU32 j;
    NvU32 nBlParts = 0;
    NvBool IsBlPart = NV_FALSE;
    char * cfg_file;

    e = NvFlashCommandGetOption(NvFlashOption_ConfigFile, (void *)&cfg_file);
    if (cfg_file)
    {
        d = s_Devices;
        p = d->Partitions;
        // check whether image to be downloaded is bootloader and also
        // find how many bootloader images are there in configuration file
        for( j = 0; j < d->nPartitions; j++, p = p->next )
        {
            if (p->Type == Nv3pPartitionType_Bootloader)
            {
                if (p->Id == a->PartitionID)
                    IsBlPart = NV_TRUE;
                nBlParts++;
            }
            if(nBlParts > 4) // max number of Bl is four
                break;
        }
        if (!IsBlPart)
            return NV_TRUE;// download other images
    }
    else
    {
        e = NvError_BadParameter;
        NvAuPrintf("--configfile needed for --setblhash\n");
        goto fail;
    }

    e = NvOsStat(bct_file, &stat);
    if(e != NvSuccess)
    {
        NvAuPrintf("file not found: %s\n", bct_file);
        goto fail;
    }
    // verify BCT size before downloading
    p = d->Partitions;
    for(j = 0; j < d->nPartitions; j++, p = p->next)
    {
        if (NvOsStrcmp(p->Name, "BCT") == 0)
            break;
    }
    if (j == d->nPartitions)
        return NV_FALSE;
    if(stat.size > p->Size)
    {
        NvAuPrintf("%s is too large for partition\n", p->Name);
        return NV_FALSE;
    }

    arg.Length = (NvU32)stat.size;
    // find which bl hash slot need to be updated in bct
    arg.BlIndex = a->PartitionID;
    cmd = Nv3pCommand_SetBlHash;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(s_hSock, cmd, &arg, 0)
    );

    //send all the data present in bct_file to  server
    b = nvflash_sendfile(bct_file, 0);
    VERIFY(b, goto fail);
    b = nvflash_wait_status();
    return b;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x \n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_setbct(const NvFlashBctFiles *bct_files)
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd = Nv3pCommand_DownloadBct;
    Nv3pCmdDownloadBct arg;
    NvOsStatType stat;
    Nv3pCmdDownloadBootloader loadEntry;
    NvFlashDevice *d;
    NvFlashPartition *p;
    NvU32 j;
    char * cfg_file;
    char *bctsave_file = (char *)bct_files->BctOrCfgFile;

    if (nvflash_match_file_ext(bct_files->BctOrCfgFile, ".cfg"))
    {
        bctsave_file = (char *)bct_files->OutputBctFile;
        if (!bctsave_file)
        {
            bctsave_file = "flash.bct";
        }
        // Use nvbuildbct library to create the bct file
        NV_CHECK_ERROR_CLEANUP(
            nvflash_util_buildbct(bct_files->BctOrCfgFile, bctsave_file)
        );
    }

    e = NvFlashCommandGetOption(NvFlashOption_ConfigFile, (void *)&cfg_file);
    if (!cfg_file)
    {
        NvAuPrintf("--configfile needed for --setbct \n");
        e = NvError_BadParameter;
        goto fail;
    }

    e = NvOsStat(bctsave_file, &stat);
    if( e != NvSuccess )
    {
        NvAuPrintf("file not found: %s\n", bctsave_file);
        goto fail;
    }

    // if no SetEntry option found,
    //   verify BCT size before downloading
    if (NvFlashCommandGetOption( NvFlashOption_EntryAndAddress,
        (void *)&loadEntry ) != NvSuccess) {

        d = s_Devices;
        p = d->Partitions;
        for( j = 0; j < d->nPartitions; j++, p = p->next )
        {
            if (NvOsStrcmp(p->Name, "BCT") == 0)
                break;
        }
        if (j == d->nPartitions)
            return NV_FALSE;
        if( stat.size > p->Size )
        {
            NvAuPrintf( "%s is too large for partition\n", p->Name );
            return NV_FALSE;
        }
    }
    arg.Length = (NvU32)stat.size;

    cmd = Nv3pCommand_DownloadBct;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );
    //send all the data present in bct_file to  server
    b = nvflash_sendfile(bctsave_file, 0);
    return b;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x \n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_updatebct( const char *bct_file, const char *bctsection )
{
    NvBool b = NV_TRUE;
    NvError e;
    Nv3pCommand cmd = Nv3pCommand_UpdateBct;
    Nv3pCmdUpdateBct arg;
    NvOsStatType stat;

    e = NvOsStat( bct_file, &stat );
    if( e != NvSuccess )
    {
        NvAuPrintf( "file not found: %s\n", bct_file );
        goto fail;
    }
    arg.BctSection = Nv3pUpdatebctSectionType_None;
    //verify whether we are asked to update a valid/supported bct section or not
    if(!NvOsStrcmp("SDRAM", bctsection))
        arg.BctSection = Nv3pUpdatebctSectionType_Sdram;
    else if(!NvOsStrcmp("DEVPARAM", bctsection))
        arg.BctSection = Nv3pUpdatebctSectionType_DevParam;
    else if(!NvOsStrcmp("BOOTDEVINFO", bctsection))
        arg.BctSection = Nv3pUpdatebctSectionType_BootDevInfo;

    if(arg.BctSection == Nv3pUpdatebctSectionType_None)
    {
        NvAuPrintf( "no section found to update bct\r\n");
        e = NvError_BadParameter;
        goto fail;
    }

    arg.Length = (NvU32)stat.size;

    cmd = Nv3pCommand_UpdateBct;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );

    if(!s_UpdateBctReveived)
    {
        //send all the data present in bct_file to  server
        b = nvflash_sendfile( bct_file, 0 );
        s_UpdateBctReveived = NV_TRUE;
    }

    return b;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x \n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_getbct( const char *bct_file )
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdGetBct arg;
    NvOsFileHandle hFile = 0;
    NvU8 *buf = 0;
    NvU32 size;

    NvAuPrintf( "retrieving bct into: %s\n", bct_file );

    cmd = Nv3pCommand_GetBct;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );
    b = nvflash_wait_status();
    VERIFY( b, goto fail );
    size = arg.Length;
    buf = NvOsAlloc( size );
    VERIFY( buf, goto fail );

    NV_CHECK_ERROR_CLEANUP(
        Nv3pDataReceive( s_hSock, buf, size, 0, 0 )
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsFopen( bct_file, NVOS_OPEN_WRITE | NVOS_OPEN_CREATE, &hFile )
    );

    NV_CHECK_ERROR_CLEANUP(
        NvOsFwrite( hFile, buf, size )
    );

    NvAuPrintf( "%s received successfully\n", bct_file );

    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;
    if(e != NvSuccess)
        NvAuPrintf("Failed sending command %d NvError %d",cmd,e);

clean:
    NvOsFclose( hFile );
    NvOsFree( buf );
    return b;
}

static NvBool
nvflash_cmd_getbit( NvFlashCmdGetBit *a)
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdGetBct arg;
    NvOsFileHandle hFile = 0;
    NvU8 *buf = 0;
    NvU32 size;
    const char *bit_file = a->filename;

    NvAuPrintf( "retrieving bit into: %s\n", bit_file );

    cmd = Nv3pCommand_GetBit;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );
    size = arg.Length;
    buf = NvOsAlloc( size );
    VERIFY( buf, goto fail );
    NV_CHECK_ERROR_CLEANUP(
        Nv3pDataReceive( s_hSock, buf, size, 0, 0 )
    );
    NV_CHECK_ERROR_CLEANUP(
        NvOsFopen( bit_file, NVOS_OPEN_WRITE | NVOS_OPEN_CREATE, &hFile )
    );
    NV_CHECK_ERROR_CLEANUP(
        NvOsFwrite( hFile, buf, size )
    );

    NvAuPrintf( "%s received successfully\n", bit_file );
    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;

clean:
    NvOsFclose( hFile );
    NvOsFree( buf );
    return b;
}

static NvBool
nvflash_cmd_dumpbit( NvFlashCmdDumpBit * a )
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdGetBct arg;
    NvU8 *buf = 0;
    NvU32 size;

    cmd = Nv3pCommand_GetBit;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );
    size = arg.Length;
    buf = NvOsAlloc( size );
    VERIFY( buf, goto fail );
    NV_CHECK_ERROR_CLEANUP(
        Nv3pDataReceive( s_hSock, buf, size, 0, 0 )
    );

    NV_CHECK_ERROR_CLEANUP(
        nvflash_util_dumpbit(buf, a->DumpbitOption)
    );
    b = NV_TRUE;
    goto clean;

fail:
    b = NV_FALSE;

clean:
    NvOsFree( buf );
    return b;
}

static NvBool
nvflash_cmd_setboot( NvFlashCmdSetBoot *a )
{
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdSetBootPartition arg;

    arg.Id = a->PartitionID;
    // FIXME: remove the rest of the command parameters
    arg.LoadAddress = 0;
    arg.EntryPoint = 0;
    arg.Version = 1;
    arg.Slot = 0;

    cmd = Nv3pCommand_SetBootPartition;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );

    return NV_TRUE;
fail:
    return NV_FALSE;
}

static NvBool
nvflash_match_file_ext( const char *filename, const char *match )
{
    const char *ext;
    const char *tmp;
    tmp = filename;
    do
    {
        /* handle pathologically-named files, such as blah.<match>.txt */
        ext = NvUStrstr( tmp, match );
        if( ext && NvOsStrcmp( ext, match ) == 0 )
        {
            return NV_TRUE;
        }
        tmp = ext;
    } while( ext );

    return NV_FALSE;
}

static NvBool
nvflash_cmd_download( NvFlashCmdDownload *a, NvU64 max_size )
{
    NvBool b;
    NvError e = NvSuccess;
    Nv3pCommand cmd = Nv3pCommand_Force32;
    Nv3pCmdDownloadPartition arg;
    NvOsStatType stat;
    char *filename;
    NvBool bPreproc = NV_FALSE;
    NvFlashPreprocType preproc_type = (NvFlashPreprocType)0;
    NvU32 blocksize = 0;
    NvFlashCmdDiskImageOptions *diskimg = 0;
    Nv3pCmdQueryPartition query_part;
    const char *err_str = 0;

    filename = (char *)a->filename;

    e = NvOsStat( filename, &stat );
    if( e != NvSuccess )
    {
        NvAuPrintf( "file not found: %s\n", filename );
        goto fail;
    }

    /* check for files that need preprocessing */
    if( nvflash_match_file_ext( a->filename, ".dio" ) )
    {
        bPreproc = NV_TRUE;
        preproc_type = NvFlashPreprocType_DIO;
    }
    else if( nvflash_match_file_ext( a->filename, ".store.bin" ) )
    {
        bPreproc = NV_TRUE;
        preproc_type = NvFlashPreprocType_StoreBin;

        /* only need block size for .store.bin */
        NvFlashCommandGetOption( NvFlashOption_DiskImgOpt, (void *)&diskimg);
        if( diskimg )
        {
            blocksize = diskimg->BlockSize;
        }
    }

    if( bPreproc )
    {
        b = nvflash_util_preprocess( a->filename, preproc_type, blocksize,
            &filename );
        VERIFY( b, goto fail );

        e = NvOsStat( filename, &stat );
        if( e != NvSuccess )
        {
            NvAuPrintf( "file not found: %s\n", filename );
            goto fail;
        }
    }

    //if Download partition is through --download command,
    //find whether file to be downloaded is greater than partition size.
    if (!max_size)
    {
        query_part.Id = a->PartitionID;
        cmd = Nv3pCommand_QueryPartition;
        NV_CHECK_ERROR_CLEANUP(
            Nv3pCommandSend(s_hSock, cmd, &query_part, 0)
        );

        /* wait for status */
        b = nvflash_wait_status();
        VERIFY(b, err_str = "failed to query partition"; goto fail);
        max_size = query_part.Size;
    }
    if( (max_size) && (stat.size > max_size) )
    {
        NvAuPrintf( "%s is too large for partition\n", filename );
        return NV_FALSE;
    }

    cmd = Nv3pCommand_DownloadPartition;
    arg.Id = a->PartitionID;
    arg.Length = stat.size;

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );
    //send all the data present in filename to  server
    b = nvflash_sendfile( filename, 0 );
    VERIFY( b, goto fail );

    return NV_TRUE;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_odmcmd_VerifySdram(NvFlashCmdSetOdmCmd *a)
{
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdOdmCommand arg;
    NvBool b =NV_FALSE;

    cmd = Nv3pCommand_OdmCommand;
    arg.odmExtCmd = Nv3pOdmExtCmd_VerifySdram;
    arg.odmExtCmdParam.verifySdram.Value =
                                        a->odmExtCmdParam.verifySdram.Value;

    if (arg.odmExtCmdParam.verifySdram.Value == 0)
        NvAuPrintf("SdRam Verification :SOFT TEST\n");
    else if (arg.odmExtCmdParam.verifySdram.Value == 1)
        NvAuPrintf("SdRam Verification :STRESS TEST\n");
    else
    {
        NvAuPrintf("Valid parameters for verifySdram extn command are");
        NvAuPrintf(" 0- softest    1- stress test \n");
        return NV_FALSE;
    }
    NvAuPrintf("Please wait\n");
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(s_hSock, cmd, &arg, 0)
    );
    b = nvflash_wait_status();
    VERIFY(b, goto fail);
    NvAuPrintf("Sdram Verification successfull\n");
    NvAuPrintf("Size verified is  :%u MB\n",arg.Data / (1024 * 1024));
    return b;

fail:
    NvAuPrintf("Sdram Verification failed\n");
    return NV_FALSE;
}

static NvBool
nvflash_cmd_bootloader( const char *filename )
{
    NvBool b;
    NvError e;
    NvError SetEntry;
    Nv3pCommand cmd;
    Nv3pCmdDownloadBootloader arg;
    NvOsStatType stat;
    NvU32 odm_data;
    NvU8 *devType;
    NvU32 devConfig;
    NvBool FormatAllPartitions;
    NvBool IsBctSet;
    const NvFlashBctFiles *bct_files;
    char *blob_filename;
    NvFlashCmdRcm *pRcmFiles;
    char *err_str = 0;
    NvBool IsVerifySdramSet;

    /* need to send odm data, if any, before loading the bootloader */
    e = NvFlashCommandGetOption( NvFlashOption_OdmData,
        (void *)&odm_data );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_SetBootDevType,
        (void *)&devType );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_SetBootDevConfig,
        (void *)&devConfig );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_FormatAll,
        (void *)&FormatAllPartitions );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_SetBct,
        (void *)&IsBctSet );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption(NvFlashOption_SetOdmCmdVerifySdram,
        (void *)&IsVerifySdramSet);
    VERIFY(e == NvSuccess, goto fail);

    if( IsBctSet )
    {
        e = NvFlashCommandGetOption( NvFlashOption_Bct,
        (void *)&bct_files );
        VERIFY( e == NvSuccess, goto fail );
        VERIFY(bct_files->BctOrCfgFile, err_str = "bct file required for this command"; goto fail);
        b = nvflash_cmd_setbct(bct_files);
        VERIFY( b, err_str = "setbct failed"; goto fail );
        b = nvflash_wait_status();
        VERIFY( b, goto fail );
    }

    if (IsVerifySdramSet)
    {
        NvFlashCmdSetOdmCmd a;
        e = NvFlashCommandGetOption( NvFlashOption_OdmCmdVerifySdramVal,
            (void *)&a.odmExtCmdParam.verifySdram.Value);
        VERIFY(e == NvSuccess, goto fail);

        if (!IsBctSet)
        {
            NvAuPrintf("--setbct required to verify Sdram\n");
            goto fail;
        }

        b = nvflash_odmcmd_VerifySdram(&a);
        VERIFY(b, goto fail);
    }

    if( (odm_data || devType || devConfig) && s_Resume )
    {
        char *str = 0;
        if( odm_data ) str = "odm data";
        if( devType ) str = "device type";
        if( devConfig ) str = "device config";

        NvAuPrintf( "warning: %s may not be specified with a resume\n", str );
        return NV_FALSE;
    }

    if( odm_data )
    {
        b = nvflash_cmd_odmdata( odm_data );
        VERIFY( b, err_str = "odm data failure"; goto fail );
    }

    if( devType )
    {
        b = nvflash_cmd_setbootdevicetype( devType );
        VERIFY( b, err_str = "boot devive type data failure"; goto fail );
    }

    if( devConfig )
    {
        b = nvflash_cmd_setbootdeviceconfig( devConfig );
        VERIFY( b, err_str = "boot device config data failure"; goto fail );
    }

    e = NvOsStat( filename, &stat );
    if( e != NvSuccess )
    {
        NvAuPrintf( "file not found: %s\n", filename );
        goto fail;
    }

    NvAuPrintf( "downloading bootloader -- " );

    cmd = Nv3pCommand_DownloadBootloader;
    arg.Length = stat.size;

    /* attempt to discover the addresses at runtime */
    b = nvflash_util_parseimage( filename, &arg.EntryPoint,
        &arg.Address );
    if( !b )
    {
        nvflash_get_bl_loadaddr(&arg.EntryPoint, &arg.Address);
    }

    /* check if entryPoint and address are provided thru command line */
    SetEntry = NvFlashCommandGetOption( NvFlashOption_EntryAndAddress,
        (void *)&arg );

    NvAuPrintf( "load address: 0x%x entry point: 0x%x\n",
        arg.Address, arg.EntryPoint );

    e = Nv3pCommandSend( s_hSock, cmd, &arg, 0 );
    VERIFY( e == NvSuccess, err_str = "download command failed"; goto fail );
    b = nvflash_wait_status();
    VERIFY( b, goto fail );

    // find whether blob is used or rcm messages in secure mode
    NvFlashCommandGetOption(NvFlashOption_Rcm, (void*)&pRcmFiles);
    VERIFY(e == NvSuccess, goto fail);
    // send command line bl hash to miniloader to validate it in secure mode
    e = NvFlashCommandGetOption(NvFlashOption_Blob, (void*)&blob_filename);
    VERIFY(e == NvSuccess, goto fail);
    if (pRcmFiles->RcmFile1)
    {
        NvU8 *blhash_buff = 0;

        // send 0s to miniloader in case --rcm option is used to disable bl validation
        blhash_buff = NvOsAlloc(NV3P_AES_HASH_BLOCK_LEN);
        NvOsMemset(blhash_buff, 0, NV3P_AES_HASH_BLOCK_LEN);
        e = Nv3pDataSend(s_hSock, blhash_buff, NV3P_AES_HASH_BLOCK_LEN, 0);
        VERIFY(e == NvSuccess, err_str = "data send failed"; goto fail);
        if (blhash_buff)
            NvOsFree(blhash_buff);
    }
    else if (blob_filename)
    {
        NvU8 *blhash_buff = 0;

        // parse blob to retrieve command line bl hash and send it to miniloader
        b = nvflash_parse_blob(blob_filename, &blhash_buff, NvFlash_BlHash);
        VERIFY(b, err_str = "bootloader hash extraction from blob failed";
            goto fail);

        e = Nv3pDataSend(s_hSock, blhash_buff, NV3P_AES_HASH_BLOCK_LEN, 0);
        VERIFY(e == NvSuccess, err_str = "data send failed"; goto fail);
        if (blhash_buff)
            NvOsFree(blhash_buff);
    }

    //send all the data present in filename to  server
    b = nvflash_sendfile( filename, 0 );
    VERIFY( b, goto fail );

    if ( SetEntry == NvSuccess ) {
        NvAuPrintf( "bootloader downloaded successfully\n" );
        return NV_TRUE;
    }

    /* wait for the bootloader to come up */
    NvAuPrintf( "waiting for bootloader to initialize\n" );
    b = nvflash_wait_status();
    VERIFY( b, err_str = "bootloader failed"; goto fail );

    NvAuPrintf( "bootloader downloaded successfully\n" );

    if ( FormatAllPartitions )
    {
        VERIFY( !IsBctSet, err_str = "--setbct not required for formatting partitions"; goto fail );
        b = nvflash_cmd_formatall();
        VERIFY( b, err_str = "format-all failed\n"; goto fail );
    }

    return NV_TRUE;

fail:
    if( err_str )
    {
        NvAuPrintf( "%s NvError 0x%x\n", err_str , e );
    }
    return NV_FALSE;
}

NvBool
nvflash_odmcmd_fuelgaugefwupgrade(NvFlashCmdSetOdmCmd *a)
{
    NvBool b;
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdOdmCommand arg;
    NvOsStatType stat;
    char *err_str = NULL;

    e = NvOsStat(a->odmExtCmdParam.fuelGaugeFwUpgrade.filename1, &stat);
    if (e != NvSuccess)
    {
        err_str = "Getting Stats for file1 Failed";
        goto fail;
    }

    arg.odmExtCmdParam.fuelGaugeFwUpgrade.FileLength1 = stat.size;

    if (a->odmExtCmdParam.fuelGaugeFwUpgrade.filename2 != NULL)
    {
       e = NvOsStat(a->odmExtCmdParam.fuelGaugeFwUpgrade.filename2, &stat);
       if (e != NvSuccess)
       {
           err_str = "Getting Stats for file2 Failed";
           goto fail;
       }

       arg.odmExtCmdParam.fuelGaugeFwUpgrade.FileLength2 = stat.size;
    }
    else
    {
       arg.odmExtCmdParam.fuelGaugeFwUpgrade.FileLength2 = 0;
    }

    cmd = Nv3pCommand_OdmCommand;
    arg.odmExtCmd = Nv3pOdmExtCmd_FuelGaugeFwUpgrade;

    e = Nv3pCommandSend(s_hSock, cmd, &arg, 0);
    if (e != NvSuccess)
    {
        err_str = "Nv3pCommandSend failed";
        goto fail;
    }
    // send all the data present in filename to server
    b = nvflash_sendfile(a->odmExtCmdParam.fuelGaugeFwUpgrade.filename1, 0);
    VERIFY(b, err_str = "sending file1 failed"; goto fail);

    if (a->odmExtCmdParam.fuelGaugeFwUpgrade.filename2 != NULL)
    {
       b = nvflash_sendfile(a->odmExtCmdParam.fuelGaugeFwUpgrade.filename2, 0);
       VERIFY(b, err_str = "sending file2 failed"; goto fail);
    }

    return NV_TRUE;

fail:
    if (err_str)
    {
        NvAuPrintf("FuelGaugeFwUpgrade: %s\n", err_str);
    }
    return NV_FALSE;
}

static NvBool
nvflash_cmd_go( void )
{
    NvError e;
    Nv3pCommand cmd;

    cmd = Nv3pCommand_Go;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, 0, 0 )
    );

    return NV_TRUE;
fail:
    return NV_FALSE;
}

static NvBool
nvflash_cmd_read( NvFlashCmdRead *a )
{
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdReadPartition read_part;
    const char *err_str = 0;

    /* read the whole partition */
    read_part.Id = a->PartitionID;
    read_part.Offset = 0;
    cmd = Nv3pCommand_ReadPartition;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &read_part, 0 )
    );
    return nvflash_recvfile( a->filename, read_part.Length );

fail:
    if( err_str )
    {
        NvAuPrintf( "%s ", err_str);
    }
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_read_raw( NvFlashCmdRawDeviceReadWrite *a )
{
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdRawDeviceAccess raw_read_dev;
    const char *err_str = 0;

    raw_read_dev.StartSector = a->StartSector;
    raw_read_dev.NoOfSectors = a->NoOfSectors;
    cmd = Nv3pCommand_RawDeviceRead;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &raw_read_dev, 0 )
    );
    NvAuPrintf("Bytes to receive %Lu\n", raw_read_dev.NoOfBytes);
    return nvflash_recvfile(a->filename, raw_read_dev.NoOfBytes);

fail:
    if( err_str )
    {
        NvAuPrintf( "%s ", err_str);
    }
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_write_raw( NvFlashCmdRawDeviceReadWrite *a )
{
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdRawDeviceAccess raw_write_dev;
    const char *err_str = 0;
    NvOsStatType stat;

    raw_write_dev.StartSector = a->StartSector;
    raw_write_dev.NoOfSectors = a->NoOfSectors;
    cmd = Nv3pCommand_RawDeviceWrite;

    e = NvOsStat( a->filename, &stat );
    if( e != NvSuccess )
    {
        NvAuPrintf( "file not found: %s\n", a->filename );
        goto fail;
    }

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &raw_write_dev, 0 )
    );

    if( (raw_write_dev.NoOfBytes) && (stat.size < raw_write_dev.NoOfBytes) )
    {
        NvAuPrintf( "%s is too small for writing %Lu bytes\n", a->filename, raw_write_dev.NoOfBytes);
        return NV_FALSE;
    }

    //send raw_write_dev.NoOfBytes present in filename to  server
    return nvflash_sendfile(a->filename, raw_write_dev.NoOfBytes);

fail:
    if( err_str )
    {
        NvAuPrintf( "%s ", err_str);
    }
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n", cmd, e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_getpartitiontable(NvFlashCmdGetPartitionTable *a, Nv3pPartitionInfo **pPartitionInfo)
{
    NvError e = 0;
    Nv3pCommand cmd;
    Nv3pCmdReadPartitionTable arg;

    NvU32 bytes;
    char* buff = 0;
    char *err_str = 0;
    NvOsFileHandle hFile = 0;
    NvU32 MaxPartitionEntries = 0;
    NvU32 Entry = 0;

    #define MAX_STRING_LENGTH 256
    buff = NvOsAlloc(MAX_STRING_LENGTH);

    // These values will be non zero in case of call from skip error check.
    arg.NumLogicalSectors = a->NumLogicalSectors;
    arg.StartLogicalSector = a->StartLogicalSector;

    cmd = Nv3pCommand_ReadPartitionTable;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(s_hSock, cmd, &arg, 0)
    );

    MaxPartitionEntries = (NvU32) arg.Length / sizeof(Nv3pPartitionInfo);
    if(MaxPartitionEntries > MAX_GPT_PARTITIONS_SUPPORTED)
    {
        e = NvError_BadParameter;
        goto fail;
    }
    *pPartitionInfo = NvOsAlloc((NvU32)arg.Length);
    if(!(*pPartitionInfo))
        NV_CHECK_ERROR_CLEANUP(NvError_InsufficientMemory);

    NV_CHECK_ERROR_CLEANUP(
        Nv3pDataReceive(s_hSock, (NvU8*)(*pPartitionInfo), (NvU32)arg.Length, &bytes, 0)
    );

    if (a->filename) // will be NULL in case of call from skip error check
    {
        e = NvOsFopen(a->filename, NVOS_OPEN_WRITE | NVOS_OPEN_CREATE, &hFile);
        VERIFY(e == NvSuccess, err_str = "file create failed"; goto fail);

        NvOsMemset(buff, 0, MAX_STRING_LENGTH);
        for (Entry = 0 ; Entry < MaxPartitionEntries ; Entry++)
        {
            NvOsSnprintf(buff,
                                    MAX_STRING_LENGTH,
                                    "PartitionId=%d\r\nName=%s\r\n"
                                    "DeviceId=%d\r\n"
                                    "StartSector=%d\r\nNumSectors=%d\r\n"
                                    "BytesPerSector=%d"
                                    "\r\n\r\n\r\n",
                                    (*pPartitionInfo)[Entry].PartId,
                                    (*pPartitionInfo)[Entry].PartName,
                                    (*pPartitionInfo)[Entry].DeviceId,
                                    (*pPartitionInfo)[Entry].StartLogicalAddress,
                                    (*pPartitionInfo)[Entry].NumLogicalSectors,
                                    (*pPartitionInfo)[Entry].BytesPerSector
                            );
            e = NvOsFwrite(hFile, buff, NvOsStrlen(buff));
            VERIFY(e == NvSuccess, err_str = "file write failed"; goto fail);
        }
        NvAuPrintf("Succesfully updated partition table information to %s\n", a->filename);
    }
fail:
    if (hFile)
        NvOsFclose(hFile);
    if (buff)
        NvOsFree(buff);
    if (a->filename && *pPartitionInfo) // Deallocation when no skip error check.
        NvOsFree(*pPartitionInfo);
    if (err_str)
    {
        NvAuPrintf("%s NvError 0x%x\n", err_str , e);
    }
    if (e != NvSuccess)
    {
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
        return NV_FALSE;
    }

    return NV_TRUE;
}

static NvBool
nvflash_cmd_sync( void )
{
    NvError e;
    Nv3pCommand cmd;

    cmd = Nv3pCommand_Sync;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, 0, 0 )
    );

    return NV_TRUE;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_odmdata( NvU32 odm_data )
{
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdOdmOptions arg;
    NvBool b;

    cmd = Nv3pCommand_OdmOptions;
    arg.Options = odm_data;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );

    b = nvflash_wait_status();
    VERIFY( b, goto fail );

    NvAuPrintf( "odm data: 0x%x\n", odm_data );

    return NV_TRUE;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_setbootdevicetype( const NvU8 *devType )
{
    NvError e = NvSuccess;
    Nv3pCommand cmd = Nv3pCommand_SetBootDevType;
    Nv3pCmdSetBootDevType arg;
    Nv3pDeviceType DeviceType;
    NvBool b;

    // convert user defined string to nv3p device
    if (!NvOsStrncmp((const char *)devType, "nand_x8", NV3P_STRING_MAX))
    {
        DeviceType = Nv3pDeviceType_Nand;
    }
    else if (!NvOsStrncmp((const char *)devType, "nand_x16", NV3P_STRING_MAX))
    {
        DeviceType = Nv3pDeviceType_Nand_x16;
    }
    else if (!NvOsStrncmp((const char *)devType, "emmc", NV3P_STRING_MAX))
    {
        DeviceType = Nv3pDeviceType_Emmc;
    }
    else if (!NvOsStrncmp((const char *)devType, "spi", NV3P_STRING_MAX))
    {
        DeviceType = Nv3pDeviceType_Spi;
    }
    else
    {
        NvAuPrintf("Invalid boot device type: %s\n", devType);
        goto fail;
    }
    arg.DevType = DeviceType;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(s_hSock, cmd, &arg, 0)
    );
    b = nvflash_wait_status();
    VERIFY(b, goto fail);

    return NV_TRUE;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_setbootdeviceconfig( const NvU32 devConfig )
{
    NvError e;
    Nv3pCommand cmd = Nv3pCommand_SetBootDevConfig;
    Nv3pCmdSetBootDevConfig arg;
    NvBool b;

    arg.DevConfig = devConfig;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend(s_hSock, cmd, &arg, 0)
    );
    b = nvflash_wait_status();
    VERIFY(b, goto fail);

    return NV_TRUE;

fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

static NvBool
nvflash_cmd_setodmcmd( NvFlashCmdSetOdmCmd *a )
{
    NvError e;
    Nv3pCommand cmd;
    Nv3pCmdOdmCommand arg;

    switch( a->odmExtCmd ) {
    case NvFlashOdmExtCmd_FuelGaugeFwUpgrade:
        return  nvflash_odmcmd_fuelgaugefwupgrade(a);
    case NvFlashOdmExtCmd_RunSdDiag:
        cmd = Nv3pCommand_OdmCommand;
        arg.odmExtCmd = Nv3pOdmExtCmd_RunSdDiag;
        arg.odmExtCmdParam.sdDiag.Value = a->odmExtCmdParam.sdDiag.Value;
        arg.odmExtCmdParam.sdDiag.TestType = a->odmExtCmdParam.sdDiag.TestType;
        NV_CHECK_ERROR_CLEANUP(
            Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
        );
        NvAuPrintf("SD Diagnostic Tests passed\n");
        break;
    case NvFlashOdmExtCmd_RunSeDiag:
        cmd = Nv3pCommand_OdmCommand;
        arg.odmExtCmd = Nv3pOdmExtCmd_RunSeDiag;
        arg.odmExtCmdParam.seDiag.Value = a->odmExtCmdParam.seDiag.Value;
        NV_CHECK_ERROR_CLEANUP(
            Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
        );
        NvAuPrintf("SE-AES Diagnostic Tests passed\n");
        break;
    case NvFlashOdmExtCmd_RunPwmDiag:
        cmd = Nv3pCommand_OdmCommand;
        arg.odmExtCmd = Nv3pOdmExtCmd_RunPwmDiag;
        NV_CHECK_ERROR_CLEANUP(
            Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
        );
        NvAuPrintf("PWM Diagnostic Tests completed\n");
        break;
    case NvFlashOdmExtCmd_RunDsiDiag:
        cmd = Nv3pCommand_OdmCommand;
        arg.odmExtCmd = Nv3pOdmExtCmd_RunDsiDiag;
        NV_CHECK_ERROR_CLEANUP(
            Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
        );
        NvAuPrintf("DSI Diagnostic Tests passed\n");
        break;
    default:
        return NV_FALSE;
    }
    return NV_TRUE;
fail:
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

static char *
nvflash_get_nack( Nv3pNackCode code )
{
    switch( code ) {
    case Nv3pNackCode_Success:
        return "success";
    case Nv3pNackCode_BadCommand:
        return "bad command";
    case Nv3pNackCode_BadData:
        return "bad data";
    default:
        return "unknown";
    }
}

/** executes the commands */
NvBool
nvflash_dispatch( void )
{
    NvBool b;
    NvBool bBL = NV_FALSE;
    NvBool bCheckStatus;
    NvBool bSync = NV_FALSE;
    NvError e;
    void *arg;
    char *cfg_filename;
    char *bct_filename = NULL;
    const NvFlashBctFiles *bct_files;
    char *bootloader_filename;
    NvU32 nCmds;
    NvFlashCommand cmd;
    const char *err_str = 0;
    NvU32 odm_data = 0;
    NvU8 *devType;
    NvU32 devConfig;
    NvBool FormatAllPartitions;
    NvBool verifyPartitionEnabled;
    NvBool IsBctSet;
    NvBool bSetEntry = NV_FALSE;
    Nv3pCmdDownloadBootloader loadPoint;
    Nv3pTransportMode TransportMode;
    NvBool SimulationMode = NV_FALSE;

    /* get all of the options, just in case */
    e = NvFlashCommandGetOption( NvFlashOption_ConfigFile,
        (void *)&cfg_filename );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption(NvFlashOption_Bct,
        (void *)&bct_files);
    VERIFY( e == NvSuccess, goto fail );
    bct_filename = (char*)bct_files->BctOrCfgFile;

    e = NvFlashCommandGetOption( NvFlashOption_Bootloader,
        (void *)&bootloader_filename );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_Quiet, (void *)&s_Quiet );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_OdmData, (void *)&odm_data );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_SetBootDevType,
      (void *)&devType );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_SetBootDevConfig,
      (void *)&devConfig );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_FormatAll,
      (void *)&FormatAllPartitions );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_VerifyEnabled, (void *)&verifyPartitionEnabled);
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_SetBct,
      (void *)&IsBctSet );
    VERIFY( e == NvSuccess, goto fail );

    e = NvFlashCommandGetOption( NvFlashOption_TransportMode,
      (void *)&TransportMode );
    VERIFY( e == NvSuccess, goto fail );

    if(TransportMode == NvFlashTransportMode_Simulation)
        SimulationMode = NV_TRUE;

    if( cfg_filename )
    {
        b = nvflash_configfile( cfg_filename );
        VERIFY( b, goto fail );
    }

#ifdef NV_EMBEDDED_BUILD
    e = nvflash_create_filesystem_image();
    VERIFY(e == NvSuccess , goto fail);

    e = nvflash_create_kernel_image();
    VERIFY(e == NvSuccess , goto fail);
#endif
    // Check all partitions added to the list are existent
    // and have associated data.
    // This should be always done after parsing of the config file
    // since the number of devices and partitions per device
    // are known only after that.
    if (verifyPartitionEnabled)
    {
        b = nvflash_check_verify_option_args();
        VERIFY( b, goto fail );
    }
    /* connect to the nvap */
    b = nvflash_connect();
    VERIFY( b, goto fail );

    nCmds = NvFlashGetNumCommands();
    VERIFY( nCmds >= 1, err_str = "no commands found"; goto fail );

    /* commands are executed in order (from the command line). all commands
     * that can be handled by the miniloader will be handled by the miniloader
     * until the bootloader is required, at which point the bootloader will
     * be downloaded.
     *
     * if resume mode is specified, then don't try to download a bootloader
     * since it should already be running.
     */
    if( s_Resume )
    {
        bBL = NV_TRUE;
    }

    if( IsBctSet && SimulationMode)
    {
        e = NvFlashCommandGetOption(NvFlashOption_Bct,
                   (void *)&bct_files);
        VERIFY( e == NvSuccess, goto fail );
        VERIFY(bct_files->BctOrCfgFile, err_str = "bct file required for this command"; goto fail);
        b = nvflash_cmd_setbct(bct_files);
        VERIFY( b, err_str = "setbct failed"; goto fail );
        b = nvflash_wait_status();
        VERIFY( b, goto fail );
        if( odm_data )
        {
            b = nvflash_cmd_odmdata( odm_data );
            VERIFY( b, err_str = "odm data failure"; goto fail );
        }
    }

    /* force a sync if the odm data, etc., has been specified -- this is
     * paranoia since the 'go' command implicitly will sync.
     */
    if( odm_data || devType || devConfig )
    {
        bSync = NV_TRUE;
    }

    #define CHECK_BL() \
        do { \
            if( !bBL && !SimulationMode) \
            { \
                if( bootloader_filename ) \
                { \
                    b = nvflash_cmd_bootloader( bootloader_filename ); \
                    VERIFY( b, err_str = "bootloader download failed"; \
                        goto fail ); \
                } \
                else \
                { \
                    err_str = "no bootloader was specified"; \
                    goto fail; \
                } \
                bBL = NV_TRUE; \
            } \
        } while( 0 )

    /* check setentry option for downloading u-boot */
    if (NvFlashCommandGetOption( NvFlashOption_EntryAndAddress,
                                (void *)&loadPoint ) == NvSuccess)
    {
        bSetEntry = NV_TRUE;
    }


    /* commands that change the partition layout or BCT will need to
     * be followed up eventually with a Sync (or Go) command. this will be
     * issued before nvflash exits and is controlled by bSync.
     */

    while( nCmds )
    {
        bCheckStatus = NV_TRUE;

        e = NvFlashCommandGetCommand( &cmd, &arg );
        VERIFY( e == NvSuccess, err_str = "invalid command"; goto fail );

        switch( cmd ) {
        case NvFlashCommand_Help:
            nvflash_usage();
            bCheckStatus = NV_FALSE;
            break;
        case NvFlashCommand_CmdHelp:
            {
                NvFlashCmdHelp *a = (NvFlashCmdHelp *)arg;
                nvflash_cmd_usage(a);
                NvOsFree( a );
                bCheckStatus = NV_FALSE;
                break;
            }
        case NvFlashCommand_Create:
            VERIFY( IsBctSet, err_str = "--setbct required for --create "; goto fail );
            /* need a cfg file */
            VERIFY( s_Devices, err_str = "config file required for this"
                " command is missing"; goto fail );
            CHECK_BL();

            /* if set entry for u-boot, exit from here */
            if (bSetEntry == NV_TRUE)
                return NV_TRUE;

            b = nvflash_cmd_create();
            VERIFY( b, err_str = "create failed"; goto fail );
            /* nvflash_cmd_create() will check for the status since there are
             * multiple nv3p commands.
             */
            bCheckStatus = NV_FALSE;
            bSync = NV_TRUE;
            break;
        case NvFlashCommand_SetTime:
            CHECK_BL();
            b = nvflash_cmd_settime();
            VERIFY( b, err_str = "create failed"; goto fail );
            bCheckStatus = NV_FALSE;
            break;
#if NV_FLASH_HAS_OBLITERATE
        case NvFlashCommand_Obliterate:
            /* need a cfg file */
            VERIFY( s_Devices, err_str = "config file required for this"
                " command is missing"; goto fail );
            CHECK_BL();
            b = nvflash_cmd_obliterate();
            VERIFY( b, err_str = "obliterate failed"; goto fail );
            bCheckStatus = NV_FALSE;
            bSync = NV_TRUE;
            break;
#endif
        case NvFlashCommand_GetBit:
        {
            NvFlashCmdGetBit *a =
                (NvFlashCmdGetBit *)arg;
            b = nvflash_cmd_getbit( a );
            NvOsFree( a );
            VERIFY( b, err_str = "getbit failed"; goto fail );
            bCheckStatus = NV_FALSE;
            break;
        }
        case NvFlashCommand_DumpBit:
        {
            NvFlashCmdDumpBit *a =
                (NvFlashCmdDumpBit *)arg;
            b = nvflash_cmd_dumpbit( a );
            NvOsFree( a );
            VERIFY( b, err_str = "dumpbit failed"; goto fail );
            bCheckStatus = NV_FALSE;
            break;
        }
        case NvFlashCommand_UpdateBct:
        {
            NvFlashCmdUpdateBct *a = (NvFlashCmdUpdateBct *)arg;
            VERIFY( bct_filename,
                err_str = "bct file required for this command";
                goto fail );
            VERIFY(!IsBctSet, err_str = "updatebct and setbct are not suported at same time";
                    goto fail);
            CHECK_BL();
            b = nvflash_cmd_updatebct( bct_filename, a->bctsection);
            bSync = NV_TRUE;
            NvOsFree( a );
            VERIFY( b, err_str = "updatebct failed"; goto fail );
            break;
        }
        case NvFlashCommand_GetBct:
            VERIFY( bct_filename,
                err_str = "bct file required for this command";
                goto fail );
            b = nvflash_cmd_getbct(bct_filename);
            VERIFY( b, err_str = "getbct failed"; goto fail );
            bCheckStatus = NV_FALSE;
            break;
        case NvFlashCommand_Go:
            CHECK_BL();

            /* if set entry for u-boot, exit from here */
            if (bSetEntry == NV_TRUE)
                return NV_TRUE;

            b = nvflash_cmd_go();
            VERIFY( b, err_str = "go failed\n"; goto fail );
            break;
        case NvFlashCommand_SetBoot:
        {
            NvFlashCmdSetBoot *a = (NvFlashCmdSetBoot *)arg;
            CHECK_BL();
            // If input argument is a partition name string , then retrieve the partition id.
            if (!a->PartitionID)
            {
                a->PartitionID = nvflash_get_partIdByName(a->PartitionName);
                VERIFY(a->PartitionID, err_str = "partition not found in partition table";
                       goto fail);
            }
            b = nvflash_cmd_setboot( a );
            NvOsFree( a );
            VERIFY( b, err_str = "setboot failed"; goto fail );
            bSync = NV_TRUE;
            break;
        }
        case NvFlashCommand_FormatPartition:
        {
            NvFlashCmdFormatPartition *a =
                (NvFlashCmdFormatPartition *)arg;
            VERIFY( !IsBctSet, err_str = "--setbct not requied --formatpatition"; goto fail );
            CHECK_BL();
            // If input argument is a partition name string , then retrieve the partition id.
            if (!a->PartitionID)
            {
                a->PartitionID = nvflash_get_partIdByName(a->PartitionName);
                VERIFY(a->PartitionID, err_str = "partition not found in partition table";
                       goto fail);
            }
            b = nvflash_cmd_formatpartition(Nv3pCommand_FormatPartition, a->PartitionID);
            NvOsFree( a );
            VERIFY( b, err_str = "format partition failed"; goto fail );
            bCheckStatus = NV_FALSE;
            bSync = NV_TRUE;
            break;
        }
        case NvFlashCommand_Download:
        {
            NvFlashCmdDownload *a = (NvFlashCmdDownload *)arg;
            NvFlashCmdRcm *pRcmFiles;
            char *blob_file;
            char *bct_file;
            NvU32 PartitionType;

            VERIFY( !IsBctSet, err_str = "--setbct not requied --download"; goto fail );

            e = NvFlashCommandGetOption(NvFlashOption_Rcm, (void*)&pRcmFiles);
            VERIFY(e == NvSuccess, goto fail);
            e = NvFlashCommandGetOption(NvFlashOption_Blob, (void*)&blob_file);
            VERIFY(e == NvSuccess, goto fail);
            if(pRcmFiles->RcmFile1 || blob_file)
            {
                e = NvFlashCommandGetOption(NvFlashOption_BlHash, (void *)&bct_file);
                VERIFY(e == NvSuccess, goto fail);
                if (bct_file)
                {
                    b = nvflash_cmd_set_blhash(a, bct_file);
                    VERIFY( b, err_str = "Error downloading bl in secure mode"; goto fail );
                }
            }
            CHECK_BL();

            // If input argument is a partition name string , then retrieve the partition id.
            if (!a->PartitionID)
            {
                a->PartitionID = nvflash_get_partIdByName(a->PartitionName);
                VERIFY(a->PartitionID, err_str = "partition not found in partition table";
                       goto fail);
            }

            b = nvflash_query_partition_type(a->PartitionID, &PartitionType);
            VERIFY(b, err_str = "Error querying partition type"; goto fail);

            // check whether --setblhash option is used to download bootloader
            if((pRcmFiles->RcmFile1 || blob_file) && !bct_file)
            {
                if (PartitionType == Nv3pPartitionType_Bootloader)
                {
                    VERIFY(0, err_str = "Error --setblhash needed to download BL in secure mode";
                    goto fail);
                }
            }
            if (verifyPartitionEnabled)
            {
                // Check if partition data needs to be verified. If yes, send
                // an auxiliary command to indicate this to the device side
                // code.
                b = nvflash_enable_verify_partition(a->PartitionID);
                VERIFY( b, err_str = "Error marking Partition for verification"; goto fail );
            }

            if (PartitionType == Nv3pPartitionType_Bootloader ||
                    PartitionType == Nv3pPartitionType_BootloaderStage2)
            {
                b = nvflash_align_bootloader_length(a->filename);
                VERIFY( b, err_str = "failed to pad bootloader"; goto fail);
            }

            b = nvflash_cmd_download( a, 0 );
            NvOsFree( a );
            VERIFY( b, err_str = "partition download failed"; goto fail );
            bSync = NV_TRUE;
            break;
        }
        case NvFlashCommand_Read:
        {
            NvFlashCmdRead *a = (NvFlashCmdRead *)arg;
            VERIFY( !IsBctSet, err_str = "--setbct not required --read"; goto fail );
            CHECK_BL();
            // If input argument is a partition name string , then retrieve the partition id.
            if (!a->PartitionID)
            {
                a->PartitionID = nvflash_get_partIdByName(a->PartitionName);
                VERIFY(a->PartitionID, err_str = "partition not found in partition table";
                       goto fail);
            }
            b = nvflash_cmd_read( a );
            NvOsFree( a );
            VERIFY( b, err_str = "read failed"; goto fail );
            break;
        }
        case NvFlashCommand_RawDeviceRead:
        {
            NvFlashCmdRawDeviceReadWrite *a = (NvFlashCmdRawDeviceReadWrite*)arg;
            CHECK_BL();
            b = nvflash_cmd_read_raw( a );
            NvOsFree( a );
            VERIFY( b, err_str = "readdeviceraw failed"; goto fail );
            break;
        }
        case NvFlashCommand_RawDeviceWrite:
        {
            NvFlashCmdRawDeviceReadWrite *a = (NvFlashCmdRawDeviceReadWrite*)arg;
            CHECK_BL();
            b = nvflash_cmd_write_raw( a );
            NvOsFree( a );
            VERIFY( b, err_str = "writedeviceraw failed"; goto fail );
            break;
        }
        case NvFlashCommand_GetPartitionTable:
        {
            NvFlashCmdGetPartitionTable *a =
                (NvFlashCmdGetPartitionTable *)arg;
            // Sending dummy to handle the double pointer. This & zero initalizations
            // also serve the purpose to notify that this call is made, when no skip.
            Nv3pPartitionInfo *dummy = NULL;
            a->NumLogicalSectors = 0;
            a->StartLogicalSector = 0;
            CHECK_BL();
            b = nvflash_cmd_getpartitiontable(a, &dummy);
            NvOsFree( a );
            VERIFY( b, err_str = "get partition table failed"; goto fail );
            break;
        }
        case NvFlashCommand_SetOdmCmd:
        {
            NvFlashCmdSetOdmCmd*a = (NvFlashCmdSetOdmCmd *)arg;
            CHECK_BL();
            b = nvflash_cmd_setodmcmd( a );
            NvOsFree( a );
            VERIFY( b, err_str = "set odm cmd failed"; goto fail );
            bSync = NV_TRUE;
            break;
        }

        case NvFlashCommand_NvPrivData:
        {
            NvFlashCmdNvPrivData *a = (NvFlashCmdNvPrivData *)arg;
            CHECK_BL();
            b = nvflash_cmd_setnvinternal(a);
            VERIFY( b, err_str = "setting nvinternal blob failed";
                    goto fail );
            bSync = NV_TRUE;
            break;
        }
        case NvFlashCommand_Sync:
        {
            bSync = NV_TRUE;
            bCheckStatus = NV_FALSE;
            break;
        }
        default:
            err_str = "unknown command";
            goto clean;
        }

        /* bootloader will send a status after every command. */
        if( bCheckStatus )
        {
            b = nvflash_wait_status();
            VERIFY( b, err_str = "bootloader error"; goto fail );
        }

        if( cmd == NvFlashCommand_Go )
        {
            b = NV_TRUE;
            break;
        }

        nCmds--;
    }

    /* about to exit, should send a sync command for last-chance error
     * handling/reporting.
     */
    if( bSync )
    {
        CHECK_BL();
        b = nvflash_cmd_sync();
        VERIFY( b, err_str = "sync failed"; goto fail );

        b = nvflash_wait_status();
        VERIFY( b, goto fail );
    }

    e = NvFlashCommandGetOption( NvFlashOption_VerifyEnabled, (void *)&verifyPartitionEnabled);
    VERIFY( e == NvSuccess, goto fail );
    if (verifyPartitionEnabled)
    {
        b = nvflash_verify_partitions();
        VERIFY( b, err_str = "verify partitions failed"; goto fail );
    }

    b = NV_TRUE;
    goto clean;

fail:

    b = NV_FALSE;
    if( err_str )
    {
        Nv3pNackCode code = Nv3pGetLastNackCode(s_hSock);

        NvAuPrintf( "command failure: %s ", err_str );

        if (code != Nv3pNackCode_Success)
        {
            NvAuPrintf( "(%s)\n", nvflash_get_nack(code) );

            /* should be a status message from the bootloader */
           (void)nvflash_wait_status();
        }

        NvAuPrintf( "\n" );
    }

clean:

    return b;
}

int
main( int argc, const char *argv[] )
{
    NvBool b;
    NvError e;
    int ret = 0;
    const char *err_str = 0;
    Nv3pTransportMode TransportMode = Nv3pTransportMode_default;
    NvAuPrintf("Nvflash %s started\n", NVFLASH_VERSION);

    if( argc < 2 )
    {
        nvflash_usage();
        return 0;
    }
    if( argc == 2 )
    {
        if( NvOsStrcmp( argv[1], "--help" ) == 0 || NvOsStrcmp( argv[1], "-h" ) == 0 )
        {
            nvflash_usage();
            return 0;
        }
    }
    if( argc == 3 )
    {
        if( NvOsStrcmp( argv[1], "--cmdhelp" ) == 0 || NvOsStrcmp( argv[1], "-ch" ) == 0 )
        {
            NvFlashCmdHelp a;
            a.cmdname = argv[2];
            nvflash_cmd_usage(&a);
            return 0;
        }
    }
    if( NvOsStrcmp( argv[1], "--resume" ) == 0 ||
        NvOsStrcmp( argv[1], "-r" ) == 0 )
    {
        NvAuPrintf( "[resume mode]\n" );
        s_Resume = NV_TRUE;
    }
    NvFlashVerifyListInitLstPartitions();

    /* parse commandline */
    e = NvFlashCommandParse( argc, argv );

    VERIFY( e == NvSuccess, err_str = NvFlashCommandGetLastError();
        goto fail );

    /*checking nvflash and nvsbktool version compatibility*/
    b = nvflash_check_compatibility();
    if(!b)
        goto fail;

    NvFlashCommandGetOption(NvFlashOption_TransportMode,(void*)&TransportMode);

    if(TransportMode == Nv3pTransportMode_Sema)
    {
        NvU32 EmulDevId = 0;
        NvFlashCommandGetOption(NvFlashOption_EmulationDevId,(void*)&EmulDevId);
        nvflash_set_devid(EmulDevId);
        e = nvflash_start_pseudoserver();
        if (e !=  NvSuccess)
        {
            NvAuPrintf("\r\n simulation mode not supported \r\n");
            goto fail;
        }
    }

    /* execute commands */
    b = nvflash_dispatch();
    if( !b )
    {
        goto fail;
    }

    ret = 0;
    goto clean;

fail:
    ret = -1;

clean:
    if(TransportMode == Nv3pTransportMode_Sema)
        nvflash_exit_pseudoserver();

    /* Free all devices, partitions, and the strings within. */
    {
        NvFlashDevice*      d;
        NvFlashDevice*      next_d;
        NvFlashPartition*   p;
        NvFlashPartition*   next_p;
        NvU32               i;
        NvU32               j;

        for (i = 0, d = s_Devices; i < s_nDevices; i++, d = next_d)
        {
            next_d = d->next;

            for (j = 0, p = d->Partitions; j < d->nPartitions; j++, p = next_p)
            {
                next_p = p->next;
#ifdef NV_EMBEDDED_BUILD
                if (p->Dirname)
                    NvOsFree(p->Dirname);
                if (p->ImagePath)
                    NvOsFree(p->ImagePath);
                if (p->OS_Args)
                    NvOsFree(p->OS_Args);
                if (p->RamDiskPath)
                    NvOsFree(p->RamDiskPath);
#endif
                if (p->Filename)
                    NvOsFree(p->Filename);
                if (p->Name)
                    NvOsFree(p->Name);

                NvOsFree(p);
            }

            NvOsFree(d);
        }
    }

    NvFlashVerifyListDeInitLstPartitions();

    if( err_str )
    {
        NvAuPrintf( "%s\n", err_str );
    }
    // Free the partition layout buffer retrieved in nvflash_get_partlayout
    if (s_pPartitionInfo)
        NvOsFree(s_pPartitionInfo);
    Nv3pClose( s_hSock );
    return ret;
}

NvBool
nvflash_check_compatibility(void)
{
    NvBool b = NV_TRUE;
    char *blob_filename;
    NvFlashCmdRcm *pRcmFiles;
    char *nvflash_version;
    NvU8 *nvsbktool_version = 0;
    int sbk, flash, i;
    const char *err_str = 0;

    NvFlashCommandGetOption(NvFlashOption_Blob, (void*)&blob_filename);
    NvFlashCommandGetOption(NvFlashOption_Rcm, (void*)&pRcmFiles);

    if (!blob_filename && !pRcmFiles->RcmFile1)
        return b;
    else if (!blob_filename && pRcmFiles->RcmFile1)
    {
        NvAuPrintf("Not Recommended using nvflash with older nvsbktool\n");
        return b;
    }
    // parse blob to retrieve nvsbktool version info
    b = nvflash_parse_blob(blob_filename, &nvsbktool_version, NvFlash_Version);
    VERIFY(b, err_str = "sbktool version extraction from blob failed"; goto fail);
    NvAuPrintf("Using blob %s\n", nvsbktool_version);

    // get major version of sbktool and nvflash and compare
    for (i = 0; nvsbktool_version[i + 1] != '.'; i++)
    {
        if (nvsbktool_version[i + 1] == '\0')
        {
            NvAuPrintf("Invalid version number nvsbktool %s\n", nvsbktool_version);
            break;
        }
    }
    sbk = nvflash_util_atoi((char*)(nvsbktool_version + 1), i);

    nvflash_version = NVFLASH_VERSION;
    for (i = 0; nvflash_version[i + 1] != '.'; i++)
    {
        if (nvflash_version[i + 1] == '\0')
        {
            NvAuPrintf("Invalid version number nvflash %s\n", nvflash_version);
            break;
        }
    }
    flash = nvflash_util_atoi(nvflash_version + 1, i);

    if (flash != sbk)
        NvAuPrintf("Not Recommended using nvflash %s with nvsbktool %s\n",
                                nvflash_version, nvsbktool_version);

fail:
    if (err_str)
    {
        NvAuPrintf("%s\n", err_str);
    }
    if (nvsbktool_version)
        NvOsFree(nvsbktool_version);
    return b;
}

NvBool
nvflash_check_verify_option_args(void)
{

    NvError e = NvSuccess;
    NvU32 PartitionId = 0xFFFFFFFF;
    char Partition[NV_FLASH_PARTITION_NAME_LENGTH];
    NvFlashDevice *d;
    NvFlashPartition *p;
    NvU32 i, j;
    NvBool PartitionFound = NV_FALSE;
    NvBool SeekLstStart = NV_TRUE;

    while(e == NvSuccess)
    {
        e = NvFlashVerifyListGetNextPartition(Partition, SeekLstStart);
        PartitionId = NvUStrtoul(Partition, 0, 0);
        SeekLstStart = NV_FALSE;
        // Thsi condition occurs if there are no elements in the verify
        // partitions list or when the end of list is reached.
        if (e !=  NvSuccess)
        {
            break;
        }
        // If all partitions are marked for verification,
        // send verify command for each partition.

        if(PartitionId != 0xFFFFFFFF)
        {
            if(s_Devices && s_nDevices)
            {
                d = s_Devices;
                //do this for every partition having data
                for( i = 0; i < s_nDevices; i++, d = d->next )
                {
                    p = d->Partitions;
                    for( j = 0; j < d->nPartitions; j++, p = p->next )
                    {
                        if (PartitionId == p->Id || !NvOsStrcmp(Partition, p->Name))
                        {
                            PartitionFound = NV_TRUE;
                            if (!p->Filename)
                            {
                                NvAuPrintf("ERROR: no data to verify for partition %s!!\n",
                                    p->Name);
                                goto fail;
                            }
                        }
                    }
                }
                if (!PartitionFound)
                {
                    NvAuPrintf("ERROR: Partition %s does not exist.\n",
                               Partition);
                    goto fail;
                }
            }
            else
            {
                NvAuPrintf("ERROR: Device not known.\n");
                goto fail;
            }
        }
    }

    return NV_TRUE;

fail:
    NvAuPrintf("ERROR: Verify partition arguments check failed: %s\n",
                Partition);
    return NV_FALSE;
}

NvBool
nvflash_enable_verify_partition( NvU32 PartitionID )
{
    Nv3pCommand cmd = 0;
    Nv3pCmdVerifyPartition arg;
    NvBool verifyPartition = NV_FALSE;
    NvBool b= NV_FALSE;
    NvError e = NvSuccess;
    const char *PartitionName = NULL;
    const char *err_str = 0;

    // Check if partition exists in the verify list.
    PartitionName = nvflash_get_partNameById(PartitionID);
    VERIFY(PartitionName, err_str = "partition not found in partition table";
           goto fail);
    // Passing both PartName and PartitionId to find in Verifiy list. This is because
    // it is not known whether the name or the Id was used to mark the partition for
    // verification.
    verifyPartition = (NvFlashVerifyListIsPartitionToVerify(PartitionID, PartitionName)
                       == NvSuccess);

    // Enable verification of partition as appropriate.
    if (verifyPartition)
    {
        NvAuPrintf("Enabling verification for partition %s...\n", PartitionName);
        // Send command and break
        cmd = Nv3pCommand_VerifyPartitionEnable;
        arg.Id = PartitionID;
        NV_CHECK_ERROR_CLEANUP(
            Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
        );
        DEBUG_VERIFICATION_PRINT(("Enabled verification...\n"));
        b = nvflash_wait_status();
        VERIFY( b, goto fail );
    }
    else
    {
        DEBUG_VERIFICATION_PRINT(("WARNING: Partition %s not marked for "
                                  "verification...\n", PartitionName));
    }

    return NV_TRUE;

fail:
    if (err_str)
        NvAuPrintf("%s\n", err_str);
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;
}

NvBool
nvflash_send_verify_command(NvU32 PartitionId)
{

    Nv3pCommand cmd;
    Nv3pCmdVerifyPartition arg;
    NvBool b= NV_FALSE;
    NvError e = NvSuccess;
    const char *PartitionName = NULL;
    const char *err_str = 0;

    cmd = Nv3pCommand_VerifyPartition;
    arg.Id = PartitionId;

    PartitionName = nvflash_get_partNameById(PartitionId);
    VERIFY(PartitionName, err_str = "partition not found in partition table";
           goto fail);
    NvAuPrintf("Verifying partition %s...Please wait!!\n", PartitionName);

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, &arg, 0 )
    );

    b = nvflash_wait_status();
    VERIFY( b, goto fail );

    NvAuPrintf("Verification successful!!\n",PartitionId);
    return NV_TRUE;

fail:
    if (err_str)
        NvAuPrintf("%s\n", err_str);
    if(e != NvSuccess)
        NvAuPrintf("failed executing command %d NvError 0x%x\n",cmd,e);
    return NV_FALSE;

}

NvBool
nvflash_verify_partitions(void)
{

    NvBool b= NV_FALSE;
    NvError e = NvSuccess;
    Nv3pCommand cmd;
    NvU32 PartitionId = 0xFFFFFFFF;
    char Partition[NV_FLASH_PARTITION_NAME_LENGTH];
    NvFlashDevice *d;
    NvFlashPartition *p;
    NvU32 i, j;
    NvBool PartitionFound = NV_FALSE;
    NvBool SeekLstStart = NV_TRUE;

    do
    {
        e = NvFlashVerifyListGetNextPartition(Partition, SeekLstStart);
        PartitionId = NvUStrtoul(Partition, 0, 0);
        SeekLstStart = NV_FALSE;
        if(e != NvSuccess)
        {
            break;
        }
        // If all partitions are marked for verification,
        // send verify command for each partition.

        // Verification should not assume that the partition
        // index specified is valid and that it has data to verify
        // In any of these conditions, throw an error.
        if(PartitionId == 0xFFFFFFFF)
        {
            d = s_Devices;
            //do this for every partition having data
            for( i = 0; i < s_nDevices; i++, d = d->next )
            {
                    p = d->Partitions;
                    for( j = 0; j < d->nPartitions; j++, p = p->next )
                    {
                        if (p->Filename)
                        {
                            PartitionId = p->Id;
                            b = nvflash_send_verify_command(PartitionId);
                            VERIFY( b, goto fail );
                        }
                    }
                }
            break;
        }
        else
        {
            d = s_Devices;
            //do this for every partition having data
            for( i = 0; i < s_nDevices; i++, d = d->next )
            {
                p = d->Partitions;
                for( j = 0; j < d->nPartitions; j++, p = p->next )
                {
                    if (PartitionId == p->Id || !NvOsStrcmp(Partition, p->Name))
                    {
                        PartitionFound = NV_TRUE;
                        if (p->Filename)
                        {
                            PartitionId = p->Id;
                            b = nvflash_send_verify_command(PartitionId);
                            VERIFY( b, goto fail );
                        }
                        else
                        {
                            NvAuPrintf("ERROR: no data to verify for partition %s!!\n",
                                p->Name);
                            goto fail;
                        }
                    }
                }
            }
            if (!PartitionFound)
            {
                NvAuPrintf("ERROR: Partition %s does not exist.\n", Partition);
                goto fail;
            }
        }
    }while(1);

    // Indicate to device that verification of partitions is over.
    cmd = Nv3pCommand_EndVerifyPartition;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandSend( s_hSock, cmd, 0, 0 )
    );

    NvAuPrintf("VERIFICATION COMPLETE....\n");

    return NV_TRUE;

fail:
    NvAuPrintf("ERROR: Verification failed for partition %s\n",
                Partition);
    return NV_FALSE;
}

NvBool
nvflash_get_partlayout(void)
{
    const char *err_str = 0;
    NvError e = NvSuccess;
    NvBool b = NV_TRUE;
    NvFlashCmdGetPartitionTable *a = NULL;

    // Get the partition table from the device
    a = NvOsAlloc(sizeof(NvFlashCmdGetPartitionTable));
    if (!a)
    {
        err_str = "Insufficient system memory";
        b = NV_FALSE;
        goto fail;
    }

    NvOsMemset(a, 0, sizeof(NvFlashCmdGetPartitionTable));
    b = nvflash_cmd_getpartitiontable(a, &s_pPartitionInfo);
    VERIFY(b, err_str = "Unable to retrieve Partition table from device"; goto fail);
    b = nvflash_wait_status();
    VERIFY(b, goto fail);

fail:
    if (a)
        NvOsFree(a);
    if (err_str)
        NvAuPrintf("%s NvError 0x%x\n", err_str , e);
    return b;
}

NvU32
nvflash_get_partIdByName(const char *pPartitionName)
{
    const char *err_str = 0;
    NvError e = NvSuccess;
    NvBool b = NV_TRUE;
    NvU32 Index = 0;

    if (!s_pPartitionInfo)
    {
        b = nvflash_get_partlayout();
        VERIFY(b, goto fail);
    }

    while(s_pPartitionInfo[Index].PartId)
    {
        if (!NvOsStrcmp((char*)s_pPartitionInfo[Index].PartName, pPartitionName))
            return s_pPartitionInfo[Index].PartId;
        Index++;
    };

fail:
    if (err_str)
        NvAuPrintf("%s NvError 0x%x\n", err_str , e);
    return 0;
}

char*
nvflash_get_partNameById(NvU32 PartitionId)
{
    const char *err_str = 0;
    NvError e = NvSuccess;
    NvBool b = NV_TRUE;
    NvU32 Index = 0;

    if (!s_pPartitionInfo)
    {
        b = nvflash_get_partlayout();
        VERIFY(b, goto fail);
    }

    while(s_pPartitionInfo[Index].PartName)
    {
        if (s_pPartitionInfo[Index].PartId == PartitionId)
            return (char*) s_pPartitionInfo[Index].PartName;
        Index++;
    };

fail:
    if (err_str)
        NvAuPrintf("%s NvError 0x%x\n", err_str , e);
    return NULL;
}
