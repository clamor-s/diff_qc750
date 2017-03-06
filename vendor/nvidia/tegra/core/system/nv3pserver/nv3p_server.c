/*
 * Copyright (c) 2008-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "nvos.h"
#include "nvcommon.h"
#include "nvassert.h"
#include "nverror.h"
#include "nv3p.h"
#include "nvrm_module.h"
#include "nvrm_pmu.h"
#include "nvpartmgr.h"
#include "nvfs_defs.h"
#include "nvddk_blockdevmgr.h"
#include "nvddk_blockdev.h"
#include "nvstormgr.h"
#include "nvsystem_utils.h"
#include "nvbct.h"
#include "nvbu.h"
#include "nvfuse.h"
#include "nvbl_query.h"
#include "nvbl_memmap_nvap.h"
#include "nvcrypto_hash.h"
#include "nvcrypto_hash_defs.h"
#include "nvodm_pmu.h"
#include "nvcrypto_cipher.h"
#include "nv3p_server_utils.h"
#include "nvodm_fuelgaugefwupgrade.h"
#include "nvsku.h"
#include "nvddk_sd_block_driver_test.h"
#include "nvddk_dsi_block_driver_test.h"
#include "se_test.h"
#include "pwm_test.h"

#define SECONDS_BTN_1970_1980 315532800
#define SECONDS_BTN_1970_2009 1230768000
/**
 * Error handler macro
 *
 * Like NV_CHECK_ERROR_CLEANUP, except additionally assigns 'status' (an
 * Nv3pStatus code) to the local variable 's' if/when 'expr' fails
 */

#define NV_CHECK_ERROR_CLEANUP_3P(expr, status)                            \
    do                                                                     \
    {                                                                      \
        e = (expr);                                                        \
        if (e != NvSuccess)                                                \
        {                                                                  \
            s = status;                                                    \
            NvOsSnprintf(Message, NV3P_STRING_MAX, "nverror:0x%x (0x%x)", e & 0xFFFFF,e);     \
            goto fail;                                                     \
        }                                                                  \
    } while (0)

#define NV_CHECK_ERROR_GOTO_CLEAN_3P(expr, status)                            \
    do                                                                     \
    {                                                                      \
        e = (expr);                                                        \
        if (e != NvSuccess)                                                \
        {                                                                  \
            s = status;                                                    \
            goto clean;                                                    \
        }                                                                  \
    } while (0)


#define CHECK_VERIFY_ENABLED() \
    if(gs_VerifyPartition) \
    { \
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidCmdAfterVerify); \
    }

/**
 * size of shared scratch buffer used when carrying out Nv3p operations
 *
 * Note that lower-level read operations currently only accept transfer sizes
 * that will fit in an NvU32, so assert that staging buffer does not exceed
 * this size; otherwise implementation of data buffering for partition reads
 * and writes must change.
 */
#define NV3P_STAGING_SIZE (64 * 1024)
#define NV3P_AES_HASH_BLOCK_LEN 16

#define NV_CHARGE_SIZE_MAX NVAP_BASE_PA_SRAM_SIZE // minimum IRAM size

#ifndef TEST_RECOVERY
#define TEST_RECOVERY 0
#endif

NV_CT_ASSERT((NvU64)(NV3P_STAGING_SIZE) < (NvU64)(~((NvU32)0x0)));

static NvBool gs_VerifyPartition = NV_FALSE;
static NvBool gs_VerifyPending = NV_FALSE;
static NvU8 *gs_UpdateBct = NULL;

/**
 * Hash for a particular partition
 */
typedef struct PartitionToVerifyRec
{
    NvU32 PartitionId;
    NvU8 PartitionHash[NVCRYPTO_CIPHER_AES_BLOCK_BYTES];
    NvU64 PartitionDataSize;
    struct PartitionToVerify *pNext;
}PartitionToVerify;

/**
 * Parition Verification information
 */
typedef struct VerifyPartitionsRec
{
    /// Total number of partitions to be verified.
    NvU32 NumPartitions;
    /// Partition Ids to be verified.
    PartitionToVerify *LstPartitions;
}VerifyPartitions;

/**
 * State information for the NvFlash command processor
 */
typedef struct Nv3pServerStateRec
{
    /// NV_TRUE if device parameters are valid, else NV_FALSE
    NvBool IsValidDeviceInfo;

    /// device parameters from previous SetDevice operation
    NvRmModuleID DeviceId;
    NvU32 DeviceInstance;

    /// NV_TRUE if BCT was loaded from boot device, else NV_FALSE
    NvBool IsLocalBct;
    /// NV_TRUE if PT was loaded from boot device, else NV_FALSE
    NvBool IsLocalPt;
    /// NV_TRUE if a valid PT is available, else NV_FALSE
    NvBool IsValidPt;
    /// NV_TRUE if partition creation is in progress, else NV_FALSE
    NvBool IsCreatingPartitions;
    /// Number of partitions remaining to be created
    NvU32 NumPartitionsRemaining;

    /// Partition id's for specialty partitions (those that required special
    /// handling)
    NvU32 PtPartitionId;

    /// staging buffer
    /// * allocated on entry to 3p server
    /// * used as scratch buffer for all commands
    /// * freed on exit from 3p server
    NvU8 *pStaging;

    //Store the IRAM BCT so we can update
    //it with new info.
    NvBctHandle BctHandle;

    // Store a handle to the BIT; we'll need it for some boot loader update
    // operations.
    NvBitHandle BitHandle;

} Nv3pServerState;


static Nv3pServerState *s_pState;
static VerifyPartitions partInfo;
/**
 * Pointer to store the nvinternal data buf and its size
 */
static NvU8 *NvPrivDataBuffer = NULL;
static NvU32 NvPrivDataBufferSize = 0;

/**
 * Convert allocation policy from 3P-specific enum to NvPartMgr-specific enum
 *
 * @param AllocNv3p 3P allocation policy type
 * @param pAllocNvPartMgr pointer to NvPartMgr allocation policy type
 *
 * @retval NvSuccess Successful conversion
 * @retval NvError_BadParameter Unknown or unconvertable 3P allocation policy
 */
static NvError
ConvertAllocPolicy(
    Nv3pPartitionAllocationPolicy AllocNv3p,
    NvPartMgrAllocPolicyType *pAllocNvPartMgr);

/**
 * Convert file system type from 3P-specific enum to NvFsMgr-specific enum
 *
 * @param FsNv3p 3P file system type
 * @param pFsNvFsMgr pointer to NvFsMgr file system type
 *
 * @retval NvSuccess Successful conversion
 * @retval NvError_BadParameter Unknown or unconvertable 3P file system type
 */

static NvError
ConvertFileSystemType(
    Nv3pFileSystemType FsNv3p,
    NvFsMgrFileSystemType *pFsNvFsMgr);


/**
 * Convert partition type from 3P-specific enum to NvPartMgr-specific enum
 *
 * @param PartNv3p 3P partition type
 * @param pPartNvPartMgr pointer to NvPartMgr partition type
 *
 * @retval NvSuccess Successful conversion
 * @retval NvError_BadParameter Unknown or unconvertable 3P partition type
 */
static NvError
ConvertPartitionType(
    Nv3pPartitionType PartNv3p,
    NvPartMgrPartitionType *pPartNvPartMgr);


/**
 * Queries the boot device type from the boot information table, and returns
 * an NvBootDevType and instance for the boot device.
 *
 * @param bootDevId pointer to boot device type
 * @param bootDevInstance pointer to instance of blockDevId
 */
static NvError
GetSecondaryBootDevice(
    NvBootDevType *bootDevId,
    NvU32 *bootDevInstance);

/**
 * Load partition table into memory. Needed to perform
 * FS operations.
 *
 *
 * @retval NvSuccess Successfully loaded partition table
 * @retval NvError_* Failed to load partition table
 */
static NvError
LoadPartitionTable(Nv3pPartitionTableLayout *arg);

/**
 * Unload partition table by resetting required global variables
 * required for --create in resume mode
 */
void
UnLoadPartitionTable(void);
/**
 * Allocate and initialize 3P Server's internal state structure
 *
 * Once allocated, pointer to state structure is stored in s_pState static
 * variable.
 *
 * @retval NvSuccess State allocated/initialized successfully
 * @retval NvError_InsufficientMemory Memory allocation failure
 */
static NvError
AllocateState(void);

/**
 * Deallocate 3P Server's internal state structure
 *
 * If state structure pointed to by s_pState is non-NULL, deallocates state
 * structure and its members.  s_pState static variable is set to NULL on
 * completion.
 */
static void
DeallocateState(void);

/**
 * Report 3P command status to client
 *
 * Sends Nv3pCommand_Status message to client, reporting status of most-recent
 * 3P command.
 *
 * @param hSock pointer to the handle for the 3P socket state
 * @param Message string describing status
 * @param Status status code
 * @param Flags Reserved, must be zero.
 *
 * @retval NvSuccess 3P Status message sent successfully
 * @retval NvError_* Unable to send message
 */
static NvError
ReportStatus(
    Nv3pSocketHandle hSock,
    char *Message,
    Nv3pStatus Status,
    NvU32 Flags);

static NvError
UpdateBlInfo(Nv3pCmdDownloadPartition *a,
        NvU8 *hash,
        NvU32 padded_length,
        Nv3pStatus *status);
/**
 * Locates the partition by PartitionId in the list
 * of partitions to verify.
 *
 * @param PartitionId Id of partition to be located.
 * @param pPartition node in the list of partitions to verify (outputArg)
 *
 * @retval NvSuccess Partition was located
 * @retval NvError_BadParameter Unable to locate partition
 */
static NvError
LocatePartitionToVerify(NvU32 PartitionId, PartitionToVerify **pPartition);

/**
 * Reads and verifies the data of a partition.
 *
 * @param pPartitionToVerify Pointer to node in list of partitions to verify
 *
 * @retval NvSuccess Partition data was verified successfully.
 * @retval NvError_Nv3pBadReturnData Partition data verification failed.
 */
static NvError
ReadVerifyData(PartitionToVerify *pPartitionToVerify, Nv3pStatus *status);

/**
 * Verifies sign of the incoming data stream
 *
 * @param pBuffer pointer to data buffer
 * @param bytesToSign number of bytes in this part of the data stream.
 * @param StartMsg marks beginning of the data stream.
 * @param EndMsg Marks the end of the msg.
 * @param ExpectedHash Hash value expected.
 * @warning Caller of this function is responsible for setting
 *            StartMsg and EndMsg flags correctly.
 *
 * @retval NvSuccess if verification of the data was successful
 * @retval NvError_* There were errors verifying the data
 */
static NvError
VerifySignature(
    NvU8 *pBuffer,
    NvU32 bytesToSign,
    NvBool StartMsg,
    NvBool EndMsg,
    NvU8 *ExpectedHash);

/**
 * Adds the partition id to the list of partitions to verify
 *
 * @param PartitionId parition id to be added to the list of partitions to verify
 * @param pNode pointer to the node that was added to the list corresponding
         to the partitionId that was given.
 * @retval NvSuccess Partition id added successfully.
 * @retval NvError_InsufficientMemory Partition id could not be added due to
 *    lack of space.
 */
static NvError
SetPartitionToVerify(NvU32 PartitionId, PartitionToVerify **pNode);

/**
 * Initialize list of partitions to verify
 *
 * @retval None
 *
 */
static void
InitLstPartitionsToVerify(void);

/**
 * Free up the list of partitions to verify
 *
 * @retval None
 *
 */
static void
DeInitLstPartitionsToVerify(void);

/**
 * Upgrades the firmware of the FuelGauge.
 *
 * @param hSock Nv3p Socket handle.
 * @param command Nv3p Command.
 *
 * @retval NvSuccess Firmware upgraded successfully.
 * @retval NvError_* There was an error while upgrading the firmware.
 *
 */

static NvError
FuelGaugeFwUpgrade(
    Nv3pSocketHandle hSock,
    Nv3pCommand command,
    void *arg);

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
    NvError                                     \
    name(                                       \
        Nv3pSocketHandle hSock,                 \
        Nv3pCommand command,                    \
        void *arg                               \
        )

static COMMAND_HANDLER(GetBct);
static COMMAND_HANDLER(DownloadBct);
static COMMAND_HANDLER(SetBlHash);
static COMMAND_HANDLER(UpdateBct);
static COMMAND_HANDLER(SetDevice);
static COMMAND_HANDLER(DeleteAll);
static COMMAND_HANDLER(FormatAll);
static COMMAND_HANDLER(ReadPartitionTable);
static COMMAND_HANDLER(StartPartitionConfiguration);
static COMMAND_HANDLER(CreatePartition);
static COMMAND_HANDLER(EndPartitionConfiguration);
static COMMAND_HANDLER(QueryPartition);
static COMMAND_HANDLER(ReadPartition);
static COMMAND_HANDLER(RawWritePartition);
static COMMAND_HANDLER(RawReadPartition);
static COMMAND_HANDLER(DownloadPartition);
static COMMAND_HANDLER(SetBootPartition);
static COMMAND_HANDLER(OdmOptions);
static COMMAND_HANDLER(OdmCommand);
static COMMAND_HANDLER(Sync);
static COMMAND_HANDLER(Obliterate);
static COMMAND_HANDLER(VerifyPartitionEnable);
static COMMAND_HANDLER(VerifyPartition);
static COMMAND_HANDLER(FormatPartition);
static COMMAND_HANDLER(SetTime);
static COMMAND_HANDLER(GetDevInfo);
static COMMAND_HANDLER(DownloadNvPrivData);

NvError
ConvertAllocPolicy(
    Nv3pPartitionAllocationPolicy AllocNv3p,
    NvPartMgrAllocPolicyType *pAllocNvPartMgr)
{

    switch (AllocNv3p)
    {
        case Nv3pPartitionAllocationPolicy_Absolute:
            *pAllocNvPartMgr = NvPartMgrAllocPolicyType_Absolute;
            break;

        case Nv3pPartitionAllocationPolicy_Sequential:
            *pAllocNvPartMgr = NvPartMgrAllocPolicyType_Relative;
            break;

        default:
            return NvError_BadParameter;
    }

    return NvSuccess;
}

