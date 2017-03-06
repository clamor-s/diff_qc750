/*
 * Copyright (c) 2010-2012 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#include "nvrm_channel.h"
#include "nvrm_memmgr.h"
#include "nvos.h"
#include "nvassert.h"
#include "nvrm_sched.h"
#include "linux/nvhost_ioctl.h"
#include "nvrm_channel_priv.h"

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define HOSTNODE_PREFIX "/dev/nvhost-"

/* Channel type */
enum NvRmChannelSyncptType
{
    /* Not known yet */
    NvRmChannelSyncptType_Unknown,
    /* Channel sync point id can change. */
    NvRmChannelSyncptType_Dynamic,
    /* Channel sync point id cannot change */
    NvRmChannelSyncptType_Static
};

struct NvRmChannelRec
{
    int fd;
    NvU32 in_submit;
    const char* device;
    NvU32 submit_version;
    /* Channel type - static or dynamic */
    int syncpt_type;
};

static int s_CtrlDev = -1;
static int s_CtrlVersion = 1;

void __attribute__ ((constructor)) nvrm_channel_init(void);
void __attribute__ ((constructor)) nvrm_channel_init(void)
{
    int err;
    struct nvhost_get_param_args args;

    s_CtrlDev = open(HOSTNODE_PREFIX "ctrl", O_RDWR | O_SYNC);
    if (s_CtrlDev < 0)
    {
        NvOsDebugPrintf("Error: Can't open " HOSTNODE_PREFIX "ctrl\n");
    }

    /* We assume version 1 if we can't get it from kernel */
    err = ioctl(s_CtrlDev, NVHOST_IOCTL_CTRL_GET_VERSION, &args);
    if (!err)
        s_CtrlVersion = args.value;
}

void __attribute__ ((destructor)) nvrm_channel_fini(void);
void __attribute__ ((destructor)) nvrm_channel_fini(void)
{
    close(s_CtrlDev);
}

static int IndexOfNthSetBit(NvU32 Bits, int n)
{
    NvU32 i;
    for (i = 0; Bits; ++i, Bits >>= 1)
        if ((Bits & 1) && (n-- == 0))
                return i;
    return -1;
}

static const char* GetDeviceNode(NvRmModuleID mod)
{
    switch (mod)
    {
    case NvRmModuleID_Display:
        return HOSTNODE_PREFIX "display";
    case NvRmModuleID_2D:
        return HOSTNODE_PREFIX "gr2d";
    case NvRmModuleID_3D:
        return HOSTNODE_PREFIX "gr3d";
    case NvRmModuleID_Isp:
        return HOSTNODE_PREFIX "isp";
    case NvRmModuleID_Vi:
        return HOSTNODE_PREFIX "vi";
    case NvRmModuleID_Mpe:
        return HOSTNODE_PREFIX "mpe";
    case NvRmModuleID_Dsi:
        return HOSTNODE_PREFIX "dsi";
    default:
        break;
    }

    return NULL;
}

NvError
NvRmChannelOpen(
    NvRmDeviceHandle hDevice,
    NvRmChannelHandle *phChannel,
    NvU32 NumModules,
    const NvRmModuleID* pModuleIDs)
{
    NvRmChannelHandle ch;
    const char* node;
    int nvmap;

    if (!NumModules)
        return NvError_BadParameter;

    node = GetDeviceNode(NVRM_MODULE_ID_MODULE(pModuleIDs[0]));
    if (!node)
    {
        NvOsDebugPrintf("Opening channel failed, unsupported module %d\n", pModuleIDs[0]);
        return NvError_BadParameter;
    }

    ch = NvOsAlloc(sizeof(*ch));
    if (!ch)
        return NvError_InsufficientMemory;
    NvOsMemset(ch, 0, sizeof(*ch));

    ch->fd = open(node, O_RDWR);
    if (ch->fd < 0)
    {
        NvOsFree(ch);
        NvOsDebugPrintf("Opening channel failed %d\n", pModuleIDs[0]);
        return NvError_KernelDriverNotFound;
    }
    ch->device = node;
    ch->submit_version = s_CtrlVersion;

    /* Set channel syncpt type as unknown */
    ch->syncpt_type = NvRmChannelSyncptType_Unknown;

    nvmap = NvRm_MemmgrGetIoctlFile();
    if (ioctl(ch->fd, NVHOST_IOCTL_CHANNEL_SET_NVMAP_FD, &nvmap) < 0)
    {
        NvRmChannelClose(ch);
        NvOsDebugPrintf("Opening channel failed, can't set nvmap fd %d\n", pModuleIDs[0]);
        return NvError_KernelDriverNotFound;
    }

    *phChannel = ch;
    return NvSuccess;
}

