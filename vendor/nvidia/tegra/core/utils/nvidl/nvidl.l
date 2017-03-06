%{
/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */

#include <string.h>
#include "nvidl.tab.h"
#include "nvidl.h"
#include "../include/nvutil.h"

// suppress echo'ing stuff to stdout
#define ECHO

int linenum = 1;
extern char *g_filename;
extern int g_error;
%}

%x COMMENT LINE_COMMENT
%x VERBATIM

%%

import          { return IMPORT; }
package         { return PACKAGE; }
interface       { return INTERFACE; }
struct          { return STRUCT; }
enum            { return ENUM; }
define          { return DEFINE; }
typedef         { return TYPEDEF; }
handle          { return HANDLE; }
NvU8            { return U8; }
NvU16           { return U16; }
NvU32           { return U32; }
NvU64           { return U64; }
NvS8            { return S8; }
NvS16           { return S16; }
NvS32           { return S32; }
NvS64           { return S64; }
NvError         { return ERROR; }
string          { return STRING; }
void            { return VOID; }
voidptr         { return VOIDPTR; }
NvBool          { return BOOL; }
NvOsSemaphoreHandle { return SEMAPHORE; }
const           { return CONST; }
in              { return IN; }
out             { return OUT; }
inout           { return INOUT; }
count           { return COUNT; }
debug           { return NVDEBUG; }
"{"             { return O_BRACE; }
"}"             { return C_BRACE; }
"("             { return O_PAREN; }
")"             { return C_PAREN; }
"["             { return O_BRACKET; }
"]"             { return C_BRACKET; }
";"             { return END; }
"="             { return EQUALS; }
","             { return COMMA; }
refadd          { return REFADD; }
refdel          { return REFDEL; }

"/!"            { BEGIN VERBATIM; }
<VERBATIM>"!/"  { BEGIN INITIAL; }
<VERBATIM>.     { NvIdlVerbatim( yytext[0] ); }
<VERBATIM>\n    { linenum++; NvIdlVerbatim( '\n' ); }

[a-zA-Z_][a-zA-Z0-9_]* { 
        yylval.id = strdup(yytext);
        return ID;
    }

0x[0-9a-fA-F]+ {
        yylval.val = strtoul(yytext, 0, 16);
        NvIdlSetCurrentValue( NvIdlValueType_Hex, yylval.val );
        return VALUE;
    }


-?[0-9]+ {
        yylval.val = strtoul(yytext, 0, 10);
        NvIdlSetCurrentValue( NvIdlValueType_Dec, yylval.val );
        return VALUE;
    }

\n  { linenum++; }

"/*"  { BEGIN COMMENT; }
<COMMENT>\n { linenum++; }
<COMMENT>"*/"  { BEGIN INITIAL; }
<COMMENT>. { ; }

"//" { BEGIN LINE_COMMENT; }
<LINE_COMMENT>\n { linenum++; BEGIN INITIAL; }
<LINE_COMMENT>. { ; }

[a-zA-Z_"-""."][a-zA-Z0-9_"-""."]* {
        yylval.id = strdup(yytext);
        return FILE_ID;
    }

%%

int yywrap( void )
{
    return 1;
}

void yyerror( char *msg )
{
    printf( "%s: %s (%d): %s\n", g_filename, msg, linenum, yytext );
    g_error = 1;
}
