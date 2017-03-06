/*
 * Copyright (c) 2010 - 2012 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_MICROBOOT_PRIV_H
#define INCLUDED_MICROBOOT_PRIV_H

#include "nvcommon.h"
#include <stdarg.h>

#define NV_BIT_ADDRESS 0x40000000

void NvMicrobootClockInit(void);
void NvMicrobootClockSetDivider(NvBool Enable, NvU32 Dividened, NvU32 Divisor);

void NvMicrobootCacheInvalidate(void);
void NvMicrobootShowAvpClock(void);
void NvMicrobootLoop4Cycle(NvU32 Count);
void NvMicrobootJump(NvU32 Addr);
void NvMicrobootCPUPowerRailOn(void);
void NvMicroBootConfigPullupPullDown(void);

void NvMicrobootVprintf( const char *format, va_list ap );
void NvMicrobootUartInit(void);

#endif // INCLUDED_MICROBOOT_PRIV_H
