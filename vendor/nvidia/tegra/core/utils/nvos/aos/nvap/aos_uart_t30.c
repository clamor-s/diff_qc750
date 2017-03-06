/**
 * Copyright (c) 2010-2011 NVIDIA Corporation.  All Rights Reserved.
 *
.* NVIDIA Corporation and its licensors retain all intellectual property and
.* proprietary rights in and to this software and related documentation.  Any
.* use, reproduction, disclosure or distribution of this software and related
.* documentation without an express license agreement from NVIDIA Corporation
.* is strictly prohibited.
*/


#include "aos_uart.h"
#include "nvrm_drf.h"
#include "aos.h"
#include "t30/arapb_misc.h"
#include "t30/arclk_rst.h"
#include "t30/aruart.h"
#include "nvodm_query_pinmux.h"

#define TRISTATE_UART(reg) \
        TristateReg = NV_READ32(MISC_PA_BASE + PINMUX_AUX_##reg##_0); \
        TristateReg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_##reg, \
        TRISTATE, NORMAL, TristateReg); \
        NV_WRITE32(MISC_PA_BASE + PINMUX_AUX_##reg##_0, TristateReg);


#define PINMUX_UART(reg, uart) \
        PinMuxReg = NV_READ32(MISC_PA_BASE + PINMUX_AUX_##reg##_0); \
        PinMuxReg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_##reg, PM, \
                    uart, PinMuxReg); \
        NV_WRITE32(MISC_PA_BASE + PINMUX_AUX_##reg##_0, PinMuxReg);

#define MISC_PA_BASE    0x70000000


NvError aos_EnableUartA (NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(UART2_RTS_N);
            TRISTATE_UART(UART2_CTS_N);
            TRISTATE_UART(ULPI_DATA0);
            TRISTATE_UART(ULPI_DATA1);
            TRISTATE_UART(ULPI_DATA2);
            TRISTATE_UART(ULPI_DATA3);
            TRISTATE_UART(ULPI_DATA4);
            TRISTATE_UART(ULPI_DATA5);
            TRISTATE_UART(ULPI_DATA6);
            TRISTATE_UART(ULPI_DATA7);
            PINMUX_UART(UART2_RTS_N, UARTA);
            PINMUX_UART(UART2_CTS_N, UARTA);
            PINMUX_UART(ULPI_DATA0, UARTA);
            PINMUX_UART(ULPI_DATA1, UARTA);
            PINMUX_UART(ULPI_DATA2, UARTA);
            PINMUX_UART(ULPI_DATA3, UARTA);
            PINMUX_UART(ULPI_DATA4, UARTA);
            PINMUX_UART(ULPI_DATA5, UARTA);
            PINMUX_UART(ULPI_DATA6, UARTA);
            PINMUX_UART(ULPI_DATA7, UARTA);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(SDMMC1_DAT0);
            TRISTATE_UART(SDMMC1_DAT1);
            TRISTATE_UART(SDMMC1_DAT2);
            TRISTATE_UART(SDMMC1_DAT3);
            TRISTATE_UART(SDMMC1_CMD);
            TRISTATE_UART(SDMMC1_CLK);
            TRISTATE_UART(GPIO_PU6);
            PINMUX_UART(SDMMC1_DAT0, UARTA);
            PINMUX_UART(SDMMC1_DAT1, UARTA);
            PINMUX_UART(SDMMC1_DAT2, UARTA);
            PINMUX_UART(SDMMC1_DAT3, UARTA);
            PINMUX_UART(SDMMC1_CMD, UARTA);
            PINMUX_UART(SDMMC1_CLK, UARTA);
            PINMUX_UART(GPIO_PU6, UARTA);
            break;
        case NvOdmUartPinMap_Config3:
            TRISTATE_UART(UART2_RTS_N);
            TRISTATE_UART(UART2_CTS_N);
            TRISTATE_UART(UART2_RXD);
            TRISTATE_UART(UART2_TXD);
            TRISTATE_UART(GPIO_PU5);
            TRISTATE_UART(GPIO_PU4);
            PINMUX_UART(UART2_RTS_N, UARTA);
            PINMUX_UART(UART2_CTS_N, UARTA);
            PINMUX_UART(UART2_RXD, UARTA);
            PINMUX_UART(UART2_TXD, UARTA);
            PINMUX_UART(GPIO_PU5, UARTA);
            PINMUX_UART(GPIO_PU4, UARTA);
            break;
        case NvOdmUartPinMap_Config4:
            TRISTATE_UART(GPIO_PU0);
            TRISTATE_UART(GPIO_PU1);
            TRISTATE_UART(GPIO_PU2);
            TRISTATE_UART(GPIO_PU3);
            PINMUX_UART(GPIO_PU0, UARTA);
            PINMUX_UART(GPIO_PU1, UARTA);
            PINMUX_UART(GPIO_PU2, UARTA);
            PINMUX_UART(GPIO_PU3, UARTA);
            break;
        case NvOdmUartPinMap_Config5:
            TRISTATE_UART(SDMMC3_CMD);
            TRISTATE_UART(SDMMC3_CLK);
            PINMUX_UART(SDMMC3_CMD, UARTA);
            PINMUX_UART(SDMMC3_CLK, UARTA);
            break;
        default:
            retval = NvError_BadParameter;
            break;
    }
    return retval;
}

NvError aos_EnableUartB (NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(UART2_CTS_N);
            TRISTATE_UART(UART2_RTS_N);
            TRISTATE_UART(UART2_RXD);
            TRISTATE_UART(UART2_TXD);
            PINMUX_UART(UART2_CTS_N, UARTB);
            PINMUX_UART(UART2_RTS_N, UARTB);
            PINMUX_UART(UART2_RXD, IRDA);
            PINMUX_UART(UART2_TXD, IRDA);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(UART2_RXD);
            TRISTATE_UART(UART2_TXD);
            PINMUX_UART(UART2_RXD,IRDA);
            PINMUX_UART(UART2_TXD,IRDA);
            break;
        default:
            retval = NvError_BadParameter;
        break;
    }
    return retval;
}