void
NvRmChannelClose(
    NvRmChannelHandle ch)
{
    if (ch != NULL)
    {
        close(ch->fd);
        NvOsFree(ch);
    }
}

NvError
NvRmChannelSetSubmitTimeout(
    NvRmChannelHandle ch,
    NvU32 SubmitTimeout)
{
    struct nvhost_set_timeout_args args;
    NvError err = NvSuccess;

    args.timeout = SubmitTimeout;    /* in milliseconds */

    if (ch == NULL)
        err = NvError_NotInitialized;
    else if (ioctl(ch->fd, NVHOST_IOCTL_CHANNEL_SET_TIMEOUT, &args) < 0) {
        NvOsDebugPrintf("Failed to set SubmitTimeout (%d ms)\n",
                SubmitTimeout);
        err = NvError_NotImplemented;
    }
    return err;
}

static NvError DoWrite(int File, const void* Ptr, NvU32 Bytes)
{
    const char* p = Ptr;
    size_t s = Bytes;
    ssize_t len;

    while (s)
    {
        len = write(File, p, s);
        if (len > 0)
        {
            p += len;
            s -= len;
        }
        else if ((len != 0) && (len != -EINTR))
            return NvError_FileWriteFailed;
    }
    return NvSuccess;
}

static NvError DoSubmit(NvRmChannelHandle ch, struct nvhost_submit_hdr_ext *ext)
{
    int ioc = NVHOST_IOCTL_CHANNEL_SUBMIT_EXT;
    NvError err = NvSuccess;

    ext->submit_version = ch->submit_version;
    while ((ext->submit_version > NVHOST_SUBMIT_VERSION_V0) &&
            ioctl(ch->fd, ioc, ext) < 0)
    {
        if ((errno == ENOTTY) || (errno == EFAULT)) {
            /* unreconized ioctl (use original submit) */
            ext->submit_version = NVHOST_SUBMIT_VERSION_V0;
        } else if (errno == EINVAL) {
            /* unreconized submit version (try next supported) */
            ext->submit_version--;
        } else if (errno != EINTR) {
            NvOsDebugPrintf("NvRmChannelSubmit: "
                    "NvError_IoctlFailed with error code %d\n", errno);
            return NvError_IoctlFailed;
        }
        /* track current version */
        ch->submit_version = ext->submit_version;
    }

    if (ext->submit_version == NVHOST_SUBMIT_VERSION_V0) {
        struct nvhost_submit_hdr hdr;

        hdr.syncpt_id = ext->syncpt_id;
        hdr.syncpt_incrs = ext->syncpt_incrs;
        hdr.num_cmdbufs = ext->num_cmdbufs;
        hdr.num_relocs = ext->num_relocs;

        /* version 0 submits don't support waitchks */
        ext->num_waitchks = 0;
        ext->waitchk_mask = 0;

        err = DoWrite(ch->fd, &hdr, sizeof(hdr));
    }
    return err;
}

#if NV_DEBUG
static NvBool
CheckSyncPointID(NvRmChannelHandle chan, NvU32 id)
{
    struct nvhost_get_param_args args;
    if (ioctl(chan->fd, NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS, &args) < 0)
    {
        NvOsDebugPrintf("CheckSyncPointID: "
                   "NvError_IoctlFailed with error code %d\n", errno);
        return NV_FALSE;
    }
    return ((1 << id) & args.value) ? NV_TRUE : NV_FALSE;
}
#endif

