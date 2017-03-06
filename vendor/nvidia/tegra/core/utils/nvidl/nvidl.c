/*
 * Copyright (c) 2009 - 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nvidl.h"
#include "nvidl.tab.h"
#include <stdlib.h>

#define MAX_INCLUDES 256
#define PATH_MAX 1024

#define FAIL( msg ) \
    printf("idl error: "); \
    printf msg; \
    printf("\n"); \
    exit( 1 );

extern int linenum;
extern int yylex( void );
extern void yyerror( char *msg );

int g_error = 0;

static void NvIdlGenerateStubs( const char *name );
static void NvIdlGenerateDispatcher( const char *name );
static void NvIdlGeneratePackageDispatcher( const char *name );
static void NvIdlGeneratePackageNumbers( FILE* file );
static void NvIdlSetImportMode( NvBool b );
static void NvIdlClearState( void );

typedef struct PackageEntry_t
{
    char *name;
    struct PackageEntry_t *next;
} PackageEntry;

typedef struct Package_t
{
    char *name;
    PackageEntry *entries;
    struct Package_t *next;
} Package;

typedef struct Field_t
{
    char *name;
    NvIdlType type;
    struct Field_t *next;
} Field;

typedef struct Value_t
{
    NvIdlValueType type;
    NvU32 val;
    struct Value_t *next;
} Value;

typedef struct Param_t
{
    NvIdlPassType pass;
    NvIdlType type;
    NvIdlRefType ref;
    char *typename; /* for structs and handles */
    char *name;
    char *attr_count;
    /* If attr_count is present, pass must be NvIdlPassType_In, and the actual
     * pass type is stored in count_pass. */
    NvIdlPassType count_pass;
    NvBool is_const;
    struct Param_t *next;
} Param;

typedef struct Function_t
{
    NvIdlType rettype;
    char *ret_typename; /* for struct, etc. return types */
    Param *params;
    NvU32 func_id;
    NvBool return_data; /* return is non-void, or has inout/out parameters */
    NvBool has_inout;
    NvBool has_out;
    NvBool isdebug;
} Function;

typedef struct Struct_t
{
    Field *fields;
    NvU32 size;
    NvU32 defined;
} Struct;

typedef struct Symbol_t
{
    char *name;
    NvIdlType type;
    void *data;
    struct Symbol_t *next;
} Symbol;

typedef struct Include_t
{
    char *name;
    struct Include_t *next;
} Include;

typedef enum
{
    GeneratorMode_Unknown = 0,
    GeneratorMode_GlobalDispatch,
    GeneratorMode_Header,
    GeneratorMode_Stub,
    GeneratorMode_Dispatch,

    GeneratorMode_ForceWord = 0x7FFFFFFF,
} GeneratorMode;

static Symbol *SymbolTable;
static Symbol *gs_CurrentSym;
static const char *gs_CurrentId;
static NvU32 gs_ArraySize;
static char *gs_ParamAttrCount;
static NvIdlPassType gs_ParamAttrCountPassType;
static NvBool gs_IsConst = NV_FALSE;
static NvIdlType gs_CurrentType;
static NvIdlPassType gs_CurrentPassType;
static NvIdlRefType gs_CurrentRefType;
static Value gs_CurrentValue;
static NvU32 gs_FunctionId;
static char *gs_InterfaceName;
static char *gs_PackageName;
static Package *gs_Packages;
static Include *gs_Includes;
static NvBool gs_FunctionDebug = NV_FALSE;
static NvU32 gs_indent;

static FILE* gs_HeaderFile;
static FILE* gs_StubFile;
static FILE* gs_DispatchFile;
static char *gs_OutputFileName;

static char *gs_IncludePaths[MAX_INCLUDES];
static int  gs_NumberIncludes;

static Symbol *ExternSymbolTable;
NvBool gs_ImportMode = NV_FALSE;
NvBool gs_TopFile = NV_TRUE;

static GeneratorMode gs_GeneratorMode = GeneratorMode_Unknown;
static NvBool gs_UserToUser = NV_FALSE;
static NvBool gs_UserToKMod = NV_FALSE;
static NvBool gs_UserToKernel = NV_FALSE;
static NvBool gs_DebugCode = NV_FALSE;
static NvBool gs_NullDispatch = NV_FALSE;
static NvBool gs_GeneratedFile = NV_FALSE;
static char *gs_DispatchType = "";
static char *gs_TargetOs = "";

static Symbol *
symbol_lookup( const char *name )
{
    Symbol *s;

    s = SymbolTable;
    while( s )
    {
        if( strcmp( name, s->name ) == 0 )
        {
            return s;
        }

        s = s->next;
    }

    s = ExternSymbolTable;
    while( s )
    {
        if( strcmp( name, s->name ) == 0 )
        {
            return s;
        }

        s = s->next;
    }

    return 0;
}

static void
symbol_add( const char *name, Symbol *symbol )
{
    if( gs_TopFile && gs_ImportMode )
    {
        /* don't add the symbols from the target file in import mode, they
         * will be added during the generation pass.
         */
        return;
    }

    if( symbol_lookup( name ) )
    {
        FAIL(( "symbol already defined: %s\n", name ));
    }

    if( gs_ImportMode )
    {
        symbol->next = ExternSymbolTable;
        ExternSymbolTable = symbol;

        //printf( "importing symbol: %s\n", name );
    }
    else
    {
        symbol->next = SymbolTable;
        SymbolTable = symbol;

        //printf( "symbol: %s\n", name );
    }
}

static Symbol *
symbol_create( void )
{
    Symbol *sym;

    sym = (Symbol *)malloc( sizeof(Symbol) );
    if( !sym )
    {
        FAIL(( "allocation failure" ));
    }

    memset( sym, 0, sizeof(Symbol) );
    return sym;
}

// In case of read mode, look for idl files in the given include
// list before trying the current working directory.
// use_include_path = 0 means do not append include path
// use_include_path = 1 means append include path
FILE *myfopen(char *fname, char *mode,int use_include_path)
{
    FILE *fp;
    int  IncludeIndex;
    char path[PATH_MAX];

    if (strcmp(mode, "r") || !use_include_path)
    {
        fp = fopen(fname, mode);
        if (fp)
            return fp;
        else
            return NULL;
    }

    for (IncludeIndex = 0; IncludeIndex < gs_NumberIncludes; ++IncludeIndex)
    {
        sprintf(path, "%s/%s", gs_IncludePaths[IncludeIndex], fname);
        fp = fopen(path, mode);
        if (fp)
            return fp;
    }

    if (!strcmp(mode, "r"))
    {
        fp = fopen(fname, mode);
        if (fp)
            return fp;
    }
    return NULL;
}

static void myfclose(FILE *f)
{
    if (!f)
    {
        return;
    }

    fclose(f);
}

static void myfprintf(FILE *f, char *format, ...)
{
    va_list ap;

    if (!f)
    {
        return;
    }

    va_start(ap, format);
    vfprintf(f, format, ap);
    va_end(ap);
}

#define EMIT( x ) \
    do { \
        if( gs_ImportMode == NV_FALSE ) \
        { \
            myfprintf x; \
        } \
    } \
    while( 0 )

static void
emit_copyright( FILE* f )
{
    // FIXME: handle current year
    EMIT(( f,
"/*\n"
" * Copyright (c) 2009 NVIDIA Corporation.  All rights reserved.\n"
" *\n"
" * NVIDIA Corporation and its licensors retain all intellectual property\n"
" * and proprietary rights in and to this software, related documentation\n"
" * and any modifications thereto.  Any use, reproduction, disclosure or\n"
" * distribution of this software and related documentation without an express\n"
" * license agreement from NVIDIA Corporation is strictly prohibited.\n"
" */\n" ));
}

static void
NvIdlClearState( void )
{
    gs_CurrentSym = 0;
    gs_CurrentId = 0;
    gs_CurrentType = 0;
    gs_CurrentPassType = 0;
    gs_CurrentRefType = NvIdlRefType_None;
    gs_CurrentValue.type = 0;
    gs_CurrentValue.val = 0;
    gs_FunctionId = 0;
    gs_ArraySize = 0;
    gs_ParamAttrCount = 0;
    gs_ParamAttrCountPassType = 0;
    gs_IsConst = NV_FALSE;
    gs_FunctionDebug = NV_FALSE;
}

static void
NvIdlSetImportMode( NvBool b )
{
    gs_ImportMode = b;

    NvIdlClearState();
}

void
NvIdlSetCurrentType( NvIdlType type )
{
    gs_CurrentType = type;
}

void
NvIdlSetCurrentId( const char *id )
{
    gs_CurrentId = id;
}

void
NvIdlSetCurrentValue( NvIdlValueType type, NvU32 val )
{
    gs_CurrentValue.type = type;
    gs_CurrentValue.val = val;
}

void
NvIdlSetArraySize( NvU32 size )
{
    gs_ArraySize = size;
}

void
NvIdlSetConst( void )
{
    gs_IsConst = NV_TRUE;
}

void
NvIdlPackageBegin( const char *name )
{
    Package *p;

    if( !gs_ImportMode )
    {
        return;
    }

    p = (Package *)malloc( sizeof(Package) );
    if( !p )
    {
        FAIL(( "allocation failure" ));
    }

    memset( p, 0, sizeof(Package) );

    p->name = (char *)name;
    p->entries = 0;
    p->next = gs_Packages;
    gs_Packages = p;
}

void
NvIdlPackageEnd( const char *name )
{
    if( gs_ImportMode )
    {
        return;
    }

    if( gs_GeneratorMode != GeneratorMode_GlobalDispatch )
    {
        return;
    }

    NvIdlGeneratePackageDispatcher( name );
}

void
NvIdlPackageEntry( const char *name )
{
    PackageEntry *e;
    Package *p = gs_Packages;

    if( !gs_ImportMode )
    {
        return;
    }

    e = (PackageEntry *)malloc( sizeof(PackageEntry) );
    if( !e )
    {
        FAIL(( "allocation failure" ));
    }

    memset( e, 0, sizeof(PackageEntry) );

    e->name = (char *)name;
    e->next = p->entries;
    p->entries = e;
}

void
NvIdlPackage( const char *name )
{
    if( gs_ImportMode )
    {
        return;
    }

    gs_PackageName = (char *)name;
}

NvError
NvIdlInterfaceBegin( const char *name )
{
    NvU32 flags;
    Include *i;
    char buff[PATH_MAX];

    if( gs_TopFile )
    {
        return NvSuccess;
    }

    if( gs_ImportMode )
    {
        Include *i;
        i = malloc( sizeof(Include) );
        if( !i )
        {
            FAIL(( "allocation failure" ));
        }

        memset( i, 0, sizeof(Include) );

        i->name = (char *)name;
        i->next = gs_Includes;
        gs_Includes = i;
        return NvSuccess;
    }

    gs_InterfaceName = (char *)name;
    gs_FunctionId = 0;

    if (gs_GeneratorMode == GeneratorMode_Header)
    {
        sprintf( buff, "%s.h", name );
        gs_HeaderFile = myfopen( (gs_OutputFileName == NULL) ? buff :
            gs_OutputFileName, "w", 0);
        if (gs_HeaderFile == NULL)
        {
            goto fail;
        }
        gs_GeneratedFile = NV_TRUE;
    }

    if (gs_GeneratorMode == GeneratorMode_Stub)
    {
        sprintf( buff, "%s_stub.c", name );
        gs_StubFile = myfopen( (gs_OutputFileName == NULL) ?  buff :
            gs_OutputFileName, "w", 0);
        if (gs_StubFile == NULL )
        {
            goto fail;
        }
        gs_GeneratedFile = NV_TRUE;
    }

    if (gs_GeneratorMode == GeneratorMode_Dispatch)
    {
        sprintf( buff, "%s_dispatch.c", name );
        gs_DispatchFile = myfopen( (gs_OutputFileName == NULL) ?  buff :
            gs_OutputFileName, "w", 0);
        if (gs_DispatchFile == NULL)
        {
            goto fail;
        }
        gs_GeneratedFile = NV_TRUE;
    }

    /* blast in the copyright and include guards */
    emit_copyright( gs_HeaderFile );
    EMIT(( gs_HeaderFile, "\n"
        "#ifndef INCLUDED_%s_H\n"
        "#define INCLUDED_%s_H\n\n"
        "\n"
        "#if defined(__cplusplus)\n"
        "extern \"C\"\n"
        "{\n"
        "#endif\n"
        "\n",
        name, name ));

    /* define NV_IDL_IS_STUB for stub file */
    EMIT(( gs_StubFile, "\n"
        "#define NV_IDL_IS_STUB\n\n" ));

    /* define NV_IDL_IS_DISPATCH for dipatch file */
    EMIT(( gs_DispatchFile, "\n"
        "#define NV_IDL_IS_DISPATCH\n\n" ));

    /* need to include the imported files */
    i = gs_Includes;
    while( i )
    {
        sprintf( buff, "%s.h", i->name );
        EMIT(( gs_HeaderFile, "#include \"%s\"\n", buff ));
        i = i->next;
    }

    return NvSuccess;

fail:
    myfclose( gs_HeaderFile );
    myfclose( gs_StubFile );
    myfclose( gs_DispatchFile );

    return NvError_BadParameter;
}