NvError
ConvertFileSystemType(
    Nv3pFileSystemType FsNv3p,
    NvFsMgrFileSystemType *pFsNvFsMgr)
{

    switch (FsNv3p)
    {
        case Nv3pFileSystemType_Basic:
            *pFsNvFsMgr = NvFsMgrFileSystemType_Basic;
            break;

        case Nv3pFileSystemType_Enhanced:
            *pFsNvFsMgr = NvFsMgrFileSystemType_Enhanced;
            break;

        case Nv3pFileSystemType_Ext2:
            *pFsNvFsMgr = NvFsMgrFileSystemType_Ext2;
            break;

        case Nv3pFileSystemType_Yaffs2:
            *pFsNvFsMgr = NvFsMgrFileSystemType_Yaffs2;
            break;

        case Nv3pFileSystemType_Ext3:
            *pFsNvFsMgr = NvFsMgrFileSystemType_Ext3;
            break;

        case Nv3pFileSystemType_Ext4:
            *pFsNvFsMgr = NvFsMgrFileSystemType_Ext4;
            break;

        default:
            return NvError_BadParameter;
    }

    return NvSuccess;
}

NvError
ConvertPartitionType(
    Nv3pPartitionType PartNv3p,
    NvPartMgrPartitionType *pPartNvPartMgr)
{

    switch (PartNv3p)
    {
        case Nv3pPartitionType_Bct:
            *pPartNvPartMgr = NvPartMgrPartitionType_Bct;
            break;

        case Nv3pPartitionType_Bootloader:
            *pPartNvPartMgr = NvPartMgrPartitionType_Bootloader;
            break;

        case Nv3pPartitionType_BootloaderStage2:
            *pPartNvPartMgr = NvPartMgrPartitionType_BootloaderStage2;
            break;

        case Nv3pPartitionType_PartitionTable:
            *pPartNvPartMgr = NvPartMgrPartitionType_PartitionTable;
            break;

        case Nv3pPartitionType_NvData:
            *pPartNvPartMgr = NvPartMgrPartitionType_NvData;
            break;

        case Nv3pPartitionType_Data:
            *pPartNvPartMgr = NvPartMgrPartitionType_Data;
            break;

        case Nv3pPartitionType_Mbr:
            *pPartNvPartMgr = NvPartMgrPartitionType_Mbr;
            break;

        case Nv3pPartitionType_Ebr:
            *pPartNvPartMgr = NvPartMgrPartitionType_Ebr;
            break;
        case Nv3pPartitionType_GP1:
            *pPartNvPartMgr = NvPartMgrPartitionType_GP1;
            break;
        case Nv3pPartitionType_GPT:
            *pPartNvPartMgr = NvPartMgrPartitionType_GPT;
            break;

        default:
            return NvError_BadParameter;
    }

    return NvSuccess;
}

NvError
ReportStatus(
    Nv3pSocketHandle hSock,
    char *Message,
    Nv3pStatus Status,
    NvU32 Flags)
{
    NvError e;
    Nv3pCmdStatus CmdStatus;

    CmdStatus.Code = Status;
    CmdStatus.Flags = Flags;
    if (Message)
        NvOsStrncpy(CmdStatus.Message, Message, NV3P_STRING_MAX);
    else
        NvOsMemset(CmdStatus.Message, 0x0, NV3P_STRING_MAX);

    e = Nv3pCommandSend(hSock, Nv3pCommand_Status, (NvU8 *)&CmdStatus, 0);

    return e;
}

COMMAND_HANDLER(GetBct)
{
    NvError e;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};;

    // Nv3pCmdGetBct *a = (Nv3pCmdGetBct *)arg;

    CHECK_VERIFY_ENABLED();

    NV_CHECK_ERROR_CLEANUP_3P(NvError_NotImplemented, Nv3pStatus_NotImplemented);

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nGetBct failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(DownloadBct)
{
    NvError e;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    NvU8 *data, *bct = NULL;
    NvU32 bytesLeftToTransfer = 0;
    NvU32 transferSize = 0;
    NvU32 BctOffset = 0;
    NvBool IsFirstChunk = NV_TRUE;
    NvBool IsLastChunk = NV_FALSE;
    Nv3pCmdDownloadBct *a = (Nv3pCmdDownloadBct *)arg;
    NvU32 bctsize = 0;
    NvU32 size, instance = 0;
    NvBlOperatingMode OpMode;
    NvU8 *pBlHash = NULL;

    if (!a->Length)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    size = sizeof(NvU8);
    NV_CHECK_ERROR_CLEANUP_3P(
        NvBctGetData(s_pState->BctHandle, NvBctDataType_BctSize,
                 &size, &instance, &bctsize),
                Nv3pStatus_InvalidBCTSize);

    bct = NvOsAlloc(a->Length);
    if (!bct)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);
    if (a->Length != bctsize)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, a, 0 )
    );
    OpMode = NvFuseGetOperatingMode();
    pBlHash = NvOsAlloc(NV3P_AES_HASH_BLOCK_LEN);
    if (!pBlHash)
    {
        e = NvError_InvalidState;
        s = Nv3pStatus_InvalidState;
        goto clean;
    }

    bytesLeftToTransfer = a->Length;
    data = bct;
    do
    {
        transferSize = (bytesLeftToTransfer > NV3P_STAGING_SIZE) ?
                        NV3P_STAGING_SIZE :
                        bytesLeftToTransfer;

        e = Nv3pDataReceive(hSock, data, transferSize, 0, 0);
        if (e)
            goto clean;

        if (OpMode == NvBlOperatingMode_OdmProductionSecure)
        {
            if (IsFirstChunk)
                BctOffset = NV3P_AES_HASH_BLOCK_LEN;
            else
                BctOffset = 0;
            if ((bytesLeftToTransfer - transferSize) == 0)
                IsLastChunk = NV_TRUE;

            NV_CHECK_ERROR_GOTO_CLEAN_3P(
                NvSysUtilSignData(data + BctOffset, transferSize - BctOffset,
                    IsFirstChunk, IsLastChunk, pBlHash, &OpMode),
                    Nv3pStatus_InvalidBCT
            );
            // need to decrypt the encrypted bct got from nvsbktool
            NV_CHECK_ERROR_GOTO_CLEAN_3P(
                NvSysUtilEncryptData(data + BctOffset, transferSize - BctOffset,
                    IsFirstChunk, IsLastChunk, NV_TRUE, OpMode),
                    Nv3pStatus_InvalidBCT
            );
            // first chunk has been processed.
            IsFirstChunk = NV_FALSE;
        }
        data += transferSize;
        bytesLeftToTransfer -= transferSize;
    } while (bytesLeftToTransfer);

    if (OpMode == NvBlOperatingMode_OdmProductionSecure)
    {
        // validate secure bct got from sbktool
        if (NvOsMemcmp(pBlHash, bct, NV3P_AES_HASH_BLOCK_LEN))
        {
            e = NvError_BadParameter;
            s = Nv3pStatus_BLValidationFailure;
            goto clean;
        }
    }
    size = bctsize;
    instance = 0;
    NV_CHECK_ERROR_GOTO_CLEAN_3P(
        NvBctSetData(s_pState->BctHandle, NvBctDataType_FullContents,
                     &size, &instance, bct),
                     Nv3pStatus_InvalidBCT
    );

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
    }
clean:
    if(bct)
        NvOsFree(bct);
    if(pBlHash)
        NvOsFree(pBlHash);
    if (e)
        NvOsDebugPrintf("\nDownloadBct failed. NvError %u NvStatus %u\n", e, s);

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(SetBlHash)
{
    NvError e;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    NvU8 *data, *bct = NULL;
    NvU32 bytesLeftToTransfer = 0;
    NvU32 transferSize = 0;
    NvU32 BctOffset = 0;
    NvBool IsFirstChunk = NV_TRUE;
    NvBool IsLastChunk = NV_FALSE;
    // store bl hash and index in odm secure mode
    NvBootHash BlHash[NV3P_AES_HASH_BLOCK_LEN >> 2];
    NvU32 BlLoadAddr[NV3P_AES_HASH_BLOCK_LEN >> 2];
    Nv3pCmdBlHash *a = (Nv3pCmdBlHash *)arg;
    NvU32 bctsize = 0;
    NvU32 i, Size, Instance;
    NvBctHandle LocalBctHandle;
    NvBlOperatingMode OpMode = NvBlOperatingMode_OdmProductionSecure;

    if (!a->Length)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidState);
    Size = sizeof(NvU8);
    Instance = 0;
    NV_CHECK_ERROR_CLEANUP_3P(
        NvBctGetData(s_pState->BctHandle, NvBctDataType_BctSize,
                     &Size, &Instance, &bctsize),
                     Nv3pStatus_InvalidBCTSize);
    bct = NvOsAlloc(a->Length);
    if (!bct)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);
    if (a->Length != bctsize)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete(hSock, command, a, 0)
    );

    bytesLeftToTransfer = a->Length;
    data = bct;
    do
    {
        transferSize = (bytesLeftToTransfer > NV3P_STAGING_SIZE) ?
                        NV3P_STAGING_SIZE :
                        bytesLeftToTransfer;

        e = Nv3pDataReceive(hSock, data, transferSize, 0, 0);
        if (e)
            goto clean;

        if (IsFirstChunk)
            BctOffset = NV3P_AES_HASH_BLOCK_LEN;
        else
            BctOffset = 0;
        if ((bytesLeftToTransfer - transferSize) == 0)
            IsLastChunk = NV_TRUE;

        NV_CHECK_ERROR_GOTO_CLEAN_3P(
            NvSysUtilSignData(data + BctOffset, transferSize - BctOffset,
                IsFirstChunk, IsLastChunk, (NvU8*)&BlHash[0], &OpMode),
                Nv3pStatus_InvalidBCT
        );
        // need to decrypt the encrypted bct got from nvsbktool
        NV_CHECK_ERROR_GOTO_CLEAN_3P(
            NvSysUtilEncryptData(data + BctOffset, transferSize - BctOffset,
                IsFirstChunk, IsLastChunk, NV_TRUE, OpMode),
                Nv3pStatus_InvalidBCT
        );

        // first chunk has been processed.
        IsFirstChunk = NV_FALSE;
        data += transferSize;
        bytesLeftToTransfer -= transferSize;
    } while (bytesLeftToTransfer);
    // validate secure bct got from sbktool
    if (NvOsMemcmp(&BlHash[0], bct, NV3P_AES_HASH_BLOCK_LEN))
    {
        e = NvError_BadParameter;
        s = Nv3pStatus_BLValidationFailure;
        goto clean;
    }

    NV_CHECK_ERROR_GOTO_CLEAN_3P(
        NvBctInit(&a->Length, bct, &LocalBctHandle),
                  Nv3pStatus_InvalidBCT
    );

    // save bl hash and index to be saved in system bct.
    for (i = 0; i < NV3P_AES_HASH_BLOCK_LEN >> 2; i++)
    {
        NvU32 LoadAddress;
        NvBootHash HashSrc;

        Size = sizeof(NvU32);
        Instance = i;

        NV_CHECK_ERROR_GOTO_CLEAN_3P(
            NvBctGetData(LocalBctHandle, NvBctDataType_BootLoaderLoadAddress,
                         &Size, &Instance, &LoadAddress),
                         Nv3pStatus_InvalidBCT
        );
        BlLoadAddr[i] = LoadAddress;
        NV_CHECK_ERROR_GOTO_CLEAN_3P(
            NvBctGetData(LocalBctHandle, NvBctDataType_BootLoaderCryptoHash,
                         &Size, &Instance, &HashSrc),
                         Nv3pStatus_InvalidBCT
        );
        NvOsMemcpy(&BlHash[i], &HashSrc, sizeof(NvBootHash));
    }

    // update hash value of bootloader whose attribute matches with it's partition id
    // given with --download option, also find out which hash needs to be updated from
    // secure bct using load address for this bootloader, used for secure BL download
    for (i = 0 ; i < NV3P_AES_HASH_BLOCK_LEN >> 2; i++)
    {
        NvU32 BlAttr;

        Size = sizeof(NvU32);
        Instance = i;

        NV_CHECK_ERROR_GOTO_CLEAN_3P(
            NvBctGetData(s_pState->BctHandle, NvBctDataType_BootLoaderAttribute,
                         &Size, &Instance, &BlAttr),
                         Nv3pStatus_InvalidBCT
        );
        if (BlAttr == a->BlIndex)
        {
            NvU32 LoadAddress, j;

            for (j = 0; j < NV3P_AES_HASH_BLOCK_LEN >> 2; j++)
            {
                // find index in secure bct from where bl hash needs to be copied
                NV_CHECK_ERROR_GOTO_CLEAN_3P(
                    NvBctGetData(s_pState->BctHandle, NvBctDataType_BootLoaderLoadAddress,
                                 &Size, &Instance, &LoadAddress),
                                 Nv3pStatus_InvalidBCT
                );
                if (BlLoadAddr[j] == LoadAddress)
                {
                    NV_CHECK_ERROR_GOTO_CLEAN_3P(
                        NvBctSetData(s_pState->BctHandle, NvBctDataType_BootLoaderCryptoHash,
                                     &Size, &Instance, &BlHash[j]),
                                     Nv3pStatus_InvalidBCT
                    );
                    break;
                }
            }
            if (j == NV3P_AES_HASH_BLOCK_LEN >> 2)
            {
                s = Nv3pStatus_InvalidBCT;
                goto clean;
            }
            break;
        }
    }

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
    }
clean:
    if(bct)
        NvOsFree(bct);
    if (e)
        NvOsDebugPrintf("\nSetBlHash failed. NvError %u NvStatus %u\n", e, s);

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(UpdateBct)
{
    NvError e;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    NvU8 *data = NULL;
    NvU32 bytesLeftToTransfer = 0;
    NvU32 transferSize = 0;
    Nv3pCmdUpdateBct *a = (Nv3pCmdUpdateBct *)arg;
    NvU32 bctsize = 0;
    NvU32 size, instance = 0;

    if (!a->Length)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);
    size = sizeof(NvU8);
    NV_CHECK_ERROR_CLEANUP_3P(
        NvBctGetData(s_pState->BctHandle, NvBctDataType_BctSize,
                 &size, &instance, &bctsize),
                Nv3pStatus_InvalidBCTSize);

    if (a->Length != bctsize)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);
    if(!gs_UpdateBct)
    {
        gs_UpdateBct = NvOsAlloc(a->Length);
        if (!gs_UpdateBct)
            NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);
    }
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete( hSock, command, a, 0 )
    );

    if(gs_UpdateBct)
    {
        bytesLeftToTransfer = a->Length;
        data = gs_UpdateBct;
        do
        {
            transferSize = (bytesLeftToTransfer > NV3P_STAGING_SIZE) ?
                            NV3P_STAGING_SIZE :
                            bytesLeftToTransfer;

            e = Nv3pDataReceive( hSock, data, transferSize,0, 0);
            if (e)
                goto clean;

            data += transferSize;
            bytesLeftToTransfer -= transferSize;
        } while (bytesLeftToTransfer);
    }

    NvBlUpdateBct(gs_UpdateBct, a->BctSection);

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
    }