NvError
NvRmChannelSubmit(
    NvRmChannelHandle hChannel,
    const NvRmCommandBuffer *pCommandBufs, NvU32 NumCommandBufs,
    const NvRmChannelSubmitReloc *pRelocations, const NvU32 *pRelocShifts,
    NvU32 NumRelocations,
    const NvRmChannelWaitChecks *pWaitChecks, NvU32 NumWaitChecks,
    NvRmContextHandle hContext,
    const NvU32 *pContextExtra, NvU32 ContextExtraCount,
    NvBool NullKickoff, NvRmModuleID ModuleID,
    NvU32 SyncPointID, NvU32 SyncPointsUsed, NvU32 SyncPointWaitMask,
    NvU32 *pCtxChanged,
    NvU32 *pSyncPointValue)
{
    struct nvhost_submit_hdr_ext hdr;
    struct nvhost_get_param_args flush;
    int ioc = NullKickoff ? NVHOST_IOCTL_CHANNEL_NULL_KICKOFF : NVHOST_IOCTL_CHANNEL_FLUSH;
    NvError err;
    int ioctlret;

    (void)hContext;
    (void)ModuleID;

    NV_ASSERT(!pContextExtra);
    NV_ASSERT(ContextExtraCount == 0);
    NV_ASSERT(!NullKickoff);
    NV_ASSERT(CheckSyncPointID(hChannel, SyncPointID));

    NvOsMemset(&hdr, 0, sizeof(hdr));
    NvOsMemset(&flush, 0, sizeof(flush));
    hdr.syncpt_id = SyncPointID;
    hdr.syncpt_incrs = SyncPointsUsed;
    hdr.num_cmdbufs = NumCommandBufs;
    hdr.num_relocs = NumRelocations;
    hdr.num_waitchks = NumWaitChecks;
    hdr.waitchk_mask = SyncPointWaitMask;

    do
    {
        err = DoSubmit(hChannel, &hdr);
        if (err != NvSuccess)
            return err;

        err = DoWrite(hChannel->fd, pCommandBufs,
                NumCommandBufs*sizeof(NvRmCommandBuffer));
        if (err != NvSuccess)
            return err;

        err = DoWrite(hChannel->fd, pRelocations,
                NumRelocations*sizeof(NvRmChannelSubmitReloc));
        if (err != NvSuccess)
            return err;

        err = DoWrite(hChannel->fd, pWaitChecks,
                hdr.num_waitchks*sizeof(NvRmChannelWaitChecks));
        if (err != NvSuccess)
            return err;

        if (hdr.submit_version >= NVHOST_SUBMIT_VERSION_V2)
            err = DoWrite(hChannel->fd, pRelocShifts,
                hdr.num_relocs*sizeof(NvU32));

        ioctlret = ioctl(hChannel->fd, ioc, &flush);
        if (ioctlret < 0)
        {
            if (errno != EINTR)
            {
                NvOsDebugPrintf("NvRmChannelSubmit: ioctl(flush)"
                    "failed with error code %d\n", errno);
                if (pSyncPointValue)
                    *pSyncPointValue = flush.value;
                return NvError_IoctlFailed;
            }
            else
                NvOsDebugPrintf("NvRmChannelSubmit: got -EINTR retrying...\n");
        }
    } while (ioctlret < 0);

    if (pSyncPointValue)
        *pSyncPointValue = flush.value;

    return NvSuccess;
}

NvError
NvRmChannelGetModuleSyncPoint(
    NvRmChannelHandle hChannel,
    NvRmModuleID Module,
    NvU32 Index,
    NvU32 *pSyncPointID)
{
    struct nvhost_get_param_args args;

    /* Ensure that this is not called for channels with dynamic syncpt id */
    if (hChannel->syncpt_type == NvRmChannelSyncptType_Dynamic)
            return NvError_InvalidState;

    if (ioctl(hChannel->fd, NVHOST_IOCTL_CHANNEL_GET_SYNCPOINTS, &args) < 0)
        return NvError_IoctlFailed;

    /* special case display */
    if (NVRM_MODULE_ID_MODULE(Module) == NvRmModuleID_Display)
    {
        Index *= 2;
        Index += NVRM_MODULE_ID_INSTANCE(Module);
    }

    *pSyncPointID = IndexOfNthSetBit(args.value, Index);
    /* Set channel syncpt type as static */
    hChannel->syncpt_type = NvRmChannelSyncptType_Static;
    return (*pSyncPointID != (NvU32)-1) ? NvSuccess : NvError_BadParameter;
}

NvS32
NvRmChannelGetModuleTimedout(
    NvRmChannelHandle hChannel,
    NvRmModuleID Module,
    NvU32 Index)
{
    struct nvhost_get_param_args args;

    args.value = 0;
    if (ioctl(hChannel->fd, NVHOST_IOCTL_CHANNEL_GET_TIMEDOUT, &args) < 0)
        return -1;

    return args.value;
}

