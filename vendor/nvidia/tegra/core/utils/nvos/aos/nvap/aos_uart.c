/**
 * Copyright (c) 2010-2012, NVIDIA CORPORATION.  All rights reserved.
 *
.* NVIDIA Corporation and its licensors retain all intellectual property and
.* proprietary rights in and to this software and related documentation.  Any
.* use, reproduction, disclosure or distribution of this software and related
.* documentation without an express license agreement from NVIDIA Corporation
.* is strictly prohibited.
*/

#include "aos_common.h"
#include "aos_uart.h"
#include "nvrm_drf.h"
#include "aos.h"
#ifndef SET_KERNEL_PINMUX
#include "nvodm_query_pinmux.h"
#endif
#include "arapb_misc.h"
#include "arclk_rst.h"
#include "nvboot_osc.h"
#include "arflow_ctlr.h"
#include "aruart.h"

#define MISC_PA_BASE    0x70000000
#define FLOW_PA_BASE    0x60007000
#define CLK_RST_PA_BASE    0x60006000


static NvU32 s_UartBaseAddress[] =
{
    0x70006000,
    0x70006040,
    0x70006200,
    0x70006300,
    0x70006400
};

enum UartPorts
{
    UARTA,
    UARTB,
    UARTC,
    UARTD,
    UARTE
};

typedef struct UartDebugRec
{
    NvU32                 SocFamily;
    NvOdmDebugConsole     DebugConsoleDevice;
    NvS8                  DccControl;
    NvBool                IsDebugInitialized;
    NvBool                IsBootloader;
    NvBool                IsConsoleAvailable;
} UartDebug;

static UartDebug s_uartDebugHandle;
static volatile void*s_pDebugUART;
#ifndef SET_KERNEL_PINMUX
UartInterface uartInterfacePtr;

NvU32 NvUartGetChipID(void)
{
    NvU8 *pMisc;
    NvU32 RegData;
    NvU32 ChipId = 0;
    NvError e;

    NV_CHECK_ERROR_CLEANUP(
      NvOsPhysicalMemMap(MISC_PA_BASE, MISC_PA_LEN,
      NvOsMemAttribute_Uncached, NVOS_MEM_READ_WRITE, (void **)&pMisc)
      );

    RegData = NV_READ32(pMisc + APB_MISC_GP_HIDREV_0);

    NvOsPhysicalMemUnmap(pMisc, MISC_PA_LEN);
    ChipId = NV_DRF_VAL(APB_MISC_GP, HIDREV, CHIPID, RegData);

fail:
    return ChipId;
}
#endif

NvU32 GetMajorVersion(void)
{
    NvU32 regVal = 0;
    NvU32 majorId = 0;
    regVal = NV_READ32(MISC_PA_BASE + APB_MISC_GP_HIDREV_0) ;
    majorId = NV_DRF_VAL(APB_MISC, GP_HIDREV, MAJORREV, regVal);
    return majorId;
}

static NvU32 aos_UartGetDefaultBaudRate(void)
{
    NvU32 baudrate= AOS_DEFAULT_UART_BAUD;
    return baudrate;
}


static NvError aos_DebugPrivateInit(void)
{
    NvError     ErrorCode = NvError_NotInitialized;
    NvU32       Channel;
    NvU32       BaudRate;

    s_pDebugUART = NULL;
    switch (s_uartDebugHandle.DebugConsoleDevice)
    {
    case NvOdmDebugConsole_None:
        ErrorCode = NvError_Success;
        break;

    case NvOdmDebugConsole_Dcc:
        s_uartDebugHandle.IsDebugInitialized = NV_TRUE;
        ErrorCode = NvError_Success;
        break;

    case NvOdmDebugConsole_UartA:
    case NvOdmDebugConsole_UartB:
    case NvOdmDebugConsole_UartC:
    case NvOdmDebugConsole_UartD:
    case NvOdmDebugConsole_UartE:

        Channel  = s_uartDebugHandle.DebugConsoleDevice - NvOdmDebugConsole_UartA;
        BaudRate = aos_UartGetDefaultBaudRate();
#ifndef SET_KERNEL_PINMUX
        s_pDebugUART = uartInterfacePtr.paos_UartInit(Channel, BaudRate);
#else
        s_pDebugUART = aos_UartInit(Channel, BaudRate);
#endif
        s_uartDebugHandle.IsDebugInitialized = NV_TRUE;
        ErrorCode = NvError_Success;
        break;

    default:
        break;
    }
    NvOsDebugPrintf("**********Aos DebugSemiHosting Initialized*******\n");
    return ErrorCode;
}