void
NvIdlInterfaceEnd( const char *name )
{
    Symbol *sym;

    if( gs_ImportMode )
    {
        return;
    }

    /* end the header guards */
    EMIT(( gs_HeaderFile,
        "\n#if defined(__cplusplus)\n"
        "}\n"
        "#endif\n"
        "\n#endif\n" ));

    if( !gs_PackageName )
    {
        FAIL(( "no package name defined\n" ));
    }

    NvIdlGenerateStubs( name );
    NvIdlGenerateDispatcher( name );

    myfclose( gs_HeaderFile );
    myfclose( gs_StubFile );
    myfclose( gs_DispatchFile );
}

static Symbol *
create_function( const char *name )
{
    Symbol *sym;
    Function *f;

    sym = symbol_create();
    sym->type = NvIdlType_Function;
    sym->name = (char *)name;

    f = (Function *)malloc( sizeof(Function) );
    if( !f )
    {
        FAIL(( "allocation failure" ));
    }

    memset( f, 0, sizeof(Function) );

    f->params = 0;
    f->func_id = gs_FunctionId++;
    f->return_data = NV_FALSE;
    f->has_inout = NV_FALSE;
    f->has_out = NV_FALSE;
    f->isdebug = NV_FALSE;
    sym->data = f;

    return sym;
}

static Symbol *
create_struct( const char *name )
{
    Symbol *sym;
    Struct *s;

    sym = symbol_create();
    sym->type = NvIdlType_Struct;
    sym->name = (char *)name;

    s = (Struct *)malloc( sizeof(Struct) );
    if( !s )
    {
        FAIL(( "allocation failure" ));
    }

    memset( s, 0, sizeof(Struct) );

    s->defined = 0;
    s->fields = 0;
    s->size = 0;
    sym->data = s;

    return sym;
}

static Symbol *
create_enum( const char *name )
{
    Symbol *sym;

    sym = symbol_create();
    sym->type = NvIdlType_Enum;
    sym->name = (char *)name;

    return sym;
}

static void
emit_type( FILE* file, NvIdlType type )
{
    Symbol *sym = 0;

    switch( type ) {
    case NvIdlType_U8:
        EMIT(( file, "NvU8 " )); break;
    case NvIdlType_U16:
        EMIT(( file, "NvU16 " )); break;
    case NvIdlType_U32:
        EMIT(( file, "NvU32 " )); break;
    case NvIdlType_U64:
        EMIT(( file, "NvU64 " )); break;
    case NvIdlType_S8:
        EMIT(( file, "NvS8 " )); break;
    case NvIdlType_S16:
        EMIT(( file, "NvS16 " )); break;
    case NvIdlType_S32:
        EMIT(( file, "NvS32 " )); break;
    case NvIdlType_S64:
        EMIT(( file, "NvS64 " )); break;
    case NvIdlType_Bool:
        EMIT(( file, "NvBool " )); break;
    case NvIdlType_Semaphore:
        EMIT(( file, "NvOsSemaphoreHandle " )); break;
    case NvIdlType_String:
        EMIT(( file, "char * " )); break;
    case NvIdlType_Error:
        EMIT(( file, "NvError " )); break;
    case NvIdlType_Void:
        EMIT(( file, "void " )); break;
    case NvIdlType_VoidPtr:
        EMIT(( file, "void* " )); break;
    case NvIdlType_ID:
    {
        /* lookup the symbol */
        sym = symbol_lookup( gs_CurrentId );
        if( !sym )
        {
            FAIL(( "symbol not found: %s\n", gs_CurrentId ));
        }
        if( sym->type == NvIdlType_Struct )
        {
            Struct *s;
            s = (Struct *)sym->data;
            if( s->defined )
            {
                EMIT(( file, "%s ", gs_CurrentId ));
            }
            else
            {
                EMIT(( file, "struct %sRec ", gs_CurrentId ));
            }
        }
        else
        {
            EMIT(( file, "%s ", gs_CurrentId ));
        }
        break;
    }
    default:
        return;
    }
}

static void
emit_default_retval( FILE* file, Function *func )
{
    switch( func->rettype ) {
    case NvIdlType_U8:
    case NvIdlType_U16:
    case NvIdlType_U32:
    case NvIdlType_U64:
    case NvIdlType_S8:
    case NvIdlType_S16:
    case NvIdlType_S32:
    case NvIdlType_S64:
        EMIT(( file, "0" )); break;
    case NvIdlType_Bool:
        EMIT(( file, "NV_FALSE" )); break;
    case NvIdlType_Error:
        EMIT(( file, "NvError_NotSupported" )); break;
    case NvIdlType_Semaphore:
    case NvIdlType_String:
    case NvIdlType_Handle:
    case NvIdlType_VoidPtr:
        EMIT(( file, "NULL" )); break;
    case NvIdlType_Struct:
    case NvIdlType_Enum:
    case NvIdlType_Custom:
        EMIT(( file, func->ret_typename ));
        EMIT(( file, " ret; return ret;" ));
        break;
    default:
        FAIL(("Unhandled function return type!"));
    }
}

void
NvIdlSetFunctionDebug( NvBool b )
{
    gs_FunctionDebug = b;
}

void
NvIdlFunctionBegin( const char *name )
{
    Symbol *sym;
    Function *f;

    if( gs_TopFile && gs_ImportMode ) return;

    sym = create_function( name );
    symbol_add( name, sym );
    gs_CurrentSym = sym;
    gs_indent = 0;

    f = (Function *)sym->data;
    f->ret_typename = 0;
    f->rettype = gs_CurrentType;
    if( f->rettype == NvIdlType_ID )
    {
        Symbol *sym;
        sym = symbol_lookup( gs_CurrentId );
        if( !sym )
        {
            FAIL(( "symbol not found: %s\n", gs_CurrentId ));
        }

        f->ret_typename = (char *)gs_CurrentId;
        f->rettype = sym->type;
    }

    if( f->rettype != NvIdlType_Void )
    {
        f->return_data = NV_TRUE;
    }

    f->isdebug = gs_FunctionDebug;
    gs_FunctionDebug = NV_FALSE;

    EMIT(( gs_HeaderFile, "\n%*s", gs_indent, " " ));
    emit_type( gs_HeaderFile, gs_CurrentType );
    EMIT(( gs_HeaderFile, "%s( \n    ", name ));
}

void
NvIdlFunctionEnd( void )
{
    Function *f;

    if( gs_TopFile && gs_ImportMode ) return;

    f = (Function *)gs_CurrentSym->data;
    if( !f->params )
    {
        EMIT(( gs_HeaderFile, "void " ));
    }

    EMIT(( gs_HeaderFile, " );\n" ));
}

void
NvIdlFunctionParamPass( NvIdlPassType t )
{
    if( gs_TopFile && gs_ImportMode ) return;

    gs_CurrentPassType = t;
}

static void
emit_param_type( FILE *file, Param *p, NvBool basetype )
{
    if( p->typename )
    {
        myfprintf( file, "%s ", p->typename );
    }
    else
    {
        emit_type( file, p->type );
    }

    if( basetype )
    {
        return;
    }

    if( p->type == NvIdlType_VoidPtr )
    {
        if( p->attr_count == 0 && ( p->pass == NvIdlPassType_Out ) )
        {
            EMIT(( file, "* " ));
        }
    }
    else
    {
        if( p->pass == NvIdlPassType_InOut ||
            p->pass == NvIdlPassType_Out ||
            p->attr_count ||
            p->type == NvIdlType_Struct )
        {
            EMIT(( file, "* " ));
        }
    }
}

void
NvIdlFunctionParam( const char *name )
{
    Function *f;
    Param *p;
    Param *tmp;
    Symbol *sym = 0;

    if( gs_TopFile && gs_ImportMode ) return;

    f = (Function *)gs_CurrentSym->data;

    if( gs_CurrentType == NvIdlType_ID )
    {
        sym = symbol_lookup( gs_CurrentId );
        if( !sym )
        {
            FAIL(( "symbol not found: %s\n", gs_CurrentId ));
        }
    }

    p = (Param *)malloc( sizeof(Param) );
    if( !p )
    {
        FAIL(( "allocation failure" ));
    }

    memset( p, 0, sizeof(Param) );

    p->name = (char *)name;
    p->pass = gs_CurrentPassType;
    p->typename = 0;
    p->attr_count = 0;
    p->next = 0;

    if( p->pass == NvIdlPassType_InOut )
    {
        f->has_inout = NV_TRUE;
    }
    else if( p->pass == NvIdlPassType_Out )
    {
        f->has_out = NV_TRUE;
    }

    if( gs_IsConst )
    {
        p->is_const = NV_TRUE;
        gs_IsConst = NV_FALSE;
    }

    /* store reference tracking information */
    p->ref = gs_CurrentRefType;
    gs_CurrentRefType = NvIdlRefType_None;
    
    /* this parameter is actually a pointer to the given type.
     * the size in bytes is given in a separate parameter, named by
     * p->attr_count.
     */
    if( gs_ParamAttrCount )
    {
        p->attr_count = gs_ParamAttrCount;
        p->count_pass = gs_ParamAttrCountPassType;
        gs_ParamAttrCount = 0;
    }

    if( p->pass == NvIdlPassType_InOut ||
        p->pass == NvIdlPassType_Out ||
        p->count_pass == NvIdlPassType_InOut ||
        p->count_pass == NvIdlPassType_Out )
    {
        f->return_data = NV_TRUE;
    }

    /* need to track the typename for structs, etc.
     */
    if( sym && ( sym->type == NvIdlType_Struct ||
                 sym->type == NvIdlType_Handle ||
                 sym->type == NvIdlType_Custom ||
                 sym->type == NvIdlType_Enum ) )
    {
        p->typename = (char *)gs_CurrentId;
        p->type = sym->type; 
    }
    else
    {
        p->type = gs_CurrentType;
    }

    /* validate the tracked handle and set destructor */
    if( p->ref != NvIdlRefType_None )
    {
        if ( p->type != NvIdlType_Handle )
        {
            FAIL(( "Referencing invalid handle" ));
        }
    }
    
    /* add the param to the end of parameter list */
    tmp = f->params;
    if( !tmp )
    {
        f->params = p;
    }
    else
    {
        while( tmp->next ) tmp = tmp->next;
        tmp->next = p;
    }

    if( p->is_const )
    {
        EMIT(( gs_HeaderFile, "const " ));
    }

    emit_param_type( gs_HeaderFile, p, NV_FALSE );

    EMIT(( gs_HeaderFile, "%s", name ));
}

void
NvIdlParamAttrCount( const char *name )
{
    if( gs_TopFile && gs_ImportMode ) return;

    gs_ParamAttrCount = (char *)name;
    gs_ParamAttrCountPassType = gs_CurrentPassType;
    gs_CurrentPassType = NvIdlPassType_In;
}

void
NvIdlParamSetRef( NvIdlRefType t )
{
    if( gs_TopFile && gs_ImportMode ) return;

    gs_CurrentRefType = t;
}

void
NvIdlFunctionParamNext( void )
{
    if( gs_TopFile && gs_ImportMode ) return;

    EMIT(( gs_HeaderFile, ",\n    " ));
}

void
NvIdlStructBegin( const char *name )
{
    Symbol *sym;

    if( gs_TopFile && gs_ImportMode ) return;

    sym = symbol_lookup( name );
    if( !sym )
    {
        sym = create_struct( name );
        symbol_add( name, sym );
    }

    gs_CurrentSym = sym;
    gs_indent = 4;

    EMIT(( gs_HeaderFile, "\ntypedef struct %sRec\n{\n", name ));
}

void
NvIdlStructEnd( const char *name )
{
    Struct *s;

    if( gs_TopFile && gs_ImportMode ) return;

    s = (Struct *)gs_CurrentSym->data;
    s->defined = 1;

    gs_CurrentSym = 0;
    gs_indent = 0;

    EMIT(( gs_HeaderFile, "} %s;\n", name ));
}

void
NvIdlStructField( const char *name )
{
    Struct *s;
    Field *f;
    Field *tmp;

    if( gs_TopFile && gs_ImportMode ) return;

    s = (Struct *)gs_CurrentSym->data;

    f = (Field *)malloc( sizeof(Field) );
    if( !f )
    {
        FAIL(( "allocation failure" ));
    }

    memset( f, 0, sizeof(Field) );

    f->name = (char *)name;
    f->type = gs_CurrentType;

    if( gs_CurrentType == NvIdlType_ID )
    {
        Symbol *sym;
        sym = symbol_lookup( gs_CurrentId );
        if( !sym )
        {
            FAIL(( "symbol not found: %s\n", gs_CurrentId ));
        }

        f->type = sym->type;
    }

    /* add to the end of the field list */
    f->next = 0;
    tmp = s->fields;
    if( !tmp )
    {
        s->fields = f;
    }
    else
    {
        while( tmp->next ) tmp = tmp->next;
        tmp->next = f;
    }

    EMIT(( gs_HeaderFile, "%*s", gs_indent, " " ));
    emit_type( gs_HeaderFile, gs_CurrentType );

    assert( gs_ArraySize != (NvU32)-1 );
    if( gs_ArraySize )
    {
        EMIT(( gs_HeaderFile, "%s[%d];\n", name, gs_ArraySize ));
        gs_ArraySize = 0;
    }
    else
    {
        EMIT(( gs_HeaderFile, "%s;\n", name ));
    }
}