NvU32 NvRmChannelGetModuleWaitBase(
    NvRmChannelHandle hChannel,
    NvRmModuleID Module,
    NvU32 Index)
{
    struct nvhost_get_param_args args;
    if (ioctl(hChannel->fd, NVHOST_IOCTL_CHANNEL_GET_WAITBASES, &args) < 0)
        return 0;
    return IndexOfNthSetBit(args.value, Index);
}

NvError
NvRmChannelGetModuleClockRate(
    NvRmChannelHandle hChannel,
    NvRmModuleID Module,
    NvU32 *Rate)
{
    NvU64 ClockRate;
    if (ioctl(hChannel->fd, NVHOST_IOCTL_CHANNEL_GET_CLK_RATE, &ClockRate) < 0) {
        if(errno != EINTR) {
            NvOsDebugPrintf("NvRmChannelGetModuleClockRate: "
                    "NvError_IoctlFailed with error code %d\n", errno);
            return NvError_IoctlFailed;
        }
    }
    *Rate = ClockRate / 1000;
    return NvSuccess;
}

NvError
NvRmChannelSetModuleClockRate(
    NvRmChannelHandle hChannel,
    NvRmModuleID Module,
    NvU32 Rate)
{
    NvU64 ClockRate = Rate * 1000;
    if (ioctl(hChannel->fd, NVHOST_IOCTL_CHANNEL_SET_CLK_RATE, &ClockRate) < 0) {
        if(errno != EINTR) {
            NvOsDebugPrintf("NvRmChannelSetModuleClockRate: "
                    "NvError_IoctlFailed with error code %d\n", errno);
            return NvError_IoctlFailed;
        }
    }
    return NvSuccess;
}

NvError
NvRmChannelSetPriority(
    NvRmChannelHandle hChannel,
    NvU32 Priority,
    NvU32 SyncPointIndex,
    NvU32 WaitBaseIndex,
    NvU32 *SyncPointID,
    NvU32 *WaitBase)
{
    struct nvhost_set_priority_args args;
    args.priority = Priority;

    /* Ensure that caller hasn't retrieved static sync point id */
    if (SyncPointID && hChannel->syncpt_type == NvRmChannelSyncptType_Static)
        return NvError_InvalidState;

    if (ioctl(hChannel->fd, NVHOST_IOCTL_CHANNEL_SET_PRIORITY, &args) < 0) {
        NvOsDebugPrintf("NvRmChannelSetPriority: "
                "NvError_IoctlFailed with error code %d\n", errno);
        return NvError_IoctlFailed;
    }

    if (SyncPointID) {
        NvError err;

        /* Allow using NvRmChannelGetModuleSyncPoint() inside this function. */
        hChannel->syncpt_type = NvRmChannelSyncptType_Unknown;

        /* Module id doesn't matter as long as it's not display */
        err = NvRmChannelGetModuleSyncPoint(hChannel,
            NvRmModuleID_GraphicsHost, SyncPointIndex, SyncPointID);
        /* If we manage to set priority, but not get the new sync point, only
         * safe thing left to do is to bail out. Otherwise kernel and user
         * space are going to have mismatch on sync point id. */
        if (err != NvSuccess)
            NV_ASSERT(!"Could not get new sync point");

        /* Assign channel type as dynamic */
        hChannel->syncpt_type = NvRmChannelSyncptType_Dynamic;
    }

    if (WaitBase) {
        /* Module id doesn't matter as long as it's not display */
        *WaitBase = NvRmChannelGetModuleWaitBase(hChannel,
            NvRmModuleID_GraphicsHost, WaitBaseIndex);
        /* If we manage to set priority, but not get the new wait base, only
         * safe thing left to do is to bail out. Otherwise kernel and user
         * space are going to have mismatch on wait base id. */
        NV_ASSERT(*WaitBase != 0);
    }

    return NvSuccess;
}

NvU32
NvRmChannelSyncPointRead(
    NvRmDeviceHandle hDevice,
    NvU32 SyncPointID)
{
    struct nvhost_ctrl_syncpt_read_args args;

    (void)hDevice;

    NvOsMemset(&args, 0, sizeof(args));
    args.id = SyncPointID;
    if (ioctl(s_CtrlDev, NVHOST_IOCTL_CTRL_SYNCPT_READ, &args) < 0)
        NV_ASSERT(!"Can't handle ioctl failure");
    return args.value;
}