clean:
    if (e)
        NvOsDebugPrintf("\nUpdateBct failed. NvError %u NvStatus %u\n", e, s);

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(SetDevice)
{
    NvError e;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};

    Nv3pCmdSetDevice *a = (Nv3pCmdSetDevice *)arg;

    CHECK_VERIFY_ENABLED();


    NV_CHECK_ERROR_CLEANUP_3P(NvBl3pConvert3pToRmDeviceType(a->Type,
                            &s_pState->DeviceId),
                              Nv3pStatus_InvalidDevice);
    s_pState->DeviceInstance = a->Instance;
    s_pState->IsValidDeviceInfo = NV_TRUE;

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nSetDevice failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(DeleteAll)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    NvDdkBlockDevIoctl_FormatDeviceInputArgs InputArgs;
    NvDdkBlockDevHandle DevHandle = NULL;
    char Message[NV3P_STRING_MAX] = {'\0'};

#if 1//def NV_EMBEDDED_BUILD
    s = Nv3pStatus_NotSupported;
    NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));
    return ReportStatus(hSock, Message, s, 0);
#endif

    InputArgs.EraseType = NvDdkBlockDevEraseType_NonFactoryBadBlocks;//NvDdkBlockDevEraseType_GoodBlocks;//

    CHECK_VERIFY_ENABLED();

    if (!s_pState->IsValidDeviceInfo)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidDevice);

    NV_CHECK_ERROR_CLEANUP_3P(NvDdkBlockDevMgrDeviceOpen(
                                  (NvDdkBlockDevMgrDeviceId)s_pState->DeviceId,
                                  s_pState->DeviceInstance,
                                  0,  /* MinorInstance */
                                  &DevHandle),
                              Nv3pStatus_MassStorageFailure);

    NV_CHECK_ERROR_CLEANUP_3P(DevHandle->NvDdkBlockDevIoctl(
                                  DevHandle,
                                  NvDdkBlockDevIoctlType_FormatDevice,
                                  sizeof(NvDdkBlockDevIoctl_FormatDeviceInputArgs),
                                  0,
                                  &InputArgs,
                                  NULL),
                              Nv3pStatus_MassStorageFailure);

fail:
    if (DevHandle)
        DevHandle->NvDdkBlockDevClose(DevHandle);

    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nDeleteAll failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));
    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(FormatAll)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    char FileName[NVPARTMGR_PARTITION_NAME_LENGTH];
    NvU32 PartitionId = 0;
    NvU32 BctPartId;

    // error if PT not available
    if (!s_pState->IsValidPt)
    {
        NV_CHECK_ERROR_CLEANUP_3P(LoadPartitionTable(NULL),
                                  Nv3pStatus_PartitionTableRequired);
        s_pState->IsValidPt = NV_TRUE;
    }

    if (s_pState->IsCreatingPartitions)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    if (s_pState->NumPartitionsRemaining)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetIdByName("BCT", &BctPartId),
                                                    Nv3pStatus_InvalidState);

    PartitionId = NvPartMgrGetNextId(PartitionId);
    while(PartitionId)
    {
#ifdef NV_EMBEDDED_BUILD
        // Protect bct
        if (PartitionId != BctPartId)
#endif
        {
            NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetNameById(PartitionId, FileName),
                                        Nv3pStatus_InvalidPartition);
            NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFormat(FileName),
                                        Nv3pStatus_InvalidPartition);
        }
        PartitionId = NvPartMgrGetNextId(PartitionId);
    }
fail:
    if (e)
        NvOsDebugPrintf("\nFormatAll failed. NvError %u NvStatus %u\n", e, s);

    NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));
    return ReportStatus(hSock, Message, s, 0);
}


COMMAND_HANDLER(Obliterate)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    NvDdkBlockDevIoctl_FormatDeviceInputArgs InputArgs;
    NvDdkBlockDevHandle DevHandle = NULL;
    char Message[NV3P_STRING_MAX] = {'\0'};
    InputArgs.EraseType = NvDdkBlockDevEraseType_NonFactoryBadBlocks;

    CHECK_VERIFY_ENABLED();

    if (!s_pState->IsValidDeviceInfo)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidDevice);

    NV_CHECK_ERROR_CLEANUP_3P(NvDdkBlockDevMgrDeviceOpen(
                                  (NvDdkBlockDevMgrDeviceId)s_pState->DeviceId,
                                  s_pState->DeviceInstance,
                                  0,  /* MinorInstance */
                                  &DevHandle),
                              Nv3pStatus_MassStorageFailure);

    NV_CHECK_ERROR_CLEANUP_3P(DevHandle->NvDdkBlockDevIoctl(
                                  DevHandle,
                                  NvDdkBlockDevIoctlType_FormatDevice,
                                  sizeof(NvDdkBlockDevIoctl_FormatDeviceInputArgs),
                                  0,
                                  &InputArgs,
                                  NULL),
                              Nv3pStatus_MassStorageFailure);

fail:
    if (DevHandle)
        DevHandle->NvDdkBlockDevClose(DevHandle);

    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nObliterate failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(ReadPartitionTable)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    NvPartInfo PTInfo;
    NvU32 NumPartitions = 0;
    NvU32 PartitionCount = 0;
    NvU32 PartitionId = 0;
    NvU8* PartitionData = 0;
    Nv3pPartitionInfo* PartInfo = 0;
    Nv3pCmdReadPartitionTable *a = (Nv3pCmdReadPartitionTable*)arg;
    char FileName[NVPARTMGR_PARTITION_NAME_LENGTH];

    CHECK_VERIFY_ENABLED();

    // verify PT is available
    if (!s_pState->IsValidPt)
    {
        Nv3pPartitionTableLayout arg;
        arg.NumLogicalSectors = a->NumLogicalSectors;
        arg.StartLogicalAddress = a->StartLogicalSector;
        // Note : Incase of LoadPartitionTable call from nvflash_check_skiperror
        // partition table will get unloaded in the 'UnLoadPartitionTable()' call
        // in StartPartitionConfiguration.
        NV_CHECK_ERROR_CLEANUP_3P(LoadPartitionTable(&arg),
                                  Nv3pStatus_PartitionTableRequired);
        s_pState->IsValidPt = NV_TRUE;
    }

    PartitionId = 0;
    PartitionId = NvPartMgrGetNextId(PartitionId);
    while(PartitionId)
    {
        NumPartitions++;
        PartitionId = NvPartMgrGetNextId(PartitionId);
    }

    a->Length = sizeof(Nv3pPartitionInfo) * NumPartitions;
    PartitionData = NvOsAlloc((NvU32)a->Length);
    if(!PartitionData)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InsufficientMemory, Nv3pStatus_BadParameter);

    PartInfo = (Nv3pPartitionInfo*)PartitionData;

    PartitionId = 0;
    PartitionId = NvPartMgrGetNextId(PartitionId);
    while(PartitionId)
    {
        NvFsMountInfo minfo;
        NvDdkBlockDevInfo dinfo;
        NvDdkBlockDevHandle hDev = 0;
        NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetPartInfo(PartitionId, &PTInfo),
                                  Nv3pStatus_InvalidPartition);
        NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetNameById(PartitionId, FileName),
                                  Nv3pStatus_InvalidPartition);
        NV_CHECK_ERROR_CLEANUP(
            NvPartMgrGetFsInfo( PartitionId, &minfo )
        );
        NV_CHECK_ERROR_CLEANUP(
            NvDdkBlockDevMgrDeviceOpen( (NvDdkBlockDevMgrDeviceId)minfo.DeviceId,
                minfo.DeviceInstance, 0, &hDev )
        );
        hDev->NvDdkBlockDevGetDeviceInfo( hDev, &dinfo );
        hDev->NvDdkBlockDevClose(hDev);

        NvOsMemcpy(PartInfo[PartitionCount].PartName, FileName, MAX_PARTITION_NAME_LENGTH);
        PartInfo[PartitionCount].PartId = PartitionId;
        PartInfo[PartitionCount].DeviceId = minfo.DeviceId;
        PartInfo[PartitionCount].StartLogicalAddress= (NvU32)PTInfo.StartLogicalSectorAddress;
        PartInfo[PartitionCount].NumLogicalSectors= (NvU32)PTInfo.NumLogicalSectors;
        PartInfo[PartitionCount].BytesPerSector = dinfo.BytesPerSector;;
        PartInfo[PartitionCount].StartPhysicalAddress= (NvU32)PTInfo.StartPhysicalSectorAddress;
        PartInfo[PartitionCount].EndPhysicalAddress= (NvU32)PTInfo.EndPhysicalSectorAddress;
        PartitionCount++;
        PartitionId = NvPartMgrGetNextId(PartitionId);
    }

    NV_CHECK_ERROR_CLEANUP_3P(Nv3pCommandComplete(hSock, command, arg, 0),
                              Nv3pStatus_BadParameter);
    NV_CHECK_ERROR_CLEANUP_3P(Nv3pDataSend(hSock, PartitionData, (NvU32)a->Length, 0),
                              Nv3pStatus_BadParameter);

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nReadPartitionTable failed. NvError %u NvStatus %u\n", e, s);
    }
    if(PartitionData)
        NvOsFree(PartitionData);

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(StartPartitionConfiguration)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};

    Nv3pCmdStartPartitionConfiguration *a = (Nv3pCmdStartPartitionConfiguration *)arg;

    CHECK_VERIFY_ENABLED();
    UnLoadPartitionTable();

    if (s_pState->IsValidPt)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_PartitionTableRequired);

    if (s_pState->IsCreatingPartitions)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidState);

    if (!a->nPartitions)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_BadParameter, Nv3pStatus_BadParameter);

    s_pState->IsValidPt = NV_FALSE;
    s_pState->IsCreatingPartitions = NV_TRUE;
    s_pState->NumPartitionsRemaining = a->nPartitions;

    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrCreateTableStart(a->nPartitions),
                              Nv3pStatus_PartitionCreation);
fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nStartPartitionConfiguration failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(SetTime)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    Nv3pCmdSetTime *a = (Nv3pCmdSetTime *)arg;
    NvOsOsInfo OsInfo;
    NvRmDeviceHandle vRmDevice = NULL;
    NV_CHECK_ERROR(NvOsGetOsInformation(&OsInfo));
    //we are getting total time in seconds from nvflash host with base time as 1970
    //adjust time to base  to 1980 if we are running on windows
    if(OsInfo.OsType == NvOsOs_Windows)
        a->Seconds += SECONDS_BTN_1970_1980;

    //Setting Baseyear to 2009 to increase the clock life
    a->Seconds -= SECONDS_BTN_1970_2009;
#if NVODM_BOARD_IS_SIMULATION==0
    NV_CHECK_ERROR_CLEANUP(NvRmOpenNew(&vRmDevice));
    if (!NvRmPmuWriteRtc(vRmDevice, a->Seconds))
        goto fail;
    NvRmClose(vRmDevice);


fail:
    if (e != NvError_BadValue)
        NvRmClose(vRmDevice);
#endif
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nSetTime failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}


COMMAND_HANDLER(CreatePartition)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;

    Nv3pCmdCreatePartition *a = (Nv3pCmdCreatePartition *)arg;
    NvPartAllocInfo AllocInfo;
    NvFsMountInfo MountInfo;
    NvPartMgrPartitionType PartitionType = (NvPartMgrPartitionType)0; // invalid
    NvU8 *customerData = NULL;
    char Message[NV3P_STRING_MAX] = {'\0'};
    CHECK_VERIFY_ENABLED();

    NvOsMemset(&MountInfo,0,sizeof(NvFsMountInfo));
    // get device info
    if (!s_pState->IsValidDeviceInfo)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_BadParameter, Nv3pStatus_InvalidDevice);

    // check that partition definition is in progress
    if (!s_pState->IsCreatingPartitions)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidState);

    // check that caller isn't attempting to create too many partitions
    if (!s_pState->NumPartitionsRemaining)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidState);

    NV_CHECK_ERROR_CLEANUP_3P(ConvertPartitionType(
                                  a->Type,
                                  &PartitionType),
                              Nv3pStatus_BadParameter);

    // check that all special partitions are allocated on the boot device
    if (a->Type == Nv3pPartitionType_PartitionTable ||
        a->Type == Nv3pPartitionType_Bct ||
        a->Type == Nv3pPartitionType_Bootloader)
    {
        NvBootDevType BootDevice;
        NvU32 BootInstance;

        NV_CHECK_ERROR_CLEANUP(
            GetSecondaryBootDevice(&BootDevice, &BootInstance)
        );

        if (NvBlValidateRmDevice(BootDevice, s_pState->DeviceId) != NV_TRUE)
            NV_CHECK_ERROR_CLEANUP_3P(NvError_BadParameter,
                                      Nv3pStatus_NotBootDevice);
    }

    // populate mount information
    MountInfo.DeviceId = s_pState->DeviceId;
    MountInfo.DeviceInstance = s_pState->DeviceInstance;
#if NV3P_STRING_MAX < NVPARTMGR_MOUNTPATH_NAME_LENGTH
    NvOsStrncpy(MountInfo.MountPath, a->Name, NV3P_STRING_MAX);
    MountInfo.MountPath[NV3P_STRING_MAX] = '\0';
#else
    if(NvOsStrlen(a->Name) >= NVPARTMGR_MOUNTPATH_NAME_LENGTH)
    {
        NV_CHECK_ERROR_CLEANUP_3P(NvError_BadParameter,
                                  Nv3pStatus_InvalidPartitionName);
    }

    NvOsStrncpy(MountInfo.MountPath, a->Name, NVPARTMGR_MOUNTPATH_NAME_LENGTH);
#endif
    // FIXME -- change FileSystemType to an enum in NvFsMountInfo
    // Convert Nv File system types
    if (a->FileSystem < Nv3pFileSystemType_External)
    {
        NV_CHECK_ERROR_CLEANUP_3P(ConvertFileSystemType(
                                      a->FileSystem,
                                      (NvFsMgrFileSystemType *)&MountInfo.FileSystemType),
                                  Nv3pStatus_BadParameter);
    }
    else
    {
        MountInfo.FileSystemType = a->FileSystem;
    }
    MountInfo.FileSystemAttr = a->FileSystemAttribute;

    // populate allocation information
    NV_CHECK_ERROR_CLEANUP_3P(ConvertAllocPolicy(a->AllocationPolicy,
                                                 &AllocInfo.AllocPolicy),
                              Nv3pStatus_BadParameter);
    AllocInfo.StartAddress = a->Address;
    AllocInfo.Size = a->Size;
    AllocInfo.PercentReserved = a->PercentReserved;
    AllocInfo.AllocAttribute = a->AllocationAttribute;
#ifdef NV_EMBEDDED_BUILD
    AllocInfo.IsWriteProtected = a->IsWriteProtected;
