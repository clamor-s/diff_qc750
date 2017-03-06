/*
 * Copyright 2007-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_AOS_CPU_H
#define INCLUDED_AOS_CPU_H

#include "nvcommon.h"
#include "aos.h"
#include "cpu.h"

#define NVAOS_CLOCK_DEFAULT 10000
#define NVAOS_STACK_SIZE (1024 * 8)

NvU32 disableInterrupts( void );
void restoreInterrupts( NvU32 oldState );
NV_NAKED void interruptHandler( void );
NV_NAKED void enableInterrupts( void );
NV_NAKED void waitForInterrupt( void );
NV_NAKED void undefHandler( void );
NV_NAKED void crashHandler( void );
NV_NAKED void cpuSetupModes( NvU32 stack );
NV_NAKED void threadSwitch( void );
NV_NAKED void threadSave( void );
NV_NAKED void threadSet( void );
NV_NAKED void nvaosBreakPoint( void );

void ppiInterruptSetup( void );
void ppiTimerInit( void );
void ppiTimerHandler( void *context );

#endif