void
NvIdlEnumBegin( const char *name )
{
    Symbol *sym;

    if (gs_TopFile && gs_ImportMode) 
        return;

    sym = symbol_lookup( name );
    if (!sym)
    {
        sym = create_enum( name );
        symbol_add( name, sym );
    }

    gs_CurrentSym = sym;
    gs_indent = 4;

    EMIT(( gs_HeaderFile, "\ntypedef enum\n{\n" ));
}

void
NvIdlEnumEnd( const char *name )
{
    if( gs_TopFile && gs_ImportMode ) return;

    EMIT(( gs_HeaderFile, "%*s%s_Num,\n", gs_indent, " ", name ));
    EMIT(( gs_HeaderFile, "%*s%s_Force32 = 0x7FFFFFFF\n",
        gs_indent, " ", name ));

    gs_CurrentSym = 0;
    gs_indent = 0;
    EMIT(( gs_HeaderFile, "} %s;\n", name ));
}

void
NvIdlEnumField( const char *name, NvBool assign )
{
    if( gs_TopFile && gs_ImportMode ) return;

    if( assign )
    {
        if( gs_CurrentValue.type == NvIdlValueType_Hex )
        {
            EMIT(( gs_HeaderFile, "%*s%s = 0x%x,\n", gs_indent, " ",
                name, gs_CurrentValue.val ));
        }
        else
        {
            EMIT(( gs_HeaderFile, "%*s%s = %d,\n", gs_indent, " ", name,
                gs_CurrentValue.val ));
        }
    }
    else
    {
        EMIT(( gs_HeaderFile, "%*s%s,\n", gs_indent, " ", name ));
    }
}

void
NvIdlHandle( const char *name )
{
    Symbol *s;
    char    StructName[512];
    char   *HandlePart;

    if (gs_TopFile && gs_ImportMode) 
        return;

    s = symbol_create();
    s->type = NvIdlType_Handle;
    s->name = (char *)name;
    symbol_add( name, s );

    strcpy(StructName, name);
    HandlePart = strstr(StructName, "Handle");
    if (!HandlePart)
    {
        sprintf(StructName, "Invalid handle name %s", name);
        yyerror(StructName);
        exit(1);
    }

    strcpy(HandlePart, "Rec");

    
    EMIT(( gs_HeaderFile, "\ntypedef struct %s *%s;\n", StructName, name ));
}

void
NvIdlTypedef( const char *name )
{
    Symbol *s;

    if( gs_TopFile && gs_ImportMode ) return;

    s = symbol_lookup( name );
    if( !s )
    {
        if( gs_CurrentType == NvIdlType_Struct )
        {
            s = create_struct( name );
        }
        else if( gs_CurrentType == NvIdlType_Enum )
        {
            s = symbol_create();
            s->type = gs_CurrentType;
            s->name = (char *)name;
        }
        else
        {
            // custom basic type
            s = symbol_create();
            s->type = NvIdlType_Custom;
            s->name = (char *)name;

            EMIT(( gs_HeaderFile, "\ntypedef " ));
            emit_type( gs_HeaderFile, gs_CurrentType );
            EMIT(( gs_HeaderFile, "%s;\n", s->name ));
        }

        symbol_add( name, s );
    }
}

void
NvIdlDefine( const char *name )
{
    if( gs_TopFile && gs_ImportMode ) return;

    if( gs_CurrentValue.type == NvIdlValueType_Hex )
    {
        EMIT(( gs_HeaderFile, "#define %s (0x%x)\n", name,
            gs_CurrentValue.val ));
    }
    else
    {
        EMIT(( gs_HeaderFile, "#define %s (%d)\n", name,
            gs_CurrentValue.val ));
    }
}

void
NvIdlVerbatim( const char c )
{
    if( gs_TopFile && gs_ImportMode ) return;

    EMIT(( gs_HeaderFile, "%c", c ));
}

/**
 * stub and dispatch generators.
 *
 * The EMIT macro is not needed since these functions will not be called
 * during the import phase of the idl parsing.
 */

static void
NvIdlGeneratePackageNumbers( FILE* file )
{
    Package *p;
    PackageEntry *e;

    p = gs_Packages;
    while( p )
    {
        myfprintf( file, "\n// %s Package\n", p->name );
        myfprintf( file, "typedef enum\n{\n" );
        gs_indent = 4;

        myfprintf( file, "%*s%s_Invalid = 0,\n", gs_indent, " ", p->name );

        e = p->entries;
        while( e )
        {
            myfprintf( file, "%*s%s_%s,\n", gs_indent, " ", p->name,
                e->name );
            e = e->next;
        }

        myfprintf( file, "%*s%s_Num,\n", gs_indent, " ", p->name );
        myfprintf( file, "%*s%s_Force32 = 0x7FFFFFFF,\n", gs_indent, " ",
            p->name );

        gs_indent = 0;
        myfprintf( file, "} %s;\n", p->name );
        p = p->next;
    }
}

/**
 * finds and emits the given parameter name (used to get the count attribute).
 */
static void
emit_count_attr_parameter( FILE* file, Function *f, const char *name,
    NvBool bDispatch )
{
    Param *p;
    char *n;

    p = f->params;
    while( p )
    {
        if( strcmp( name, p->name ) == 0 )
        {
            n = "";
            if( bDispatch )
            {
                if( p->pass == NvIdlPassType_In )
                {
                    n = "p_in->";
                }
                else if( p->pass == NvIdlPassType_InOut )
                {
                    n = "p_inout->";
                }
                else
                {
                    // FIXME: put this assert in when the current idl is fixed
                    //assert( !"out params not allowed for count attribute" );
                    n = "p_out->";
                }
            }
            else
            {
                if( p->pass == NvIdlPassType_InOut )
                {
                    myfprintf( file, "*" );
                }
            }

            myfprintf( file, "%s%s", n, p->name );
            return;
        }

        p = p->next;
    }
}

static void
emit_stub_preamble( const char *name )
{
    /* copyright */
    emit_copyright( gs_StubFile );

    /* ioctl file and package define, etc. */
    myfprintf( gs_StubFile, "\n"
"#include \"nvcommon.h\"\n"
"#include \"nvos.h\"\n"
"#include \"nvassert.h\"\n"
"#include \"nvidlcmd.h\"\n"
"#include \"%s.h\"\n", name );

    if( !gs_UserToUser )
    {
        myfprintf( gs_StubFile,
"\n"
"static NvOsFileHandle gs_IoctlFile;\n"
"static NvU32 gs_IoctlCode;\n"
"\n"
"static void NvIdlIoctlFile( void )\n"
"{\n"
"    if( !gs_IoctlFile )\n"
"    {\n"
"        gs_IoctlFile = %s_NvIdlGetIoctlFile%s(); \n"
"        gs_IoctlCode = %s_NvIdlGetIoctlCode%s(); \n"
"    }\n"
"}\n"
"\n",
gs_PackageName,
gs_UserToKMod ? "KMod" : "",
gs_PackageName,
gs_UserToKMod ? "KMod" : "");
    }

    if (gs_DebugCode)
    {
        myfprintf( gs_StubFile,
"\n"
"static int Nv_show_%s = 1;\n"
"static void NvIdlShow( int line, const char *fmt, ... )\n"
"{\n"
"    va_list ap;\n"
"    if (Nv_show_%s) {\n"
"        NvOsDebugPrintf(\"%%s:%%d: \",\n"
"            \"%s\",\n"
"            line);\n"
"        va_start(ap, fmt);\n"
"        NvOsDebugVprintf(fmt, ap);\n"
"        va_end(ap);\n"
"    }\n"
"}\n"
"\n", gs_PackageName, gs_PackageName, gs_OutputFileName );
    }

    myfprintf( gs_StubFile,
"#define OFFSET( s, e )       (NvU32)(void *)(&(((s*)0)->e))\n\n" );
}

static void
emit_params_struct( FILE* file, Symbol *sym );

static void
emit_params_structs( FILE* file )
{
    Symbol *sym = SymbolTable;
    while( sym )
    {
        if( sym->type == NvIdlType_Function )
        {
            emit_params_struct( file, sym );
        }

        sym = sym->next;
    }
}

static void
emit_params_struct( FILE* file, Symbol *sym )
{
    Function *f;
    Param *p;
    NvU32 num;

    f = (Function *)sym->data;

    /* emit three structs, one for IN, INOUT, and OUT params. always have the
     * package and function numbers in the IN struct.
     */

    /* in params */
    myfprintf( file, "\ntypedef struct %s_in_t\n{\n", sym->name );
    gs_indent = 4;

    if( gs_UserToUser )
    {
        /* this must be first */
        myfprintf( file, "%*sNvU32 code_;\n", gs_indent, " " );
    }
    myfprintf( file, "%*sNvU32 package_;\n", gs_indent, " " );
    myfprintf( file, "%*sNvU32 function_;\n", gs_indent, " " );

    /* scan all of the params to set the inout and out flags */
    num = 0;
    p = f->params;
    while( p )
    {
        if( p->pass == NvIdlPassType_In )
        {
            if( p->typename )
            {
                myfprintf( file, "%*s%s ", gs_indent, " ", p->typename );
            }
            else if( p->type == NvIdlType_String )
            {
                myfprintf( file, "%*s", gs_indent, " " );

                emit_type( file, p->type );

                myfprintf( file, "%s_data;\n", p->name );
                myfprintf( file, "%*sNvU32 %s_len;\n", gs_indent, " ",
                    p->name );
            }
            else
            {
                myfprintf( file, "%*s", gs_indent, " " );
                emit_type( file, p->type );
            }

            if( p->attr_count && p->type != NvIdlType_VoidPtr )
            {
                myfprintf( file, " * " );
            }

            if( p->type != NvIdlType_String )
            {
                myfprintf( file, "%s;\n", p->name );
            }

            num++;
        }

        p = p->next;
    }

    gs_indent = 0;
    myfprintf( file, "} NV_ALIGN(4) %s_in;\n", sym->name );

    // FIMXE: support strings for inout/out?

    /* inout params */
    num = 0;
    myfprintf( file, "\ntypedef struct %s_inout_t\n{\n", sym->name );
    gs_indent = 4;
    p = f->params;
    while( p )
    {
        if( p->pass == NvIdlPassType_InOut )
        {
            if( p->typename )
            {
                myfprintf( file, "%*s%s ", gs_indent, " ", p->typename );
            }
            else
            {
                myfprintf( file, "%*s", gs_indent, " " );
                emit_type( file, p->type );
            }

            if( p->attr_count && p->type != NvIdlType_VoidPtr )
            {
                myfprintf( file, " * " );
            }

            myfprintf( file, "%s;\n", p->name );

            num++;
        }
        p = p->next;
    }

    /* put in a dummy field - empty structs are actually not allowed in
     * ANSI C.  GCC will compile them anyway though.
     */
    if( !num )
    {
        myfprintf( file, "%*sNvU32 dummy_;\n", gs_indent, " " );
    }

    gs_indent = 0;
    myfprintf( file, "} NV_ALIGN(4) %s_inout;\n", sym->name );

    /* out params */
    num = 0;
    myfprintf( file, "\ntypedef struct %s_out_t\n{\n", sym->name );
    gs_indent = 4;
    if( f->rettype != NvIdlType_Void )
    {
        myfprintf( file, "%*s", gs_indent, " " );
        if( f->ret_typename )
        {
            myfprintf( file, "%s ", f->ret_typename );
        }
        else
        {
            emit_type( file, f->rettype );
        }
        myfprintf( file, "ret_;\n" );
        num++;
    }

    p = f->params;
    while( p )
    {
        if( p->pass == NvIdlPassType_Out )
        {
            if( p->typename )
            {
                myfprintf( file, "%*s%s ", gs_indent, " ", p->typename );
            }
            else
            {
                myfprintf( file, "%*s", gs_indent, " " );
                emit_type( file, p->type );
            }

            if( p->attr_count && p->type != NvIdlType_VoidPtr )
            {
                myfprintf( file, " * " );
            }

            myfprintf( file, "%s;\n", p->name );
            num++;
        }
        p = p->next;
    }

    /* put in a dummy field */
    if( !num )
    {
        myfprintf( file, "%*sNvU32 dummy_;\n", gs_indent, " " );
    }

    gs_indent = 0;
    myfprintf( file, "} NV_ALIGN(4) %s_out;\n", sym->name );

    /* the parameter struct */
    myfprintf( file, "\ntypedef struct %s_params_t\n{\n", sym->name );
    gs_indent = 4;
    myfprintf( file, "%*s%s_in in;\n", gs_indent, " ", sym->name );
    myfprintf( file, "%*s%s_inout inout;\n", gs_indent, " ", sym->name );
    myfprintf( file, "%*s%s_out out;\n", gs_indent, " ", sym->name );
    gs_indent = 0;
    myfprintf( file, "} %s_params;\n", sym->name );
}

/**
 * emits the formal parameters of the stub function.
 */
static void
emit_stub_params( FILE* file, Function *f )
{
    Param *p;

    p = f->params;
    if( !p )
    {
        myfprintf( file, "void" );
    }
    else
    {
        while( p )
        {
            if( p->is_const )
            {
                myfprintf( file, "const " );
            }

            emit_param_type( file, p, NV_FALSE );

            myfprintf( file, "%s", p->name );

            p = p->next;
            if( p )
            {
                myfprintf( file, ", " );
            }
        }
    }
}

