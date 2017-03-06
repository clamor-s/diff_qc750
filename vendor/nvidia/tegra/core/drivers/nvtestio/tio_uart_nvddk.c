/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

//###########################################################################
//############################### INCLUDES ##################################
//###########################################################################

#include "tio_local.h"
#include "nvddk_uart.h"
#include "nvos.h"
#include "nvassert.h"

//###########################################################################
//############################### DEFINES ###################################
//###########################################################################

#define NV_TIO_UART_CNT 3

//###########################################################################
//############################### TYPEDEFS ##################################
//###########################################################################

typedef struct NvTioDdkUartInstRec {
    NvDdkUartHandle ddkUart;
} NvTioDdkInstInfo;

typedef struct NvTioDdkUartInfoRec {
    NvTioStreamOps      ops;
    NvRmDeviceHandle    rm;
    NvU32               openCnt;
    NvTioDdkInstInfo    uart[NV_TIO_UART_CNT];
} NvTioDdkUartInfo;

//###########################################################################
//############################### GLOBALS ###################################
//###########################################################################

#define NV_TIO_UART_HW_DEBUG 1
#if NV_TIO_UART_HW_DEBUG
volatile struct {
    volatile NvU8 ier;
    volatile NvU8 iir;
    volatile NvU8 lcr;
    volatile NvU8 mcr;
    volatile NvU8 lsr;
    volatile NvU8 msr;
    volatile NvU8 spr;
    volatile NvU8 csr;
    volatile NvU8 asr;
} uartregs;
#endif

//###########################################################################
//############################### CODE ######################################
//###########################################################################

//===========================================================================
// NvTioDdkUartCheckPath() - check filename to see if it is a uart port
//===========================================================================
static NvError NvTioDdkUartCheckPath(const char *path)
{
    //
    // return NvSuccess if path is a uart port
    //
    if (!NvTioOsStrncmp(path, "uart:", 5))
        return NvSuccess;
    if (!NvTioOsStrncmp(path, "debug:", 6))
        return NvSuccess;
    return DBERR(NvError_BadValue);
}

//===========================================================================
// NvTioDdkUartClose()
//===========================================================================
static void NvTioDdkUartClose(NvTioStream *stream)
{
    NvTioDdkUartInfo *info = (NvTioDdkUartInfo*)stream->ops;

    NV_ASSERT(info->openCnt != 0);
    NV_ASSERT(info->rm);
    NV_ASSERT(stream->f.uartIndex < NV_TIO_UART_CNT);

    if (info->uart[stream->f.uartIndex].ddkUart) {
        NvDdkUartClose(info->uart[stream->f.uartIndex].ddkUart);
        info->uart[stream->f.uartIndex].ddkUart = 0;
    }
    

    //
    // release RM handle
    //
    if (--info->openCnt == 0) {
        NvRmClose(info->rm);
        info->rm = 0;
    }
}

