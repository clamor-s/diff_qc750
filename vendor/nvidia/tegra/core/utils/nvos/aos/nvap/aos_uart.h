/*
.* Copyright (c) 2010-2012, NVIDIA CORPORATION.  All rights reserved.
.*
.* NVIDIA Corporation and its licensors retain all intellectual property and
.* proprietary rights in and to this software and related documentation.  Any
.* use, reproduction, disclosure or distribution of this software and related
.* documentation without an express license agreement from NVIDIA Corporation
.* is strictly prohibited.
 */

/**
 * @file
 * <b>NVIDIA Tegra Aos API</b>
 *
 */

#ifndef INCLUDED_AOS_UART_H
#define INCLUDED_AOS_UART_H

/**
 * @defgroup aos_debug_group aos Debug API
 *
 *
 *
 *
 * @ingroup aos_group
 * @{
 */
#include "nvcommon.h"
#include "nverror.h"
#include "nvassert.h"
#include "nvrm_hardware_access.h"

#define NV_CAR_REGR(pCar, reg)              NV_READ32( (((NvUPtr)(pCar)) + CLK_RST_CONTROLLER_##reg##_0))
#define NV_CAR_REGW(pCar, reg, val)         NV_WRITE32((((NvUPtr)(pCar)) + CLK_RST_CONTROLLER_##reg##_0), (val))
#define NV_UART_REGR(pBase, reg)                NV_READ32( (((NvUPtr)(pBase)) + UART_##reg##_0))
#define NV_UART_REGW(pBase, reg, val)           NV_WRITE32((((NvUPtr)(pBase)) + UART_##reg##_0), (val))
#define NV_FLOW_REGR(pFlow, reg)        NV_READ32((((NvUPtr)(pFlow)) + FLOW_CTLR_##reg##_0))
#define NV_FLOW_REGW(pFlow, reg, val)   NV_WRITE32((((NvUPtr)(pFlow)) + FLOW_CTLR_##reg##_0), (val))

#define AOS_DEFAULT_UART_BAUD 115200
// Ensure that CONFIG_PLLP_BASE_AS_408MHZ is defined.
extern NvU32 ct_assert_config_pllp[CONFIG_PLLP_BASE_AS_408MHZ?1:1];
// fixed PLLP output frequency in kHz (override enabled)
#if CONFIG_PLLP_BASE_AS_408MHZ
#define AOS_PLLP_FIXED_FREQ_KHZ (408000)
#else
#define AOS_PLLP_FIXED_FREQ_KHZ (216000)
#endif

#ifndef SET_KERNEL_PINMUX
typedef struct UartInterfaceRec
{
    volatile void* (*paos_UartInit)(NvU32, NvU32);
    void (*paos_UartWriteByte)(volatile void*, NvU8);
}UartInterface;
#endif

/**
 * @brief Initializes debug services and the resource manager.
 *
 * @retval ::NvError_Success
 * @retval ::NvError_NotInitialized
 */
NvError aosDebugInit(void);

/**
 * @brief Returns whether or not the debug port is initialized.
 *
 * @returns NV_TRUE if debug has been initialized, NV_FALSE otherwise.
 */
NvBool aosDebugIsInitialized(void);

/**
 * @brief Writes a byte to the debug port.
 *
 * @param str A pointer to a string.
 */
void aosWriteDebugString(const char* str);

/**
 * @brief Gets the default debug serial baud rate.
 *
 * @returns The debug console port serial baud rate.
 */
NvU32 aosDebugGetDefaultBaudRate(void);

/** @} */

NvU32 GetMajorVersion(void);

#ifndef SET_KERNEL_PINMUX
NvU32 NvUartGetChipID(void);
#endif

void aos_UartWriteByte (volatile void* pUart, NvU8 ch);

volatile void* aos_UartInit (NvU32 portNumber, NvU32 baud_rate);

#ifndef SET_KERNEL_PINMUX
NvError aos_EnableUartA(NvU32 configuration);

NvError aos_EnableUartB(NvU32 configuration);

NvError aos_EnableUartC(NvU32 configuration);

NvError aos_EnableUartD(NvU32 configuration);

NvError aos_EnableUartE(NvU32 configuration);
#endif

#endif  // INCLUDED_AOS_UART_H