/**
 * assigns the stub paramters to the marshaling structs.
 */
static void
emit_stub_param_marshal( Function *f, char *func_name )
{
    Param *p;
    char *n;
    char *ref;
    char bCheckNull;

    /* package and function id */
    myfprintf( gs_StubFile, "\n%*sp_.in.package_ = %s_%s;\n", gs_indent, " ",
        gs_PackageName, gs_InterfaceName );
    myfprintf( gs_StubFile, "%*sp_.in.function_ = %d;\n", gs_indent, " ",
        f->func_id );

    p = f->params;
    while( p )
    {
        bCheckNull = 0;

        if( p->pass == NvIdlPassType_In )
        {
            n = "p_.in.";
            ref = "";
        }
        else if( p->pass == NvIdlPassType_InOut )
        {
            n = "p_.inout.";
            if( p->attr_count )
            {
                ref = "";
            }
            else
            {
                ref = "*";
                bCheckNull = 1;
            }
        }
        else
        {
            /* only marshal voidptr out params */
            if( p->type == NvIdlType_VoidPtr )
            {
                n = "p_.out.";
                ref = "";
            }
            else
            {
                p = p->next;
                continue;
            }
        }

        if( p->type == NvIdlType_Struct )
        {
            if( !p->attr_count )
            {
                ref = "*";
                bCheckNull = 1;
            }
            else
            {
                ref = "";
            }

            if( bCheckNull )
            {
                myfprintf( gs_StubFile, "%*sif(%s)\n%*s", gs_indent, " ",
                   p->name, gs_indent, " " );
            }

            myfprintf( gs_StubFile, "%*s%s%s = ", gs_indent,
                " ", n, p->name );
        }
        else if( p->type == NvIdlType_String )
        {
            myfprintf( gs_StubFile, "%*s%s%s_len = "
                "( %s == 0 ) ? 0 : NvOsStrlen( %s ) + 1;\n",
                gs_indent, " ", n, p->name, p->name, p->name );
            myfprintf( gs_StubFile, "%*s%s%s_data = ", gs_indent,
                " ", n, p->name );
        }
        else
        {
            if( bCheckNull )
            {
                myfprintf( gs_StubFile, "%*sif(%s)\n%*s", gs_indent, " ",
                    p->name, gs_indent, " " );
            }

            myfprintf( gs_StubFile, "%*s%s%s = ", gs_indent, " ",
                n, p->name );
        }

        /* handle const - need to cast the const away to assign to the
         * marshaling structure.
         */
        if( p->type == NvIdlType_Struct && p->is_const )
        {
            myfprintf( gs_StubFile, "%s ", ref );
            myfprintf( gs_StubFile, "( " );
            emit_param_type( gs_StubFile, p, NV_FALSE );
            myfprintf( gs_StubFile, " )%s;\n", p->name );
        }
        else
        {
            if( p->is_const )
            {
                myfprintf( gs_StubFile, "( " );
                emit_param_type( gs_StubFile, p, NV_FALSE );
                myfprintf( gs_StubFile, ")" );
            }

            myfprintf( gs_StubFile, "%s%s;\n", ref, p->name );
        }

        p = p->next;
    }
}

/* for user-to-user mode, write the pointer data directly into the fifo */
static void
emit_stub_user_pointer_marshal( Function *f )
{
    Param *p;

    p = f->params;
    while( p )
    {
        if( (p->pass == NvIdlPassType_In ||
             p->pass == NvIdlPassType_InOut) &&
            (p->type == NvIdlType_String) ||
            (p->attr_count && p->count_pass != NvIdlPassType_Out) )
        {
            myfprintf( gs_StubFile, "%*sif( %s )\n", gs_indent, " ", p->name );
            gs_indent += 4;
            myfprintf( gs_StubFile, "%*sNvIdlHelperFifoWrite( f_->FifoIn, %s, ",
                gs_indent, " ", p->name );
            if( p->attr_count )
            {
                emit_count_attr_parameter( gs_StubFile, f, p->attr_count,
                    NV_FALSE );

                if( p->type != NvIdlType_VoidPtr )
                {
                    myfprintf( gs_StubFile, " * sizeof( " );
                    emit_param_type( gs_StubFile, p, NV_TRUE );
                    myfprintf( gs_StubFile, " )" );
                }
            }
            else
            {
                if( p->pass == NvIdlPassType_In )
                {
                    myfprintf( gs_StubFile, "p_.in.%s_len", p->name );
                }
                else
                {
                    assert( "[inout] strings aren't supported" );
                }
            }

            myfprintf( gs_StubFile, " );\n" );
            gs_indent -= 4;
        }

        p = p->next;
    }
}

/**
 * handles the return data after the real function has been called.
 */
static void
emit_stub_return( Symbol *sym, Function *f )
{
    Param *p;
    char *n;

    if( gs_UserToUser )
    {
        // debug
        if( 0 )
        {
            myfprintf( gs_StubFile, "NvOsDebugPrintf( \"waiting for inout/out "
                "params\\n\" );\n" );
        }

        /* read back the basic types (not the count attribute ones) */
        if( f->has_inout )
        {
            myfprintf( gs_StubFile, "%*sNvIdlHelperFifoRead( f_->FifoOut, "
                "&p_.inout, io_size_, 0 );\n", gs_indent, " " );
        }
        if( f->has_out || f->rettype != NvIdlType_Void )
        {
            myfprintf( gs_StubFile, "%*sNvIdlHelperFifoRead( f_->FifoOut, "
                "&p_.out, o_size_, 0 );\n", gs_indent, " " );
        }
    }

    p = f->params;
    while( p )
    {
        if( p->pass == NvIdlPassType_InOut )
        {
            n = "p_.inout.";
        }
        else if( p->pass == NvIdlPassType_Out )
        {
            n = "p_.out.";
        }
        else
        {
            p = p->next;
            continue;
        }

        if( p->type == NvIdlType_String )
        {
            // FIXME: support strings?
            assert( "[inout] or [out] strings aren't supported" );
        }
        else if( p->attr_count == 0 )
        {
            myfprintf( gs_StubFile, "%*sif(%s)\n%*s{\n%*s*%s = %s%s;\n%*s}\n",
                gs_indent, " ", p->name, gs_indent, " ", (gs_indent+4), " ",
                p->name, n, p->name, gs_indent, " ");
        }

        p = p->next;
    }

    if( gs_UserToUser )
    {
        p = f->params;
        while( p )
        {
            if( p->attr_count &&
                (p->count_pass == NvIdlPassType_InOut ||
                 p->count_pass == NvIdlPassType_Out ) )
            {
                myfprintf( gs_StubFile, "%*sif( %s )\n", gs_indent,
                    " ", p->name );
                gs_indent += 4;

                myfprintf( gs_StubFile, "%*sNvIdlHelperFifoRead( f_->FifoOut,"
                    " %s, ",gs_indent, " ", p->name );
                emit_count_attr_parameter( gs_StubFile, f, p->attr_count,
                    NV_FALSE );
                if( p->type != NvIdlType_VoidPtr )
                {
                    myfprintf( gs_StubFile, " * sizeof( " );
                    emit_param_type( gs_StubFile, p, NV_TRUE );
                    myfprintf( gs_StubFile, " )" );
                }
                myfprintf( gs_StubFile, ", 0 );\n" );

                gs_indent -= 4;
            }

            p = p->next;
        }
    }

    /* for user-to-user, wait for the dispatcher to finish.
     * it may be possible to just write to the fifo, then
     * continue without waiting for the dispatcher. also,
     * should check for any dispatch errors.
     */
    if( gs_UserToUser )
    {
        myfprintf( gs_StubFile, "%*sNvIdlHelperFifoRead( f_->FifoOut, &err_, "
            "sizeof(err_), 0 );\n", gs_indent, " " );
        // FIXME: do something other than assert?
        myfprintf( gs_StubFile, "%*sNV_ASSERT( err_ == NvSuccess );\n",
            gs_indent, " " );

        /* release the fifos */
        myfprintf( gs_StubFile, "%*sNvIdlHelperReleaseFifoPair( f_ );\n",
            gs_indent, " " );
    }

    if( gs_DebugCode )
    {
        myfprintf( gs_StubFile,
            "\n"
            "    NvIdlShow(__LINE__,\n"
            "        \"%s() %s stub return %%08x\\n\",\n"
            "        (int)%s);\n"
            "", sym->name, 
                gs_UserToUser ? "daemon":"kernel",
                (f->rettype == NvIdlType_Void) ? "0" : "p_.out.ret_" );
    }

    if( f->rettype != NvIdlType_Void )
    {
        myfprintf( gs_StubFile, "\n%*sreturn p_.out.ret_;\n", gs_indent,
            " " );
    }
}

/**
 * generates the stub function that will marshal the parameters, call the
 * real function via NvOsIoctl and handle the return data.
 */
static void
emit_stub_function( Symbol *sym )
{
    Function *f;
    char *name;

    name = sym->name;
    f = (Function *)sym->data;

    myfprintf( gs_StubFile, "\n" );
    if( f->ret_typename )
    {
        myfprintf( gs_StubFile, "%s ", f->ret_typename );
    }
    else
    {
        emit_type( gs_StubFile, f->rettype );
    }
    myfprintf( gs_StubFile, "%s( ", name );

    /* parameters */
    emit_stub_params( gs_StubFile, f );

    /* function implementation */

    myfprintf( gs_StubFile, " )\n{\n" );
    gs_indent = 4;

    if( f->isdebug )
    {
        myfprintf( gs_StubFile, "#if NV_DEBUG\n" );
    }

    /* error checking (a good thing)
     * also use for synchronization with the user-to-user dispatcher
     */
    myfprintf( gs_StubFile, "%*sNvError err_;\n", gs_indent, " " );

    if( gs_UserToUser )
    {
        myfprintf( gs_StubFile, "%*sNvIdlFifoPair *f_;\n", gs_indent, " " );
    }

    /* in, inout, out sizes */
    myfprintf( gs_StubFile, "%*sNvU32 i_size_ = "
        "OFFSET(%s_params, inout);\n", gs_indent, " ", sym->name );
    if( gs_UserToUser == NV_FALSE || f->has_inout )
    {
        myfprintf( gs_StubFile, "%*sNvU32 io_size_ = "
            "OFFSET(%s_params, out) - OFFSET(%s_params, inout);\n",
            gs_indent, " ", sym->name, sym->name );
    }
    if( gs_UserToUser == NV_FALSE || f->has_out ||
        f->rettype != NvIdlType_Void )
    {
        myfprintf( gs_StubFile, "%*sNvU32 o_size_ = "
            "sizeof(%s_params) - OFFSET(%s_params, out);\n", gs_indent, " ",
            sym->name, sym->name );
    }

    /* the parameter structure */
    myfprintf( gs_StubFile, "%*s%s_params p_;\n", gs_indent, " ",
        name );

    /* marshal the params */
    emit_stub_param_marshal( f, name );

    if( gs_DebugCode )
    {
        myfprintf( gs_StubFile,
            "\n"
            "    NvIdlShow(__LINE__,\n"
            "        \"%s() %s stub call\\n\");\n"
            "\n", name, gs_UserToUser ? "daemon":"kernel" );
    }

    /* check/open the ioctl file */
    if( gs_UserToUser )
    {
        myfprintf( gs_StubFile, "%*serr_ = NvIdlHelperGetFifoPair( &f_ );\n",
            gs_indent, " " );

        myfprintf( gs_StubFile, "%*sif(err_ != NvSuccess)\n",
            gs_indent, " " );

        if( f->rettype == NvIdlType_Void )
        {
            myfprintf( gs_StubFile, "%*sreturn;\n",
                gs_indent + 4, " ");
        }
        else if( f->rettype == NvIdlType_Error)
        {
            myfprintf( gs_StubFile, "%*sreturn err_;\n",
                gs_indent + 4, " ");
        }
        else if( f->rettype == NvIdlType_Struct ||
                 f->rettype == NvIdlType_Enum ||
                 f->rettype == NvIdlType_Custom )
        {
            myfprintf( gs_StubFile, "%*s{",
                gs_indent, " " );
            emit_default_retval( gs_StubFile, f );
            myfprintf( gs_StubFile, " }\n" );
        }
        else
        {
            myfprintf( gs_StubFile, "%*sreturn ",
                gs_indent + 4, " " );
            emit_default_retval( gs_StubFile, f );
            myfprintf( gs_StubFile, ";\n" );
        }

        myfprintf( gs_StubFile, "\n%*sp_.in.code_ = NvRmDaemonCode_%s;\n\n",
            gs_indent, " ",  gs_PackageName);

        /* write the params, then the pointed-to data, etc.
         * just write the [in] and maybe [inout] params.
         */
        myfprintf( gs_StubFile, "%*sNvIdlHelperFifoWrite( f_->FifoIn, &p_.in, "
            "i_size_ );\n", gs_indent, " " );
        if( f->has_inout )
        {
            myfprintf( gs_StubFile, "%*sNvIdlHelperFifoWrite( f_->FifoOut, "
                "&p_.inout, io_size_ );\n", gs_indent, " " );
        }

        emit_stub_user_pointer_marshal( f );

        if( !strcmp( gs_TargetOs, "qnx" ) )
        {
            myfprintf( gs_StubFile, "%*sNvIdlHelperFifoFlush( f_->FifoIn);\n",
                gs_indent, " ");
        }

    }
    else
    {
#if 0
        myfprintf( gs_StubFile,
            "\n"
            "%*sif (NV_UNLIKELY(!gs_IoctlFile))\n"
            "%*s    NvIdlIoctlFile();\n",
            gs_indent, " ",
            gs_indent, " " );
#else
        myfprintf( gs_StubFile, "\n%*sNvIdlIoctlFile();\n", gs_indent, " " );

#endif

        /* do the ioctl */
        myfprintf( gs_StubFile, "\n%*serr_ = NvOsIoctl( "
            "gs_IoctlFile, gs_IoctlCode, &p_, i_size_, io_size_, "
            "o_size_ );\n", gs_indent, " " );

        myfprintf( gs_StubFile, "%*sNV_ASSERT( err_ == NvSuccess );\n",
            gs_indent, " " );
    }

    /* unmarshal return data */
    emit_stub_return( sym, f );

    if( f->isdebug )
    {
        if( f->rettype != NvIdlType_Void )
        {
            myfprintf( gs_StubFile, "#else\n" );
            if( f->rettype == NvIdlType_Struct ||
                f->rettype == NvIdlType_Enum )
            {
                myfprintf( gs_StubFile, "    { " );
                emit_default_retval( gs_StubFile, f );
                myfprintf( gs_StubFile, " }\n" );
            }
            else
            {
                myfprintf( gs_StubFile, "   return " );
                emit_default_retval( gs_StubFile, f );
                myfprintf( gs_StubFile, ";\n" );
            }
        }
        myfprintf( gs_StubFile, "#endif\n" );
    }

    gs_indent = 0;
    myfprintf( gs_StubFile, "}\n" );
}

