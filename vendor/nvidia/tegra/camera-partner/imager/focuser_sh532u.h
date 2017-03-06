/*
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef FOCUSER_H
#define FOCUSER_H

#include "nvodm_imager.h"
#include "nvodm_query_discovery_imager.h"
#include "sh532u.h"
#include "focuser_ad5820.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/*
  Most of these values are exposed from the driver up to the camera HAL.
*/

typedef struct sh532u_config  SH532U_PARAM;

NvBool FocuserSH532U_GetHal(NvOdmImagerHandle hImager);

typedef struct {
    int focuser_fd;
    NvOdmImagerPowerLevel PowerLevel;
    NvU32 cmdTime;             // time of latest focus command issued
    NvU32 settleTime;          // time of latest focus command issued
    NvU32 Position;            // The last settled focus position.
    NvU32 RequestedPosition;   // The last requested focus position.
    NvS32 DelayedFPos;         // Focusing position for delayed request
    SH532U_PARAM ModuleParam;
} FocuserCtxt;


#if defined(__cplusplus)
}
#endif

#endif  //FOCUSER_H
