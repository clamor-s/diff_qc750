/*
 * Copyright (c) 2007-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_PANEL_SONYVGA_H
#define INCLUDED_PANEL_SONYVGA_H

#include "panels.h"

#if defined(__cplusplus)
extern "C"
{
#endif
#define SONY_VGA_GUID NV_ODM_GUID('f','f','a','_','l','c','d','0')

void sonyvga_GetHal( NvOdmDispDeviceHandle hDevice );

#if defined(__cplusplus)
}
#endif

#endif