static void
NvIdlGenerateStubs( const char *name )
{
    Symbol *sym;

    emit_stub_preamble( name );
    NvIdlGeneratePackageNumbers( gs_StubFile );
    emit_params_structs( gs_StubFile );

    /* generate stubs */
    sym = SymbolTable;
    while( sym )
    {
        if( sym->type == NvIdlType_Function )
        {
            emit_stub_function( sym );
        }

        sym = sym->next;
    }
}

static void
emit_dispatch_preamble( const char *name )
{
    /* copyright */
    emit_copyright( gs_DispatchFile );

    myfprintf( gs_DispatchFile, "\n"
"#include \"nvcommon.h\"\n"
"#include \"nvos.h\"\n"
"#include \"nvassert.h\"\n"
"#include \"nvreftrack.h\"\n"
"#include \"nvidlcmd.h\"\n"
"#include \"%s.h\"\n"
"\n"
"#define OFFSET( s, e ) (NvU32)(void *)(&(((s*)0)->e))\n"
"\n", name );
}

/**
 * Puts strings, pointers, and semaphores on the stack to be copied later.
 * This only handles IN strings (INOUT and OUT aren't supported - not sure
 * they make sense). Also declare stack variables for reference tracking
 * objects for handles.
 */
static void
emit_dispatch_pointer_params( Function *f )
{
    Param *p;
    char *ref;

    p = f->params;
    while( p )
    {
        if( p->type == NvIdlType_String &&
            p->pass == NvIdlPassType_In )
        {
            myfprintf( gs_DispatchFile, "%*schar *%s = NULL;\n", gs_indent,
                " ", p->name );
        }
        else if( p->type == NvIdlType_Semaphore )
        {
            myfprintf( gs_DispatchFile,"%*sNvOsSemaphoreHandle %s = NULL;\n",
                gs_indent, " ", p->name );
        }
        else if( p->attr_count )
        {
            if( p->typename )
            {
                myfprintf( gs_DispatchFile, "%*s%s", gs_indent, " ",
                    p->typename );
                ref = "*";
            }
            else
            {
                myfprintf( gs_DispatchFile, "%*s", gs_indent, " " );
                emit_type( gs_DispatchFile, p->type );
                if( p->type == NvIdlType_VoidPtr )
                {
                    ref = "";
                }
                else
                {
                    ref = "*";
                }
            }

            myfprintf( gs_DispatchFile, " %s%s = NULL;\n", ref, p->name );
        }
        else if( p->type == NvIdlType_Handle &&
                 p->ref == NvIdlRefType_Add )
        {
            myfprintf( gs_DispatchFile, "%*sNvRtObjRefHandle ref_%s = 0;\n",
                       gs_indent, " ", p->name );                    
        }

        p = p->next;
    }
}

/**
 * frees resources that were allocated for this call.
 */
static void
emit_dispatch_resource_free( Function *f )
{
    Param *p;
    NvBool bClean = NV_FALSE;

    p = f->params;
    while( p )
    {
        if( p->type == NvIdlType_String ||
            p->attr_count ||
            p->type == NvIdlType_Semaphore ||
            p->ref == NvIdlRefType_Add )
        {
            if( !bClean )
            {
                myfprintf( gs_DispatchFile, "clean:\n" );
                bClean = NV_TRUE;
            }

            if( p->type == NvIdlType_Semaphore )
            {
                myfprintf( gs_DispatchFile, "%*sNvOsSemaphoreDestroy( %s );\n",
                    gs_indent, " ", p->name );
            }
            else if( p->ref == NvIdlRefType_Add )
            {
                myfprintf( gs_DispatchFile, "%*sif (ref_%s) NvRtDiscardObjRef(Ctx, ref_%s);\n",
                           gs_indent, " ", p->name, p->name );
            }
            else
            {
                myfprintf( gs_DispatchFile, "%*sNvOsFree( %s );\n", gs_indent,
                    " ", p->name );
            }
        }

        p = p->next;
    }
}

/**
 * copies string parameters from user space.
 */
static void
emit_dispatch_string_unmarshal( Param *p )
{
    assert( p->type == NvIdlType_String && p->pass == NvIdlPassType_In );

    myfprintf( gs_DispatchFile, "%*sif( p_in->%s_len )\n",
        gs_indent, " ", p->name );
    myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );
    gs_indent += 4;
    myfprintf( gs_DispatchFile,
        "%*s%s = NvOsAlloc( p_in->%s_len );\n", gs_indent, " ",
        p->name, p->name );
    myfprintf( gs_DispatchFile,
        "%*sif( !%s )\n", gs_indent, " ", p->name );
    myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );

    gs_indent += 4;
    myfprintf( gs_DispatchFile,
        "%*serr_ = NvError_InsufficientMemory;\n", gs_indent, " ");
    myfprintf( gs_DispatchFile,
        "%*sgoto clean;\n", gs_indent, " " );
    gs_indent -= 4;
    myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );

    if( gs_UserToUser )
    {
        myfprintf( gs_DispatchFile,
            "%*serr_ = NvIdlHelperFifoRead( hFifoIn, %s, "
            "p_in->%s_len, 0 );\n", gs_indent, " ", p->name, p->name );
    }
    else
    {
        myfprintf( gs_DispatchFile,
            "%*serr_ = NvOsCopyIn( %s, p_in->%s_data, "
            "p_in->%s_len );\n", gs_indent, " ", p->name, p->name,
            p->name );
    }
    myfprintf( gs_DispatchFile,
        "%*sif( err_ != NvSuccess )\n", gs_indent, " " );
    myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );

    gs_indent += 4;
    myfprintf( gs_DispatchFile,
        "%*serr_ = NvError_BadParameter;\n", gs_indent, " " );
    myfprintf( gs_DispatchFile,
        "%*sgoto clean;\n", gs_indent, " " );
    gs_indent -= 4;

    myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
    myfprintf( gs_DispatchFile,
        "%*sif( %s[p_in->%s_len - 1] != 0 )\n", gs_indent, " ",
        p->name, p->name );
    myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );

    gs_indent += 4;
    myfprintf( gs_DispatchFile,
        "%*serr_ = NvError_BadParameter;\n", gs_indent, " " );
    myfprintf( gs_DispatchFile,
        "%*sgoto clean;\n", gs_indent, " ");
    gs_indent -= 4;

    myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
    gs_indent -= 4;
    myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
}

/**
 * copies data pointed to by pointer parameters from user space.
 */
static void
emit_dispatch_pointer_unmarshal( Function *f, Param *p )
{
    char *n;
    char *star;

    assert( p->attr_count );

    if( p->pass == NvIdlPassType_In )
    {
        n = "p_in->";
    }
    else if( p->pass == NvIdlPassType_InOut )
    {
        n = "p_inout->";
    }
    else
    {
        n = "p_out->";
    }

    /* the count can be zero */
    myfprintf( gs_DispatchFile, "%*sif( ", gs_indent, " " );
    emit_count_attr_parameter( gs_DispatchFile, f, p->attr_count,
        NV_TRUE );
    /* allow null pointer passing -- don't allocate a copy buffer in that
     * case.
     */
    myfprintf( gs_DispatchFile, " && %s%s )\n", n, p->name );
    myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );
    gs_indent += 4;

    /* use the local pointer in the user-mode case from here onward */
    if( gs_UserToUser )
    {
        n = "";
    }

    myfprintf( gs_DispatchFile, "%*s%s = (", gs_indent, " ",  p->name );

    if( p->typename )
    {
        myfprintf( gs_DispatchFile, "%s ", p->typename );
        star = "*";
    }
    else
    {
        emit_type( gs_DispatchFile, p->type );
        if( p->type == NvIdlType_VoidPtr )
        {
            star = ""; /* no star! */
        }
        else
        {
            star = "*";
        }
    }

    /* allocate the pointer for the user data and copy it in for [in]
     * and [inout].
     * need to cast to the correct pointer type and also search
     * for the length parameter as specified by the count attribute.
     * multiply by the size of the type (size is the number of
     * elements).
     */
    myfprintf( gs_DispatchFile, " %s)NvOsAlloc( ", star );
    emit_count_attr_parameter( gs_DispatchFile, f, p->attr_count,
        NV_TRUE );
    /* don't multiply by sizeof(void*) if the type is a voidptr */
    if( p->type != NvIdlType_VoidPtr )
    {
        myfprintf( gs_DispatchFile, " * sizeof( " );
        emit_param_type( gs_DispatchFile, p, NV_TRUE );
        myfprintf( gs_DispatchFile, " )" );
    }
    myfprintf( gs_DispatchFile, " );\n" );
    myfprintf( gs_DispatchFile, "%*sif( !%s )\n", gs_indent, " ",
        p->name );
    myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " ");

    gs_indent += 4;
    myfprintf( gs_DispatchFile,
       "%*serr_ = NvError_InsufficientMemory;\n", gs_indent, " " );
    myfprintf( gs_DispatchFile, "%*sgoto clean;\n", gs_indent, " " );
    gs_indent -= 4;

    myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );

    if( p->count_pass == NvIdlPassType_In ||
        p->count_pass == NvIdlPassType_InOut )
    {
        myfprintf( gs_DispatchFile,
            "%*sif( %s%s )\n", gs_indent, " ", n, p->name );
        myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );
        gs_indent += 4;

        if( gs_UserToUser )
        {
            myfprintf( gs_DispatchFile,
                "%*serr_ = NvIdlHelperFifoRead( hFifoIn, %s%s, ",
                gs_indent, " ", n, p->name );
        }
        else
        {
            myfprintf( gs_DispatchFile,
                "%*serr_ = NvOsCopyIn( %s, %s%s, ",
                gs_indent, " ", p->name, n, p->name );
        }
        emit_count_attr_parameter( gs_DispatchFile, f, p->attr_count,
            NV_TRUE );
        /* don't multiply by sizeof(void*) if the type is a voidptr */
        if( p->type != NvIdlType_VoidPtr )
        {
            myfprintf( gs_DispatchFile, " * sizeof( " );
            emit_param_type( gs_DispatchFile, p, NV_TRUE );
            myfprintf( gs_DispatchFile, " )" );
        }
        if( gs_UserToUser )
        {
            myfprintf( gs_DispatchFile, ", 0" );
        }
        myfprintf( gs_DispatchFile, " );\n" );
        myfprintf( gs_DispatchFile,
            "%*sif( err_ != NvSuccess )\n", gs_indent, " " );
        myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " ");

        gs_indent += 4;
        myfprintf( gs_DispatchFile,
            "%*serr_ = NvError_BadParameter;\n", gs_indent, " " );
        myfprintf( gs_DispatchFile,
            "%*sgoto clean;\n", gs_indent, " " );
        gs_indent -= 4;

        myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
        gs_indent -= 4;
        myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
    }

    gs_indent -= 4;
    myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
}

/**
 * creates a kernel semaphore from a user semaphore
 */