static void aos_DebugWriteByte(NvU8 ch)
{
    if (aosDebugIsInitialized())
    {
        switch (s_uartDebugHandle.DebugConsoleDevice)
        {
        case NvOdmDebugConsole_None:
            break;
        case NvOdmDebugConsole_Dcc:
            break;
        case NvOdmDebugConsole_UartA:
        case NvOdmDebugConsole_UartB:
        case NvOdmDebugConsole_UartC:
        case NvOdmDebugConsole_UartD:
        case NvOdmDebugConsole_UartE:
            NV_ASSERT(s_pDebugUART != NULL);
#ifndef SET_KERNEL_PINMUX
            uartInterfacePtr.paos_UartWriteByte(s_pDebugUART, ch);
#else
            aos_UartWriteByte(s_pDebugUART, ch);
#endif
            break;
        default:
            break;
        }
    }
}

NvError aosDebugInit(void)
{
    NvError     e;
    NvS32       i;

#ifndef SET_KERNEL_PINMUX
    uartInterfacePtr.paos_UartInit = aos_UartInit;
    uartInterfacePtr.paos_UartWriteByte = aos_UartWriteByte;
#endif

    s_uartDebugHandle.DebugConsoleDevice = aosGetOdmDebugConsole();
    NV_CHECK_ERROR_CLEANUP(aos_DebugPrivateInit());

    if (s_uartDebugHandle.IsDebugInitialized)
    {
        for (i = 50; i >= 0; --i)
        {
            aos_DebugWriteByte('-');
        }

        aos_DebugWriteByte('\r');
        aos_DebugWriteByte('\n');
    }
fail:
    return e;
}

NvBool aosDebugIsInitialized(void)
{
    return s_uartDebugHandle.IsDebugInitialized;
}

void aosWriteDebugString(const char* str)
{
    if (aosDebugIsInitialized())
    {
        while (str && *str)
        {
            aos_DebugWriteByte(*str++);
        }
    }
}

static void aos_UartClockEnable(NvU32 port, NvBool Enable)
{
    NvU32 Reg;

    switch (port)
    {
        case UARTA:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_L);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_L,
                                 CLK_ENB_UARTA, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_L, Reg);
            break;
        case UARTB:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_L);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_L,
                                 CLK_ENB_UARTB, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_L, Reg);
            break;
        case UARTC:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_H);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_H,
                                 CLK_ENB_UARTC, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_H, Reg);
            break;
        case UARTD:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_U);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_U,
                                 CLK_ENB_UARTD, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_U, Reg);
            break;
        case UARTE:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, CLK_OUT_ENB_U);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, CLK_OUT_ENB_U,
                                 CLK_ENB_UARTE, Enable, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_OUT_ENB_U, Reg);
            break;
        default:
            NV_ASSERT(0);
    }
}

static void aos_UartReset(NvU32 port, NvBool Assert)
{
    NvU32 Reg;

    switch (port)
    {
        case UARTA:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_L);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_L,
                                 SWR_UARTA_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_L, Reg);
            break;
        case UARTB:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_L);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_L,
                                 SWR_UARTB_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_L, Reg);
            break;
        case UARTC:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_H);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_H,
                                 SWR_UARTC_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_H, Reg);
            break;
        case UARTD:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_U);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_U,
                                 SWR_UARTD_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_U, Reg);
            break;
        case UARTE:
            Reg = NV_CAR_REGR(CLK_RST_PA_BASE, RST_DEVICES_U);
            Reg = NV_FLD_SET_DRF_NUM(CLK_RST_CONTROLLER, RST_DEVICES_U,
                                 SWR_UARTE_RST, Assert, Reg);
            NV_CAR_REGW(CLK_RST_PA_BASE, RST_DEVICES_U, Reg);
            break;
        default:
            NV_ASSERT(0);
    }
}

