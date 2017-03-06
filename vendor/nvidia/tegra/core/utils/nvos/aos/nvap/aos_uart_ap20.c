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
#include "ap20/aruart.h"
#include "ap20/arapb_misc.h"
#include "ap20/arclk_rst.h"
#include "ap20/aruart.h"
#include "nvodm_query_pinmux.h"

#define TRISTATE_UART(reg,group) \
        TristateReg = NV_READ32(MISC_PA_BASE + APB_MISC_PP_TRISTATE_REG_##reg##_0); \
        TristateReg = NV_FLD_SET_DRF_DEF(APB_MISC_PP, TRISTATE_REG_##reg, \
        Z_##group, NORMAL, TristateReg); \
        NV_WRITE32(MISC_PA_BASE + APB_MISC_PP_TRISTATE_REG_##reg##_0, TristateReg);

#define PINMUX_UART(reg,group,uart) \
        PinMuxReg = NV_READ32(MISC_PA_BASE + APB_MISC_PP_PIN_MUX_CTL_##reg##_0); \
        PinMuxReg = NV_FLD_SET_DRF_DEF(APB_MISC_PP, PIN_MUX_CTL_##reg, group##_SEL, \
            UART##uart, PinMuxReg); \
        NV_WRITE32(MISC_PA_BASE + APB_MISC_PP_PIN_MUX_CTL_##reg##_0, PinMuxReg);

#define MISC_PA_BASE    0x70000000

NvError aos_EnableUartA (NvU32 configuration)
{
    NvError retval = NvSuccess;
    NvU32 TristateReg;
    NvU32 PinMuxReg;
    switch (configuration)
    {
        case NvOdmUartPinMap_Config1:
            TRISTATE_UART(B, UAA);
            TRISTATE_UART(B, UAB);
            PINMUX_UART(A, UAA, A);
            PINMUX_UART(A, UAB, A);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(A, GPU);
            PINMUX_UART(D, GPU, A);
            break;
        case NvOdmUartPinMap_Config3:
            TRISTATE_UART(A, IRRX);
            TRISTATE_UART(A, IRTX);
            TRISTATE_UART(B, UAD);
            PINMUX_UART(C, IRRX, A);
            PINMUX_UART(C, IRTX, A);
            PINMUX_UART(A, UAD, A);
            break;
        case NvOdmUartPinMap_Config4:
            TRISTATE_UART(A, IRRX);
            TRISTATE_UART(A, IRTX);
            PINMUX_UART(C, IRRX, A);
            PINMUX_UART(C, IRTX, A);
            break;
        case NvOdmUartPinMap_Config5:
            TRISTATE_UART(B, SDD);
            TRISTATE_UART(D, SDB);
            PINMUX_UART(D, SDD, A);
            PINMUX_UART(D, SDB, A);
            break;
        case NvOdmUartPinMap_Config6:
            TRISTATE_UART(B, UAA);
            PINMUX_UART(A, UAA, A);
            break;
        case NvOdmUartPinMap_Config7:
            TRISTATE_UART(A, SDIO1);
            PINMUX_UART(A, SDIO1, A);
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
            TRISTATE_UART(A, IRRX);
            TRISTATE_UART(A, IRTX);
            TRISTATE_UART(B, UAD);
            PINMUX_UART(C, IRRX, A);
            PINMUX_UART(C, IRTX, A);
            PINMUX_UART(A, UAD, A);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(B, UAD);
            PINMUX_UART(A, UAD, A);
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
            TRISTATE_UART(B, UCA);
            TRISTATE_UART(B, UCB);
            PINMUX_UART(B, UCA, C);
            PINMUX_UART(B, UCB, C);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(B, UCA);
            PINMUX_UART(B, UCA, C);
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
            TRISTATE_UART(D, UDA);
            PINMUX_UART(A, UDA, D);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(A, GMC);
            PINMUX_UART(B, GMC, D);
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
            TRISTATE_UART(A, SDIO1);
            PINMUX_UART(A, SDIO1, A);
            break;
        case NvOdmUartPinMap_Config2:
            TRISTATE_UART(A, GMA);
            PINMUX_UART(B, GMA, E);
            break;
        default:
            retval = NvError_BadParameter;
            break;
    }
    return retval;
}