/* Read contents of a sync point sysfs file */
static NvError ReadFile(char *Name, char *Buffer, int Size)
{
    NvError err;
    NvOsFileHandle file;
    size_t count = 0;

    /* open file */
    err = NvOsFopen(Name, NVOS_OPEN_READ, &file);
    if(err) {
        NvOsDebugPrintf("Couldn't open %s\n", Name);
        return err;
    }

    /* read the contents of the file */
    err = NvOsFread(file, Buffer, Size-1, &count);
    /* End of file means the number was shorter than Size - that's ok */
    if (err == NvError_EndOfFile)
        err = NvSuccess;

    /*  add zero to make it a string */
    Buffer[count] = '\0';

    NvOsFclose(file);
    return err;
}

/* Name of the sysfs file for the syncpt max */
#define MAX_NAME_TEMPLATE "/sys/bus/nvhost/devices/host1x/syncpt/%d/max"
/* Length of UINT_MAX in characters */
#define MAX_INT_LENGTH 10

NvError
NvRmChannelSyncPointReadMax(
    NvRmDeviceHandle hDevice,
    NvU32 SyncPointID,
    NvU32 *Value)
{
    NvError err;
    /* Allocate space for path - %d holds two digits, so count that out */
    char name[sizeof(MAX_NAME_TEMPLATE)+MAX_INT_LENGTH-2];
    char valueStr[MAX_INT_LENGTH];
    const char *tmpl = MAX_NAME_TEMPLATE;

    NvOsSnprintf(name, sizeof(name), tmpl, SyncPointID);
    err = ReadFile(name, valueStr, MAX_INT_LENGTH);
    if (!err)
        *Value = atoi(valueStr);

    return err;
}

NvError
NvRmFenceTrigger(
    NvRmDeviceHandle hDevice,
    const NvRmFence *Fence)
{
    struct nvhost_ctrl_syncpt_incr_args args;
    NvU32 cur = NvRmChannelSyncPointRead(hDevice, Fence->SyncPointID);
    NvError err = NvSuccess;

    /* Can trigger only fence that is eligble (=next sync point val) */
    if (Fence->Value == cur+1) {
        args.id = Fence->SyncPointID;
        if (ioctl(s_CtrlDev, NVHOST_IOCTL_CTRL_SYNCPT_INCR, &args) < 0)
            err = NvError_IoctlFailed;
    } else {
        err = NvError_InvalidState;
    }

    return err;
}

void
NvRmChannelSyncPointIncr(
    NvRmDeviceHandle hDevice,
    NvU32 SyncPointID)
{
    struct nvhost_ctrl_syncpt_incr_args args;

    (void)hDevice;

    args.id = SyncPointID;
    if (ioctl(s_CtrlDev, NVHOST_IOCTL_CTRL_SYNCPT_INCR, &args) < 0)
        NV_ASSERT(!"Can't handle ioctl failure");
}

typedef struct
{
    NvU32 SyncPointID;
    NvU32 Threshold;
    NvOsSemaphoreHandle hSema;
} SyncPointSignalArgs;

static void* SyncPointSignalSemaphoreThread(void* vargs)
{
    SyncPointSignalArgs* args = (SyncPointSignalArgs*)vargs;
    NvRmChannelSyncPointWait(NULL, args->SyncPointID, args->Threshold, NULL);
    NvOsSemaphoreSignal(args->hSema);
    NvOsSemaphoreDestroy(args->hSema);
    NvOsFree(args);
    pthread_exit(NULL);
    return NULL;
}

NvBool
NvRmChannelSyncPointSignalSemaphore(
    NvRmDeviceHandle hDevice,
    NvU32 SyncPointID,
    NvU32 Threshold,
    NvOsSemaphoreHandle hSema)
{
    SyncPointSignalArgs* args;
    pthread_t thread;
    pthread_attr_t tattr;
    NvU32 val;

    val = NvRmChannelSyncPointRead(hDevice, SyncPointID);
    if (NvSchedValueWrappingComparison(val, Threshold))
    {
        return NV_TRUE;
    }

    args = NvOsAlloc(sizeof(SyncPointSignalArgs));
    if (!args)
        goto poll;

    args->SyncPointID = SyncPointID;
    args->Threshold = Threshold;
    (void)NvOsSemaphoreClone(hSema, &args->hSema);

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &tattr, SyncPointSignalSemaphoreThread, args))
    {
        NvOsSemaphoreDestroy(args->hSema);
        NvOsFree(args);
        pthread_attr_destroy(&tattr);
        goto poll;
    }
    pthread_attr_destroy(&tattr);
    return NV_FALSE;

 poll:
    do
    {
        val = NvRmChannelSyncPointRead(hDevice, SyncPointID);
    } while ((NvS32)(val - Threshold) < 0);
    return NV_TRUE;
}