//===========================================================================
// NvTioDdkUartFopen()
//===========================================================================
static NvError NvTioDdkUartFopen(
                    const char *path,
                    NvU32 flags,
                    NvTioStream *stream)
{
    NvError err;
    NvDdkUartConfiguarations config;
    NvTioDdkUartInfo *info = (NvTioDdkUartInfo*)stream->ops;

    // TODO: support access to other UARTs
    stream->f.uartIndex = 0;

    if (info->uart[stream->f.uartIndex].ddkUart)
        return NvError_AlreadyAllocated;

    //
    // get RM handle
    //
    if (!info->rm) {
        NV_ASSERT(info->openCnt == 0);
        err = NvRmOpen(&info->rm, 0);
        if (err)
            return err;
    }
    info->openCnt++;

    //
    // open uart
    //
    err = NvDdkUartOpen(
                info->rm, 
                stream->f.uartIndex,
                &info->uart[stream->f.uartIndex].ddkUart);
    if (err)
        goto trouble;

    err = NvDdkUartGetConfiguration(
                info->uart[stream->f.uartIndex].ddkUart,
                &config);
    if (err)
        goto trouble;

    //
    // set uart config
    //
    config.UartBaudRate = 57600;
    config.UartParityBit = NvDdkUartParity_None;
    config.UartStopBit = NvDdkUartStopBit_1;
    config.UartDataLength = 8;
    config.UartLineInterfacingType = NvDdkUartInterfacing_2Line;
    config.IsEnableDriverFlowControl = NV_FALSE;
    config.IsEnableIrdaModulation = NV_FALSE;
    err = NvDdkUartSetConfiguration(
                info->uart[stream->f.uartIndex].ddkUart, 
                &config);
    if (err)
        goto trouble;

    err = NvDdkUartConfigureLocalReceiveBuffer(
                info->uart[stream->f.uartIndex].ddkUart, 
                0x1000,
                NULL);
    if (err)
        goto trouble;

#if NV_TIO_UART_CONNECT_SEND_CR
    {
        NvU32 len=2;
        err = NvDdkUartWrite(
                    info->uart[stream->f.uartIndex].ddkUart, 
                    "\r\n",
                    &len,
                    NV_WAIT_INFINITE,
                    NULL);

        if (err)
            goto trouble;
    }
#endif

    return NvSuccess;

trouble:
    NvTioDdkUartClose(stream);
    return err;
}

//===========================================================================
// NvTioDdkUartFwrite()
//===========================================================================
static NvError NvTioDdkUartFwrite(
                    NvTioStream *stream,
                    const void *ptr,
                    size_t size)
{
    NvError err = NvSuccess;
    NvU8 *p;
    NvTioDdkUartInfo *info = (NvTioDdkUartInfo*)stream->ops;

    NV_ASSERT(info->openCnt != 0);
    NV_ASSERT(info->rm);
    NV_ASSERT(stream->f.uartIndex < NV_TIO_UART_CNT);
    NV_ASSERT(info->uart[stream->f.uartIndex].ddkUart);

    p = (NvU8*)(void*)ptr;
    while (size) {
        NvU32 len = size;
        err = NvDdkUartWrite(
                info->uart[stream->f.uartIndex].ddkUart, 
                p,
                &len,
                NV_WAIT_INFINITE,
                NULL);
        if (err)
            break;
        p += len;
        size -= len;
    }

    return err;
}

//===========================================================================
// NvTioDdkUartFread()
//===========================================================================
static NvError NvTioDdkUartFread(
                    NvTioStream *stream,
                    void        *buf,
                    size_t       buf_count,
                    size_t      *recv_count,
                    NvU32        timeout_msec)
{
    NvError err = NvSuccess;
    NvU32 len = buf_count;
    NvTioDdkUartInfo *info = (NvTioDdkUartInfo*)stream->ops;

    NV_ASSERT(info->openCnt != 0);
    NV_ASSERT(info->rm);
    NV_ASSERT(stream->f.uartIndex < NV_TIO_UART_CNT);
    NV_ASSERT(info->uart[stream->f.uartIndex].ddkUart);

    err = NvDdkUartWrite(
                info->uart[stream->f.uartIndex].ddkUart, 
                buf,
                &len,
                timeout_msec,
                NULL);
    if (err)
        return err;
    if (recv_count)
        *recv_count = len;

    return NvSuccess;
}

static void bar(void)
{

}
//===========================================================================
// NvTioRegisterUart()
//===========================================================================
void NvTioRegisterUart(void)
{
    static NvTioDdkUartInfo s_info = {{0},0,0};

    if (0) {
        static volatile int foo=1;
        while(foo) {
            bar();
        }
    }
    if (!s_info.ops.sopCheckPath) {
        NvOsMemset(&s_info,0,sizeof(s_info));
        s_info.ops.sopCheckPath   = NvTioDdkUartCheckPath;
        s_info.ops.sopFopen       = NvTioDdkUartFopen;
        s_info.ops.sopFread       = NvTioDdkUartFread;
        s_info.ops.sopFwrite      = NvTioDdkUartFwrite;
        s_info.ops.sopClose       = NvTioDdkUartClose;

        NvTioRegisterOps(&s_info.ops);
    }
}