#endif

    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrAddTableEntry(
                                  a->Id,
                                  a->Name,
                                  PartitionType,
                                  a->PartitionAttribute,
                                  &AllocInfo,
                                  &MountInfo),
                              Nv3pStatus_PartitionCreation);

    // latch info about special partitions
    if (a->Type == Nv3pPartitionType_PartitionTable)
    {
        NvBctAuxInfo *auxInfo;
        NvPartInfo PTInfo;
        NvU32 size = 0;
        NvU32 instance = 0;

        NV_CHECK_ERROR_CLEANUP_3P(
            NvBctGetData(s_pState->BctHandle, NvBctDataType_AuxDataAligned,
                         &size, &instance, NULL),
            Nv3pStatus_InvalidBCT
        );

        customerData = NvOsAlloc(size);

        if (!customerData)
        {
            goto fail;
        }

        NvOsMemset(customerData, 0, size);

        // get the customer data
        NV_CHECK_ERROR_CLEANUP_3P(
            NvBctGetData(s_pState->BctHandle, NvBctDataType_AuxDataAligned,
                         &size, &instance, customerData),
            Nv3pStatus_InvalidBCT
        );

        //Fill out partinfo stuff
        auxInfo = (NvBctAuxInfo*)customerData;

        NV_CHECK_ERROR_CLEANUP_3P(
            NvPartMgrGetPartInfo( a->Id, &PTInfo),
            Nv3pStatus_InvalidPartition
            );

        auxInfo->StartLogicalSector = (NvU16)PTInfo.StartLogicalSectorAddress;
        NV_ASSERT(auxInfo->StartLogicalSector == PTInfo.StartLogicalSectorAddress);
        auxInfo->NumLogicalSectors = (NvU16)PTInfo.NumLogicalSectors;

        s_pState->PtPartitionId = a->Id;

        NV_CHECK_ERROR_CLEANUP_3P(
            NvBctSetData(s_pState->BctHandle, NvBctDataType_AuxDataAligned,
                         &size, &instance, customerData),
            Nv3pStatus_InvalidBCT);
    }
    else if (a->Type == Nv3pPartitionType_Bct)
    {
        NvU8 bctPartId = (NvU8)a->Id;
        NvU32 size, instance = 0;

        size = sizeof(NvU8);
        instance = 0;
        NV_CHECK_ERROR_CLEANUP_3P(
            NvBctSetData(s_pState->BctHandle, NvBctDataType_BctPartitionId,
                         &size, &instance, &bctPartId),
            Nv3pStatus_InvalidBCT);
    }
    else if (a->Type == Nv3pPartitionType_Bootloader)
    {
        NV_CHECK_ERROR_CLEANUP_3P(
            NvBuAddBlPartition(s_pState->BctHandle, a->Id),
            Nv3pStatus_TooManyBootloaders);
    }
    else if (a->Type == Nv3pPartitionType_BootloaderStage2)
    {
        NV_CHECK_ERROR_CLEANUP_3P(
            NvBuAddHashPartition(s_pState->BctHandle, a->Id),
            Nv3pStatus_TooManyHashPartition);
    }
    else
    {
        //Unknown partition type
    }

    s_pState->NumPartitionsRemaining--;

fail:
    NvOsFree(customerData);

    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nCreatePartition failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}


COMMAND_HANDLER(EndPartitionConfiguration)
{
    NvError e;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    // Force Signing of the Partition Table which is
    // necessary & sufficient for its verification.
    NvBool SignTable = NV_TRUE;
    NvBool EncryptTable = NV_FALSE;

    CHECK_VERIFY_ENABLED();

    if (s_pState->IsValidPt)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_PartitionTableRequired);

    if (!s_pState->IsCreatingPartitions)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    if (s_pState->NumPartitionsRemaining)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    if (!s_pState->PtPartitionId)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidPartition);

    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrCreateTableFinish(),
                              Nv3pStatus_PartitionCreation);

    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrSaveTable(s_pState->PtPartitionId,
                                                 SignTable, EncryptTable),
                              Nv3pStatus_MassStorageFailure);

    s_pState->IsValidPt = NV_TRUE;
    s_pState->IsCreatingPartitions = NV_FALSE;
    s_pState->NumPartitionsRemaining = 0; // redundant, but add clarity

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nEndPartitionConfiguration failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(FormatPartition)
{
    NvError e;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    char FileName[NVPARTMGR_PARTITION_NAME_LENGTH];

    Nv3pCmdFormatPartition *a = (Nv3pCmdFormatPartition *)arg;

    // error if PT not available
        NV_CHECK_ERROR_CLEANUP_3P(LoadPartitionTable(NULL),
                                  Nv3pStatus_PartitionTableRequired);
        s_pState->IsValidPt = NV_TRUE;

#ifdef NV_EMBEDDED_BUILD
        {
            NvPartInfo PartInfo;
            NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetPartInfo(a->PartitionId, &PartInfo),
                    Nv3pStatus_InvalidPartition);
            // Protect bct
            if (PartInfo.PartitionType == NvPartMgrPartitionType_Bct) {
                // just skip format
                goto fail;
            }
        }
#endif
    if (s_pState->IsCreatingPartitions)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    if (s_pState->NumPartitionsRemaining)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidState);

    if (!a->PartitionId)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState,
                                  Nv3pStatus_InvalidPartition);

    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetNameById(a->PartitionId, FileName),
                              Nv3pStatus_InvalidPartition);
    // PT partition has data before this call, hence it cannot be formatted
    if (s_pState->PtPartitionId != a->PartitionId)
    {
        NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFormat(FileName),
                              Nv3pStatus_InvalidPartition);
    }
fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nFormatPartition failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(QueryPartition)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    Nv3pCmdQueryPartition *a = (Nv3pCmdQueryPartition *)arg;
    NvPartInfo PartInfo;
    char FileName[NVPARTMGR_PARTITION_NAME_LENGTH];
    NvFileStat FileStat;
    NvDdkBlockDevHandle DevHandle = NULL;
    NvFsMountInfo FsMountInfo;
    NvDdkBlockDevInfo BlockDevInfo;

    CHECK_VERIFY_ENABLED();

    // error if PT not available
    if (!s_pState->IsValidPt)
    {
        NV_CHECK_ERROR_CLEANUP_3P(LoadPartitionTable(NULL),
                                  Nv3pStatus_PartitionTableRequired);
        s_pState->IsValidPt = NV_TRUE;
    }

    // look up id; return error if not found
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetPartInfo(a->Id, &PartInfo),
                              Nv3pStatus_InvalidPartition);

    // open device to query bytes per sector
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetFsInfo(a->Id, &FsMountInfo),
                              Nv3pStatus_InvalidPartition);
    NV_CHECK_ERROR_CLEANUP_3P(NvDdkBlockDevMgrDeviceOpen(
                                  (NvDdkBlockDevMgrDeviceId)FsMountInfo.DeviceId,
                                  FsMountInfo.DeviceInstance,
                                  a->Id,
                                  &DevHandle),
                              Nv3pStatus_MassStorageFailure);

    DevHandle->NvDdkBlockDevGetDeviceInfo(DevHandle, &BlockDevInfo);
    a->Address = PartInfo.StartLogicalSectorAddress * BlockDevInfo.BytesPerSector;

    // get size of file from file system mounted on partition
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetNameById(a->Id, FileName),
                              Nv3pStatus_InvalidPartition);
    NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileQueryStat(FileName, &FileStat),
                              Nv3pStatus_InvalidPartition);
    a->Size = FileStat.Size;
    a->PartType = PartInfo.PartitionType;

fail:
    if (DevHandle)
        DevHandle->NvDdkBlockDevClose(DevHandle);

    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nQueryPartition failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}


COMMAND_HANDLER(RawWritePartition)
{
    NvBootDevType bootDevId;
    NvDdkBlockDevMgrDeviceId blockDevId;
    NvDdkBlockDevHandle hBlockDevHandle;
    NvU32 blockDevInstance;
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    Nv3pCmdRawDeviceAccess *a = (Nv3pCmdRawDeviceAccess *)arg;
    NvU64 BytesRemaining;
    NvDdkBlockDevInfo BlockDevInfo;
    NvU32 StartSector = a->StartSector;

    NV_CHECK_ERROR_CLEANUP(
        GetSecondaryBootDevice(&bootDevId, &blockDevInstance)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvBl3pConvertBootToRmDeviceType(bootDevId, &blockDevId)
    );

    if(a->NoOfSectors <= 0)
    {
        e = NvError_BadParameter;
        goto fail;
    }
//open block device in raw access mode (partitionid=0)
    NV_CHECK_ERROR_CLEANUP(
        NvDdkBlockDevMgrDeviceOpen(blockDevId,
                              blockDevInstance,
                              0,
                              &hBlockDevHandle));
    /// Get the device properties
    (hBlockDevHandle)->NvDdkBlockDevGetDeviceInfo(
                         hBlockDevHandle,
                         &BlockDevInfo);

    //return expected input data size to host
    a->NoOfBytes = (NvU64)a->NoOfSectors * BlockDevInfo.BytesPerSector;
    NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    BytesRemaining = a->NoOfBytes;

    while (BytesRemaining)
    {
        NvU32 BytesToWrite;
        NvDdkBlockDevIoctl_WritePhysicalSectorInputArgs WrPhysicalSectorArgs;
        WrPhysicalSectorArgs.SectorNum = StartSector;
        WrPhysicalSectorArgs.pBuffer = s_pState->pStaging;
        if (BytesRemaining > NV3P_STAGING_SIZE)
            BytesToWrite = NV3P_STAGING_SIZE;
        else
            BytesToWrite = (NvU32)BytesRemaining;

        e = Nv3pDataReceive(hSock, s_pState->pStaging, BytesToWrite,
                              &BytesToWrite, 0);
        if (e)
            goto fail;
        WrPhysicalSectorArgs.NumberOfSectors = BytesToWrite/BlockDevInfo.BytesPerSector;

        //This write mechanism will work only with emmc type block device drivers. It wont work
        //nand block device drivers. Because for nand we have take of bad blocks in case of raw
        //read/write operations
        NV_CHECK_ERROR_CLEANUP(hBlockDevHandle->NvDdkBlockDevIoctl(hBlockDevHandle,
            NvDdkBlockDevIoctlType_WritePhysicalSector,
            sizeof(NvDdkBlockDevIoctl_WritePhysicalSectorInputArgs),
            0, &WrPhysicalSectorArgs, NULL));

        BytesRemaining -= BytesToWrite;
        StartSector += WrPhysicalSectorArgs.NumberOfSectors;
    }

fail:
    if(hBlockDevHandle)
        hBlockDevHandle->NvDdkBlockDevClose(hBlockDevHandle);
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nRawWritePartition failed. NvError %u NvStatus %u\n", e, s);
    }
    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(RawReadPartition)
{
    NvBootDevType bootDevId;
    NvDdkBlockDevMgrDeviceId blockDevId;
    NvDdkBlockDevHandle hBlockDevHandle;
    NvU32 blockDevInstance;
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    Nv3pCmdRawDeviceAccess *a = (Nv3pCmdRawDeviceAccess *)arg;
    NvU64 BytesRemaining;
    NvDdkBlockDevInfo BlockDevInfo;
    NvU32 StartSector = a->StartSector;

    NV_CHECK_ERROR_CLEANUP(
        GetSecondaryBootDevice(&bootDevId, &blockDevInstance)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvBl3pConvertBootToRmDeviceType(bootDevId, &blockDevId)
    );

    if(a->NoOfSectors <= 0)
    {
        e = NvError_BadParameter;
        goto fail;
    }

//open block device in raw access mode (partitionid=0)
    NV_CHECK_ERROR_CLEANUP(
        NvDdkBlockDevMgrDeviceOpen(blockDevId,
                              blockDevInstance,
                              0,
                              &hBlockDevHandle));
    /// Get the device properties
    (hBlockDevHandle)->NvDdkBlockDevGetDeviceInfo(
                         hBlockDevHandle,
                         &BlockDevInfo);

    //return expected input data size to host
    a->NoOfBytes = (NvU64)a->NoOfSectors * BlockDevInfo.BytesPerSector;
    NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    BytesRemaining = a->NoOfBytes;

    while (BytesRemaining)
    {
        NvU32 BytesToRead;
        NvDdkBlockDevIoctl_ReadPhysicalSectorInputArgs RdPhysicalSectorArgs;

        RdPhysicalSectorArgs.SectorNum = StartSector;
        RdPhysicalSectorArgs.pBuffer = s_pState->pStaging;
        if (BytesRemaining > NV3P_STAGING_SIZE)
            BytesToRead = NV3P_STAGING_SIZE;
        else
            BytesToRead = (NvU32)BytesRemaining;

        RdPhysicalSectorArgs.NumberOfSectors = BytesToRead/BlockDevInfo.BytesPerSector;
        //This write mechanism will work only with emmc type block device drivers. It wont work
        //nand block device drivers. Because for nand we have take of bad blocks in case of raw
        //read/write operations
        NV_CHECK_ERROR_CLEANUP(hBlockDevHandle->NvDdkBlockDevIoctl(hBlockDevHandle,
            NvDdkBlockDevIoctlType_ReadPhysicalSector,
            sizeof(NvDdkBlockDevIoctl_ReadPhysicalSectorInputArgs),
            0, &RdPhysicalSectorArgs, NULL));
        e = Nv3pDataSend(hSock, s_pState->pStaging, (NvU32)BytesToRead, 0);
        if (e)
            goto fail;

        BytesRemaining -= BytesToRead;
        StartSector += RdPhysicalSectorArgs.NumberOfSectors;
    }

fail:
    if(hBlockDevHandle)
        hBlockDevHandle->NvDdkBlockDevClose(hBlockDevHandle);
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nRawReadPartition failed. NvError %u NvStatus %u\n", e, s);
    }
    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(ReadPartition)
{
    NvError e = NvSuccess;
    NvError e3p = NvSuccess;

    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    NvBool IsGoodHeader = NV_FALSE;
    NvBool IsTransfer = NV_FALSE;

    Nv3pCmdReadPartition *a = (Nv3pCmdReadPartition *)arg;
    NvPartInfo PartInfo;
    NvFileStat FileStat;

    NvU64 BytesRemaining;

    char FileName[NVPARTMGR_PARTITION_NAME_LENGTH];

    NvStorMgrFileHandle hFile = NULL;

    CHECK_VERIFY_ENABLED();

    // NOTE: treat command like "read file"; later come back and add a
    // "read raw partition call"

    // TODO -- API should return size itself, not take if from caller?

    // verify offset is zero; non-zero offset not supported
    if (a->Offset)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_BadParameter, Nv3pStatus_BadParameter);

    // verify PT is available
    if (!s_pState->IsValidPt)
    {
        NV_CHECK_ERROR_CLEANUP_3P(LoadPartitionTable(NULL),
                                  Nv3pStatus_PartitionTableRequired);
        s_pState->IsValidPt = NV_TRUE;
    }

    // verify requested partition exists
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetPartInfo(a->Id, &PartInfo),
                              Nv3pStatus_InvalidPartition);

    // get file size
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetNameById(a->Id, FileName),
                              Nv3pStatus_InvalidPartition);
    NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileQueryStat(FileName, &FileStat),
                              Nv3pStatus_InvalidPartition);

    a->Length = FileStat.Size;

    // Decreasing bootloader partition (Bootloader or Microboot) size by 16 bytes
    // while reading bootloader partition so as to avoid failure of download command
    // after read command for bootloader partition due to addition of 16
    // bytes padding as a WAR of bootrom bug 378464
    // Decreasing it everytime we read this partition because whenever we read a partition,
    // we read whole parition
    if (PartInfo.PartitionType == NvPartMgrPartitionType_Bootloader)
        a->Length-= NV3P_AES_HASH_BLOCK_LEN;

    // return file size to nv3p client
    IsGoodHeader = NV_TRUE;
    e3p = Nv3pCommandComplete(hSock, command, arg, 0);
    if (e3p)
        goto fail;

    // open file
    NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileOpen(FileName,
                                                NVOS_OPEN_READ,
                                                &hFile),
                              Nv3pStatus_MassStorageFailure);
    BytesRemaining = a->Length;

    while (BytesRemaining)
    {
        NvU32 BytesToRead;
        NvU32 BytesRead = 0;

        if (BytesRemaining > NV3P_STAGING_SIZE)
            BytesToRead = NV3P_STAGING_SIZE;
        else
            BytesToRead = (NvU32)BytesRemaining;

        NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileRead(hFile,
                                                    s_pState->pStaging,
                                                    BytesToRead,
                                                    &BytesRead),
                                  Nv3pStatus_MassStorageFailure);


        e3p = Nv3pDataSend(hSock, s_pState->pStaging, (NvU32)BytesRead, 0);
        if (e3p)
            goto fail;

        IsTransfer = NV_TRUE;

        BytesRemaining -= BytesRead;
    }

    IsTransfer = NV_FALSE;

