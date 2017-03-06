/*
 * Copyright (c) 2007-2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#ifndef INCLUDED_NVIDL_H
#define INCLUDED_NVIDL_H

#include "../include/nvos.h"

// FIXME: should document this header.

typedef enum
{
    NvIdlValueType_Hex = 0,
    NvIdlValueType_Dec,

    NvIdlValueType_Num,
    NvIdlValueType_Force32 = 0x7FFFFFFF,
} NvIdlValueType;

typedef enum
{
    NvIdlPassType_In = 0,
    NvIdlPassType_Out,
    NvIdlPassType_InOut,

    NvIdlPassType_Num,
    NvIdlPassType_Force32 = 0x7FFFFFFF,
} NvIdlPassType;

typedef enum
{
    NvIdlRefType_None = 0,
    NvIdlRefType_Add,
    NvIdlRefType_Del,

    NvIdlRefType_Num,
    NvIdlRefType_Force32 = 0x7FFFFFFF,
} NvIdlRefType;

typedef enum
{
    NvIdlType_U8 = 0,
    NvIdlType_U16,
    NvIdlType_U32,
    NvIdlType_U64,
    NvIdlType_S8,
    NvIdlType_S16,
    NvIdlType_S32,
    NvIdlType_S64,
    NvIdlType_Bool,
    NvIdlType_Semaphore,
    NvIdlType_String,
    NvIdlType_Struct,
    NvIdlType_Enum,
    NvIdlType_Function,
    NvIdlType_Handle,
    NvIdlType_Error,
    NvIdlType_Void,
    NvIdlType_VoidPtr,
    NvIdlType_Custom,
    NvIdlType_ID,

    NvIdlType_Num,
    NvIdlType_Force32 = 0x7FFFFFFF,
} NvIdlType;

/** language handlers */

void
NvIdlPackageBegin( const char *name );
void
NvIdlPackageEntry( const char *name );
void
NvIdlPackageEnd( const char *name );
void
NvIdlPackage( const char *name );

NvError
NvIdlInterfaceBegin( const char *name );
void
NvIdlInterfaceEnd( const char *name );

void
NvIdlFunctionBegin( const char *name );
void
NvIdlFunctionEnd( void );
void
NvIdlFunctionParamPass( NvIdlPassType t );
void
NvIdlFunctionParam( const char *name );
void
NvIdlFunctionParamNext( void );
void
NvIdlParamAttrCount( const char *name );
void
NvIdlParamSetRef( NvIdlRefType t );
void
NvIdlSetFunctionDebug( NvBool b );

void
NvIdlStructBegin( const char *name );
void
NvIdlStructEnd( const char *name );
void
NvIdlStructField( const char *name );

void
NvIdlEnumBegin( const char *name );
void
NvIdlEnumEnd( const char *name );
void
NvIdlEnumField( const char *name, NvBool assign );

void
NvIdlHandle( const char *name );

void
NvIdlTypedef( const char *name );

void
NvIdlDefine( const char *name );

void
NvIdlImport( const char *name );

void
NvIdlVerbatim( const char c );

/** help api */

void NvIdlSetCurrentType( NvIdlType type );
void NvIdlSetCurrentId( const char *id );
void NvIdlSetCurrentValue( NvIdlValueType type, NvU32 val );
void NvIdlSetArraySize( NvU32 size );
void NvIdlSetConst( void );

/** entry point */

int
NvIdlMain( int argc, char *argv[] );

#endif
