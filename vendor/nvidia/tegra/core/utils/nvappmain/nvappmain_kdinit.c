/*
 * Copyright (c) 2007 - 2011 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

//###########################################################################
//############################### INCLUDES ##################################
//###########################################################################

#include "nvappmain.h"
#include "KD/NV_initialize.h"

//###########################################################################
//############################### CODE ######################################
//###########################################################################

//===========================================================================
// main() - entry point
//===========================================================================
int main(int argc, char *argv[])
{
    int ret;

    kdInitializeNV();
    ret = (NvAppMain(argc, argv) == NvSuccess) ? 0 : 1;
    kdTerminateNV();

    return ret;
}
