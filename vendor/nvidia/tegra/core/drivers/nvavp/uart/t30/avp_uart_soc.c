/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
#include "avp_uart_headers.h"
#include "avp_uart.h"
#include "arapbpm.h"
#include "nvodm_query_pinmux.h"

#define TRISTATE_UART(reg) \
        TristateReg = NV_READ32(NV_ADDRESS_MAP_APB_MISC_BASE + PINMUX_AUX_##reg##_0); \
        TristateReg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_##reg, \
        TRISTATE, NORMAL, TristateReg); \
        NV_WRITE32(NV_ADDRESS_MAP_APB_MISC_BASE + PINMUX_AUX_##reg##_0, TristateReg);

#define PINMUX_UART(reg, uart) \
        PinMuxReg = NV_READ32(NV_ADDRESS_MAP_APB_MISC_BASE + PINMUX_AUX_##reg##_0); \
        PinMuxReg = NV_FLD_SET_DRF_DEF(PINMUX, AUX_##reg, PM, \
            UART##uart, PinMuxReg); \
        NV_WRITE32(NV_ADDRESS_MAP_APB_MISC_BASE + PINMUX_AUX_##reg##_0, PinMuxReg);

void NvAvpSetUartConfig(NvU32 port)
{
#ifndef SET_KERNEL_PINMUX
    const NvU32* PinMuxTable;
    NvU32 PinMuxCount;
    NvU32 TristateReg;
    NvU32 PinMuxReg;

    // Get UART pin mux table.
    NvOdmQueryPinMux(NvOdmIoModule_Uart, &PinMuxTable, &PinMuxCount);
    NV_ASSERT(PinMuxCount > 0);
    NV_ASSERT(PinMuxCount >= port);

    // Set pin mux and tristate for UART
    switch (PinMuxTable[port])
    {
    case NvOdmUartPinMap_Config1:
        switch(port)
        {
        case 0://uartA
            TRISTATE_UART(UART2_RTS_N);
            TRISTATE_UART(UART2_CTS_N);
            TRISTATE_UART(ULPI_DATA0);
            PINMUX_UART(ULPI_DATA0,A);
            TRISTATE_UART(ULPI_DATA1);
            TRISTATE_UART(ULPI_DATA2);
            TRISTATE_UART(ULPI_DATA3);
            TRISTATE_UART(ULPI_DATA4);
            TRISTATE_UART(ULPI_DATA5);
            TRISTATE_UART(ULPI_DATA6);
            TRISTATE_UART(ULPI_DATA7);
            PINMUX_UART(UART2_RTS_N,A);
            PINMUX_UART(UART2_CTS_N,A);
            PINMUX_UART(ULPI_DATA1,A);
            PINMUX_UART(ULPI_DATA2,A);
            PINMUX_UART(ULPI_DATA3,A);
            PINMUX_UART(ULPI_DATA4,A);
            PINMUX_UART(ULPI_DATA5,A);
            PINMUX_UART(ULPI_DATA6,A);
            PINMUX_UART(ULPI_DATA7,A);
            break;
        case 1://uartB
            TRISTATE_UART(UART2_CTS_N);
            TRISTATE_UART(UART2_RTS_N);
            TRISTATE_UART(UART2_RXD);
            TRISTATE_UART(UART2_TXD);
            PINMUX_UART(UART2_TXD,A);
            PINMUX_UART(UART2_CTS_N,B);
            PINMUX_UART(UART2_RTS_N,B);
            PINMUX_UART(UART2_RXD,A);
            break;
        case 2://uartC
            TRISTATE_UART(UART3_CTS_N);
            TRISTATE_UART(UART3_RTS_N);
            TRISTATE_UART(UART3_TXD);
            PINMUX_UART(UART3_TXD,C);
            TRISTATE_UART(UART3_RXD);
            PINMUX_UART(UART3_CTS_N,C);
            PINMUX_UART(UART3_RTS_N,C);
            PINMUX_UART(UART3_RXD,C);
            break;
        case 3://uartD
            TRISTATE_UART(ULPI_CLK);
            PINMUX_UART(ULPI_CLK,D);
            TRISTATE_UART(ULPI_DIR);
            TRISTATE_UART(ULPI_STP);
            TRISTATE_UART(ULPI_NXT);
            PINMUX_UART(ULPI_DIR,D);
            PINMUX_UART(ULPI_STP,D);
            PINMUX_UART(ULPI_NXT,D);
            break;
        case 4://uartE
            TRISTATE_UART(SDMMC1_DAT0);
            TRISTATE_UART(SDMMC1_DAT1);
            TRISTATE_UART(SDMMC1_DAT2);
            TRISTATE_UART(SDMMC1_DAT3);
            PINMUX_UART(SDMMC1_DAT3,E);
            PINMUX_UART(SDMMC1_DAT0,E);
            PINMUX_UART(SDMMC1_DAT1,E);
            PINMUX_UART(SDMMC1_DAT2,E);
            break;
        default:
            NV_ASSERT(0);
        }
        break;
    case NvOdmUartPinMap_Config2:
        switch(port)
        {
        case 0://uartA
            TRISTATE_UART(SDMMC1_DAT0);
            TRISTATE_UART(SDMMC1_DAT1);
            TRISTATE_UART(SDMMC1_DAT2);
            TRISTATE_UART(SDMMC1_DAT3);
            PINMUX_UART(SDMMC1_DAT3,A);
            TRISTATE_UART(SDMMC1_CMD);
            TRISTATE_UART(SDMMC1_CLK);
            TRISTATE_UART(GPIO_PU6);
            PINMUX_UART(SDMMC1_DAT0,A);
            PINMUX_UART(SDMMC1_DAT1,A);
            PINMUX_UART(SDMMC1_DAT2,A);
            PINMUX_UART(SDMMC1_CMD,A);
            PINMUX_UART(SDMMC1_CLK,A);
            PINMUX_UART(GPIO_PU6,A);
            break;
        case 1://uartB
            NV_ASSERT(0);
        case 2://uartC
            NV_ASSERT(0);
        case 3://uartD
            TRISTATE_UART(GMI_A16);
            PINMUX_UART(GMI_A16,D);
            TRISTATE_UART(GMI_A17);
            TRISTATE_UART(GMI_A18);
            TRISTATE_UART(GMI_A19);
            PINMUX_UART(GMI_A17,D);
            PINMUX_UART(GMI_A18,D);
            PINMUX_UART(GMI_A19,D);
            break;
        case 4://uartE
            TRISTATE_UART(SDMMC1_DAT0);
            PINMUX_UART(SDMMC1_DAT0,E);
            TRISTATE_UART(SDMMC1_DAT1);
            TRISTATE_UART(SDMMC1_DAT2);
            TRISTATE_UART(SDMMC1_DAT3);
            PINMUX_UART(SDMMC1_DAT1,E);
            PINMUX_UART(SDMMC1_DAT2,E);
            PINMUX_UART(SDMMC1_DAT3,E);
            break;
        default:
            NV_ASSERT(0);
        }
        break;
    case NvOdmUartPinMap_Config3:
        if(port != 0)
            NV_ASSERT(0);
        TRISTATE_UART(UART2_RTS_N);
        PINMUX_UART(UART2_RTS_N,A);
        TRISTATE_UART(UART2_CTS_N);
        TRISTATE_UART(UART2_RXD);
        TRISTATE_UART(UART2_TXD);
        TRISTATE_UART(GPIO_PU5);
        TRISTATE_UART(GPIO_PU4);
        PINMUX_UART(UART2_CTS_N,A);
        PINMUX_UART(UART2_RXD,A);
        PINMUX_UART(UART2_TXD,A);
        PINMUX_UART(GPIO_PU5,A);
        PINMUX_UART(GPIO_PU4,A);
        break;
    case NvOdmUartPinMap_Config4:
        if(port != 0)
            NV_ASSERT(0);
        TRISTATE_UART(GPIO_PU0);
        PINMUX_UART(GPIO_PU0,A);
        TRISTATE_UART(GPIO_PU1);
        TRISTATE_UART(GPIO_PU2);
        TRISTATE_UART(GPIO_PU3);
        PINMUX_UART(GPIO_PU1,A);
        PINMUX_UART(GPIO_PU2,A);
        PINMUX_UART(GPIO_PU3,A);
        break;
    case NvOdmUartPinMap_Config5:
        if(port != 0)
            NV_ASSERT(0);
        TRISTATE_UART(SDMMC3_CMD);
        TRISTATE_UART(SDMMC3_CLK);
        PINMUX_UART(SDMMC3_CLK,A);
        PINMUX_UART(SDMMC3_CMD,A);
        break;
    case NvOdmUartPinMap_Config6:
        NV_ASSERT(0);
    case NvOdmUartPinMap_Config7:
        NV_ASSERT(0);
    default:
        NV_ASSERT(!"Should not come here.");
    break;
    }
#endif
}