NvBool
NvRmFenceSignalSemaphore(
    NvRmDeviceHandle hDevice,
    const NvRmFence *Fence,
    NvOsSemaphoreHandle hSema)
{
    SyncPointSignalArgs* args;
    pthread_t thread;
    pthread_attr_t tattr;
    NvU32 val;

    val = NvRmChannelSyncPointRead(hDevice, Fence->SyncPointID);
    if (NvSchedValueWrappingComparison(val, Fence->Value))
    {
        return NV_TRUE;
    }

    args = NvOsAlloc(sizeof(SyncPointSignalArgs));
    if (!args)
        goto poll;

    args->SyncPointID = Fence->SyncPointID;
    args->Threshold = Fence->Value;
    (void)NvOsSemaphoreClone(hSema, &args->hSema);

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread, &tattr, SyncPointSignalSemaphoreThread, args))
    {
        NvOsSemaphoreDestroy(args->hSema);
        NvOsFree(args);
        pthread_attr_destroy(&tattr);
        goto poll;
    }
    pthread_attr_destroy(&tattr);
    return NV_FALSE;

 poll:
    do
    {
        val = NvRmChannelSyncPointRead(hDevice, Fence->SyncPointID);
    } while ((NvS32)(val - Fence->Value) < 0);
    return NV_TRUE;
}


void
NvRmChannelSyncPointWait(
    NvRmDeviceHandle hDevice,
    NvU32 SyncPointID,
    NvU32 Threshold,
    NvOsSemaphoreHandle hSema)
{
    (void)NvRmChannelSyncPointWaitTimeout(hDevice, SyncPointID, Threshold, hSema, NVHOST_NO_TIMEOUT);
}

NvError
NvRmChannelSyncPointWaitTimeout(
    NvRmDeviceHandle hDevice,
    NvU32 SyncPointID,
    NvU32 Threshold,
    NvOsSemaphoreHandle hSema,
    NvU32 Timeout)
{
    return NvRmChannelSyncPointWaitexTimeout(hDevice, SyncPointID, Threshold, hSema, Timeout, NULL);
}

NvError
NvRmChannelSyncPointWaitexTimeout(
    NvRmDeviceHandle hDevice,
    NvU32 SyncPointID,
    NvU32 Threshold,
    NvOsSemaphoreHandle hSema,
    NvU32 Timeout,
    NvU32 *Actual)
{
    struct nvhost_ctrl_syncpt_waitex_args args = {0, 0, 0, 0};
    int requesttype = NVHOST_IOCTL_CTRL_SYNCPT_WAITEX;

    (void)hDevice;
    (void)hSema;

    args.id = SyncPointID;
    args.thresh = Threshold;
    args.timeout = Timeout;

    while (ioctl(s_CtrlDev, requesttype, &args) < 0)
    {
        if ( (errno == ENOTTY) || (errno == EFAULT) )
            requesttype = NVHOST_IOCTL_CTRL_SYNCPT_WAIT;
        else if (errno != EINTR)
            return errno == EAGAIN ? NvError_Timeout : NvError_InsufficientMemory;
    }

    if (requesttype == NVHOST_IOCTL_CTRL_SYNCPT_WAIT && Actual)
    {
        args.value = NvRmChannelSyncPointRead(hDevice, SyncPointID);
    }

    if (Actual)
    {
        *Actual = args.value;
    }
    return NvSuccess;
}

NvError
NvRmFenceWait(
    NvRmDeviceHandle hDevice,
    const NvRmFence *Fence,
    NvU32 Timeout)
{
    struct nvhost_ctrl_syncpt_waitex_args args = {0, 0, 0, 0};
    int requesttype = NVHOST_IOCTL_CTRL_SYNCPT_WAITEX;

    args.id = Fence->SyncPointID;
    args.thresh = Fence->Value;
    args.timeout = Timeout;

    while (ioctl(s_CtrlDev, requesttype, &args) < 0)
    {
        if ( (errno == ENOTTY) || (errno == EFAULT) )
            requesttype = NVHOST_IOCTL_CTRL_SYNCPT_WAIT;
        else if (errno != EINTR)
            return errno == EAGAIN ? NvError_Timeout : NvError_InsufficientMemory;
    }

    return NvSuccess;
}