fail:
    if (e)
        NvOsDebugPrintf("\nReadPartition failed. NvError %u NvStatus %u\n", e, s);
    // dealloc any resources
    if (hFile)
    {
        e = NvStorMgrFileClose(hFile);
        if (e != NvSuccess && s != Nv3pStatus_Ok)
            s = Nv3pStatus_MassStorageFailure;
    }

    // short circuit if a low-level Nv3p protocol error occurred
    NV_CHECK_ERROR(e3p);

    if (!IsGoodHeader)
    {
        //Do something interesting here
    }

    if (IsTransfer)
    {
      //Do something interesting here
    }
    return ReportStatus(hSock, Message, s, 0);
}

NvError
VerifySignature(
    NvU8 *pBuffer,
    NvU32 bytesToSign,
    NvBool StartMsg,
    NvBool EndMsg,
    NvU8 *ExpectedHash)
{
    NvError e = NvSuccess;
    //Need to use a constant instead of a value
    NvU32 paddingSize = NV3P_AES_HASH_BLOCK_LEN -
                            (bytesToSign % NV3P_AES_HASH_BLOCK_LEN);
    NvU8 *bufferToSign = NULL;
    static NvCryptoHashAlgoHandle pAlgo = (NvCryptoHashAlgoHandle)NULL;
    NvBool isValidHash = NV_FALSE;

    bufferToSign = pBuffer;

    if (StartMsg)
    {
        NvCryptoHashAlgoParams Params;
        NV_CHECK_ERROR_CLEANUP(NvCryptoHashSelectAlgorithm(
                                        NvCryptoHashAlgoType_AesCmac,
                                         &pAlgo));

        NV_ASSERT(pAlgo);

        // configure algorithm
        Params.AesCmac.IsCalculate = NV_TRUE;
        Params.AesCmac.KeyType = NvCryptoCipherAlgoAesKeyType_UserSpecified;
        Params.AesCmac.KeySize = NvCryptoCipherAlgoAesKeySize_128Bit;
        NvOsMemset(Params.AesCmac.KeyBytes, 0, sizeof(Params.AesCmac.KeyBytes));
        Params.AesCmac.PaddingType = NvCryptoPaddingType_ExplicitBitPaddingOptional;

        NV_CHECK_ERROR_CLEANUP(pAlgo->SetAlgoParams(pAlgo, &Params));
    }
    else
    {
        NV_ASSERT(pAlgo);
    }

    // Since Padding for the cmac algorithm is going to be decided at runtime,
    // we need to compute padding size on the fly and decide on the padding
    // size accordingly.
    // Since the staging buffer is utilized whose size is always x16 bytes,
    // there's enough room for padding bytes.

    NV_CHECK_ERROR_CLEANUP(pAlgo->QueryPaddingByPayloadSize(pAlgo, bytesToSign,
                                        &paddingSize,
                                        ((NvU8 *)(bufferToSign) + bytesToSign) ));

    bytesToSign += paddingSize;

    // process data
    NV_CHECK_ERROR_CLEANUP(pAlgo->ProcessBlocks(pAlgo, bytesToSign,
                             (void *)(bufferToSign),
                             StartMsg, EndMsg));
    if (EndMsg == NV_TRUE)
    {
        e = pAlgo->VerifyHash(pAlgo,
                                    ExpectedHash,
                                    &isValidHash);
        if (!isValidHash)
        {
            e = NvError_InvalidState;
            NV_CHECK_ERROR_CLEANUP(e);
        }
    }

fail:
    if (EndMsg == NV_TRUE)
    {
        pAlgo->ReleaseAlgorithm(pAlgo);
    }
    if (e)
        NvOsDebugPrintf("\nVerifySignature failed. NvError %u NvStatus %u\n", e, 0);
    return e;
}

NvError
ReadVerifyData(PartitionToVerify *pPartitionToVerify, Nv3pStatus *status)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    NvPartInfo PartInfo;
    NvU64 BytesRemaining;
    char Message[NV3P_STRING_MAX] = {'\0'};
    char FileName[NVPARTMGR_PARTITION_NAME_LENGTH];

    NvStorMgrFileHandle hFile = NULL;
    NvBool StartMsg = NV_TRUE;
    NvBool EndMsg = NV_FALSE;

    BytesRemaining = pPartitionToVerify->PartitionDataSize;
    // verify requested partition exists
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetPartInfo(
                                pPartitionToVerify->PartitionId,
                              &PartInfo), Nv3pStatus_InvalidPartition);

    if (PartInfo.PartitionType == NvPartMgrPartitionType_Bootloader)
    {
        NvBlOperatingMode OpMode;
        OpMode = NvFuseGetOperatingMode();
         NV_CHECK_ERROR_CLEANUP(NvSysUtilBootloaderReadVerify(
         s_pState->pStaging,
         pPartitionToVerify->PartitionId, OpMode, BytesRemaining,
         pPartitionToVerify->PartitionHash, NV3P_STAGING_SIZE));
        return e;
    }
    // get file size
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetNameById(
                              pPartitionToVerify->PartitionId, FileName),
                              Nv3pStatus_InvalidPartition);

    // open file
    NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileOpen(FileName,
                                                NVOS_OPEN_READ,
                                                &hFile),
                              Nv3pStatus_MassStorageFailure);

    while (BytesRemaining)
    {
        NvU32 BytesToRead;
        NvU32 BytesRead = 0;
        // Use the staging buffer to verify partition data--this is questionable
        if (BytesRemaining > NV3P_STAGING_SIZE)
            BytesToRead = NV3P_STAGING_SIZE;
        else
            BytesToRead = (NvU32)BytesRemaining;

        NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileRead(hFile,
                                                    s_pState->pStaging,
                                                    BytesToRead,
                                                    &BytesRead),
                                  Nv3pStatus_MassStorageFailure);

        // Keep computing hash
        EndMsg = (BytesRemaining == BytesRead);

        NV_CHECK_ERROR_CLEANUP_3P(VerifySignature(s_pState->pStaging,
            BytesRead, StartMsg, EndMsg,
            pPartitionToVerify->PartitionHash),
            Nv3pStatus_CryptoFailure);

        StartMsg = NV_FALSE;
        BytesRemaining -= BytesRead;
    }

fail:
    if (e)
        NvOsDebugPrintf("\nReadVerifyData failed. NvError %u NvStatus %u\n", e, s);
    *status=s;
    // dealloc any resources
    if (hFile)
    {
        e = NvStorMgrFileClose(hFile);
    }
    if (s != Nv3pStatus_Ok)
    {
        e = NvError_Nv3pBadReturnData;
        NvOsDebugPrintf("\nReadVerifyData failed. NvError %u NvStatus %u\n", e, s);
    }
    return e;
}

NvError
UpdateBlInfo(Nv3pCmdDownloadPartition *a,
        NvU8 *hash,
        NvU32 padded_length, Nv3pStatus *status)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    NvU32 instance = 0;
    NvU32 size = 0;
    NvU32 sectorsPerBlock;
    NvU32 bytesPerSector;
    NvU32 physicalSector;
    NvU32 logicalSector;
    NvU32 bootromBlockSizeLog2;
    NvU32 bootromPageSizeLog2;
    NvU32 loadAddress;
    NvU32 entryPoint;
    NvU32 version;
    NvU32 startBlock;
    NvU32 startPage;
    NvPartInfo PartInfo;
    NvBlOperatingMode OpMode;
    char Message[NV3P_STRING_MAX] = {'\0'};

    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetPartInfo(a->Id, &PartInfo),
                              Nv3pStatus_InvalidPartition);
    e = NvSysUtilImageInfo( (NvU32)a->Id,
                        &loadAddress,
                        &entryPoint,
                        &version, (NvU32)a->Length );

    // If we can't obtain the bootloader entry point
    // info, its probably an AOS/Quickboot image. We hardcode
    // this info for AOS.
    if (e != NvSuccess)
    {
        if (a->Length > NV_CHARGE_SIZE_MAX)
        {
            loadAddress = NV_AOS_ENTRY_POINT;
            entryPoint = NV_AOS_ENTRY_POINT;
            version = 1;
        }
        // If the bootloader is small enough to fit on IRAM,
        // it's probably NvCharge.
        else
        {
            // Suppress DRAM initialization.
            NvU32 size = sizeof(NvU32);
            NvU32 instance = 0;
            NvU32 zero = 0;
            NV_CHECK_ERROR_CLEANUP_3P(
                NvBctSetData(s_pState->BctHandle,
                             NvBctDataType_NumValidSdramConfigs,
                             &size, &instance, &zero),
                Nv3pStatus_InvalidBCT);
            loadAddress = NV_CHARGE_ENTRY_POINT;
            entryPoint = NV_CHARGE_ENTRY_POINT;
            version = 1;
        }
    }

    // make hash null to avoid updating hash, load and entry address of bl in secure mode
    OpMode = NvFuseGetOperatingMode();
    if (OpMode == NvBlOperatingMode_OdmProductionSecure)
        hash = NULL;

    logicalSector = (NvU32)PartInfo.StartLogicalSectorAddress;

    //Get various device parameters, as part-id=0 as we call logical2physical
    if (NvSysUtilGetDeviceInfo( a->Id, logicalSector, &sectorsPerBlock,
        &bytesPerSector, &physicalSector) == NV_FALSE)
    {
        NV_CHECK_ERROR_CLEANUP_3P(NvError_BadValue,
                                  Nv3pStatus_InvalidPartitionTable);
    }

    //Get the blockSize/PageSize from the BCT

    size = sizeof(NvU32);

    NV_CHECK_ERROR_CLEANUP_3P(
        NvBctGetData(s_pState->BctHandle, NvBctDataType_BootDeviceBlockSizeLog2,
                     &size, &instance, &bootromBlockSizeLog2),
        Nv3pStatus_InvalidBCT
        );

    NV_CHECK_ERROR_CLEANUP_3P(
        NvBctGetData(s_pState->BctHandle, NvBctDataType_BootDevicePageSizeLog2,
                     &size, &instance, &bootromPageSizeLog2),
        Nv3pStatus_InvalidBCT
        );

    // Convert the logicalSectors into a startBlock and StartPage in terms
    // of the page/block size that the bootrom uses (ie, in terms of the log2
    // values stored in the bct)
    startBlock = (physicalSector * bytesPerSector) /
        (1 << bootromBlockSizeLog2);
    startPage = (physicalSector * bytesPerSector) %
        (1 << bootromBlockSizeLog2);
    startPage /= (1 << bootromPageSizeLog2);

    //Set the bootloader in the BCT
    NV_CHECK_ERROR_CLEANUP_3P(
        NvBuUpdateBlInfo(s_pState->BctHandle,
                          a->Id,
                          version,
                          startBlock,
                          startPage,
                          padded_length,
                          loadAddress,
                          entryPoint,
                          NVCRYPTO_CIPHER_AES_BLOCK_BYTES,
                          hash),
        Nv3pStatus_InvalidBCT);

    // Make bootloader bootable
    NV_CHECK_ERROR_CLEANUP_3P(
        NvBuSetBlBootable(s_pState->BctHandle,
                          s_pState->BitHandle,
                          a->Id),
        Nv3pStatus_InvalidBCT);

fail:
    *status = s;
    if (e)
        NvOsDebugPrintf("\nUpdateBlInfo failed. NvError %u NvStatus %u\n", e, s);
    return e;
}

NvError
LocatePartitionToVerify(NvU32 PartitionId, PartitionToVerify **pPartition)
{
    NvError e = NvError_BadParameter;
    PartitionToVerify *pLookupPart;

    *pPartition = NULL;
    pLookupPart = partInfo.LstPartitions;

    while (pLookupPart)
    {
        if (PartitionId == pLookupPart->PartitionId)
        {
            *pPartition = pLookupPart;
            e = NvSuccess;
            break;
        }
        pLookupPart = (PartitionToVerify *)pLookupPart->pNext;
    }

    if (e)
        NvOsDebugPrintf("\nLocatePartitionToVerify failed. NvError %u NvStatus %u\n", e, 0);
    return e;
}

void
InitLstPartitionsToVerify(void)
{
    partInfo.NumPartitions = 0;
    partInfo.LstPartitions = NULL;
}

void
DeInitLstPartitionsToVerify(void)
{
    PartitionToVerify *nextElem = NULL;

    if (partInfo.LstPartitions)
    {
        nextElem = (PartitionToVerify *)partInfo.LstPartitions->pNext;
        while(nextElem)
        {
            nextElem = (PartitionToVerify *)partInfo.LstPartitions->pNext;
            NvOsFree(partInfo.LstPartitions);
            partInfo.LstPartitions = nextElem;
        }
        partInfo.LstPartitions = NULL;
    }
    partInfo.NumPartitions = 0;
}

