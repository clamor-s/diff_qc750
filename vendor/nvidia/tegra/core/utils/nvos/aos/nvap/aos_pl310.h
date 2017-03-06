/*
 * Copyright 2008 - 2009 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */

#ifndef INCLUDED_AOS_PL310_H
#define INCLUDED_AOS_PL310_H

#include "nvcommon.h"
#include "ap20/arpl310.h"

#define PL310_BASE              0x50043000
#define PL310_CACHE_LINE_SIZE   32

/* Pl310 is physically tagged and physically index cache. But, in AOS virtual
 * address == physical address,
 * so there is no need to convert to physical addresses, in any of the
 * cache operations.
 */

#define NV_PL310_REGR(reg)      AOS_REGR(PL310_BASE + PL310_##reg##_0)
#define NV_PL310_REGW(reg, val) AOS_REGW((PL310_BASE + PL310_##reg##_0), (val))

#define nvaosPl310Sync()        NV_PL310_REGW(CACHE_SYNC, 0);

void
nvaosPl310Init( void );

void
nvaosPl310Enable( void );

void
nvaosPl310InvalidateDisable( void );

void
nvaosPl310Writeback( void );

void
nvaosPl310WritebackInvalidate( void );

void
nvaosPL310WritebackRange( void *start, NvU32 TotalLength );

void
nvaosPl310WritebackInvalidateRange( void *start, NvU32 TotalLength );

#endif // INCLUDED_AOS_PL310_H