static void
emit_dispatch_semaphore_unmarshal( Param *p )
{
    char *n;

    if( p->pass == NvIdlPassType_Out )
    {
        // FIXME: out semaphores?
        return;
    }

    if( p->pass == NvIdlPassType_In )
    {
        n = "p_in->";
    }
    else if( p->pass == NvIdlPassType_InOut )
    {
        n = "p_inout->";
    }

    myfprintf( gs_DispatchFile,
        "%*sif( %s%s )\n", gs_indent, " ", n, p->name );
    myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );

    gs_indent += 4;
    myfprintf( gs_DispatchFile,
        "%*serr_ = NvOsSemaphoreUnmarshal( %s%s, &%s );\n",
        gs_indent, " ", n, p->name, p->name );
    myfprintf( gs_DispatchFile,
        "%*sif( err_ != NvSuccess )\n", gs_indent, " " );
    myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );

    gs_indent += 4;
    myfprintf( gs_DispatchFile,
        "%*serr_ = NvError_BadParameter;\n", gs_indent, " " );
    myfprintf( gs_DispatchFile,
        "%*sgoto clean;\n", gs_indent, " " );
    gs_indent -= 4;

    myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
    gs_indent -= 4;
    myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
}

/**
 * handles the inout data unmarshaling before the real function call.
 */
static void
emit_dispatch_inout_unmarshal( Param *p )
{
    assert( p->pass == NvIdlPassType_InOut );

    if( gs_UserToUser )
    {
        return;
    }

    /* pointers already taken care of, this is just for basic types */
    if( p->attr_count == 0 || p->type == NvIdlType_Semaphore )
    {
        myfprintf( gs_DispatchFile, "%*sinout.%s = p_inout->%s;\n",
            gs_indent, " ", p->name, p->name );
    }
}

/* unmarshals all parameters in-order */
static void
emit_dispatch_param_unmarshal( Function *f )
{
    Param *p;
    p = f->params;
    while( p )
    {
        if( p->type == NvIdlType_String && p->pass == NvIdlPassType_In )
        {
            emit_dispatch_string_unmarshal( p );
        }
        else if( p->type == NvIdlType_Semaphore )
        {
            emit_dispatch_semaphore_unmarshal( p );
        }
        else if( p->pass == NvIdlPassType_InOut )
        {
            emit_dispatch_inout_unmarshal( p );
        }
        else if( p->attr_count )
        {
            emit_dispatch_pointer_unmarshal( f, p );
        }

        p = p->next;
    }
}

/**
 * fills in the parameters to the real function call.
 */
static void
emit_dispatch_params( Function *f )
{
    Param *p;
    char *n;

    p = f->params;
    while( p )
    {
        if( p->pass == NvIdlPassType_In )
        {
            if( p->type == NvIdlType_Struct )
            {
                n = "&p_in->";
            }
            else
            {
                n = "p_in->";
            }
        }
        else if( p->pass == NvIdlPassType_InOut )
        {
            if( gs_UserToUser )
            {
                n = "&p_inout->";
            }
            else
            {
                n = "&inout.";
            }
        }
        else
        {
            n = "&p_out->";
        }

        if( p->type == NvIdlType_String ||
            p->attr_count ||
            p->type == NvIdlType_Semaphore )
        {
            /* strings will be on the stack with the same name as the param's
             * formal declaration since they will have to be allocated and
             * copied from user space.
             *
             * Or - data has been copied from user space, use a local pointer.
             */
            myfprintf( gs_DispatchFile, "%s", p->name );
        }
        else
        {
            myfprintf( gs_DispatchFile, "%s%s", n, p->name );
        }

        p = p->next;
        if( p )
        {
            myfprintf( gs_DispatchFile, ", " );
        }
    }
}

/**
 * marshals the inout parameters after the real function call.
 */
static void
emit_dispatch_inout_marshal( Function *f )
{
    Param *p;

    if( gs_UserToUser )
    {
        // just in case this gets called
        return;
    }

    p = f->params;
    while( p )
    {
        if( p->pass != NvIdlPassType_InOut )
        {
            p = p->next;
            continue;
        }

        /* pointers already taken care of */
        if( p->attr_count == 0 )
        {
            myfprintf( gs_DispatchFile, "%*sp_inout->%s = inout.%s;\n",
                gs_indent, " ", p->name, p->name );
        }

        p = p->next;
    }
}

/**
 * copies [out] data back to user space.
 */
static void
emit_dispatch_pointer_marshal( Function *f )
{
    Param *p;
    char *n;

    p = f->params;
    while( p )
    {
        if( ( p->count_pass == NvIdlPassType_InOut ||
            ( p->count_pass == NvIdlPassType_Out ) ) &&
            p->attr_count )
        {
            /* count(*) pointers are always passed as [in] parameters */
            assert( p->pass == NvIdlPassType_In );
            n = "p_in->";
            myfprintf( gs_DispatchFile,
                "%*sif(%s%s && %s)\n", gs_indent, " ", n, p->name, p->name);
            myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " ");
            gs_indent += 4;
            if( gs_UserToUser )
            {
                myfprintf( gs_DispatchFile,
                    "%*serr_ = NvIdlHelperFifoWrite( hFifoOut, %s, ", 
                    gs_indent, " ", p->name );    
            }
            else
            {
                myfprintf( gs_DispatchFile,
                    "%*serr_ = NvOsCopyOut( %s%s, %s, ", gs_indent, " ",
                    n, p->name, p->name );
            }
            emit_count_attr_parameter( gs_DispatchFile, f, p->attr_count,
                NV_TRUE );
            if( p->type != NvIdlType_VoidPtr )
            {
                myfprintf( gs_DispatchFile, " * sizeof( " );
                emit_param_type( gs_DispatchFile, p, NV_TRUE );
                myfprintf( gs_DispatchFile, " )" );
            }
            myfprintf( gs_DispatchFile, " );\n" );
            myfprintf( gs_DispatchFile,
                    "%*sif( err_ != NvSuccess )\n", gs_indent, " " );
            myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " ");
            myfprintf( gs_DispatchFile,
                "%*s    err_ = NvError_BadParameter;\n", gs_indent, " " );
            myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
            gs_indent -= 4;
            myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
        }

        p = p->next;
    }
}

static void
emit_dispatch_reftrack_pre( Function *f )
{
    Param *p = f->params;

    while( p )
    {
        if( p->ref == NvIdlRefType_Add )
        {
            assert( p->type == NvIdlType_Handle );
            assert( p->pass == NvIdlPassType_Out );

            myfprintf( gs_DispatchFile, "%*serr_ = NvRtAllocObjRef(Ctx, &ref_%s);\n",
                       gs_indent, " ", p->name );
            myfprintf( gs_DispatchFile, "%*sif (err_ != NvSuccess)\n", gs_indent, " " );
            myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );
            myfprintf( gs_DispatchFile, "%*sgoto clean;\n", gs_indent+4, " " );
            myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
        }
        else if ( p->ref == NvIdlRefType_Del )
        {
            assert( p->type == NvIdlType_Handle );
            assert( p->pass == NvIdlPassType_In );

            myfprintf( gs_DispatchFile,
                       "%*sif (p_in->%s != NULL) "
                       "NvRtFreeObjRef(Ctx, "
                       "NvRtObjType_%s_%s, "
                       "p_in->%s);\n",
                       gs_indent, " ",
                       p->name,
                       gs_PackageName,
                       p->typename,
                       p->name );
        }            

        p = p->next;
    }
}

static void
emit_dispatch_reftrack_post( Function *f )
{
    Param *p = f->params;
    NvBool first = NV_TRUE;

    while( p )
    {
        if( p->ref == NvIdlRefType_Add )
        {
            if (first)
            {
                myfprintf( gs_DispatchFile, "%*sif ( p_out->ret_ == NvSuccess )\n", gs_indent, " " );
                myfprintf( gs_DispatchFile, "%*s{\n", gs_indent, " " );
                gs_indent += 4;
                first = NV_FALSE;
            }
            
            myfprintf( gs_DispatchFile, "%*sNvRtStoreObjRef(Ctx, ref_%s, "
                       "NvRtObjType_%s_%s, p_out->%s);\n",
                       gs_indent, " ", p->name, gs_PackageName, p->typename, p->name );
            myfprintf( gs_DispatchFile, "%*sref_%s = 0;\n",
                       gs_indent, " ", p->name );

        }

        p = p->next;
    }

    if (!first)
    {
        gs_indent -= 4;
        myfprintf( gs_DispatchFile, "%*s}\n", gs_indent, " " );
    }
}


/**
 * emits the dispatcher code per function in the interface.
 */
static void
emit_dispatch_function( Symbol *sym )
{
    Function *f;
    f = (Function *)sym->data;

    myfprintf( gs_DispatchFile, "\nstatic NvError %s_dispatch_( ",
        sym->name );
    myfprintf( gs_DispatchFile, "void *InBuffer, NvU32 InSize, "
               "void *OutBuffer, NvU32 OutSize, NvDispatchCtx* Ctx )\n{\n" );
    gs_indent = 4;

    myfprintf( gs_DispatchFile, "%*sNvError err_ = NvSuccess;\n",
        gs_indent, " " );

    if( f->isdebug )
    {
        myfprintf( gs_DispatchFile, "\n#if NV_DEBUG\n" );
    }

    if( f->params || f->rettype != NvIdlType_Void )
    {
        /* parameter structures */
        if( f->params )
        {
            myfprintf( gs_DispatchFile, "%*s%s_in *p_in;\n", gs_indent, " ",
                sym->name ); 
        }
        if( f->has_inout )
        {
            myfprintf( gs_DispatchFile, "%*s%s_inout *p_inout;\n",
                gs_indent, " ", sym->name ); 
        }
        if( f->has_out || f->rettype != NvIdlType_Void )
        {
            myfprintf( gs_DispatchFile, "%*s%s_out *p_out;\n", gs_indent, " ",
                sym->name );
        }

        /* inout params need to be on the stack since the InBuffer may not be
         * writable.
        */
        if( f->has_inout )
        {
            myfprintf( gs_DispatchFile, "%*s%s_inout inout;\n",
                gs_indent, " ", sym->name );
        }

        /* allocate strings and pointers on the stack */
        emit_dispatch_pointer_params( f );

        /* p_in, p_inout, p_out setup */
        if( f->params )
        {
            myfprintf( gs_DispatchFile, "\n%*sp_in = (%s_in *)InBuffer;\n",
                gs_indent, " ", sym->name );
        }
        if( f->has_inout )
        {
            myfprintf( gs_DispatchFile, "%*sp_inout = (%s_inout *)"
                "((NvU8 *)InBuffer + "
                "OFFSET(%s_params, inout));\n", gs_indent, " ", sym->name,
                sym->name, sym->name );
        }
        if( f->has_out || f->rettype != NvIdlType_Void )
        {
            myfprintf( gs_DispatchFile, "%*sp_out = (%s_out *)"
                "((NvU8 *)OutBuffer + OFFSET(%s_params, out) - "
                "OFFSET(%s_params, inout));\n",
                gs_indent, " ", sym->name, sym->name, sym->name, sym->name );
        }
        myfprintf( gs_DispatchFile, "\n" );

        /* inout param setup */
        if( f->has_inout )
        {
            /* hack to quiet warnings - the issue is that if there is an
             * [inout] pointer, the pointer logic will not use the inout
             * struct, so an unused variable warning shows up.
            */
            myfprintf( gs_DispatchFile, "%*s(void)inout;\n", gs_indent, " " );
        }

        /* unmarshal params */
        emit_dispatch_param_unmarshal( f );

        /* reftracking code to be executed before the call */
        emit_dispatch_reftrack_pre( f );
    }

    /* call the function */
    if( f->rettype != NvIdlType_Void )
    {
        myfprintf( gs_DispatchFile, "\n%*sp_out->ret_ = ", gs_indent, " " );
    }
    else
    {
        myfprintf( gs_DispatchFile, "\n%*s", gs_indent, " " );
    }

    myfprintf( gs_DispatchFile, "%s( ", sym->name );
    emit_dispatch_params( f );
    myfprintf( gs_DispatchFile, " );\n\n" );

    /* reftracking code to be executed after the call */
    emit_dispatch_reftrack_post( f );
    
    if( f->has_inout )
    {
        /* marshal inout data - setup p_inout to point to OutBuffer */
        myfprintf( gs_DispatchFile, "\n%*sp_inout = "
            "(%s_inout *)OutBuffer;\n", gs_indent, " ", sym->name );
        emit_dispatch_inout_marshal( f );
    }    
    
    /* copy out the data to user space */
    emit_dispatch_pointer_marshal( f );

    /* free resources */
    emit_dispatch_resource_free( f );

    if( f->isdebug )
    {
        myfprintf( gs_DispatchFile, "#endif\n" );
    }

    myfprintf( gs_DispatchFile, "%*sreturn err_;\n", gs_indent, " " );
    myfprintf( gs_DispatchFile, "}\n" );
}

/**
 * emits function dispatcher code for user-to-user mode.
 */