NvError
SetPartitionToVerify(NvU32 PartitionId, PartitionToVerify **pNode)
{
    NvError e = NvError_Success;
    NvU32 i;
    PartitionToVerify *pListElem;

    pListElem = (PartitionToVerify *)partInfo.LstPartitions;
    if(partInfo.NumPartitions)
    {
        for (i = 0;i< (partInfo.NumPartitions-1); i++)
        {
            pListElem = (PartitionToVerify *)pListElem->pNext;
        }
        pListElem->pNext = NvOsAlloc(sizeof(PartitionToVerify));
        /// If there's no more memory for storing the partition Ids
        /// return an error.
        if (!pListElem->pNext)
        {
                e = NvError_InsufficientMemory;
                goto fail;
        }
        ((PartitionToVerify *)(pListElem->pNext))->PartitionId = PartitionId;
        ((PartitionToVerify *)(pListElem->pNext))->PartitionDataSize = 0;
        ((PartitionToVerify *)(pListElem->pNext))->pNext = NULL;
        *pNode = (PartitionToVerify *)pListElem->pNext;
    }
    else
    {
        partInfo.LstPartitions = NvOsAlloc(sizeof(PartitionToVerify));
        /// If there's no more memory for storing the partition Ids
        /// return an error.
        if (!partInfo.LstPartitions)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }
        partInfo.LstPartitions->PartitionId = PartitionId;
        partInfo.LstPartitions->PartitionDataSize = 0;
        partInfo.LstPartitions->pNext = NULL;
        *pNode = (PartitionToVerify *)partInfo.LstPartitions;
    }

    partInfo.NumPartitions++;

fail:
    if (e)
        NvOsDebugPrintf("\nSetPartitionToVerify failed. NvError %u NvStatus %u\n", e, 0);
    return e;
}

COMMAND_HANDLER(DownloadNvPrivData)
{
    NvError e = NvSuccess;
    NvU32 BytesReceived;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};

    Nv3pCmdNvPrivData *a = (Nv3pCmdNvPrivData *)arg;
    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete(hSock, command, arg, 0)
    );

    NvPrivDataBufferSize = a->Length;
    NvPrivDataBuffer = NvOsAlloc(a->Length);
    if (NvPrivDataBuffer == NULL)
        goto clean;
    NvOsMemset(NvPrivDataBuffer, 0, NvPrivDataBufferSize);
    NV_CHECK_ERROR_CLEANUP(
        Nv3pDataReceive(hSock, NvPrivDataBuffer,
            NvPrivDataBufferSize, &BytesReceived, 0)
    );

fail:
    if (e)
       Nv3pNack(hSock, Nv3pNackCode_BadData);
clean:
    return ReportStatus(hSock, Message, s, 0);
}


COMMAND_HANDLER(DownloadPartition)
{
    NvError e = NvSuccess;
    NvError e3p = NvSuccess;

    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    NvBool IsGoodHeader = NV_FALSE;
    NvBool IsTransfer = NV_FALSE;
    NvBool IsLastBuffer = NV_FALSE;
    NvBool LookForSparseHeader = NV_TRUE;

    Nv3pCmdDownloadPartition *a = (Nv3pCmdDownloadPartition *)arg;
    NvPartInfo PartInfo;
    NvPartitionStat PartitionStat;
    NvU64 BytesRemaining;
    NvU32 PaddedLength = 0;
    char FileName[NVPARTMGR_PARTITION_NAME_LENGTH];
    NvError partitionLocated = NvError_NotInitialized;
    NvStorMgrFileHandle hFile = NULL;
    PartitionToVerify *partitionToLocate = NULL;
    PartitionToVerify BlPartitionInfo;
    NvBool SparseImage = NV_FALSE;
    NvBool IsSparseStart = NV_FALSE;
    // verify PT is available
    if (!s_pState->IsValidPt)
    {
        NV_CHECK_ERROR_CLEANUP_3P(LoadPartitionTable(NULL),
                                  Nv3pStatus_PartitionTableRequired);
        s_pState->IsValidPt = NV_TRUE;
    }

    // verify requested partition exists
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetPartInfo(a->Id, &PartInfo),
                              Nv3pStatus_InvalidPartition);
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetNameById(a->Id, FileName),
                              Nv3pStatus_InvalidPartition);

    // verify partition is large enough to hold data
    NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrPartitionQueryStat(FileName,
                                                          &PartitionStat),
                              Nv3pStatus_InvalidPartition);

    if (a->Length > PartitionStat.PartitionSize)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_BadParameter, Nv3pStatus_BadParameter);

    if (PartInfo.PartitionType == NvPartMgrPartitionType_Bct ||
        PartInfo.PartitionType == NvPartMgrPartitionType_PartitionTable)
    {
        //Downloading a BCT to a partition makes no sense.
        //This functionality is covered by the DownloadBct
        //commmand already.
        Nv3pNack(hSock, Nv3pNackCode_BadCommand);
    }
    else
    {
        NvBool StartMsg = NV_TRUE;
        NvBool EndMsg = NV_FALSE;
        NvBool SignRequired = NV_FALSE;
        NvBlOperatingMode OpMode;
        NvBlOperatingMode* pOpMode = NULL;
        NvBool DownloadingBl = NV_FALSE;
        NvU8 *HashPtr = NULL;

        // Get current operating mode
        OpMode = NvFuseGetOperatingMode();
        if (OpMode == NvBlOperatingMode_Undefined)
            NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidState);

        if(PartInfo.PartitionType == NvPartMgrPartitionType_Bootloader ||
            PartInfo.PartitionType == NvPartMgrPartitionType_BootloaderStage2)
        {
            DownloadingBl = NV_TRUE;
            pOpMode = &OpMode;
        }


        NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileOpen(FileName,
                                                    NVOS_OPEN_WRITE,
                                                    &hFile),
                                  Nv3pStatus_MassStorageFailure);

        // ack the command now that we know we can complete the requested
        // operation, barring a mass storage error

        // NOTE! Nv3p protocol requires that after this command is ack'ed, at least one
        // call to Nv3pDataReceive() must be attempted.  Only after this requirement has
        // been met can Nv3pTransferFail() be called to terminate the transfer in case
        // of an error.

        IsGoodHeader = NV_TRUE;
        e3p = Nv3pCommandComplete(hSock, command, arg, 0);
        if (e3p)
            goto fail;

        BytesRemaining = a->Length;

        // If verification is required, store hash information for this partition
        if(DownloadingBl)
        {
            // If BootLoader is to be verified, set its partition for
            // verification as for other partitions.
            if (gs_VerifyPartition)
            {
                partitionLocated = SetPartitionToVerify(a->Id, &partitionToLocate);
                gs_VerifyPartition = NV_FALSE;
            }
            // If BootLoader is not set for verification
            else
                partitionToLocate = &BlPartitionInfo;
        }
        else if (gs_VerifyPartition)
        {
            partitionLocated = SetPartitionToVerify(a->Id, &partitionToLocate);
            gs_VerifyPartition = NV_FALSE;
        }
        if ((partitionLocated == NvSuccess) || DownloadingBl)
        {
            NvOsMemset(&(partitionToLocate->PartitionHash[0]), 0,
                                NV3P_AES_HASH_BLOCK_LEN);
        }
        NvOsDebugPrintf("\nStart Downloading %s\n", FileName);
        while (BytesRemaining)
        {
            NvU32 BytesToWrite;
            NvU32 BytesToReceive = 0;

            if (BytesRemaining > NV3P_STAGING_SIZE)
                BytesToReceive = NV3P_STAGING_SIZE;
            else
                BytesToReceive = (NvU32)BytesRemaining;

            e3p = Nv3pDataReceive(hSock, s_pState->pStaging, BytesToReceive,
                                  &BytesToWrite, 0);
            if (e3p)
                goto fail;

            IsTransfer = NV_TRUE;
            // If verification is required,check if it is sparsed image or not
            if (LookForSparseHeader)
            {
                if(NvSysUtilCheckSparseImage(s_pState->pStaging, BytesToWrite))
                {
                    SparseImage = NV_TRUE;
                    IsSparseStart = NV_TRUE;
                }
                LookForSparseHeader = NV_FALSE;
            }

            //Compute hash over the data here on the staging buffer.
            // End of data received when BytesRemaining = BytesToWrite
            if ((partitionLocated == NvSuccess) || DownloadingBl)
            {
                EndMsg = (BytesRemaining == BytesToWrite);
                if(DownloadingBl && EndMsg)
                {
                    //we just got last buffer, see if the buffer isalligned to AES_BLOCK length
                    PaddedLength = (NvU32)a->Length;
                    if ((PaddedLength % NV3P_AES_HASH_BLOCK_LEN) != 0)
                    {
                        NvOsDebugPrintf("size not aligned to AES block length\n");
                        e = NvError_InvalidSize;
                        s = Nv3pStatus_InvalidBCTSize;
                        goto fail;
                    }
                    BytesRemaining = BytesToWrite;
                }
                // If the Image is not sparsed image,compute the hash on the input buffer,else do
                // do it in NvSysUtilUnSparse
                if (!SparseImage)
                {
                    NV_CHECK_ERROR_CLEANUP(
                        NvSysUtilSignData(
                            s_pState->pStaging,
                            BytesToWrite,
                            StartMsg,
                            EndMsg,
                            &(partitionToLocate->PartitionHash[0]),
                            pOpMode)
                    );
                    StartMsg = NV_FALSE;
                }
                else
                    SignRequired = NV_TRUE;
            }

            if(!SparseImage)
            {
                while (BytesToWrite)
                {
                    NvU32 BytesWritten = 0;

                    NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileWrite(hFile,
                                                                 s_pState->pStaging,
                                                                 BytesToWrite,
                                                                 &BytesWritten),
                                              Nv3pStatus_MassStorageFailure);

                    BytesToWrite -= BytesWritten;
                    BytesRemaining -= BytesWritten;
                }
            }
            else
            {
                if(!(BytesRemaining - BytesToWrite))
                    IsLastBuffer = NV_TRUE;
                if (SignRequired)
                    HashPtr = &(partitionToLocate->PartitionHash[0]);

                    NV_CHECK_ERROR_CLEANUP_3P(
                        NvSysUtilUnSparse(
                            hFile,
                            s_pState->pStaging,
                            BytesToWrite,
                            IsSparseStart,
                            IsLastBuffer,
                            SignRequired,
                            HashPtr,
                            pOpMode),
                        Nv3pStatus_UnsparseFailure);

                BytesRemaining  -= BytesToWrite;
                IsSparseStart = NV_FALSE;
            }
        }
        if ((partitionLocated == NvSuccess) || DownloadingBl)
        {
            partitionToLocate->PartitionDataSize = a->Length;
        }
        if (DownloadingBl)
        {
            if (hFile)
            {
                NV_CHECK_ERROR_CLEANUP_3P(NvStorMgrFileClose(hFile),
                                             Nv3pStatus_MassStorageFailure);
            }
            hFile = 0;
            if(PartInfo.PartitionType == NvPartMgrPartitionType_Bootloader) // Stage1 bootloader
            {
                UpdateBlInfo(a,
                    &(partitionToLocate->PartitionHash[0]),
                    PaddedLength, &s);
            }
            else if(PartInfo.PartitionType == NvPartMgrPartitionType_BootloaderStage2) // Stage2 bootloader
            {
                NV_CHECK_ERROR_CLEANUP_3P(
                    NvBuUpdateHashPartition(s_pState->BctHandle,
                        a->Id,
                        sizeof(BlPartitionInfo.PartitionHash),
                        &BlPartitionInfo.PartitionHash[0]
                        ),
                    Nv3pStatus_InvalidBCT);
            }
        }
        IsTransfer = NV_FALSE;
        NvOsDebugPrintf("\nEnd Downloading %s\n", FileName);
    }
fail:
    if (e)
        NvOsDebugPrintf("\nDownloadPartition failed. NvError %u NvStatus %u\n", e, s);
    // dealloc any resources
    if (hFile)
    {
        // In case of Sparsed image ,where the verification is required,we need to calculate
        // the unsparsed image size here by knowing the current position of handle before close
        // so that this value can be used in verify data
#if NVODM_BOARD_IS_SIMULATION == 0
        //verifypart not supported in simulation mode.returns error if used.
        if ((partitionLocated == NvSuccess) && SparseImage)
        {
            NvU64 Offset = 0;
            e = NvStorMgrFtell(hFile, &Offset);
            if (e != NvSuccess)
                s = Nv3pStatus_MassStorageFailure;
            partitionToLocate->PartitionDataSize = Offset;
        }
#endif
        e = NvStorMgrFileClose(hFile);
        if (e != NvSuccess && s != Nv3pStatus_Ok)
            s = Nv3pStatus_MassStorageFailure;
    }

    // short circuit if a low-level Nv3p protocol error occurred
    NV_CHECK_ERROR(e3p);

    if (!IsGoodHeader)
    {
        //Do something interesting here
    }

    if (IsTransfer)
    {
        //Do something interesting here
    }

    return ReportStatus(hSock, Message, s, 0);
}


COMMAND_HANDLER(SetBootPartition)
{
    NvError e;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    Nv3pCmdSetBootPartition *a = (Nv3pCmdSetBootPartition *)arg;
    NvPartInfo PartInfo;

    CHECK_VERIFY_ENABLED();

    // error if PT not available
    if (!s_pState->IsValidPt)
    {
        NV_CHECK_ERROR_CLEANUP_3P(LoadPartitionTable(NULL),
                                  Nv3pStatus_PartitionTableRequired);
        s_pState->IsValidPt = NV_TRUE;
    }

    // look up id; return error if not found
    NV_CHECK_ERROR_CLEANUP_3P(NvPartMgrGetPartInfo(a->Id, &PartInfo),
                              Nv3pStatus_InvalidPartition);

    // verify that partition contains a bootloader
    if (PartInfo.PartitionType != NvPartMgrPartitionType_Bootloader)
        NV_CHECK_ERROR_CLEANUP_3P(NvError_BadParameter,
                                  Nv3pStatus_InvalidPartition);

    // set partition as bootable
    NV_CHECK_ERROR_CLEANUP_3P(
        NvBuSetBlBootable(s_pState->BctHandle,
                          s_pState->BitHandle,
                          a->Id),
        Nv3pStatus_InvalidBCT);

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nSetBootPartition failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}

