/*
* Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
*
* NVIDIA Corporation and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA Corporation is strictly prohibited.
*/

#include "nvos.h"
#include "ap15rm_private.h"
#include "nvrm_interrupt.h"
#include "nvrm_chiplib.h"
#include "nvintr.h"

#if (NV_DEF_ENVIRONMENT_SUPPORTS_SIM)
void
NvRmPrivHandleOsInterrupt( void *arg );

NvError
NvRmInterruptRegister(
    NvRmDeviceHandle hRmDevice,
    NvU32 IrqListSize,
    const NvU32 *pIrqList,
    const NvOsInterruptHandler *pIrqHandlerList,
    void *context,
    NvOsInterruptHandle *handle,
    NvBool InterruptEnable)
{
    NvError err;
    NvOsMutexLock(hRmDevice->mutex);

    err = NvIntrRegister( pIrqHandlerList , context,
                IrqListSize , pIrqList, handle, InterruptEnable);

    NvOsMutexUnlock(hRmDevice->mutex);
    return err;
}

void
NvRmInterruptUnregister(
    NvRmDeviceHandle hRmDevice,
    NvOsInterruptHandle handle)
{
    NvOsMutexLock(hRmDevice->mutex);

    NvIntrUnRegister( handle );

    NvOsMutexUnlock(hRmDevice->mutex);
}

NvError
NvRmInterruptEnable(
    NvRmDeviceHandle hRmDevice,
    NvOsInterruptHandle handle)
{
    return NvIntrSet( handle , NV_TRUE );
}

void
NvRmInterruptDone( NvOsInterruptHandle handle )
{
    NvU32 irq = (NvU32)handle;

    if (irq != NVRM_IRQ_INVALID)
    {
        NvIntrSet( handle , NV_TRUE );
    }
    return;
}

void
NvRmPrivHandleOsInterrupt( void *arg )
{
    NvRmDeviceHandle hRm = (NvRmDeviceHandle)arg;

    /* Chiplib passes null and expects this API to handle interrupts */
    if (hRm == NULL) hRm = NvRmPrivGetRmDeviceHandle();

    NvIntrHandler((void *)hRm);
}

#else

void
NvRmPrivChiplibInterruptHandler( void );

NvError
NvRmInterruptRegister(
    NvRmDeviceHandle hRmDevice,
    NvU32 IrqListSize,
    const NvU32 *pIrqList,
    const NvOsInterruptHandler *pIrqHandlerList,
    void *context,
    NvOsInterruptHandle *handle,
    NvBool InterruptEnable)
{
    NvError err;

    err = NvOsInterruptRegister(IrqListSize,
            pIrqList,
            pIrqHandlerList,
            context,
            handle,
            InterruptEnable);

    return err;
}

void
NvRmInterruptUnregister(
    NvRmDeviceHandle hRmDevice,
    NvOsInterruptHandle handle)
{
    NvOsInterruptUnregister( handle );
}

NvError
NvRmInterruptEnable(
    NvRmDeviceHandle hRmDevice,
    NvOsInterruptHandle handle)
{
    return NvOsInterruptEnable(handle);
}

void NvRmInterruptDone( NvOsInterruptHandle handle )
{
    NvOsInterruptDone( handle );
}

/* There is no chiplib interrupt handler for wince */
void
NvRmPrivChiplibInterruptHandler( void )
{
    return;
}

#endif

void
NvRmPrivInterruptStart(NvRmDeviceHandle hRmDevice)
{
    return;
}

void NvRmPrivInterruptShutdown(NvRmDeviceHandle handle)
{
    return;
}