static void aos_SetUartClockSource (NvU32 port)
{
    NvU32 Reg;
    switch (port)
    {
        case UARTA:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTA,
                             UARTA_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTA, Reg);
            break;
        case UARTB:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTB,
                             UARTB_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTB, Reg);
            break;
        case UARTC:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTC,
                             UARTC_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTC, Reg);
            break;
        case UARTD:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTD,
                             UARTD_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTD, Reg);
            break;
        case UARTE:
            Reg = NV_DRF_DEF(CLK_RST_CONTROLLER, CLK_SOURCE_UARTE,
                             UARTE_CLK_SRC, PLLP_OUT0);
            NV_CAR_REGW(CLK_RST_PA_BASE, CLK_SOURCE_UARTE, Reg);
            break;
        default:
            NV_ASSERT(0);
    }
}

static void aos_CpuStallUs (NvU32 MicroSec)
{
    NvU32 Reg;
    NvU32 Delay;
    NvU32 MaxUs;

    // Get the maxium delay per loop.
    MaxUs = NV_DRF_NUM(FLOW_CTLR, HALT_CPU_EVENTS, ZERO, 0xFFFFFFFF);
    MicroSec += 1;

    while (MicroSec)
    {
        Delay   = (MicroSec > MaxUs) ? MaxUs : MicroSec;
        NV_ASSERT(Delay != 0);
        NV_ASSERT(Delay <= MicroSec);
        MicroSec -= Delay;

        Reg = NV_DRF_NUM(FLOW_CTLR, HALT_CPU_EVENTS, ZERO, Delay)
         | NV_DRF_NUM(FLOW_CTLR, HALT_CPU_EVENTS, uSEC, 1)
         | NV_DRF_DEF(FLOW_CTLR, HALT_CPU_EVENTS, MODE, FLOW_MODE_WAITEVENT);
        NV_FLOW_REGW(FLOW_PA_BASE, HALT_CPU_EVENTS, Reg);
        do
        {
             Reg = NV_FLOW_REGR(FLOW_PA_BASE, HALT_CPU_EVENTS);
             Reg = NV_DRF_VAL(FLOW_CTLR, HALT_CPU_EVENTS, ZERO, Reg);
        }while (Reg);
    }
}

static void aos_UartResetPort (NvU32 portNumber)
{
    aos_UartClockEnable (portNumber, NV_TRUE);
    aos_UartReset (portNumber, NV_TRUE);
    aos_CpuStallUs (10);
    aos_UartReset (portNumber, NV_FALSE);
}

static NvError aos_UartSetBaudRate (NvU32 portNumber, NvU32 baud_rate)
{
    NvU32   BaudRateDivisor;
    NvU32   LineControlReg;
    volatile void* pUartRegs = 0;
    pUartRegs = (void*)s_UartBaseAddress[portNumber];
    // Compute the baudrate divisor
    // Prepare the divisor value.

    if (GetMajorVersion() == 0)
        BaudRateDivisor = (13000 * 1000) / (baud_rate *16); //for emulation
    else
        BaudRateDivisor = (AOS_PLLP_FIXED_FREQ_KHZ * 1000) / (baud_rate *16);

    // Set the divisor latch bit to allow access to divisor registers (DLL/DLM)
    LineControlReg = NV_UART_REGR(pUartRegs, LCR);
    LineControlReg |= NV_DRF_NUM(UART, LCR, DLAB, 1);
    NV_UART_REGW(pUartRegs, LCR, LineControlReg);

    // First write the LSB of the baud rate divisor
    NV_UART_REGW(pUartRegs, THR_DLAB_0, BaudRateDivisor & 0xFF);

    // Now write the MSB of the baud rate divisor
    NV_UART_REGW(pUartRegs, IER_DLAB_0, (BaudRateDivisor >> 8) & 0xFF);

    // Now that the baud rate has been set turn off divisor latch
    // bit to allow access to receive/transmit holding registers
    // and interrupt enable register.
    LineControlReg &= ~NV_DRF_NUM(UART, LCR, DLAB, 1);
    NV_UART_REGW(pUartRegs, LCR, LineControlReg);

    return NvError_Success;
}


