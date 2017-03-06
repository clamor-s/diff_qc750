/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IMPORT = 258,
     PACKAGE = 259,
     INTERFACE = 260,
     STRUCT = 261,
     ENUM = 262,
     DEFINE = 263,
     TYPEDEF = 264,
     HANDLE = 265,
     U8 = 266,
     U16 = 267,
     U32 = 268,
     U64 = 269,
     S8 = 270,
     S16 = 271,
     S32 = 272,
     S64 = 273,
     BOOL = 274,
     SEMAPHORE = 275,
     STRING = 276,
     VOID = 277,
     VOIDPTR = 278,
     ERROR = 279,
     IN = 280,
     OUT = 281,
     INOUT = 282,
     COUNT = 283,
     O_BRACE = 284,
     C_BRACE = 285,
     O_PAREN = 286,
     C_PAREN = 287,
     O_BRACKET = 288,
     C_BRACKET = 289,
     END = 290,
     EQUALS = 291,
     COMMA = 292,
     CONST = 293,
     NVDEBUG = 294,
     REFADD = 295,
     REFDEL = 296,
     ID = 297,
     FILE_ID = 298,
     VALUE = 299
   };
#endif
/* Tokens.  */
#define IMPORT 258
#define PACKAGE 259
#define INTERFACE 260
#define STRUCT 261
#define ENUM 262
#define DEFINE 263
#define TYPEDEF 264
#define HANDLE 265
#define U8 266
#define U16 267
#define U32 268
#define U64 269
#define S8 270
#define S16 271
#define S32 272
#define S64 273
#define BOOL 274
#define SEMAPHORE 275
#define STRING 276
#define VOID 277
#define VOIDPTR 278
#define ERROR 279
#define IN 280
#define OUT 281
#define INOUT 282
#define COUNT 283
#define O_BRACE 284
#define C_BRACE 285
#define O_PAREN 286
#define C_PAREN 287
#define O_BRACKET 288
#define C_BRACKET 289
#define END 290
#define EQUALS 291
#define COMMA 292
#define CONST 293
#define NVDEBUG 294
#define REFADD 295
#define REFDEL 296
#define ID 297
#define FILE_ID 298
#define VALUE 299




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 21 "nvidl.y"
{
    char *id;
    int val;
}
/* Line 1489 of yacc.c.  */
#line 142 "nvidl.tab.h"
    YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

