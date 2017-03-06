/*
 * Copyright (c) 2010-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_FOCUSER_T8EV5_H
#define INCLUDED_FOCUSER_T8EV5_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include "nvodm_imager.h"

#define T8EV5_IOCTL_GET_CONFIG   _IOR('o', 0xa, struct t8ev5_config)
#define T8EV5_IOCTL_SET_POSITION _IOW('o', 0xb, NvU32)

struct t8ev5_config
{
    NvU32 settle_time;
    NvU32 actuator_range;
    NvU32 pos_low;
    NvU32 pos_high;
	NvU32 focus_macro;
    NvU32 focus_infinity;
    float focal_length;
    float fnumber;
    float max_aperture;
};

NvBool Focuser_T8ev5_GetHal(NvOdmImagerHandle hImager);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif // INCLUDED_FOCUSER_AD5820_H
