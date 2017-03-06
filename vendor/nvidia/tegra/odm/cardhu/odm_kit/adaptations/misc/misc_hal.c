/*
 * Copyright (c) 2008-2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvcommon.h"
#include "nvodm_sdio.h"
#include "nvodm_uart.h"
#include "nvodm_usbulpi.h"
#include "misc_hal.h"
#include "nvodm_query_discovery.h"
#include "nvodm_services.h"

#define NUM_SDIO_INSTANCES 4
#define NUM_UART_INSTANCES 2

#define SDIO_HANDLE(x) ((NvOdmSdioHandle)(((x)&0x3fffffffUL) | 0xc0000000UL))
#define UART_HANDLE(x) ((NvOdmUartHandle)(((x)&0x3fffffffUL) | 0x80000000UL))

#define IS_SDIO_HANDLE(x) (((NvU32)(x)&0xc0000000UL)==0xc0000000UL)
#define IS_UART_HANDLE(x) (((NvU32)(x)&0xc0000000UL)==0x80000000UL)
#define INVALID_HANDLE ((NvOdmMiscHandle)0)

#define HANDLE_INDEX(x) ((NvU32)(x)&0x3fffffffUL)
#define WLAN_GUID NV_ODM_GUID('s','d','i','o','w','l','a','n')
#define NVODM_QUERY_MAX_BUS_SEGMENTS          4   // # of Bus Segments defined by I2C extender
#define NVODM_QUERY_I2C_EXTENDER_ADDRESS      0xE8   // I2C bus extender address (7'h74)
#define MIC2826_I2C_SEGMENT                   (0x03)

typedef struct NvOdmSdioRec
{
    // NvODM PMU device handle
    NvOdmServicesPmuHandle hPmu;
    // Gpio Handle
    NvOdmServicesGpioHandle hGpio;
    // Pin handle to Wlan Reset Gpio pin
    NvOdmGpioPinHandle hResetPin;
    // Pin handle to Wlan PWR GPIO Pin
    NvOdmGpioPinHandle hPwrPin;
    NvOdmPeripheralConnectivity *pConnectivity;
    // Power state
    NvBool PoweredOn;
    // Instance
    NvU32 Instance;
} NvOdmSdio;

typedef struct NvOdmUsbUlpiRec
{
    NvU64    CurrentGUID;
} NvOdmUsbUlpi;


typedef struct
{
    NvU8 PmuRegAddr;
    NvU8 PmuRegData;
} NvOdmPmuSettings;


static void SdioOdmWlanSetPowerOn(NvOdmSdioHandle hOdmSdio, NvBool IsEnable);
static NvError PowerOnOffVCoreWF(NvBool IsEnable);
static NvBool NvOdmPeripheralSetBusSegment(NvOdmServicesI2cHandle hOdmI2c, NvU16 BusSegment);


static NvBool NvOdmPeripheralSetBusSegment(
    NvOdmServicesI2cHandle hOdmI2c,
    NvU16 BusSegment)
{
    NvU8 WriteBuffer[2];
    NvOdmI2cStatus Error;
    NvOdmI2cTransactionInfo TransactionInfo;
    NvU8 Data;

    Data = (NvU8)((BusSegment << 6) | 0x3F);

    WriteBuffer[0] = 0x7;
    WriteBuffer[1] = 0x3F;

    TransactionInfo.Address = NVODM_QUERY_I2C_EXTENDER_ADDRESS;
    TransactionInfo.Buf = WriteBuffer;
    TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
    TransactionInfo.NumBytes = 2;

    Error = NvOdmI2cTransaction(
              hOdmI2c, &TransactionInfo, 1, 400, NV_WAIT_INFINITE);
    if (Error != NvOdmI2cStatus_Success)
    {
         NvOdmOsDebugPrintf(" I2C Extender Programming Error:  %x",Error);
         NvOdmOsDebugPrintf(" Data:  %x  %x: ",WriteBuffer[0],WriteBuffer[1]);
         return NV_FALSE;
    }

    WriteBuffer[0] = 0x3;
    WriteBuffer[1] = Data;

    Error = NvOdmI2cTransaction(
            hOdmI2c, &TransactionInfo, 1, 400, NV_WAIT_INFINITE);
    if (Error != NvOdmI2cStatus_Success)
    {
         NvOdmOsDebugPrintf(" I2C Extender Programming Error:  %x",Error);
         NvOdmOsDebugPrintf(" Data:  %x  %x: ",WriteBuffer[0],WriteBuffer[1]);
         return NV_FALSE;
    }

    return NV_TRUE;
}


static NvError PowerOnOffVCoreWF(NvBool IsEnable)
{
//TODO: Implement power off

// Timeout for I2C transaction
#define I2C_TRANSACTION_TIMEOUT_MS  1000

    NvOdmServicesI2cHandle hOdmI2c;
    NvOdmI2cTransactionInfo TransactionInfo;
    NvU8  DataBuffer[2];
    NvU32 size = 0;
    NvU32 i= 0;
    NvOdmI2cStatus status = NvOdmI2cStatus_WriteFailed;
    NvOdmPmuSettings PmuConfig[]  = {
        {0x0, 0x19}, {0x5, 0x45}
    };

    hOdmI2c = NvOdmI2cOpen(NvOdmIoModule_I2c, 0x4);
    NV_ASSERT(hOdmI2c);

    // To program MIC2826, Set the I2C Extender Segment to 3.
    NvOdmPeripheralSetBusSegment(hOdmI2c, MIC2826_I2C_SEGMENT);

    size = NV_ARRAY_SIZE(PmuConfig);

    for (i = 0; i < size; i++)
    {
        DataBuffer[0] = PmuConfig[i].PmuRegAddr;
        DataBuffer[1] = PmuConfig[i].PmuRegData;

        TransactionInfo.Flags = NVODM_I2C_IS_WRITE;
        TransactionInfo.Address = 0xB4;
        TransactionInfo.NumBytes = 2;
        TransactionInfo.Buf = DataBuffer;

        status = NvOdmI2cTransaction(
            hOdmI2c,
            &TransactionInfo,
            1,
            400,
            I2C_TRANSACTION_TIMEOUT_MS);

        if (status != NvOdmI2cStatus_Success)
        {
            NvOdmOsDebugPrintf("WiFi MIC2826 Power On - ERROR \n");
            break;
        }
    }
    // Close the I2C driver
    NvOdmI2cClose(hOdmI2c);

    return status;
}


static void SdioOdmWlanSetPowerOn(NvOdmSdioHandle hOdmSdio, NvBool IsEnable)
{

    // Turn on/off Vcore power
    PowerOnOffVCoreWF(IsEnable);

    if (IsEnable)
    {
        // Wlan Power On Reset Sequence
        NvOdmGpioSetState(hOdmSdio->hGpio, hOdmSdio->hPwrPin, 0x0);      //PWD -> Low
        NvOdmGpioSetState(hOdmSdio->hGpio, hOdmSdio->hResetPin, 0x0);    //RST -> Low
        NvOdmOsWaitUS(2000);
        NvOdmGpioSetState(hOdmSdio->hGpio, hOdmSdio->hPwrPin, 0x1);      //PWD -> High
        NvOdmGpioSetState(hOdmSdio->hGpio, hOdmSdio->hResetPin, 0x1);    //RST -> High
     }
     else
     {
         // Power Off sequence
         NvOdmGpioSetState(hOdmSdio->hGpio, hOdmSdio->hPwrPin, 0x0);     //PWD -> Low
     }
}

static NvOdmMiscHandle
GetMiscInstance(NvOdmMiscHandle PseudoHandle)
{
    static NvOdmMisc SdioInstances[NUM_SDIO_INSTANCES];
    static NvOdmUart UartInstances[NUM_UART_INSTANCES];
    static NvBool first = NV_TRUE;

    if (first)
    {
        NvOdmOsMemset(SdioInstances, 0, sizeof(SdioInstances));
        NvOdmOsMemset(UartInstances, 0, sizeof(UartInstances));
        first = NV_FALSE;
    }

    if (IS_SDIO_HANDLE(PseudoHandle) && (HANDLE_INDEX(PseudoHandle)<NUM_SDIO_INSTANCES))
        return (NvOdmMiscHandle)&SdioInstances[HANDLE_INDEX(PseudoHandle)];

    else if (IS_UART_HANDLE(PseudoHandle) && (HANDLE_INDEX(PseudoHandle)<NUM_UART_INSTANCES))
        return (NvOdmMiscHandle)&UartInstances[HANDLE_INDEX(PseudoHandle)];

    return NULL;
}

static NvOdmMiscHandle
OdmMiscOpen(NvOdmIoModule Module,
            NvU32 Instance)
{
    NvU64 Guid;
    NvOdmMiscHandle misc;
    static NvOdmSdio* pDevice = NULL;
    NvOdmServicesGpioHandle hGpioTemp = NULL;
    NvOdmServicesPmuHandle hPmuTemp = NULL;
    NvOdmPeripheralSearch SearchAttrs[] =
    {
        NvOdmPeripheralSearch_IoModule,
        NvOdmPeripheralSearch_Instance
    };
    NvU32 SearchVals[2];

    SearchVals[0] = Module;
    SearchVals[1] = Instance;

    if (!NvOdmPeripheralEnumerate(SearchAttrs,
        SearchVals, NV_ARRAY_SIZE(SearchAttrs), &Guid, 1))
        return INVALID_HANDLE;

    if (Module==NvOdmIoModule_Uart)
        misc = GetMiscInstance((NvOdmMiscHandle)UART_HANDLE(Instance));
    else
        misc = GetMiscInstance((NvOdmMiscHandle)SDIO_HANDLE(Instance));

    if (misc && !misc->pConn)
    {
        misc->pConn = NvOdmPeripheralGetGuid(Guid);
        //  fill in HAL based on GUID here
    }

    if (!misc || !misc->pConn)
        return INVALID_HANDLE;

    if (misc->pfnOpen && !misc->pfnOpen(misc))
        return INVALID_HANDLE;

    if (misc->pfnSetPowerState && !misc->PowerOnRefCnt)
    {
        if (!misc->pfnSetPowerState(misc, NV_TRUE))
            return INVALID_HANDLE;
        else
            misc->PowerOnRefCnt = 1;
    }

//  For SD Card, CardhuPowerRails_VDDIO_SDMMC1 should be powered up
    if(Instance == 0 && Module == NvOdmIoModule_Sdio)
    {
        hPmuTemp = NvOdmServicesPmuOpen();
        NvOdmServicesPmuSetVoltage(hPmuTemp,misc->pConn->AddressList[3].Address,
                                            3300,NULL);
        NvOdmServicesPmuClose(hPmuTemp);
    }

    if (misc->pConn->Guid == WLAN_GUID)
    {
        pDevice = NvOdmOsAlloc(sizeof(NvOdmSdio));
        if (pDevice == NULL)
            return (NvOdmMiscHandle)pDevice;

        // Getting the OdmGpio Handle
        hGpioTemp = NvOdmGpioOpen();
        if (hGpioTemp == NULL)
        {
            NvOdmOsFree(pDevice);
            pDevice = NULL;
            return (NvOdmMiscHandle)pDevice;
        }

        // Search for the Vdd rail and set the proper volage to the rail.
        if (misc->pConn->AddressList[1].Interface == NvOdmIoModule_Gpio)
        {
             // Acquiring Pin Handles for Power Pin
             pDevice->hPwrPin= NvOdmGpioAcquirePinHandle(hGpioTemp,
                   misc->pConn->AddressList[1].Instance,
                   misc->pConn->AddressList[1].Address);
        }

        if (misc->pConn->AddressList[2].Interface == NvOdmIoModule_Gpio)
        {
             // Acquiring Pin Handles for Reset Pin
             pDevice->hResetPin= NvOdmGpioAcquirePinHandle(hGpioTemp,
                   misc->pConn->AddressList[2].Instance,
                   misc->pConn->AddressList[2].Address);
        }

        // Setting the ON/OFF pin to output mode.
        NvOdmGpioConfig(hGpioTemp, pDevice->hPwrPin, NvOdmGpioPinMode_Output);
        NvOdmGpioConfig(hGpioTemp, pDevice->hResetPin, NvOdmGpioPinMode_Output);

        // Setting the Output Pin to Low
        NvOdmGpioSetState(hGpioTemp, pDevice->hPwrPin, 0x0);
        NvOdmGpioSetState(hGpioTemp, pDevice->hResetPin, 0x0);

        pDevice->hGpio = hGpioTemp;

        SdioOdmWlanSetPowerOn(pDevice, NV_TRUE);
        pDevice->PoweredOn = NV_TRUE;
        return (NvOdmMiscHandle)pDevice;
    }

    if (Module==NvOdmIoModule_Uart)
        return (NvOdmMiscHandle)UART_HANDLE(Instance);
    else
        return (NvOdmMiscHandle)SDIO_HANDLE(Instance);
}

NvOdmSdioHandle
NvOdmSdioOpen(NvU32 Instance)
{
    return (NvOdmSdioHandle) OdmMiscOpen(NvOdmIoModule_Sdio, Instance);
}

NvOdmUartHandle
NvOdmUartOpen(NvU32 Instance)
{
    return (NvOdmUartHandle) OdmMiscOpen(NvOdmIoModule_Uart, Instance);
}

static void
OdmMiscClose(NvOdmMiscHandle hOdmMisc)
{
    NvOdmMisc *misc = GetMiscInstance(hOdmMisc);

    if (misc)
    {
        if (misc->pfnSetPowerState && misc->PowerOnRefCnt)
        {
            if (misc->pfnSetPowerState(misc, NV_FALSE))
                misc->PowerOnRefCnt = 0;
        }

        if (misc->pfnClose)
            misc->pfnClose(misc);

        misc->pfnOpen = NULL;
        misc->pfnSetPowerState = NULL;
        misc->pfnClose = NULL;
    }
}

void
NvOdmSdioClose(NvOdmSdioHandle hOdmSdio)
{

    NvOdmMisc *misc = GetMiscInstance((NvOdmMiscHandle)hOdmSdio);

    if (misc && (misc->pConn->Guid == WLAN_GUID))
    {
        if (hOdmSdio->PoweredOn)
        {
            // Call Turn off power when close is Called
            SdioOdmWlanSetPowerOn(hOdmSdio, NV_FALSE);

            NvOdmGpioReleasePinHandle(hOdmSdio->hGpio, hOdmSdio->hPwrPin);
            NvOdmGpioReleasePinHandle(hOdmSdio->hGpio, hOdmSdio->hResetPin);
            NvOdmGpioClose(hOdmSdio->hGpio);
        }
    }

    OdmMiscClose((NvOdmMiscHandle)hOdmSdio);
}

void
NvOdmUartClose(NvOdmUartHandle hOdmUart)
{
    OdmMiscClose((NvOdmMiscHandle)hOdmUart);
}

static NvError
OdmMiscSuspend(NvOdmMiscHandle hOdmMisc)
{
    NvOdmMisc *misc = GetMiscInstance(hOdmMisc);

    if (misc && misc->PowerOnRefCnt)
    {
        if (misc->pfnSetPowerState && (misc->PowerOnRefCnt==1))
        {
            if (!misc->pfnSetPowerState(misc, NV_FALSE))
                return NvError_NotSupported;
        }
        misc->PowerOnRefCnt--;
    }

    return NvSuccess;
}

NvBool NvOdmSdioSuspend(NvOdmSdioHandle hOdmSdio)
{
    NvError e = OdmMiscSuspend((NvOdmMiscHandle)hOdmSdio);
    if (e==NvSuccess)
    {
        return NV_TRUE;
    }
    else
    {
        return NV_FALSE;
    }
}

NvBool
NvOdmUartSuspend(NvOdmUartHandle hOdmUart)
{
    unsigned int i;
    NvError e = NvSuccess;
    for (i=0; i<NUM_UART_INSTANCES; i++)
    {
        NvOdmMiscHandle misc = GetMiscInstance((NvOdmMiscHandle)UART_HANDLE(i));
        NvError Err = OdmMiscSuspend(misc);
        if (e==NvSuccess && (e!=Err))
            e = Err;
    }
    if (e==NvSuccess)
    {
        return NV_TRUE;
    }
    else
    {
        return NV_FALSE;
    }
}

static NvError
OdmMiscResume(NvOdmMiscHandle hOdmMisc)
{
    NvOdmMisc *misc = GetMiscInstance(hOdmMisc);

    if (misc && misc->PowerOnRefCnt)
    {
        if (misc->pfnSetPowerState && !misc->PowerOnRefCnt)
        {
            if (!misc->pfnSetPowerState(misc, NV_TRUE))
                return NvError_NotSupported;
        }
        misc->PowerOnRefCnt++;
    }

    return NvSuccess;
}



NvBool
NvOdmSdioResume(NvOdmSdioHandle hOdmSdio)
{
    NvError e = OdmMiscSuspend((NvOdmMiscHandle)hOdmSdio);
    if (e==NvSuccess)
    {
        return NV_TRUE;
    }
    else
    {
        return NV_FALSE;
    }
}

NvBool
NvOdmUartResume(NvOdmUartHandle hOdmUart)
{
    unsigned int i;
    NvError e = NvSuccess;
    for (i=0; i<NUM_UART_INSTANCES; i++)
    {
        NvOdmMiscHandle misc = GetMiscInstance((NvOdmMiscHandle)UART_HANDLE(i));
        NvError Err = OdmMiscResume(misc);
        if (e==NvSuccess && (e!=Err))
            e = Err;
    }
    if (e==NvSuccess)
    {
        return NV_TRUE;
    }
    else
    {
        return NV_FALSE;
    }
}


NvOdmUsbUlpiHandle NvOdmUsbUlpiOpen(NvU32 Instance)
{
    // dummy implementation
    return NULL;
}

void NvOdmUsbUlpiClose(NvOdmUsbUlpiHandle hOdmUlpi)
{
    return;
}