static void
emit_dispatch_function_user( Symbol *sym )
{
    Function *f;
    f = (Function *)sym->data;

    myfprintf( gs_DispatchFile, "\nstatic NvError %s_dispatch_( ",
        sym->name );
    myfprintf( gs_DispatchFile, "void *hFifoIn, "
        "void *hFifoOut, NvDispatchCtx* Ctx )\n{\n" );
    gs_indent = 4;

    myfprintf( gs_DispatchFile, "%*sNvError err_ = NvSuccess;\n",
        gs_indent, " " );

    if( f->isdebug )
    {
        myfprintf( gs_DispatchFile, "\n#if NV_DEBUG\n" );
    }

    /* parameter structures */
    if( f->params || f->rettype != NvIdlType_Void )
    {        
        if( f->params || f->rettype != NvIdlType_Void )
        {
            myfprintf( gs_DispatchFile, "%*s%s_params p_;\n",
                gs_indent, " ", sym->name );
        }        
        if( f->params )
        {
            myfprintf( gs_DispatchFile, "%*s%s_in *p_in;\n", gs_indent, " ",
                sym->name );
        }
        if( f->has_inout )
        {
            myfprintf( gs_DispatchFile, "%*s%s_inout *p_inout;\n",
                gs_indent, " ", sym->name );
        }
        if( f->has_out || f->rettype != NvIdlType_Void )
        {
            myfprintf( gs_DispatchFile, "%*s%s_out *p_out;\n",
                gs_indent, " ", sym->name );
        }
        if( f->params )
        {
            myfprintf( gs_DispatchFile, "%*sNvU32 i_size_ = "
                "OFFSET(%s_params, inout);\n", gs_indent, " ", sym->name );
        }
        if( f->has_inout )
        {
            myfprintf( gs_DispatchFile, "%*sNvU32 io_size_ = "
                "OFFSET(%s_params, out) - OFFSET(%s_params, inout);\n",
                gs_indent, " ", sym->name, sym->name );
        }
        if( f->has_out || f->rettype != NvIdlType_Void )
        {
            myfprintf( gs_DispatchFile, "%*sNvU32 o_size_ = "
                "sizeof(%s_params) - OFFSET(%s_params, out);\n",
                gs_indent, " ", sym->name, sym->name );
        }
        if( f->params )
        {
            myfprintf( gs_DispatchFile, "%*sNvU8 *param_ptr_;\n", gs_indent, " " );
        }

        /* allocate strings and pointers on the stack */
        emit_dispatch_pointer_params( f );

        /* setup basic parameters */
        myfprintf( gs_DispatchFile, "\n" );
        if( f->params )
        {
            myfprintf( gs_DispatchFile, "%*sp_in = &p_.in;\n",
                gs_indent, " " );
        }
        if( f->has_inout )
        {
            myfprintf( gs_DispatchFile, "%*sp_inout = &p_.inout;\n",
                gs_indent, " " );
        }
        if( f->has_out || f->rettype != NvIdlType_Void )
        {
            myfprintf( gs_DispatchFile, "%*sp_out = &p_.out;\n",
                gs_indent, " " );
        }

        /* read in the params, the global dispatcher has already read
         * the package_, function_ and code_ fields, so skip 12 bytes.
         */
        if( f->params )
        {
            myfprintf( gs_DispatchFile, "\n" );
            myfprintf( gs_DispatchFile,
                "%*sparam_ptr_ = ((NvU8 *)&p_) + 12;\n",
                gs_indent, " " );
            myfprintf( gs_DispatchFile, "%*sNvIdlHelperFifoRead( hFifoIn, "
                "param_ptr_, i_size_ - 12, 0 );\n", gs_indent, " ", sym->name );
        }
        if( f->has_inout )
        {
            myfprintf( gs_DispatchFile, "%*sNvIdlHelperFifoRead( hFifoIn, "
                "p_inout, io_size_, 0 );\n", gs_indent, " ", sym->name );
        }
        myfprintf( gs_DispatchFile, "\n" );

        /* unmarshal params */
        emit_dispatch_param_unmarshal( f );

        /* reftracking code to be executed before the call */
        emit_dispatch_reftrack_pre( f );
    }

    // debug
    if( 0 )
    {
        myfprintf( gs_DispatchFile,
                "NvOsDebugPrintf( \"%s dispatch\\n\" );\n" , sym->name );
    }

    /* call the function */
    myfprintf( gs_DispatchFile, "\n" );
    if( f->rettype != NvIdlType_Void )
    {
        myfprintf( gs_DispatchFile, "%*sp_out->ret_ = ", gs_indent, " " );
    }
    else
    {
        myfprintf( gs_DispatchFile, "%*s", gs_indent, " " );
    }

    myfprintf( gs_DispatchFile, "%s( ", sym->name );
    emit_dispatch_params( f );
    myfprintf( gs_DispatchFile, " );\n\n" );

    /* reftracking code to be executed after the call */
    emit_dispatch_reftrack_post( f );
    
    /* write [inout] and [out] params */
    if( f->has_inout )
    {
        myfprintf( gs_DispatchFile, "%*sNvIdlHelperFifoWrite( hFifoOut, "
            "p_inout, io_size_ );\n", gs_indent, " ", sym->name );
    }
    if( f->has_out || f->rettype != NvIdlType_Void )
    {
        myfprintf( gs_DispatchFile, "%*sNvIdlHelperFifoWrite( hFifoOut, "
            "p_out, o_size_ );\n", gs_indent, " ", sym->name );
    }

    /* write pointer data back to the client */
    emit_dispatch_pointer_marshal( f );

    if( !strcmp( gs_TargetOs, "qnx" ) )
    {
        /* sync with the client -- send the error code back. */
        myfprintf( gs_DispatchFile, "%*sNvIdlHelperFifoWrite( hFifoOut, &err_, "
            "sizeof(err_) );\n", gs_indent, " " );

        myfprintf( gs_DispatchFile, "%*sNvIdlHelperFifoFlush( hFifoOut );\n",
            gs_indent, " " );
    }

    /* free resources */
    emit_dispatch_resource_free( f );

    if( f->isdebug )
    {
        myfprintf( gs_DispatchFile, "#endif\n" );
    }

    myfprintf( gs_DispatchFile, "%*sreturn err_;\n", gs_indent, " " );
    myfprintf( gs_DispatchFile, "}\n" );
}

void
emit_dispatcher( const char *name )
{
    Symbol *sym;

    emit_dispatch_preamble( name );

    emit_params_structs( gs_DispatchFile );

    /* emit the dispatch functions */
    if (!gs_NullDispatch)
    {
        sym = SymbolTable;
        while( sym )
        {
            if( sym->type == NvIdlType_Function )
            {
                emit_dispatch_function( sym );
            }

            sym = sym->next;
        }
    }

    /* package dispatcher function prototype */
    myfprintf( gs_DispatchFile, "\nNvError %s_%sDispatch( NvU32 function, ",
                name,
                gs_DispatchType );
    myfprintf( gs_DispatchFile, "void *InBuffer, NvU32 InSize, "
               "void *OutBuffer, NvU32 OutSize, NvDispatchCtx* Ctx );" );

    /* package dispatcher function header */
    myfprintf( gs_DispatchFile, "\nNvError %s_%sDispatch( NvU32 function, ",
                name,
                gs_DispatchType );
    myfprintf( gs_DispatchFile, "void *InBuffer, NvU32 InSize, "
               "void *OutBuffer, NvU32 OutSize, NvDispatchCtx* Ctx )" );
    myfprintf( gs_DispatchFile, "\n{\n" );

    if (gs_NullDispatch)
    {
        /* dispatch function footer */
        myfprintf( gs_DispatchFile,
            "    NV_ASSERT(0);\n"
            "    return NvError_NotImplemented;\n"
            "}\n" );
    }
    else
    {
        myfprintf( gs_DispatchFile,
            "    NvError err_ = NvSuccess;\n"
            "\n"
            "    switch( function ) {\n" );

        /* dispatch each function */
        sym = SymbolTable;
        while( sym )
        {
            if( sym->type == NvIdlType_Function )
            {
                Function *f;
                f = (Function *)sym->data;

                myfprintf( gs_DispatchFile, "    case %d:\n", f->func_id );
                myfprintf( gs_DispatchFile, "        err_ = %s_dispatch_( "
                           "InBuffer, InSize, OutBuffer, OutSize, Ctx );\n",
                           sym->name );
                myfprintf( gs_DispatchFile, "        break;\n" );
            }

            sym = sym->next;
        }

        /* dispatch function footer */
        myfprintf( gs_DispatchFile,
            "    default:\n"
            "        err_ = NvError_BadParameter;\n"
            "        break;\n"
            "    }\n"
            "\n"
            "    return err_;\n"
            "}\n" );
    }
}

void
emit_dispatcher_user( const char *name )
{
    Symbol *sym;

    emit_dispatch_preamble( name );
    emit_params_structs( gs_DispatchFile );

    /* emit the dispatch functions */
    if (!gs_NullDispatch)
    {
        sym = SymbolTable;
        while( sym )
        {
            if( sym->type == NvIdlType_Function )
            {
                emit_dispatch_function_user( sym );
            }

            sym = sym->next;
        }
    }

    /* package dispatcher function prototype */
    myfprintf( gs_DispatchFile, "\nNvError %s_%sDispatch( NvU32 function, ",
        name,
        gs_DispatchType );
    myfprintf( gs_DispatchFile, "void *hFifoIn, "
        "void *hFifoOut, NvDispatchCtx* Ctx );" );

    /* package dispatcher function header */
    myfprintf( gs_DispatchFile, "\nNvError %s_%sDispatch( NvU32 function, ",
        name,
        gs_DispatchType );
    myfprintf( gs_DispatchFile, "void *hFifoIn, "
        "void *hFifoOut, NvDispatchCtx* Ctx )" );
    myfprintf( gs_DispatchFile, "\n{\n" );

    if (gs_NullDispatch)
    {
        /* dispatch function footer */
        myfprintf( gs_DispatchFile,
            "    NV_ASSERT(0);\n"
            "    return NvError_NotImplemented;\n"
            "}\n" );
    }
    else
    {
        myfprintf( gs_DispatchFile,
            "    NvError err_ = NvSuccess;\n"
            "\n"
            "    switch( function ) {\n" );

        /* dispatch each function */
        sym = SymbolTable;
        while( sym )
        {
            if( sym->type == NvIdlType_Function )
            {
                Function *f;
                f = (Function *)sym->data;

                myfprintf( gs_DispatchFile, "    case %d:\n", f->func_id );
                myfprintf( gs_DispatchFile, "        err_ = %s_dispatch_( "
                    "hFifoIn, hFifoOut, Ctx );\n", sym->name );
                myfprintf( gs_DispatchFile, "        break;\n" );
            }

            sym = sym->next;
        }

        /* dispatch function footer */
        myfprintf( gs_DispatchFile,
            "    default:\n"
            "        err_ = NvError_BadParameter;\n"
            "        break;\n"
            "    }\n"
            "\n" );

        if( strcmp( gs_TargetOs, "qnx" ) )
        {
            /* sync with the client -- send the error code back. */
            myfprintf( gs_DispatchFile, "%*sNvIdlHelperFifoWrite( hFifoOut, &err_, "
                "sizeof(err_) );\n", gs_indent, " " );
        }

        myfprintf( gs_DispatchFile, "    return err_;\n}\n" );
    }
}

static void
NvIdlGenerateDispatcher( const char *name )
{
    if( gs_UserToUser )
    {
        emit_dispatcher_user( name );
    }
    else
    {
        emit_dispatcher( name );
    }
}

static void
emit_package_dispatcher_user( FILE* file, const char *name )
{
    PackageEntry *e;
    Package *p = gs_Packages;
    while( p )
    {
        if( strcmp( p->name, name ) == 0 )
        {
            break;
        }

        p = p->next;
    }

    if( !p )
    {
        return;
    }

    /* function preamble */
    emit_copyright( file );

    myfprintf( file, "\n"
"#include \"nvcommon.h\"\n"
"#include \"nvos.h\"\n"
"#include \"nvassert.h\"\n"
"#include \"nvidlcmd.h\"\n"
"#include \"nvreftrack.h\"\n" );

    /* include all of the interface headers */
    e = p->entries;
    while( e )
    {
        myfprintf( file, "#include \"%s.h\"\n", e->name );
        e = e->next;
    }

    NvIdlGeneratePackageNumbers( file );

    /* declare the dispatchers */
    e = p->entries;
    while( e )
    {
        myfprintf( file, "NvError %s_%sDispatch( NvU32 function, "
            "void *hFifoIn, void *hFifoOut, NvDispatchCtx* Ctx );\n",
            e->name,
            gs_DispatchType );
        e = e->next;
    }


    /* dispatch function type */
    myfprintf( file, "\ntypedef NvError (* NvIdlDispatchFunc)( "
        "NvU32 function, void *hFifoIn, "
        "void *hFifoOut, NvDispatchCtx* Ctx );\n" );
    myfprintf( file, "\ntypedef struct NvIdlDispatchTableRec\n"
"{\n"
"    NvU32 PackageId;\n"
"    NvIdlDispatchFunc DispFunc;\n"
"} NvIdlDispatchTable;\n" );

    myfprintf( file,
        "\nstatic NvIdlDispatchTable gs_DispatchTable[] =\n{\n" );

    gs_indent = 4;
    e = p->entries;
    while( e )
    {
        myfprintf( file, "%*s{ %s_%s, %s_%sDispatch },\n", gs_indent, " ",
            name, e->name, e->name, gs_DispatchType );
        e = e->next;
    }
    myfprintf( file, "%*s{ 0,0 },\n", gs_indent, " " );
    gs_indent = 0;

    myfprintf( file, "};\n" );

    myfprintf( file,
"\nNvError %s_%sDispatch( void *hFifoIn, void *hFifoOut, NvDispatchCtx* Ctx )\n"
"{\n"
"    NvU32 id_[2];  // {package ID, function ID} \n"
"    NvError e;\n"
"    NvIdlDispatchTable *table_;\n"
"\n"
"    e = NvIdlHelperFifoRead( hFifoIn, id_, sizeof(id_), 0 );\n"
"    if ( e != NvSuccess )\n"
"        return e;\n"
"\n"
"    table_ = gs_DispatchTable;\n"
"\n",
name,
gs_DispatchType);

    if( gs_DebugCode )
    {
        myfprintf( file,
            "\n"
            "    NvOsDebugPrintf(\n"
            "        \"%%s:%%d: %s_%sDispatch() packid=%%d funcid=%%d\\n\",\n"
            "        __FILE__,\n"
            "        __LINE__,\n"
            "        id_[0],\n"
            "        id_[1]);\n"
            "\n",
            name,
            gs_DispatchType);
    }

    myfprintf( file,
"    if ( id_[0]-1 >= NV_ARRAY_SIZE(gs_DispatchTable) ||\n"
"         !table_[id_[0] - 1].DispFunc )\n"
"        return NvError_IoctlFailed;\n"
"\n"
"    return table_[id_[0] - 1].DispFunc( id_[1], hFifoIn, hFifoOut, Ctx );\n"
"}\n");
}