#if NV_DEBUG
static NvError
GetRealModuleMutex(
    NvRmModuleID Module,
    NvU32 Index,
    NvU32 *realMutex)
{
    NvRmChannelHandle ch;
    NvError err;
    struct nvhost_get_param_args args;

    if (!realMutex)
        return NvError_BadParameter;

    err = NvRmChannelOpen(NULL, &ch, 1, &Module);
    if (err)
        return err;

    if (ioctl(ch->fd, NVHOST_IOCTL_CHANNEL_GET_MODMUTEXES, &args) < 0)
        args.value = 0;

    NvRmChannelClose(ch);

    *realMutex = IndexOfNthSetBit(args.value, Index);
    return NvSuccess;
}
#endif

#define NVMODMUTEX_2D_FULL   (1)
#define NVMODMUTEX_2D_SIMPLE (2)
#define NVMODMUTEX_2D_SB_A   (3)
#define NVMODMUTEX_2D_SB_B   (4)
#define NVMODMUTEX_3D        (5)
#define NVMODMUTEX_DISPLAYA  (6)
#define NVMODMUTEX_DISPLAYB  (7)
#define NVMODMUTEX_VI        (8)
#define NVMODMUTEX_DSI       (9)

/**
 * NvRmChannelGetModuleMutex does not take a channel parameter.
 * For not having to temporarily open channels (potentially leading into
 * continuous channel init/deinit), we duplicate the module mutex allocation
 * here. In debug builds we check that the allocation matches the kernel.
 */
NvU32
NvRmChannelGetModuleMutex(
    NvRmModuleID Module,
    NvU32 Index)
{
    NvU32 Mutex = 0;

    switch (NVRM_MODULE_ID_MODULE(Module))
    {
    case NvRmModuleID_Display:
        if (Index == 0)
            Mutex = NVMODMUTEX_DISPLAYA;
        else if (Index == 1)
            Mutex = NVMODMUTEX_DISPLAYB;
        break;
    case NvRmModuleID_2D:
        if (Index == 0)
            Mutex = NVMODMUTEX_2D_FULL;
        else if (Index == 1)
            Mutex = NVMODMUTEX_2D_SIMPLE;
        else if (Index == 2)
            Mutex = NVMODMUTEX_2D_SB_A;
        else if (Index == 3)
            Mutex = NVMODMUTEX_2D_SB_B;
        break;
    case NvRmModuleID_3D:
        if (Index == 0)
            Mutex = NVMODMUTEX_3D;
        break;
    case NvRmModuleID_Vi:
        if (Index == 0)
            Mutex = NVMODMUTEX_VI;
        break;
    case NvRmModuleID_Dsi:
        if (Index == 0)
            Mutex = NVMODMUTEX_DSI;
        break;
    default:
        break;
    }

    if (Mutex == 0)
    {
        NV_ASSERT(!"No matching module mutex");
        return 0;
    }

#if NV_DEBUG
    {
        NvU32 realMutex = 0;
        if (NvSuccess == GetRealModuleMutex(Module, Index, &realMutex))
            NV_ASSERT(Mutex == realMutex);
    }
#endif
    return Mutex;
}

static void ModuleMutexLock(NvU32 Index, int lock)
{
    struct nvhost_ctrl_module_mutex_args args;

    args.id = Index;
    args.lock = lock;

    while (ioctl(s_CtrlDev, NVHOST_IOCTL_CTRL_MODULE_MUTEX, &args) < 0)
    {
        NV_ASSERT(errno == EBUSY);
    }
}

void
NvRmChannelModuleMutexLock(
    NvRmDeviceHandle hDevice,
    NvU32 Index)
{
    (void)hDevice;
    ModuleMutexLock(Index, 1);
}

void
NvRmChannelModuleMutexUnlock(
    NvRmDeviceHandle hDevice,
    NvU32 Index)
{
    (void)hDevice;
    ModuleMutexLock(Index, 0);
}

