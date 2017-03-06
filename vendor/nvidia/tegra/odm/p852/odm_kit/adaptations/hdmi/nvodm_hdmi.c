/*
 * Copyright (c) 2008-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvodm_services.h"
#include "nvodm_query_pinmux.h"
#include "nvodm_query_discovery.h"

#if NVOS_IS_WINDOWS
#include <windows.h>
#endif

static NvOdmServicesI2cHandle s_hOdmI2cHdmi = NULL;

NvOdmI2cStatus
NvOdmDispHdmiI2cTransaction(
    NvOdmServicesI2cHandle hOdmI2c,
    NvOdmI2cTransactionInfo *TransactionInfo,
    NvU32 NumberOfTransactions,
    NvU32 ClockSpeedKHz,
    NvU32 WaitTimeoutInMilliSeconds);
NvBool NvOdmDispHdmiI2cOpen(void);
void NvOdmDispHdmiI2cClose(void);

#if NVOS_IS_WINDOWS
/**
 * @brief If the function is used, it is called by the system when processes
 * and threads are initialized and terminated, or on calls to the LoadLibrary
 * and FreeLibrary functions.
 * @param hInstance [IN] Handle to the dll
 * @param Reason    [IN] Specifies a flag indicating why the DLL entry-point
 *                       function is being called.
 * @param pReserved [IN] Specifies further aspects of DLL initialization and
 *                       cleanup.
 * @retval BOOL : returns TRUE or FALSE.
 */
BOOL WINAPI DllMain(HINSTANCE  hInstance, DWORD Reason, LPVOID pReserved)
{
    BOOL Ret = TRUE;

    if (Reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls((HMODULE)hInstance);
    }

    return Ret;
}
#endif

#if NVOS_IS_LINUX || NVOS_IS_UNIX
//===========================================================================
// _init() - entry function for linux .so
//===========================================================================
void _init(void);
void _init(void)
{
}

//===========================================================================
// _fini() - exit function for linux .so
//===========================================================================
void _fini(void);
void _fini(void)
{
}
#endif

NvOdmI2cStatus
NvOdmDispHdmiI2cTransaction(
    NvOdmServicesI2cHandle hOdmI2c,
    NvOdmI2cTransactionInfo *TransactionInfo,
    NvU32 NumberOfTransactions,
    NvU32 ClockSpeedKHz,
    NvU32 WaitTimeoutInMilliSeconds)
{
    if (!s_hOdmI2cHdmi)
        return NvOdmI2cStatus_InternalError;

    return NvOdmI2cTransaction(s_hOdmI2cHdmi,
               TransactionInfo, NumberOfTransactions,
               ClockSpeedKHz, WaitTimeoutInMilliSeconds);
}

NvBool NvOdmDispHdmiI2cOpen(void)
{
    NvU64 guid;
    const NvOdmPeripheralConnectivity *conn = NULL;
    const NvOdmIoAddress *addrList = NULL;
    NvU32 instance = 0;
    NvU32 i;
    NvU32 NumI2cConfigs;
    const NvU32 *pI2cConfigs;

    guid = NV_ODM_GUID('f','f','a','_','h','d','m','i');

    /* get the connectivity info */
    conn = NvOdmPeripheralGetGuid( guid );
    if ( !conn )
    {
        guid = NV_ODM_GUID('f','f','a','2','h','d','m','i');
        conn = NvOdmPeripheralGetGuid( guid );
        if ( !conn )
            return NV_FALSE;
    }

    for ( i = 0; i < conn->NumAddress && !s_hOdmI2cHdmi; i++)
    {
        if ( conn->AddressList[i].Interface == NvOdmIoModule_I2c )
        {
            addrList = &conn->AddressList[i];
            instance = addrList->Instance;
            NvOdmQueryPinMux(NvOdmIoModule_I2c, &pI2cConfigs,
                             &NumI2cConfigs);
            if (instance >= NumI2cConfigs)
                goto fail;

            if (pI2cConfigs[instance]==NvOdmI2cPinMap_Multiplexed)
                s_hOdmI2cHdmi = NvOdmI2cPinMuxOpen(NvOdmIoModule_I2c,
                     instance, NvOdmI2cPinMap_Config2);
            else
                s_hOdmI2cHdmi = NvOdmI2cOpen(NvOdmIoModule_I2c, instance);
            if (!s_hOdmI2cHdmi)
                goto fail;
        }
    }

    return NV_TRUE;
 fail:
    NvOdmDispHdmiI2cClose();
    return NV_FALSE;
}

void NvOdmDispHdmiI2cClose(void)
{
    NvOdmI2cClose(s_hOdmI2cHdmi);
}