COMMAND_HANDLER(OdmOptions)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    Nv3pCmdOdmOptions *a = (Nv3pCmdOdmOptions *)arg;

    NvU32 size = 0, instance = 0;

    CHECK_VERIFY_ENABLED();

    //Fill in customer opt
    size = sizeof(NvU32);

    NV_CHECK_ERROR_CLEANUP_3P(
        NvBctSetData(s_pState->BctHandle, NvBctDataType_OdmOption,
                     &size, &instance, &a->Options),
        Nv3pStatus_InvalidBCT);

fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nOdmOptions failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

    return ReportStatus(hSock, Message, s, 0);
}

#if NVODM_BOARD_IS_SIMULATION==0
NvError
FuelGaugeFwUpgrade(
    Nv3pSocketHandle hSock,
    Nv3pCommand command,
    void *arg)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    Nv3pCmdOdmCommand *b = (Nv3pCmdOdmCommand *)arg;
    Nv3pOdmExtCmdFuelGaugeFwUpgrade *a = (Nv3pOdmExtCmdFuelGaugeFwUpgrade *)
                                   &b->odmExtCmdParam.fuelGaugeFwUpgrade;
    struct NvOdmFFUBuff *pHeadBuff1 = NULL;
    struct NvOdmFFUBuff *pHeadBuff2 = NULL;
    struct NvOdmFFUBuff *pTempBuff = NULL;
    NvU64 BytesRemaining = 0;
    NvU32 BytesWritten = 0;
    NvU32 count = 0;
    NvU32 BytesToReceive;
    NvU32 NoOfFiles = 1;
    NvU64 max_filelen2 = 0;

    e = Nv3pCommandComplete(hSock, command, arg, 0);
    if (e)
        goto fail;

    max_filelen2 = a->FileLength2;
    BytesRemaining = a->FileLength1;

    if (max_filelen2)
        NoOfFiles = 2;
    // Creating the First Buffer node
    pHeadBuff1 = (struct NvOdmFFUBuff *)NvOsAlloc(sizeof(struct NvOdmFFUBuff));
    if (pHeadBuff1 == NULL)
    {
        NvOsDebugPrintf("\nError in memory allocation\n");
        e =  NvError_InsufficientMemory;
        goto fail;
    }
    pHeadBuff1->pNext = NULL;

    if (max_filelen2)
    {
        // Creating the First Buffer node for Buffer2
        pHeadBuff2 = (struct NvOdmFFUBuff *)NvOsAlloc(sizeof(struct NvOdmFFUBuff));
        if (pHeadBuff2 == NULL)
        {
            NvOsDebugPrintf("\nError in memory allocation\n");
            e =  NvError_InsufficientMemory;
            goto fail;
        }
        pHeadBuff2->pNext = NULL;
    }

    pTempBuff = pHeadBuff1;
    do
    {
        count++;
        do
        {
            // Deciding the Bytes to receive
            if (BytesRemaining > NvOdmFFU_BUFF_LEN)
                BytesToReceive = NvOdmFFU_BUFF_LEN;
            else
                BytesToReceive = (NvU32)BytesRemaining;

            // Upgradation file reception
            e = Nv3pDataReceive(hSock, (void *)pTempBuff->data, BytesToReceive,
                                  &BytesWritten, 0);
            if (e)
                goto fail;

            pTempBuff->data_len = BytesWritten;

            BytesRemaining -= BytesWritten;

            if (BytesRemaining)
            {
                /*
                 * If the file is not received completely create
                 * the next buffer node
                 */
                pTempBuff->pNext = (struct NvOdmFFUBuff *)
                                    NvOsAlloc(sizeof(struct NvOdmFFUBuff));
                if (pTempBuff->pNext == NULL)
                {
                    NvOsDebugPrintf("\nError in memory allocation\n");
                    e = NvError_InsufficientMemory;
                    goto fail;
                }
                pTempBuff = pTempBuff->pNext;
                pTempBuff->pNext = NULL;
            }
        } while (BytesRemaining);

        /*
         * Condiion will pass if two files are passed
         * and only one file is received
         */
        if (count < NoOfFiles)
        {
            pTempBuff = pHeadBuff2;
            BytesRemaining = max_filelen2;
        }

    } while (BytesRemaining);

    e = NvOdmFFUMain(pHeadBuff1, pHeadBuff2) ;
    if (!e)
        return ReportStatus(hSock, "", s, 0);

fail:
    s = Nv3pStatus_FuelGaugeFwUpgradeFailure;
    if (e)
        NvOsDebugPrintf("\nFuelGaugeFwUpgrade failed. NvError %u NvStatus %u\n", e, s);
    return ReportStatus(hSock, "", s, 0);
}
#endif

COMMAND_HANDLER(OdmCommand)
{
    NvError e = NvError_NotInitialized;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    Nv3pCmdOdmCommand *a = (Nv3pCmdOdmCommand *)arg;

#if NVODM_BOARD_IS_SIMULATION==0
    CHECK_VERIFY_ENABLED();

    switch ( a->odmExtCmd )
    {
        case Nv3pOdmExtCmd_FuelGaugeFwUpgrade:
            return  FuelGaugeFwUpgrade(hSock, command, arg);
        case Nv3pOdmExtCmd_RunSdDiag:
            e = SdDiagInit(a->odmExtCmdParam.sdDiag.Value);
            if(e == NvSuccess)
                e = SdDiagTest(a->odmExtCmdParam.sdDiag.TestType);
            SdDiagClose();
            break;
        case Nv3pOdmExtCmd_RunSeDiag:
            e = SeAesVerifyTest(a->odmExtCmdParam.seDiag.Value);
            break;
        case Nv3pOdmExtCmd_VerifySdram:
            NvOsDebugPrintf("sdram validation can not be done at bootloader level\n");
            e = NvError_NotImplemented;
            break;
        case Nv3pOdmExtCmd_RunPwmDiag:
            e = NvPwmTest();
            break;
        case Nv3pOdmExtCmd_RunDsiDiag:
            e = DsiBasicTest();
            break;
        default:
            NV_CHECK_ERROR_CLEANUP_3P(
                NvError_NotImplemented,
                Nv3pStatus_NotImplemented
            );
            break;
    }
fail:
    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nOdmCommand failed. NvError %u NvStatus %u\n", e, s);
    }
    else
        NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));
#endif
    return ReportStatus(hSock, Message, s, 0);
}

NvError
LoadPartitionTable(Nv3pPartitionTableLayout *arg)
{
    NvError e = NvSuccess;
    NvU8 *customerData = NULL;
    NvU32 size = 0, instance = 0;
    NvBctAuxInfo *auxInfo;
    NvU32 startLogicalSector, numLogicalSectors;
    NvU32 blockDevInstance;
    NvBootDevType bootDevId;
    NvDdkBlockDevMgrDeviceId blockDevId;

    NV_CHECK_ERROR_CLEANUP(
        GetSecondaryBootDevice(&bootDevId, &blockDevInstance)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvBl3pConvertBootToRmDeviceType(bootDevId, &blockDevId)
    );

    // When ReadPartitionTable command is called, but not from --skip.
    if (!arg || (!arg->NumLogicalSectors && !arg->StartLogicalAddress))
    {
        NV_CHECK_ERROR_CLEANUP(
            NvBctGetData(s_pState->BctHandle, NvBctDataType_AuxDataAligned,
                         &size, &instance, NULL)
        );

        customerData = NvOsAlloc(size);

        if (!customerData)
        {
            e = NvError_InsufficientMemory;
            goto fail;
        }

        NvOsMemset(customerData, 0, size);
        // get the customer data
        NV_CHECK_ERROR_CLEANUP(
            NvBctGetData(s_pState->BctHandle, NvBctDataType_AuxDataAligned,
                         &size, &instance, customerData)
        );
        auxInfo = (NvBctAuxInfo*)customerData;
        startLogicalSector = auxInfo->StartLogicalSector;
        numLogicalSectors = auxInfo->NumLogicalSectors;
    }
    // This handles ReadPartitionTable command called from --skip &
    // values(StartLogicalSector & NumLogicalSectors) of PT partition
    // are passed as arguments.
    else
    {
        startLogicalSector = arg->StartLogicalAddress;
        numLogicalSectors = arg->NumLogicalSectors;
    }

    //FIXME: Should probably look at the opmode to
    //decide whether the table is encrypted or not.
    NV_CHECK_ERROR_CLEANUP(
        NvPartMgrLoadTable(blockDevId, blockDevInstance, startLogicalSector,
                           numLogicalSectors, NV_TRUE, NV_FALSE)
    );

fail:
    if (customerData)
        NvOsFree(customerData);
    if (e)
        NvOsDebugPrintf("\nLoadPartitionTable failed. NvError %u NvStatus %u\n", e, 0);
    return e;
}

void
UnLoadPartitionTable(void)
{
    if (s_pState->IsValidPt)
    {
        s_pState->IsValidPt = NV_FALSE;
        NvPartMgrUnLoadTable();
    }
}

/* this may be called with hSock=NULL (during a Go command) */
COMMAND_HANDLER(Sync)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    NvU32 size, instance = 0, numBootloaders;
    NvDdkBlockDevHandle DevHandle = NULL;
    NvU32 BctLength = 0;
    NvU8* buffer = 0;
    NvBlOperatingMode OpMode;
    NvU8 bctPartId;
    char Message[NV3P_STRING_MAX] = {'\0'};
#if TEST_RECOVERY
    NvU8 *tempPtr = NULL;
#endif
    NvFsMountInfo minfo;

    CHECK_VERIFY_ENABLED();

    size = sizeof(NvU8);
    NV_CHECK_ERROR_CLEANUP_3P(
        NvBctGetData(s_pState->BctHandle, NvBctDataType_BctSize,
                 &size, &instance, &BctLength),
        Nv3pStatus_InvalidBCTSize);

    buffer = NvOsAlloc(BctLength);
    if(!buffer)
        goto fail;
    size = sizeof(NvU8);
    NV_CHECK_ERROR_CLEANUP_3P(
        NvBctGetData(s_pState->BctHandle, NvBctDataType_BctPartitionId,
                 &size, &instance, &bctPartId),
        Nv3pStatus_InvalidBCTPartitionId);

    NV_CHECK_ERROR_CLEANUP_3P(LoadPartitionTable(NULL),
        Nv3pStatus_InvalidPartitionTable);

    if (bctPartId)
    {
        size = sizeof(NvU32);
        NV_CHECK_ERROR_CLEANUP_3P(
            NvBctGetData(s_pState->BctHandle, NvBctDataType_NumEnabledBootLoaders,
                     &size, &instance, &numBootloaders),
            Nv3pStatus_InvalidBCT);

        if (!numBootloaders)
            NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_NoBootloader);

        NV_CHECK_ERROR_CLEANUP_3P(
            NvBuBctCreateBadBlockTable( s_pState->BctHandle, bctPartId),
            Nv3pStatus_ErrorBBT);
#ifdef NV_EMBEDDED_BUILD
        {
            NvU8 EnableFailBack = 1;
            NV_CHECK_ERROR_CLEANUP_3P(
                NvBctSetData(s_pState->BctHandle, NvBctDataType_EnableFailback,
                    &size, &instance, &EnableFailBack),
                Nv3pStatus_InvalidBCT
            );
        }

        NV_CHECK_ERROR_CLEANUP_3P(
            NvProtectBctInvariant(s_pState->BctHandle,
                s_pState->BitHandle, bctPartId, NvPrivDataBuffer,
                NvPrivDataBufferSize),
                Nv3pStatus_BctInvariant
        );
        NvOsFree(NvPrivDataBuffer);
        NvPrivDataBufferSize = 0;
#endif
        size = BctLength;

        // Get current operating mode
        OpMode = NvFuseGetOperatingMode();

        if (OpMode == NvBlOperatingMode_Undefined)
            NV_CHECK_ERROR_CLEANUP_3P(NvError_InvalidState, Nv3pStatus_InvalidState);

        //Sign/Write the BCT here
        NV_CHECK_ERROR_CLEANUP_3P(
            NvBuBctCryptoOps(s_pState->BctHandle, OpMode, &size, buffer,
                NvBuBlCryptoOp_EncryptAndSign), Nv3pStatus_CryptoFailure);

#if TEST_RECOVERY
        //FIXME: Remove explicit BCT corruption before release
        //Don't corrupt the BCT hash, don't do an explicit recovery
        tempPtr = buffer;
        tempPtr[0]++;
#endif
        size = BctLength;

        NV_CHECK_ERROR_CLEANUP_3P(
            NvBuBctUpdate(s_pState->BctHandle, s_pState->BitHandle, bctPartId, size, buffer),
            Nv3pStatus_BctWriteFailure);

#if TEST_RECOVERY
        NV_CHECK_ERROR_CLEANUP_3P(
            NvBuBctRecover(s_pState->BctHandle, s_pState->BitHandle, bctPartId),
            Nv3pStatus_MassStorageFailure);
#endif

        NV_CHECK_ERROR_CLEANUP_3P(
            NvBuBctReadVerify(s_pState->BctHandle, s_pState->BitHandle, bctPartId, OpMode, size),
            Nv3pStatus_BctReadVerifyFailure);
    }
    // sanity check
    if (s_pState->IsValidDeviceInfo)
    {
        // Alternate way to get the device Id for region table verification
        // Assuming that device for PT partition has the region table
        NV_CHECK_ERROR_CLEANUP(
            NvPartMgrGetFsInfo(s_pState->PtPartitionId, &minfo )
        );

        // verify region table
        NV_CHECK_ERROR_CLEANUP_3P(NvDdkBlockDevMgrDeviceOpen(
            (NvDdkBlockDevMgrDeviceId)minfo.DeviceId, minfo.DeviceInstance,
                              0,  /* MinorInstance */
                              &DevHandle),
                              Nv3pStatus_MassStorageFailure);
        NV_CHECK_ERROR_CLEANUP_3P(DevHandle->NvDdkBlockDevIoctl(
                              DevHandle,
                              NvDdkBlockDevIoctlType_VerifyCriticalPartitions,
                              0,
                              0,
                              0,
                              NULL),
                              Nv3pStatus_MassStorageFailure);
    }
fail:
    if(buffer)
        NvOsFree(buffer);
    if (e)
        NvOsDebugPrintf("\nSync failed. NvError %u NvStatus %u\n", e, s);
    if( hSock )
    {
        if( e )
            Nv3pNack(hSock, Nv3pNackCode_BadData);
        else
            NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

        return ReportStatus(hSock, Message, s, 0);
    }
    else
    {
        return e;
    }
}

