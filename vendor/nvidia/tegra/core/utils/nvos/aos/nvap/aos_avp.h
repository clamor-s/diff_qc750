/*
 * Copyright 2007-2011 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_AOS_AVP_H
#define INCLUDED_AOS_AVP_H

#include "nvcommon.h"
#include "aos.h"
#include "avp.h"

#define NVAOS_CLOCK_DEFAULT 5000
#define NVAOS_STACK_SIZE (1024 * 4)

//Some useful defines for the AVP interrupt API
typedef NvU32 IrqSource;

#if !__GNUC__
__asm void interruptHandler( void );
__asm void enableInterrupts( void );
NvU32 disableInterrupts( void );
void restoreInterrupts( NvU32 oldState );
__asm void waitForInterrupt( void );
__asm void undefHandler( void );
__asm void crashHandler( void );
__asm void cpuSetupModes( NvU32 stack );
__asm void threadSwitch( void );
__asm void threadSave( void );
__asm void threadSet( void );
__asm void suspend( void );
__asm void resume( void );
#else // !__GNUC__
NV_NAKED void interruptHandler( void );
void enableInterrupts( void );
NvU32 disableInterrupts( void );
void restoreInterrupts( NvU32 oldState );
NV_NAKED void waitForInterrupt( void );
NV_NAKED void undefHandler( void );
NV_NAKED void crashHandler( void );
NV_NAKED void threadSwitch( void );
NV_NAKED void threadSave( void );
NV_NAKED void threadSet( void );
NV_NAKED void suspend( void );
NV_NAKED void resume( void );
#endif // !__GNUC__

void ppiInterruptSetup( void );
void ppiTimerInit( void );
void ppiTimerHandler( void *context );

// FIXME: these names suck
void avpCacheEnable( void );
void restoreIram( void );
void saveIram( void );
NvU32 Src2Idx(IrqSource Irq);
NvU32 NvAvpPlatBitFind(NvU32 Word);
IrqSource IrqSourceFromIrq(NvU32 Irq);

#endif