void aos_UartWriteByte (volatile void* pUart, NvU8 ch)
{
    NvU32 LineStatusReg;
    // Wait for transmit buffer to be empty.
    do
    {
        LineStatusReg = NV_UART_REGR(pUart, LSR);
    } while (!(LineStatusReg & NV_DRF_NUM(UART, LSR, THRE, 1)));
    // Write the character.
    NV_UART_REGW(pUart, THR_DLAB_0, ch);
}

#ifndef SET_KERNEL_PINMUX
static NvError aos_EnableUart (NvU32 port)
{
    NvU32 *pPinMuxConfigTable = NULL;
    NvU32 Count = 0;
    NvU32 configuration;
    NvU32 ChipId;
    NvError e = NvSuccess;
    NvOdmQueryPinMux(NvOdmIoModule_Uart,
           (const NvU32 **)&pPinMuxConfigTable,
                      &Count);
    configuration = pPinMuxConfigTable[port];
    ChipId = NvUartGetChipID();
    switch (port)
    {
            case UARTA:
                e = aos_EnableUartA(configuration);
                break;
            case UARTB:
                e = aos_EnableUartB(configuration);
                break;
            case UARTC:
                e = aos_EnableUartC(configuration);
                break;
            case UARTD:
                e = aos_EnableUartD(configuration);
                break;
            case UARTE:
                e = aos_EnableUartE(configuration);
                break;
            default:
                NV_ASSERT(0);
    }
    return e;
}
#endif

volatile void* aos_UartInit (NvU32 portNumber, NvU32 baud_rate)
{
    volatile void* pUart;
    NvU32   LineControlReg;
    NvU32   FifoControlReg;
    NvU32   ModemControlReg;

    pUart = (void*)s_UartBaseAddress[portNumber];

#ifndef SET_KERNEL_PINMUX
    aos_EnableUart (portNumber);
#endif
    aos_UartResetPort (portNumber);
    // Select the clock source for the UART.
    aos_SetUartClockSource (portNumber);
    // Initialize line control register: no parity, 1 stop bit, 8 data bits.
    LineControlReg = NV_DRF_NUM(UART, LCR, PAR, 0)
         | NV_DRF_NUM(UART, LCR, STOP, 0)
         | NV_DRF_NUM(UART, LCR, WD_SIZE, 3);
    NV_UART_REGW(pUart, LCR, LineControlReg);

    // Setup baud rate
    if (aos_UartSetBaudRate (portNumber, baud_rate) != NvError_Success)
    {
        return 0;
    }
    // Clear and enable the transmit and receive FIFOs.
    FifoControlReg = NV_DRF_NUM(UART, IIR_FCR, TX_CLR, 1)
         | NV_DRF_NUM(UART, IIR_FCR, RX_CLR, 1)
         | NV_DRF_NUM(UART, IIR_FCR, FCR_EN_FIFO, 1);
    NV_UART_REGW(pUart, IIR_FCR, FifoControlReg);

    // Assert DTR (Data Terminal Ready) so that the other side knows we're ready.
    ModemControlReg  = NV_UART_REGR(pUart, MCR);
    ModemControlReg |= NV_DRF_NUM(UART, MCR, DTR, 1);
    NV_UART_REGW(pUart, MCR, ModemControlReg);
    return pUart;
}