COMMAND_HANDLER(VerifyPartitionEnable)
{

    NvError e = NvError_InvalidState;
    Nv3pStatus s = Nv3pStatus_Ok;

    // If verification for a partition has not been enabled,
    // enable it else return error.
    // For the first request for verification, mark verification
    // as pending.
    if (!gs_VerifyPartition)
    {
        gs_VerifyPartition= NV_TRUE;
        gs_VerifyPending = NV_TRUE;
        e = NvSuccess;
    }

    if( hSock )
    {
        if( e )
            Nv3pNack(hSock, Nv3pNackCode_BadData);
        else
            NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

        return ReportStatus(hSock, "", s, 0);
    }
    else
    {
        return e;
    }

}

COMMAND_HANDLER(VerifyPartition)
{
    NvError e = NvError_InvalidState;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};

    PartitionToVerify *pPartitionToVerify;
    Nv3pCmdVerifyPartition *a = (Nv3pCmdVerifyPartition *)arg;

    CHECK_VERIFY_ENABLED();

    // check if partition was marked for verification
    NV_CHECK_ERROR_CLEANUP(
        LocatePartitionToVerify(a->Id, &pPartitionToVerify)
    );

    // read and verify partition data
    NV_CHECK_ERROR_CLEANUP(
        ReadVerifyData(pPartitionToVerify, &s)
    );

fail:
    if (e)
        NvOsDebugPrintf("\nVerifyPartition failed. NvError %u NvStatus %u\n", e, s);
    if( hSock )
    {
        if( e )
            Nv3pNack(hSock, Nv3pNackCode_BadData);
        else
            NV_CHECK_ERROR(Nv3pCommandComplete(hSock, command, arg, 0));

        return ReportStatus(hSock, "", s, 0);
    }
    else
    {
        return e;
    }
}

COMMAND_HANDLER(GetDevInfo)
{
    NvError e = NvSuccess;
    Nv3pStatus s = Nv3pStatus_Ok;
    char Message[NV3P_STRING_MAX] = {'\0'};
    Nv3pCmdGetDevInfo a;
    NvDdkBlockDevHandle BlockDevHandle = NULL;
    NvDdkBlockDevInfo BlockDevInfo;
    NvBootDevType BootDevId;
    NvDdkBlockDevMgrDeviceId BlockDevId;
    NvU32 BlockDevInstance;

    CHECK_VERIFY_ENABLED();

    NV_CHECK_ERROR_CLEANUP(
        GetSecondaryBootDevice(&BootDevId, &BlockDevInstance)
    );

    NV_CHECK_ERROR_CLEANUP(
        NvBl3pConvertBootToRmDeviceType(BootDevId, &BlockDevId)
    );

    //open block device in raw access mode (partitionid=0)
    NV_CHECK_ERROR_CLEANUP_3P(NvDdkBlockDevMgrDeviceOpen(
                              BlockDevId,
                              BlockDevInstance,
                              0,
                              &BlockDevHandle),
                              Nv3pStatus_MassStorageFailure);

    // Get the device properties
    (BlockDevHandle)->NvDdkBlockDevGetDeviceInfo(
                        BlockDevHandle, &BlockDevInfo);

    a.BytesPerSector = BlockDevInfo.BytesPerSector;
    a.SectorsPerBlock = BlockDevInfo.SectorsPerBlock;
    a.TotalBlocks = BlockDevInfo.TotalBlocks;

    NV_CHECK_ERROR_CLEANUP(
        Nv3pCommandComplete(hSock, command, &a, 0)
    );

fail:
    if (BlockDevHandle)
        BlockDevHandle->NvDdkBlockDevClose(BlockDevHandle);

    if (e)
    {
        Nv3pNack(hSock, Nv3pNackCode_BadData);
        NvOsDebugPrintf("\nGetDevInfo failed. NvError %u NvStatus %u\n", e, s);
    }

    return ReportStatus(hSock, "", s, 0);
}

NvError
AllocateState()
{
    NvError e;
    Nv3pServerState *p = NULL;
    NvU32 BctLength = 0;
#if NVODM_BOARD_IS_SIMULATION
    NvU8 *BctBuffer = 0;
#endif
    p = (Nv3pServerState *)NvOsAlloc(sizeof(Nv3pServerState));

    if (!p)
        return NvError_InsufficientMemory;

    p->pStaging = NvOsAlloc(NV3P_STAGING_SIZE);

    if (!p->pStaging)
    {
        e = NvError_InsufficientMemory;
        goto fail;
    }

    p->IsValidDeviceInfo = NV_FALSE;
    p->IsCreatingPartitions = NV_FALSE;
    p->NumPartitionsRemaining = 0;
    p->PtPartitionId = 0;

    p->IsLocalBct = NV_FALSE;
    p->IsLocalPt = NV_FALSE;
    p->IsValidPt = NV_FALSE;

    p->BctHandle = NULL;
    p->BitHandle = NULL;
#if NVODM_BOARD_IS_SIMULATION
    NV_CHECK_ERROR_CLEANUP(NvBctInit(&BctLength, NULL, NULL));
    NV_CHECK_ERROR_CLEANUP(NvPartMgrInit());
    NV_CHECK_ERROR_CLEANUP(NvDdkBlockDevMgrInit());
    BctBuffer = NvOsAlloc(BctLength);
    NV_CHECK_ERROR_CLEANUP(NvBctInit(&BctLength, BctBuffer, &p->BctHandle));
#else
    NV_CHECK_ERROR_CLEANUP(NvBctInit(&BctLength, NULL, &p->BctHandle));
    NV_CHECK_ERROR_CLEANUP(NvBctInit(&BctLength, NULL, &p->BctHandle));
#endif
    NV_CHECK_ERROR_CLEANUP(NvBitInit(&p->BitHandle));

    s_pState = p;

    return NvSuccess;

fail:
    NvOsFree(p);
    if (e)
        NvOsDebugPrintf("\nAllocateState failed. NvError %u NvStatus %u\n", e, 0);
    return e;
}

void
DeallocateState(void)
{
    if (s_pState)
    {
        NvBctDeinit(s_pState->BctHandle);
        NvBitDeinit(s_pState->BitHandle);
        NvOsFree(s_pState->pStaging);
        NvOsFree(s_pState);
        s_pState = NULL;
        if(gs_UpdateBct)
            NvOsFree(gs_UpdateBct);
    }
}

static NvError
GetSecondaryBootDevice(
    NvBootDevType *bootDevId,
    NvU32         *bootDevInstance)
{
    NvBitHandle   hBit = NULL;
    NvU32         Size = sizeof(NvBootDevType);
    NvError       e;
    *bootDevInstance = 0;
    NvFuseGetSecondaryBootDevice((NvU32*)bootDevId, bootDevInstance);

    if (*bootDevId != NvBootDevType_None)
        return NvSuccess;

    NV_CHECK_ERROR_CLEANUP(NvBitInit(&hBit));

    NV_CHECK_ERROR_CLEANUP(
        NvBitGetData(hBit, NvBitDataType_SecondaryDevice,
           &Size, bootDevInstance, bootDevId)
    );

    e = NvSuccess;

 fail:
    NvBitDeinit(hBit);
    if (e)
        NvOsDebugPrintf("\nGetSecondaryBootDevice failed. NvError %u NvStatus %u\n", e, 0);
    return e;
}

NvError
Nv3pServer(void);
NvError
Nv3pServer(void)
{
    NvError e;
    Nv3pSocketHandle hSock = 0;
    Nv3pCommand command;
    Nv3pCmdStatus cmd_stat;
    void *arg;
    NvBool bDone = NV_FALSE;
    NvBool bSyncDone = NV_FALSE;

    NV_CHECK_ERROR(AllocateState());

    InitLstPartitionsToVerify();

#if NVODM_BOARD_IS_SIMULATION
    NV_CHECK_ERROR_CLEANUP(Nv3pOpen(&hSock, Nv3pTransportMode_Sema, -1));
#else
    NV_CHECK_ERROR_CLEANUP(Nv3pOpen(&hSock, Nv3pTransportMode_default, 0));

    /* send an ok -- the nvflash application waits for the bootloader
     * (this code) to initialize.
     */
    command = Nv3pCommand_Status;
    cmd_stat.Code = Nv3pStatus_Ok;
    cmd_stat.Flags = 0;

    NvOsMemset( cmd_stat.Message, 0, NV3P_STRING_MAX );
    NV_CHECK_ERROR_CLEANUP(Nv3pCommandSend( hSock, command,
        (NvU8 *)&cmd_stat, 0 ));
#endif

    for( ;; )
    {
        NV_CHECK_ERROR_CLEANUP(Nv3pCommandReceive(hSock, &command, &arg, 0));

        switch(command) {
            case Nv3pCommand_GetBct:
                NV_CHECK_ERROR_CLEANUP(
                    GetBct(hSock, command, arg)
                );
                break;
            case Nv3pCommand_DownloadBct:
                NV_CHECK_ERROR_CLEANUP(
                    DownloadBct(hSock, command, arg)
                );
                break;
            case Nv3pCommand_SetBlHash:
                NV_CHECK_ERROR_CLEANUP(
                    SetBlHash(hSock, command, arg)
                );
                break;
            case Nv3pCommand_UpdateBct:
                NV_CHECK_ERROR_CLEANUP(
                    UpdateBct(hSock, command, arg)
                );
                bSyncDone = NV_FALSE;
                break;
            case Nv3pCommand_SetDevice:
                NV_CHECK_ERROR_CLEANUP(
                    SetDevice(hSock, command, arg)
                );
                break;
            case Nv3pCommand_StartPartitionConfiguration:
                NV_CHECK_ERROR_CLEANUP(
                    StartPartitionConfiguration(hSock, command, arg)
                );
                break;
            case Nv3pCommand_EndPartitionConfiguration:
                NV_CHECK_ERROR_CLEANUP(
                    EndPartitionConfiguration(hSock, command, arg)
                );
                break;
            case Nv3pCommand_FormatPartition:
                NV_CHECK_ERROR_CLEANUP(
                    FormatPartition(hSock, command, arg)
                );
                bSyncDone = NV_FALSE;
                break;
            case Nv3pCommand_DownloadPartition:
                NV_CHECK_ERROR_CLEANUP(
                    DownloadPartition(hSock, command, arg)
                );
                bSyncDone = NV_FALSE;
                break;
            case Nv3pCommand_QueryPartition:
                NV_CHECK_ERROR_CLEANUP(
                    QueryPartition(hSock, command, arg)
                );
                break;
            case Nv3pCommand_CreatePartition:
                NV_CHECK_ERROR_CLEANUP(
                    CreatePartition(hSock, command, arg)
                );
                bSyncDone = NV_FALSE;
                break;
            case Nv3pCommand_ReadPartition:
                NV_CHECK_ERROR_CLEANUP(
                    ReadPartition(hSock, command, arg)
                );
                break;
            case Nv3pCommand_RawDeviceRead:
                NV_CHECK_ERROR_CLEANUP(
                    RawReadPartition(hSock, command, arg)
                );
                break;
            case Nv3pCommand_RawDeviceWrite:
                NV_CHECK_ERROR_CLEANUP(
                    RawWritePartition(hSock, command, arg)
                );
                break;
            case Nv3pCommand_SetBootPartition:
                NV_CHECK_ERROR_CLEANUP(
                    SetBootPartition(hSock, command, arg)
                );
                bSyncDone = NV_FALSE;
                break;
            case Nv3pCommand_ReadPartitionTable:
                NV_CHECK_ERROR_CLEANUP(
                    ReadPartitionTable(hSock, command, arg)
                );
                break;
            case Nv3pCommand_DeleteAll:
                NV_CHECK_ERROR_CLEANUP(
                    DeleteAll(hSock, command, arg)
                );
                break;
            case Nv3pCommand_FormatAll:
                NV_CHECK_ERROR_CLEANUP(
                    FormatAll(hSock, command, arg)
                );
                break;
            case Nv3pCommand_Obliterate:
                NV_CHECK_ERROR_CLEANUP(
                    Obliterate(hSock, command, arg)
                );
                bSyncDone = NV_FALSE;
                break;
            case Nv3pCommand_OdmOptions:
                NV_CHECK_ERROR_CLEANUP(
                    OdmOptions(hSock, command, arg)
                );
                break;
            case Nv3pCommand_OdmCommand:
                NV_CHECK_ERROR_CLEANUP(
                    OdmCommand(hSock, command, arg)
                );
                bSyncDone = NV_FALSE;
                break;
            case Nv3pCommand_Go:
                NV_CHECK_ERROR_CLEANUP(
                    Nv3pCommandComplete(hSock, command, arg, 0)
                );

                NV_CHECK_ERROR_CLEANUP(
                    ReportStatus(hSock, "", Nv3pStatus_Ok, 0)
                );
                bDone = NV_TRUE;

                break;
            case Nv3pCommand_Sync:
                NV_CHECK_ERROR_CLEANUP(
                    Sync( hSock, command, arg )
                );
                bSyncDone = NV_TRUE;
                break;
            case Nv3pCommand_VerifyPartitionEnable:
                NV_CHECK_ERROR_CLEANUP(
                    VerifyPartitionEnable( hSock, command, arg )
                );
                break;
            case Nv3pCommand_VerifyPartition:
                NV_CHECK_ERROR_CLEANUP(
                    VerifyPartition( hSock, command, arg )
                );
                break;
            case Nv3pCommand_EndVerifyPartition:
                gs_VerifyPending = NV_FALSE;
                NV_CHECK_ERROR_CLEANUP(
                    Nv3pCommandComplete(hSock, command, arg, 0)
                );
                break;
            case Nv3pCommand_SetTime:
                NV_CHECK_ERROR_CLEANUP(
                    SetTime(hSock, command, arg)
                );
                break;
            case Nv3pCommand_GetDevInfo:
                NV_CHECK_ERROR_CLEANUP(
                    GetDevInfo(hSock, command, arg)
                );
                break;
            case Nv3pCommand_NvPrivData:
                NV_CHECK_ERROR_CLEANUP(
                    DownloadNvPrivData(hSock, command, arg)
                );
                bSyncDone = NV_FALSE;
                break;

            default:
                NvOsDebugPrintf("Warning: Request for an unknown command");
                Nv3pNack(hSock, Nv3pNackCode_BadCommand);
                break;
        }

        /* Break when go command was received, sync done
          * and partition verification is not pending.
          */
        if( bDone  && bSyncDone && !gs_VerifyPending)
        {
            break;
        }

    }

    goto clean;

fail:
    if (hSock)
    {
        Nv3pNack( hSock, Nv3pNackCode_BadCommand );
        NV_CHECK_ERROR_CLEANUP(
           ReportStatus(hSock, "", Nv3pStatus_Unknown, 0)
        );
    }

clean:

    // Free memory occupied for Partitionverification, if any
    DeInitLstPartitionsToVerify();

    DeallocateState();

    if (hSock)
        Nv3pClose(hSock);

    return e;
}

