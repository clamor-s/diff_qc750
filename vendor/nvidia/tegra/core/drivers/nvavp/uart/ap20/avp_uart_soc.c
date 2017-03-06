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

#define CONFIG(regt, regm, field, value)                                      \
    do                                                                        \
    {                                                                         \
        NvU32 Reg;                                                            \
        NV_MISC_READ(PP_PIN_MUX_CTL_##regm, Reg);                             \
        Reg = NV_FLD_SET_DRF_DEF(APB_MISC, PP_PIN_MUX_CTL_##regm,             \
              field##_SEL, value, Reg);                                       \
        NV_MISC_WRITE(PP_PIN_MUX_CTL_##regm, Reg);                            \
        NV_MISC_READ(PP_TRISTATE_REG_##regt, Reg);                            \
        Reg = NV_FLD_SET_DRF_DEF(APB_MISC, PP_TRISTATE_REG_##regt,            \
              Z_##field, NORMAL, Reg);                                        \
        NV_MISC_WRITE(PP_TRISTATE_REG_##regt, Reg);                           \
    } while (0)

void NvAvpSetUartConfig(NvU32 port)
{
#ifndef SET_KERNEL_PINMUX
    const NvU32* PinMuxTable;
    NvU32 PinMuxCount;

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
            CONFIG(B,A,UAA,UARTA);
            CONFIG(B,A,UAB,UARTA);
            break;
        case 1://uartB
            CONFIG(A,C,IRRX,UARTB);
            CONFIG(A,C,IRTX,UARTB);
            CONFIG(B,A,UAD,IRDA);
            break;
        case 2://uartC
            CONFIG(B,B,UCA,UARTC);
            CONFIG(B,B,UCB,UARTC);
            break;
        case 3://uartD
            CONFIG(D,A,UDA,UARTD);
            break;
        case 4://uartE
            CONFIG(A,A,SDIO1,UARTE);
            break;
        default:
            NV_ASSERT(0);
        }
        break;
    case NvOdmUartPinMap_Config2:
        switch(port)
        {
        case 0://uartA
            CONFIG(A,D,GPU,UARTA);
            break;
        case 1://uartB
            CONFIG(B,A,UAD,IRDA);
            break;
        case 2://uartC
            CONFIG(B,B,UCA,UARTC);
            break;
        case 3://uartD
            CONFIG(A,B,GMC,UARTD);
            break;
        case 4://uartE
            CONFIG(A,B,GMA,UARTE);
            break;
        default:
            NV_ASSERT(0);
        }
        break;
    case NvOdmUartPinMap_Config3:
        if(port != 0)
            NV_ASSERT(0);
        CONFIG(A,C,IRRX,UARTA);
        CONFIG(A,C,IRTX,UARTA);
        CONFIG(B,A,UAD,UARTA);
        break;
    case NvOdmUartPinMap_Config4:
        if(port != 0)
            NV_ASSERT(0);
        CONFIG(A,C,IRRX,UARTA);
        CONFIG(A,C,IRTX,UARTA);
        break;
    case NvOdmUartPinMap_Config5:
        if(port != 0)
            NV_ASSERT(0);
        CONFIG(B,D,SDD,UARTA);
        CONFIG(D,D,SDB,UARTA);
        break;
    case NvOdmUartPinMap_Config6:
        if(port != 0)
            NV_ASSERT(0);
        CONFIG(B,A,UAA,UARTA);
        break;
    case NvOdmUartPinMap_Config7:
        if(port != 0)
            NV_ASSERT(0);
        CONFIG(A,A,SDIO1,UARTA);
        break;
    default:
        NV_ASSERT(!"Should not come here.");
        break;
    }
#endif
}
