/*
 * Copyright 2009-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_AOS_AP_H
#define INCLUDED_AOS_AP_H

#include "nvcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* AP-specific functions to initialize PMC scratch registers (including
 * BCT ODM option */
void nvaosAp20InitPmcScratch( void );
void nvaosT30InitPmcScratch( void );
/*
.* AP-specific functions to Read PMC scratch registers (including
 * BCT ODM Option)
 */
NvU32 nvaosAp20GetOdmData( void );
NvU32 nvaosT30GetOdmData( void );

#ifdef __cplusplus
}
#endif

#endif