static NvU32 GetModuleIndex(NvRmModuleID id)
{
    NvU32 mod = NVRM_MODULE_ID_MODULE(id);
    NvU32 idx = NVRM_MODULE_ID_INSTANCE(id);

#if 0
    /* TODO: [ahatala 2010-07-02] find out if these are needed */
    MODULE_FUSE,
    MODULE_APB_MISC,
    MODULE_CLK_RESET,
#endif

    switch (mod)
    {
    case NvRmModuleID_Display:
        if (idx == 0) return 0;
        if (idx == 1) return 1;
        break;
    case NvRmModuleID_Vi:
        if (idx == 0) return 2;
        break;
    case NvRmModuleID_Isp:
        if (idx == 0) return 3;
        break;
    case NvRmModuleID_Mpe:
        if (idx == 0) return 4;
        break;
    default:
        break;
    }

    return (NvU32)-1;
}

static NvError ModuleRegReadWrite(
    NvRmModuleID id,
    NvU32 num_offsets,
    NvU32 block_size,
    const NvU32 *offsets,
    NvU32 *values,
    NvBool write)
{
    struct nvhost_ctrl_module_regrdwr_args args;
    NvError err = NvSuccess;

    args.id = GetModuleIndex(id);
    args.num_offsets = num_offsets;
    args.block_size = block_size;
    args.offsets = (__u32 *)offsets;
    args.values = values;
    args.write = write ? 1 : 0;

    if (ioctl(s_CtrlDev, NVHOST_IOCTL_CTRL_MODULE_REGRDWR, &args) < 0)
    {
        err = NvError_IoctlFailed;
    }

    return err;
}

NvError
NvRmHostModuleRegRd(
    NvRmDeviceHandle device,
    NvRmModuleID id,
    NvU32 num,
    const NvU32 *offsets,
    NvU32 *values)
{
    (void)device;
    return ModuleRegReadWrite(id, num, 4, offsets, values, NV_FALSE);
}

NvError
NvRmHostModuleRegWr(
    NvRmDeviceHandle device,
    NvRmModuleID id,
    NvU32 num,
    const NvU32 *offsets,
    const NvU32 *values)
{
    (void)device;
    return ModuleRegReadWrite(id, num, 4, offsets, (NvU32 *)values, NV_TRUE);
}

NvError
NvRmHostModuleBlockRegRd(
    NvRmDeviceHandle device,
    NvRmModuleID id,
    NvU32 num,
    NvU32 offset,
    NvU32 *values)
{
    (void)device;
    return ModuleRegReadWrite(id, 1, num*4, &offset, values, NV_FALSE);
}

NvError
NvRmHostModuleBlockRegWr(
    NvRmDeviceHandle device,
    NvRmModuleID id,
    NvU32 num,
    NvU32 offset,
    const NvU32 *values)
{
    (void)device;
    return ModuleRegReadWrite(id, 1, num*4, &offset, (NvU32 *)values, NV_TRUE);
}


/** Not needed or deprecated **/

NvError
NvRmContextAlloc(
    NvRmDeviceHandle hDevice,
    NvRmModuleID Module,
    NvRmContextHandle *phContext)
{
    *phContext = (NvRmContextHandle)1;
    return NvSuccess;
}

void
NvRmContextFree(
    NvRmContextHandle hContext)
{
}

NvError
NvRmChannelAlloc(
    NvRmDeviceHandle hDevice,
    NvRmChannelHandle *phChannel)
{
    NV_ASSERT(!"NvRmChannelAlloc not suppported");
    return NvError_NotSupported;
}

NvError
NvRmChannelSyncPointAlloc(
    NvRmDeviceHandle hDevice,
    NvU32 *pSyncPointID)
{
    return NvError_NotSupported;
}

void
NvRmChannelSyncPointFree(
    NvRmDeviceHandle hDevice,
    NvU32 SyncPointID)
{
}


NvError NvRmChannelRead3DRegister(
    NvRmChannelHandle hChannel,
    NvU32 Offset,
    NvU32* Value)
{
    struct nvhost_read_3d_reg_args args;

    args.value = 0;
    args.offset = Offset;

    if (ioctl(hChannel->fd, NVHOST_IOCTL_CHANNEL_READ_3D_REG, &args) < 0)
    {
        return NvError_NotSupported;
    } else {
        *Value = args.value;
        return NvSuccess;
    }
}