static void
emit_package_dispatcher( FILE* file, const char *name )
{
    PackageEntry *e;
    Package *p = gs_Packages;
    
    while( p )
    {
        if( strcmp( p->name, name ) == 0 )
        {
            break;
        }

        p = p->next;
    }

    if( !p )
    {
        return;
    }

    /* function preamble */
    emit_copyright( file );

    myfprintf( file, "\n"
"#include \"nvcommon.h\"\n"
"#include \"nvos.h\"\n"
"#include \"nvassert.h\"\n"
"#include \"nvidlcmd.h\"\n"
"#include \"nvreftrack.h\"\n");

    /* include all of the interface headers */
    e = p->entries;
    while( e )
    {
        myfprintf( file, "#include \"%s.h\"\n", e->name );
        e = e->next;
    }

    /* declare the function dispatcher (can't do it in the header since there
     * may be either kernel or user-to-user mode modules using the same headers
     */
    e = p->entries;
    while( e )
    {
        myfprintf( file, "NvError %s_%sDispatch( NvU32 function, "
                   "void *InBuffer, NvU32 InSize, void *OutBuffer, "
                   "NvU32 OutSize, NvDispatchCtx* Ctx );\n",
                    e->name,
                    gs_DispatchType);
        e = e->next;
    }

    NvIdlGeneratePackageNumbers( file );

    /* dispatch function type */
    myfprintf( file, "\ntypedef NvError (* NvIdlDispatchFunc)( "
               "NvU32 function, void *InBuffer, NvU32 InSize, void *OutBuffer, "
               "NvU32 OutSize, NvDispatchCtx* Ctx );\n" );
    myfprintf( file, "\ntypedef struct NvIdlDispatchTableRec\n"
"{\n"
"    NvU32 PackageId;\n"
"    NvIdlDispatchFunc DispFunc;\n"
"} NvIdlDispatchTable;\n" );

    myfprintf( file,
        "\nstatic NvIdlDispatchTable gs_DispatchTable[] =\n{\n" );

    gs_indent = 4;
    e = p->entries;
    while( e )
    {
        myfprintf( file, "%*s{ %s_%s, %s_%sDispatch },\n", gs_indent, " ",
            name, e->name, e->name, gs_DispatchType );
        e = e->next;
    }
    myfprintf( file, "%*s{ 0,0 },\n", gs_indent, " " );
    gs_indent = 0;

    myfprintf( file, "};\n" );

    myfprintf( file,
"\nNvError %s_%sDispatch( void *InBuffer, NvU32 InSize, void *OutBuffer, "
"NvU32 OutSize, NvDispatchCtx* Ctx )\n"
"{\n"
"    NvU32 packid_;\n"
"    NvU32 funcid_;\n"
"    NvIdlDispatchTable *table_;\n"
"\n"
"    NV_ASSERT( InBuffer );\n"
"    NV_ASSERT( OutBuffer );\n"
"\n"
"    packid_ = ((NvU32 *)InBuffer)[0];\n"
"    funcid_ = ((NvU32 *)InBuffer)[1];\n"
"    table_ = gs_DispatchTable;\n"
"\n",
name,
gs_DispatchType);

    if( gs_DebugCode )
    {
        myfprintf( file,
            "\n"
            "    NvOsDebugPrintf(\n"
            "        \"%%s:%%d: %s_%sDispatch() packid=%%d funcid=%%d\\n\",\n"
            "        __FILE__,\n"
            "        __LINE__,\n"
            "        packid_,\n"
            "        funcid_);\n"
            "\n"
            "{\n"
            "    int i;\n"
            "    for (i=0; i<InSize/4; i++) {\n"
            "        NvOsDebugPrintf(\"IN[%%2d]: %%08x %%d\\n\",\n"
            "            i,\n"
            "            ((NvU32 *)InBuffer)[i],\n"
            "            ((NvU32 *)InBuffer)[i]);\n"
            "    }\n"
            "}   \n"
            "\n",
            name,
            gs_DispatchType);
    }

    myfprintf( file,
"    if ( packid_-1 >= NV_ARRAY_SIZE(gs_DispatchTable) ||\n"
"         !table_[packid_ - 1].DispFunc )\n"
"        return NvError_IoctlFailed;\n"
"\n"
"    return table_[packid_ - 1].DispFunc( funcid_, InBuffer, InSize,\n"
"        OutBuffer, OutSize, Ctx );\n"
"}\n");
}

static void
NvIdlGeneratePackageDispatcher( const char *name )
{
    NvError err;
    FILE* file;
    char buff[NVOS_PATH_MAX];

    sprintf( buff, "%s_%sDispatch.c", name, gs_DispatchType );
    file = myfopen( (gs_OutputFileName == NULL) ?  buff :
            gs_OutputFileName, "w", 0);

    if (file == NULL)
    {
        return;
    }

    if( gs_UserToUser )
    {
        emit_package_dispatcher_user( file, name );
    }
    else
    {
        emit_package_dispatcher( file, name );
    }

    gs_GeneratedFile = NV_TRUE;
    myfclose( file );
}

extern FILE *yyin;
char *g_filename;
static char *gs_InputFileName;

typedef struct ImportFile_t
{
    char *name;
    struct ImportFile_t *next;
} ImportFile;

static ImportFile *ImportFiles;

static void usage( void )
{
    printf( 
"usage: [OPTIONS] <input file>\n"
"  Read <input file> and generate one output file.\n"
"Exactly one of -g -G -h -s or -d must be specified\n"
"  -g: generate the top-level package dispatcher.\n"
"  -h: generate the header file for the given input idl file.\n"
"  -s: generate the stub file for the given input idl file.\n"
"  -d: generate the dispatch file for the given input idl file.\n"
"Only one of -u -m or -k can be specified\n"
"  -k: emit code for user space to kernel Ioctls (default)\n"
"  -u: emit code for user space to user space function calls rather than\n"
"      ioctls\n"
"  -m: emit code for user space to kernel module Ioctls\n"
"Any number of -I options may be specified\n"
"  -I: <path>: path to find other idl files.  This can be passed multiple\n"
"      times\n"
"Other options\n"
"  -o: output file: specify the name of an output file, if this is omitted\n"
"      a default name will be chosen\n"
"  -N: generate NULL dispatcher - always asserts and fails every function\n"
"  -f: Add dispatch type (D/K/M) to dispatch function names\n"
"  -P: emit debug printfs for functions\n"
"  -O: target OS: specify target OS\n"
    );
}

int
NvIdlMain( int argc, char *argv[] )
{
    NvError err;
    ImportFile *ifiles;
    NvU32 idx = 1;
    NvBool ModeFound = NV_FALSE;
    char *CurrentArg;

    if ( argc < 2 )
    {
        usage();
        return -1;
    }

    g_error = 0;

    while( idx < argc )
    {
        CurrentArg = argv[idx];
        
        if (CurrentArg[0] == '-')
        {
            if (CurrentArg[2] != 0)
            {
                // unknown arg
                usage();
                return -1;
            }

            switch (CurrentArg[1]) {
            case 'G':
            case 'g':
            case 'h':
            case 's':
            case 'd':
                if (gs_GeneratorMode != GeneratorMode_Unknown)
                {
                    usage();
                    return -1;
                }
                switch (CurrentArg[1]) {
                case 'g':
                    gs_GeneratorMode = GeneratorMode_GlobalDispatch;
                    break;
                case 'h':
                    gs_GeneratorMode = GeneratorMode_Header;
                    break;
                case 's':
                    gs_GeneratorMode = GeneratorMode_Stub;
                    break;
                case 'd':
                    gs_GeneratorMode = GeneratorMode_Dispatch;
                    break;
                default:
                    printf("Program error %s:%d\n", __FILE__,__LINE__);
                    return -1;
                }
                break;

            case 'k':
            case 'm':
            case 'u':
                if (gs_UserToUser || gs_UserToKMod || gs_UserToKernel)
                {
                    usage();
                    return -1;
                }
                switch (CurrentArg[1]) {
                case 'k': gs_UserToKernel = NV_TRUE; break;
                case 'm': gs_UserToKMod   = NV_TRUE; break;
                case 'u': gs_UserToUser   = NV_TRUE; break;
                default:
                    printf("Program error %s:%d\n", __FILE__,__LINE__);
                    return -1;
                }
                break;

            case 'I':
                ++idx;
                gs_IncludePaths[gs_NumberIncludes] = argv[idx];
                ++gs_NumberIncludes;
                break;

            case 'o':
                ++idx;
                gs_OutputFileName = argv[idx];
                break;

            case 'P':
                gs_DebugCode = NV_TRUE;
                break;

            case 'N':
                gs_NullDispatch = NV_TRUE;
                break;

            case 'f':
                gs_DispatchType = "x";
                break;

            case 'O':
                ++idx;
                gs_TargetOs = argv[idx];
                break;

            default:
                // unknown arg:
                printf("unknown argument: %s\n", argv[idx]);
                usage();
                return -1;
            }
        }
        else
        {
            // argument does not begin with a dash. Has to be the last argument
            if ( (idx + 1) != argc )
            {
                printf("too many arguments\n");
                usage();
                return -1;
            }

            gs_InputFileName = argv[idx];
        }

        idx++;
    }

    if (!gs_UserToUser && !gs_UserToKMod && !gs_UserToKernel)
        gs_UserToKernel = NV_TRUE;
    if (gs_DispatchType[0])
    {
        if (gs_UserToUser)
            gs_DispatchType = "D";
        if (gs_UserToKMod)
            gs_DispatchType = "M";
        if (gs_UserToKernel)
            gs_DispatchType = "K";
    }

    /* do the idl imports */
    yyin = myfopen( gs_InputFileName, "r", 0);
    if( !yyin )
    {
        printf( "file open failed: %s\n", gs_InputFileName );
        return -1;
    }

    linenum = 0;
    g_filename = gs_InputFileName;

    NvIdlSetImportMode( NV_TRUE );
    gs_TopFile = NV_TRUE;
    yyparse();
    myfclose( yyin );
    yyin = NULL;

    gs_TopFile = NV_FALSE;
    NvIdlClearState();

    ifiles = ImportFiles;
    while( ifiles )
    {
        yyin = myfopen( ifiles->name, "r", 1);
        if( !yyin )
        {
            printf( "file open failed: %s\n", ifiles->name );
            return -1;
        }

        linenum = 0;
        g_filename = ifiles->name;

        yyparse();
        myfclose( yyin );
        yyin = NULL;

        ifiles = ifiles->next;
    }

    NvIdlSetImportMode( NV_FALSE );
    yyin = myfopen( gs_InputFileName, "r", 0);
    if( !yyin )
    {
        printf( "file open failed: %s\n", gs_InputFileName );
        return -1;
    }

    /* generate header, stubs, dispatcher */
    yyparse();

    myfclose(yyin);
    yyin = NULL;


    if (g_error) {
        printf( "Error: nvidl failed to parse %s\n", gs_InputFileName );
    }
    if (gs_GeneratorMode == GeneratorMode_Unknown ||
        !gs_GeneratedFile)
    {
        printf( "WARNING: NO FILES GENERATED.\n");
        usage();
        return -1;
    }
    return g_error;
}

static NvBool
ifile_lookup( const char *name )
{
    ImportFile *f = ImportFiles;
    while( f )
    {
        if( strcmp( name, f->name ) == 0 )
        {
            return NV_TRUE;
        }

        f = f->next;
    }

    return NV_FALSE;
}

void
NvIdlImport( const char *name )
{
    if( gs_ImportMode )
    {
        ImportFile *f;
        ImportFile *tmp;

        if( ifile_lookup( name ) )
        {
            return;
        }

        /* add file to the end of the import list */
        f = (ImportFile *)malloc( sizeof(ImportFile) );
        if( !f )
        {
            FAIL(( "allocation failure" ));
        }

        f->name = (char *)name;
        f->next = 0;
        tmp = ImportFiles;
        if( !tmp )
        {
            ImportFiles = f;
        }
        else
        {
            while( tmp->next ) tmp = tmp->next;
            tmp->next = f;
        }
    }
}
