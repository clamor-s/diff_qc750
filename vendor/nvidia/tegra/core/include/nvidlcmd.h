/*
 * Copyright (c) 2009-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVIDLCMD_H
#define INCLUDED_NVIDLCMD_H

#include "nvos.h"
#include "nvrm_fifo.h"
#include "nvreftrack.h"

#if NVOS_IS_QNX
// pathname-to-server mapping used to connect to daemon
#define NVRM_DAEMON_PATH "nvrm_daemon"

#define NV_DAEMON_MODULES(F) \
    F(NvRm)

#define NV_KERNEL_MODULES(F)
#else
// name of the master FIFO socket on systems which use the master FIFO
#define NVRM_DAEMON_SOCKNAME "/dev/nvrm_daemon"

#define NV_DAEMON_MODULES(F) \
    F(NvRmGraphics) \
    F(NvDispMgr)

#define NV_KERNEL_MODULES(F) \
    F(NvRm) \
    F(NvECPackage) \
    F(NvStorManager) \
    F(NvVib) \
    F(NvDdkAes)
#endif


// These codes are sent to the daemon to initiate commands from each module.
typedef enum NvRmDaemonCodeEnum
{
    NvRmDaemonCode_FifoCreate   =   0x281e0001,
    NvRmDaemonCode_FifoDelete,

#define F(X) NvRmDaemonCode_##X,
    NV_DAEMON_MODULES(F)
#undef F
    NvRmDaemonCode_Garbage      =   0xdeadbeef,

    NvRmDaemonCode_Force32      =   0x7FFFFFFF
} NvRmDaemonCode;

/* Defines a pair of objects for transferring data to and from the daemon.
 * FifoIn is used to read data from the daemon; FifoOut is used to write data
 * to the daemon
 */
typedef struct NvIdlFifoPairRec
{
    void *FifoIn;
    void *FifoOut;
} NvIdlFifoPair;


#if NVOS_IS_QNX
typedef enum NvIdlOriginCodeEnum
{
    NvIdlOriginCode_Client,
    NvIdlOriginCode_Daemon

} NvIdlOriginCode;
#endif

/* These functions are called by the IDL-generated code:
 *
 * *_NvIdlGetIoctlCode() - get code to use to identify module
 * *_NvIdlGetFifos()     - get a fifo pair for communication
 * *_NvIdlReleaseFifos() - get a fifo pair for communication
 * *_NvIdlGetIoctlFile() - get the file to use for ioctl
 */

#define NV_IDL_DECLS_STUB(pfx) \
        NvU32          pfx##_NvIdlGetIoctlCode(void); \
        NvOsFileHandle pfx##_NvIdlGetIoctlFile(void);

#define NV_IDL_DISPATCH_KERNEL(pfx) \
        NvError pfx##_Dispatch( \
                    void *InBuffer, \
                    NvU32 InSize, \
                    void *OutBuffer, \
                    NvU32 OutSize, \
                    NvDispatchCtx* Ctx)

#define NV_IDL_DECLS_DISPATCH_KERNEL(pfx) \
    NV_IDL_DISPATCH_KERNEL(pfx);

#if NVOS_IS_LINUX || NVOS_IS_QNX
#define NV_IDL_DISPATCH_DAEMON(pfx) \
        NvError pfx##_Dispatch( \
            void* hFifoIn, \
            void* hFifoOut, \
            NvDispatchCtx* Ctx)
#define NV_IDL_DECLS_DISPATCH_DAEMON(pfx) \
    NV_IDL_DISPATCH_DAEMON(pfx);
#else
#define NV_IDL_DISPATCH_DAEMON(pfx) \
        NV_IDL_DISPATCH_KERNEL(pfx)
#define NV_IDL_DECLS_DISPATCH_DAEMON(p) \
        NV_IDL_DECLS_DISPATCH_KERNEL(p)
#endif

#define F(X) NV_IDL_DECLS_STUB(X)
NV_DAEMON_MODULES(F)
NV_KERNEL_MODULES(F)
#undef F

#define F(X) NV_IDL_DECLS_DISPATCH_DAEMON(X)
NV_DAEMON_MODULES(F)
#undef F

#define F(X) NV_IDL_DECLS_DISPATCH_KERNEL(X)
NV_KERNEL_MODULES(F)
#undef F

#if NVOS_IS_QNX
/* utility functions to init / deinit a FIFO object
 * (prepare underlying message passing mechanism) */
NvError NvIdlHelperFifoInit(void *fifo, NvIdlOriginCode origin);
void    NvIdlHelperFifoDeInit(void *fifo);
/* utility function perform stub->dispatch transition or vice-versa */
void    NvIdlHelperFifoFlush(void *fifo);
#endif

/* utility functions called by stubs & dispatchers for transferring data
 * over FIFO objects. semantics are identical to NvOsFread / NvOsFwrite */
NvError NvIdlHelperFifoRead(void *fifo, void *ptr, size_t len, size_t *read);
NvError NvIdlHelperFifoWrite(void *fifo, const void *ptr, size_t len);

/* utility functions called by stub helpers to allocate and free FIFOs */
NvError NvIdlHelperGetFifoPair(NvIdlFifoPair **pFifo);
void    NvIdlHelperReleaseFifoPair(NvIdlFifoPair *pFifo);

#endif
