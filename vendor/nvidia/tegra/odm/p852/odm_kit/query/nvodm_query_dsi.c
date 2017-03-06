/*
 * Copyright (c) 2007-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

/**
 * @file
 * @brief <b>NVIDIA Driver Development Kit:
 *                  ODM Dsi interface</b>
 *
 */

#include "nvodm_dsi.h"

static const NvOdmQueryDsiInfo s_NvOdmQueryDsiConfiguration =
{
    NvOdmDsiDataFormat_24BitP,      //DSI color format
    NvOdmDsiVideoModeType_NonBurst, //DSI Video mode
    NvOdmDsiVirtualChannel_0,       //DSI virtual channel number
    1                               //DSI number of data lanes
};

const NvOdmQueryDsiInfo * NvOdmQueryDsiConfiguration(void)
{
    return &s_NvOdmQueryDsiConfiguration;
}


