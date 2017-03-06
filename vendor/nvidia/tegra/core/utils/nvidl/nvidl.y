%{
/*
 * Copyright (c) 2007-2008 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include "nvidl.h"
#include <stdlib.h> // FIXME: need exit()
#include <stdio.h>

extern void myfprintf(FILE *f, char *format, ...);
#define YYFPRINTF myfprintf

%}

%union {
    char *id;
    int val;
}

%token IMPORT PACKAGE INTERFACE STRUCT ENUM DEFINE TYPEDEF HANDLE
%token U8 U16 U32 U64 S8 S16 S32 S64 BOOL SEMAPHORE STRING VOID VOIDPTR ERROR
%token IN OUT INOUT COUNT O_BRACE C_BRACE O_PAREN C_PAREN O_BRACKET C_BRACKET
%token END EQUALS COMMA CONST NVDEBUG REFADD REFDEL

%token <id> ID FILE_ID
%token <val> VALUE

%%

top:
      package
    | interface
    | import
    | top package
    | top interface
    | top import
    ;

import: IMPORT FILE_ID END  { NvIdlImport( $2 ); }
    ;

package:
      PACKAGE ID O_BRACE { NvIdlPackageBegin( $2 ); } package_decls C_BRACE { NvIdlPackageEnd( $2 ); }
    | PACKAGE ID END { NvIdlPackage( $2 ); }
    ;

package_decls:
      package_decl
    | package_decls package_decl
    ;

package_decl:
      ID            { NvIdlPackageEntry( $1 ); }
    | ID COMMA      { NvIdlPackageEntry( $1 ); }
    ;

interface: INTERFACE ID { NvError err = NvIdlInterfaceBegin( $2 ); if( err != NvSuccess ) exit(0); } O_BRACE interface_decls C_BRACE { NvIdlInterfaceEnd( $2 ); }
    ;

interface_decls:
      interface_decl
    | interface_decls interface_decl
    ;

interface_decl:
      struct_decl
    | enum_decl
    | define_decl
    | handle_decl
    | typedef_decl
    | function_decl
    ;

basic_type:
      U8    { NvIdlSetCurrentType( NvIdlType_U8 ); }
    | U16   { NvIdlSetCurrentType( NvIdlType_U16 ); }
    | U32   { NvIdlSetCurrentType( NvIdlType_U32 ); }
    | U64   { NvIdlSetCurrentType( NvIdlType_U64 ); }
    | S8    { NvIdlSetCurrentType( NvIdlType_S8 ); }
    | S16   { NvIdlSetCurrentType( NvIdlType_S16 ); }
    | S32   { NvIdlSetCurrentType( NvIdlType_S32 ); }
    | S64   { NvIdlSetCurrentType( NvIdlType_S64 ); }
    | BOOL  { NvIdlSetCurrentType( NvIdlType_Bool ); }
    | ERROR { NvIdlSetCurrentType( NvIdlType_Error ); }
    ;

complex_type:
      basic_type
    | STRING { NvIdlSetCurrentType( NvIdlType_String ); }
    | ID { NvIdlSetCurrentType( NvIdlType_ID ); NvIdlSetCurrentId( $1 ); }
    ;

define_decl: DEFINE ID VALUE    { NvIdlDefine( $2 ); }
    ;

handle_decl: HANDLE ID END      { NvIdlHandle( $2 ); }
    ;

typedef_decl: TYPEDEF typedef_type ID END   { NvIdlTypedef( $3 ); }
    ;

typedef_type:
      STRUCT    { NvIdlSetCurrentType( NvIdlType_Struct ); }
    | ENUM      { NvIdlSetCurrentType( NvIdlType_Enum ); }
    | basic_type
    ;

struct_decl: STRUCT ID { NvIdlStructBegin( $2 ); } O_BRACE struct_field_decls C_BRACE { NvIdlStructEnd( $2 ); }
    ;

struct_field_decls:
      struct_field_decl
    | struct_field_decls struct_field_decl
    ;

struct_field_decl:
      struct_type ID END { NvIdlStructField( $2 ); }
    | struct_type ID O_BRACKET VALUE C_BRACKET END { NvIdlSetArraySize( $4 ); NvIdlStructField( $2 ); }
    | define_decl
    ;

struct_type:
      complex_type
    | VOIDPTR   { NvIdlSetCurrentType( NvIdlType_VoidPtr ); }
    ;

enum_decl: ENUM ID { NvIdlEnumBegin( $2 ); } O_BRACE enum_field_decls C_BRACE { NvIdlEnumEnd( $2 ); }
    ;

enum_field_decls:
      enum_field_decl
    | enum_field_decls enum_field_decl
    ;

enum_field_decl:
      ID COMMA                  { NvIdlEnumField( $1, NV_FALSE ); }
    | ID EQUALS VALUE COMMA     { NvIdlEnumField( $1, NV_TRUE ); }
    | define_decl
    ;

function_decl: func_debug func_ret_type ID { NvIdlFunctionBegin( $3 ); } O_PAREN func_args C_PAREN END { NvIdlFunctionEnd(); }
    ;

func_debug:
      NVDEBUG       { NvIdlSetFunctionDebug( NV_TRUE ); }
    |
    ;

func_ret_type:
      complex_type
    | VOID          { NvIdlSetCurrentType( NvIdlType_Void ); }
    ;

func_args:
      VOID
    | func_arg
    | func_args COMMA { NvIdlFunctionParamNext(); } func_arg
    ;

func_arg:
      param_pass const func_type ID { NvIdlFunctionParam( $4 ); }
    ;

func_type:
      complex_type
    | SEMAPHORE { NvIdlSetCurrentType( NvIdlType_Semaphore ); }
    | VOIDPTR   { NvIdlSetCurrentType( NvIdlType_VoidPtr ); }
    ;

param_pass:
      O_BRACKET pass_type param_attribute C_BRACKET
    ;

pass_type:
      IN        { NvIdlFunctionParamPass( NvIdlPassType_In ); }
    | INOUT     { NvIdlFunctionParamPass( NvIdlPassType_InOut ); }
    | OUT       { NvIdlFunctionParamPass( NvIdlPassType_Out ); }

param_attribute:
      COMMA count_attribute
    | COMMA REFADD { NvIdlParamSetRef( NvIdlRefType_Add ); }
    | COMMA REFDEL { NvIdlParamSetRef( NvIdlRefType_Del ); }
    |
    ;

count_attribute:
      COUNT O_PAREN ID C_PAREN { NvIdlParamAttrCount( $3 ); }
    ;

const:
      CONST     { NvIdlSetConst(); }
    |
    ;

%%

int main( int argc, char *argv[] )
{
    return NvIdlMain( argc, argv );
}