NvError aos_EnableUartC (NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(UART3_CTS_N);
            TRISTATE_UART(UART3_RTS_N);
            TRISTATE_UART(UART3_TXD);
            TRISTATE_UART(UART3_RXD);
            PINMUX_UART(UART3_CTS_N, UARTC);
            PINMUX_UART(UART3_RTS_N, UARTC);
            PINMUX_UART(UART3_TXD, UARTC);
            PINMUX_UART(UART3_RXD, UARTC);
            break;
        default:
            retval = NvError_BadParameter;
            break;
   }
   return retval;
}

NvError aos_EnableUartD (NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(ULPI_CLK);
            TRISTATE_UART(ULPI_DIR);
            TRISTATE_UART(ULPI_STP);
            TRISTATE_UART(ULPI_NXT);
            PINMUX_UART(ULPI_CLK, UARTD);
            PINMUX_UART(ULPI_DIR, UARTD);
            PINMUX_UART(ULPI_STP, UARTD);
            PINMUX_UART(ULPI_NXT, UARTD);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(GMI_A16);
            TRISTATE_UART(GMI_A17);
            TRISTATE_UART(GMI_A18);
            TRISTATE_UART(GMI_A19);
            PINMUX_UART(GMI_A16, UARTD);
            PINMUX_UART(GMI_A17, UARTD);
            PINMUX_UART(GMI_A18, UARTD);
            PINMUX_UART(GMI_A19, UARTD);
            break;
        default:
            retval = NvError_BadParameter;
            break;
    }
    return retval;
}

NvError aos_EnableUartE (NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(SDMMC1_DAT0);
            TRISTATE_UART(SDMMC1_DAT1);
            TRISTATE_UART(SDMMC1_DAT2);
            TRISTATE_UART(SDMMC1_DAT3);
            PINMUX_UART(SDMMC1_DAT0, UARTE);
            PINMUX_UART(SDMMC1_DAT1, UARTE);
            PINMUX_UART(SDMMC1_DAT2, UARTE);
            PINMUX_UART(SDMMC1_DAT3, UARTE);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(SDMMC4_DAT0);
            TRISTATE_UART(SDMMC4_DAT1);
            TRISTATE_UART(SDMMC4_DAT2);
            TRISTATE_UART(SDMMC4_DAT3);
            PINMUX_UART(SDMMC4_DAT0, UARTE);
            PINMUX_UART(SDMMC4_DAT1, UARTE);
            PINMUX_UART(SDMMC4_DAT2, UARTE);
            PINMUX_UART(SDMMC4_DAT3, UARTE);
            break;
        default:
            retval = NvError_BadParameter;
            break;
    }
    return retval;
}



