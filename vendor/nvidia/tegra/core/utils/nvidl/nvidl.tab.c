/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 1 "nvidl.y"

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



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 21 "nvidl.y"
{
    char *id;
    int val;
}
/* Line 187 of yacc.c.  */
#line 209 "nvidl.tab.c"
    YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 222 "nvidl.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
         && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
     || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)        \
      do                    \
    {                    \
      YYSIZE_T yyi;                \
      for (yyi = 0; yyi < (Count); yyi++)    \
        (To)[yyi] = (From)[yyi];        \
    }                    \
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)                    \
    do                                    \
      {                                    \
    YYSIZE_T yynewbytes;                        \
    YYCOPY (&yyptr->Stack, Stack, yysize);                \
    Stack = &yyptr->Stack;                        \
    yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
    yyptr += yynewbytes / sizeof (*yyptr);                \
      }                                    \
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  11
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   159

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  45
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  39
/* YYNRULES -- Number of rules.  */
#define YYNRULES  85
/* YYNRULES -- Number of states.  */
#define YYNSTATES  134

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   299

#define YYTRANSLATE(YYX)                        \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    12,    15,    18,    22,
      23,    30,    34,    36,    39,    41,    44,    45,    52,    54,
      57,    59,    61,    63,    65,    67,    69,    71,    73,    75,
      77,    79,    81,    83,    85,    87,    89,    91,    93,    95,
      99,   103,   108,   110,   112,   114,   115,   122,   124,   127,
     131,   138,   140,   142,   144,   145,   152,   154,   157,   160,
     165,   167,   168,   177,   179,   180,   182,   184,   186,   188,
     189,   194,   199,   201,   203,   205,   210,   212,   214,   216,
     219,   222,   225,   226,   231,   233
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      46,     0,    -1,    48,    -1,    52,    -1,    47,    -1,    46,
      48,    -1,    46,    52,    -1,    46,    47,    -1,     3,    43,
      35,    -1,    -1,     4,    42,    29,    49,    50,    30,    -1,
       4,    42,    35,    -1,    51,    -1,    50,    51,    -1,    42,
      -1,    42,    37,    -1,    -1,     5,    42,    53,    29,    54,
      30,    -1,    55,    -1,    54,    55,    -1,    62,    -1,    67,
      -1,    58,    -1,    59,    -1,    60,    -1,    71,    -1,    11,
      -1,    12,    -1,    13,    -1,    14,    -1,    15,    -1,    16,
      -1,    17,    -1,    18,    -1,    19,    -1,    24,    -1,    56,
      -1,    21,    -1,    42,    -1,     8,    42,    44,    -1,    10,
      42,    35,    -1,     9,    61,    42,    35,    -1,     6,    -1,
       7,    -1,    56,    -1,    -1,     6,    42,    63,    29,    64,
      30,    -1,    65,    -1,    64,    65,    -1,    66,    42,    35,
      -1,    66,    42,    33,    44,    34,    35,    -1,    58,    -1,
      57,    -1,    23,    -1,    -1,     7,    42,    68,    29,    69,
      30,    -1,    70,    -1,    69,    70,    -1,    42,    37,    -1,
      42,    36,    44,    37,    -1,    58,    -1,    -1,    73,    74,
      42,    72,    31,    75,    32,    35,    -1,    39,    -1,    -1,
      57,    -1,    22,    -1,    22,    -1,    77,    -1,    -1,    75,
      37,    76,    77,    -1,    79,    83,    78,    42,    -1,    57,
      -1,    20,    -1,    23,    -1,    33,    80,    81,    34,    -1,
      25,    -1,    27,    -1,    26,    -1,    37,    82,    -1,    37,
      40,    -1,    37,    41,    -1,    -1,    28,    31,    42,    32,
      -1,    38,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    37,    37,    38,    39,    40,    41,    42,    45,    49,
      49,    50,    54,    55,    59,    60,    63,    63,    67,    68,
      72,    73,    74,    75,    76,    77,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    94,    95,    96,    99,
     102,   105,   109,   110,   111,   114,   114,   118,   119,   123,
     124,   125,   129,   130,   133,   133,   137,   138,   142,   143,
     144,   147,   147,   151,   152,   156,   157,   161,   162,   163,
     163,   167,   171,   172,   173,   177,   181,   182,   183,   186,
     187,   188,   189,   193,   197,   198
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IMPORT", "PACKAGE", "INTERFACE",
  "STRUCT", "ENUM", "DEFINE", "TYPEDEF", "HANDLE", "U8", "U16", "U32",
  "U64", "S8", "S16", "S32", "S64", "BOOL", "SEMAPHORE", "STRING", "VOID",
  "VOIDPTR", "ERROR", "IN", "OUT", "INOUT", "COUNT", "O_BRACE", "C_BRACE",
  "O_PAREN", "C_PAREN", "O_BRACKET", "C_BRACKET", "END", "EQUALS", "COMMA",
  "CONST", "NVDEBUG", "REFADD", "REFDEL", "ID", "FILE_ID", "VALUE",
  "$accept", "top", "import", "package", "@1", "package_decls",
  "package_decl", "interface", "@2", "interface_decls", "interface_decl",
  "basic_type", "complex_type", "define_decl", "handle_decl",
  "typedef_decl", "typedef_type", "struct_decl", "@3",
  "struct_field_decls", "struct_field_decl", "struct_type", "enum_decl",
  "@4", "enum_field_decls", "enum_field_decl", "function_decl", "@5",
  "func_debug", "func_ret_type", "func_args", "@6", "func_arg",
  "func_type", "param_pass", "pass_type", "param_attribute",
  "count_attribute", "const", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    45,    46,    46,    46,    46,    46,    46,    47,    49,
      48,    48,    50,    50,    51,    51,    53,    52,    54,    54,
      55,    55,    55,    55,    55,    55,    56,    56,    56,    56,
      56,    56,    56,    56,    56,    56,    57,    57,    57,    58,
      59,    60,    61,    61,    61,    63,    62,    64,    64,    65,
      65,    65,    66,    66,    68,    67,    69,    69,    70,    70,
      70,    72,    71,    73,    73,    74,    74,    75,    75,    76,
      75,    77,    78,    78,    78,    79,    80,    80,    80,    81,
      81,    81,    81,    82,    83,    83
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     2,     2,     2,     3,     0,
       6,     3,     1,     2,     1,     2,     0,     6,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     4,     1,     1,     1,     0,     6,     1,     2,     3,
       6,     1,     1,     1,     0,     6,     1,     2,     2,     4,
       1,     0,     8,     1,     0,     1,     1,     1,     1,     0,
       4,     4,     1,     1,     1,     4,     1,     1,     1,     2,
       2,     2,     0,     4,     1,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     4,     2,     3,     0,     0,
      16,     1,     7,     5,     6,     8,     9,    11,     0,     0,
      64,    14,     0,    12,     0,     0,     0,     0,     0,    63,
      64,    18,    22,    23,    24,    20,    21,    25,     0,    15,
      10,    13,    45,    54,     0,    42,    43,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    44,     0,     0,
      17,    19,    37,    66,    38,    36,    65,     0,     0,     0,
      39,     0,    40,    61,     0,     0,    41,     0,    53,    52,
      51,     0,    47,     0,     0,    60,     0,    56,     0,    46,
      48,     0,     0,    58,    55,    57,    67,     0,     0,    68,
      85,     0,    49,     0,    76,    78,    77,    82,     0,    69,
      84,     0,     0,    59,     0,     0,    62,     0,    73,    74,
      72,     0,     0,     0,    80,    81,    79,    75,    70,    71,
      50,     0,     0,    83
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     4,     5,     6,    19,    22,    23,     7,    18,    30,
      31,    65,    79,    32,    33,    34,    58,    35,    68,    81,
      82,    83,    36,    69,    86,    87,    37,    77,    38,    67,
      98,   117,    99,   121,   100,   107,   115,   126,   111
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -42
static const yytype_int8 yypact[] =
{
      66,   -41,    -4,    -3,    63,   -42,   -42,   -42,    15,    19,
     -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,    24,    22,
      50,    49,   -27,   -42,    30,    57,    60,    98,    65,   -42,
      35,   -42,   -42,   -42,   -42,   -42,   -42,   -42,    79,   -42,
     -42,   -42,   -42,   -42,    76,   -42,   -42,   -42,   -42,   -42,
     -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,    81,    73,
     -42,   -42,   -42,   -42,   -42,   -42,   -42,    82,   103,   104,
     -42,    99,   -42,   -42,    13,     5,   -42,   105,   -42,   -42,
     -42,    -7,   -42,    93,   -17,   -42,    10,   -42,     0,   -42,
     -42,    96,    94,   -42,   -42,   -42,   -42,   101,    14,   -42,
     102,    95,   -42,   100,   -42,   -42,   -42,   106,   107,   -42,
     -42,    64,   110,   -42,    21,   111,   -42,   108,   -42,   -42,
     -42,   109,   112,   115,   -42,   -42,   -42,   -42,   -42,   -42,
     -42,   113,   116,   -42
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -42,   -42,   145,   146,   -42,   -42,   130,   149,   -42,   -42,
     124,   129,   -38,    44,   -42,   -42,   -42,   -42,   -42,   -42,
      77,   -42,   -42,   -42,   -42,    71,   -42,   -42,   -42,   -42,
     -42,   -42,    42,   -42,   -42,   -42,   -42,   -42,   -42
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      66,    26,     8,    40,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    26,    62,    21,    78,    56,    26,    92,
      93,    26,    96,    89,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    97,    62,    64,    78,    56,     9,    10,
      94,    24,    25,    26,    27,    28,   108,    84,    16,   123,
      15,   109,    84,    20,    17,    64,    24,    25,    26,    27,
      28,   124,   125,    11,    21,    60,     1,     2,     3,     1,
       2,     3,    42,   120,    29,    47,    48,    49,    50,    51,
      52,    53,    54,    55,   118,    62,    39,   119,    56,    29,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    43,
      62,    63,    44,    56,    45,    46,    64,    59,    72,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    80,    85,
      70,    64,    56,    71,    73,    80,   104,   105,   106,   101,
      85,   102,    74,    75,    76,    91,    88,   113,   103,   112,
     110,    97,   116,   114,   122,   127,   131,   130,   133,    12,
      13,   129,    41,    14,    61,   132,    57,    95,    90,   128
};

static const yytype_uint8 yycheck[] =
{
      38,     8,    43,    30,    11,    12,    13,    14,    15,    16,
      17,    18,    19,     8,    21,    42,    23,    24,     8,    36,
      37,     8,    22,    30,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    33,    21,    42,    23,    24,    42,    42,
      30,     6,     7,     8,     9,    10,    32,    42,    29,    28,
      35,    37,    42,    29,    35,    42,     6,     7,     8,     9,
      10,    40,    41,     0,    42,    30,     3,     4,     5,     3,
       4,     5,    42,   111,    39,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    37,    23,    24,    39,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    42,
      21,    22,    42,    24,     6,     7,    42,    42,    35,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    74,    75,
      44,    42,    24,    42,    42,    81,    25,    26,    27,    33,
      86,    35,    29,    29,    35,    42,    31,    37,    44,    44,
      38,    33,    35,    37,    34,    34,    31,    35,    32,     4,
       4,    42,    22,     4,    30,    42,    27,    86,    81,   117
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,    46,    47,    48,    52,    43,    42,
      42,     0,    47,    48,    52,    35,    29,    35,    53,    49,
      29,    42,    50,    51,     6,     7,     8,     9,    10,    39,
      54,    55,    58,    59,    60,    62,    67,    71,    73,    37,
      30,    51,    42,    42,    42,     6,     7,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    24,    56,    61,    42,
      30,    55,    21,    22,    42,    56,    57,    74,    63,    68,
      44,    42,    35,    42,    29,    29,    35,    72,    23,    57,
      58,    64,    65,    66,    42,    58,    69,    70,    31,    30,
      65,    42,    36,    37,    30,    70,    22,    33,    75,    77,
      79,    33,    35,    44,    25,    26,    27,    80,    32,    37,
      38,    83,    44,    37,    37,    81,    35,    76,    20,    23,
      57,    78,    34,    28,    40,    41,    82,    34,    77,    42,
      35,    31,    42,    32
};

#define yyerrok        (yyerrstatus = 0)
#define yyclearin    (yychar = YYEMPTY)
#define YYEMPTY        (-2)
#define YYEOF        0

#define YYACCEPT    goto yyacceptlab
#define YYABORT        goto yyabortlab
#define YYERROR        goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL        goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                    \
do                                \
  if (yychar == YYEMPTY && yylen == 1)                \
    {                                \
      yychar = (Token);                        \
      yylval = (Value);                        \
      yytoken = YYTRANSLATE (yychar);                \
      YYPOPSTACK (1);                        \
      goto yybackup;                        \
    }                                \
  else                                \
    {                                \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                            \
    }                                \
while (YYID (0))


#define YYTERROR    1
#define YYERRCODE    256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                \
    do                                    \
      if (YYID (N))                                                    \
    {                                \
      (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;    \
      (Current).first_column = YYRHSLOC (Rhs, 1).first_column;    \
      (Current).last_line    = YYRHSLOC (Rhs, N).last_line;        \
      (Current).last_column  = YYRHSLOC (Rhs, N).last_column;    \
    }                                \
      else                                \
    {                                \
      (Current).first_line   = (Current).last_line   =        \
        YYRHSLOC (Rhs, 0).last_line;                \
      (Current).first_column = (Current).last_column =        \
        YYRHSLOC (Rhs, 0).last_column;                \
    }                                \
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)            \
     fprintf (File, "%d.%d-%d.%d",            \
          (Loc).first_line, (Loc).first_column,    \
          (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)            \
do {                        \
  if (yydebug)                    \
    YYFPRINTF Args;                \
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)              \
do {                                      \
  if (yydebug)                                  \
    {                                      \
      YYFPRINTF (stderr, "%s ", Title);                      \
      yy_symbol_print (stderr,                          \
          Type, Value); \
      YYFPRINTF (stderr, "\n");                          \
    }                                      \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
    break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                \
do {                                \
  if (yydebug)                            \
    yy_stack_print ((Bottom), (Top));                \
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
         yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
               &(yyvsp[(yyi + 1) - (yynrhs)])
                              );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)        \
do {                    \
  if (yydebug)                \
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef    YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
    switch (*++yyp)
      {
      case '\'':
      case ',':
        goto do_not_strip_quotes;

      case '\\':
        if (*++yyp != '\\')
          goto do_not_strip_quotes;
        /* Fall through.  */
      default:
        if (yyres)
          yyres[yyn] = *yyp;
        yyn++;
        break;

      case '"':
        if (yyres)
          yyres[yyn] = '\0';
        return yyn;
      }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
     constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
            + sizeof yyexpecting - 1
            + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
               * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
     YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
      {
        if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
          {
        yycount = 1;
        yysize = yysize0;
        yyformat[sizeof yyunexpected - 1] = '\0';
        break;
          }
        yyarg[yycount++] = yytname[yyx];
        yysize1 = yysize + yytnamerr (0, yytname[yyx]);
        yysize_overflow |= (yysize1 < yysize);
        yysize = yysize1;
        yyfmt = yystpcpy (yyfmt, yyprefix);
        yyprefix = yyor;
      }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
    return YYSIZE_MAXIMUM;

      if (yyresult)
    {
      /* Avoid sprintf, as that infringes on the user's name space.
         Don't have undefined behavior even if the translation
         produced a string with the wrong number of "%s"s.  */
      char *yyp = yyresult;
      int yyi = 0;
      while ((*yyp = *yyf) != '\0')
        {
          if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyf += 2;
        }
          else
        {
          yyp++;
          yyf++;
        }
        }
    }
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
    break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;        /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
    /* Give user a chance to reallocate the stack.  Use copies of
       these so that the &'s don't force the real ones into
       memory.  */
    YYSTYPE *yyvs1 = yyvs;
    yytype_int16 *yyss1 = yyss;


    /* Each stack pointer address is followed by the size of the
       data in use in that stack, in bytes.  This used to be a
       conditional around just the two extra args, but that might
       be undefined if yyoverflow is a macro.  */
    yyoverflow (YY_("memory exhausted"),
            &yyss1, yysize * sizeof (*yyssp),
            &yyvs1, yysize * sizeof (*yyvsp),

            &yystacksize);

    yyss = yyss1;
    yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
    goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
    yystacksize = YYMAXDEPTH;

      {
    yytype_int16 *yyss1 = yyss;
    union yyalloc *yyptr =
      (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
    if (! yyptr)
      goto yyexhaustedlab;
    YYSTACK_RELOCATE (yyss);
    YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
    if (yyss1 != yyssa)
      YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
          (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
    YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
    goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 8:
#line 45 "nvidl.y"
    { NvIdlImport( (yyvsp[(2) - (3)].id) ); ;}
    break;

  case 9:
#line 49 "nvidl.y"
    { NvIdlPackageBegin( (yyvsp[(2) - (3)].id) ); ;}
    break;

  case 10:
#line 49 "nvidl.y"
    { NvIdlPackageEnd( (yyvsp[(2) - (6)].id) ); ;}
    break;

  case 11:
#line 50 "nvidl.y"
    { NvIdlPackage( (yyvsp[(2) - (3)].id) ); ;}
    break;

  case 14:
#line 59 "nvidl.y"
    { NvIdlPackageEntry( (yyvsp[(1) - (1)].id) ); ;}
    break;

  case 15:
#line 60 "nvidl.y"
    { NvIdlPackageEntry( (yyvsp[(1) - (2)].id) ); ;}
    break;

  case 16:
#line 63 "nvidl.y"
    { NvError err = NvIdlInterfaceBegin( (yyvsp[(2) - (2)].id) ); if( err != NvSuccess ) exit(0); ;}
    break;

  case 17:
#line 63 "nvidl.y"
    { NvIdlInterfaceEnd( (yyvsp[(2) - (6)].id) ); ;}
    break;

  case 26:
#line 81 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_U8 ); ;}
    break;

  case 27:
#line 82 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_U16 ); ;}
    break;

  case 28:
#line 83 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_U32 ); ;}
    break;

  case 29:
#line 84 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_U64 ); ;}
    break;

  case 30:
#line 85 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_S8 ); ;}
    break;

  case 31:
#line 86 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_S16 ); ;}
    break;

  case 32:
#line 87 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_S32 ); ;}
    break;

  case 33:
#line 88 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_S64 ); ;}
    break;

  case 34:
#line 89 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_Bool ); ;}
    break;

  case 35:
#line 90 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_Error ); ;}
    break;

  case 37:
#line 95 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_String ); ;}
    break;

  case 38:
#line 96 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_ID ); NvIdlSetCurrentId( (yyvsp[(1) - (1)].id) ); ;}
    break;

  case 39:
#line 99 "nvidl.y"
    { NvIdlDefine( (yyvsp[(2) - (3)].id) ); ;}
    break;

  case 40:
#line 102 "nvidl.y"
    { NvIdlHandle( (yyvsp[(2) - (3)].id) ); ;}
    break;

  case 41:
#line 105 "nvidl.y"
    { NvIdlTypedef( (yyvsp[(3) - (4)].id) ); ;}
    break;

  case 42:
#line 109 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_Struct ); ;}
    break;

  case 43:
#line 110 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_Enum ); ;}
    break;

  case 45:
#line 114 "nvidl.y"
    { NvIdlStructBegin( (yyvsp[(2) - (2)].id) ); ;}
    break;

  case 46:
#line 114 "nvidl.y"
    { NvIdlStructEnd( (yyvsp[(2) - (6)].id) ); ;}
    break;

  case 49:
#line 123 "nvidl.y"
    { NvIdlStructField( (yyvsp[(2) - (3)].id) ); ;}
    break;

  case 50:
#line 124 "nvidl.y"
    { NvIdlSetArraySize( (yyvsp[(4) - (6)].val) ); NvIdlStructField( (yyvsp[(2) - (6)].id) ); ;}
    break;

  case 53:
#line 130 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_VoidPtr ); ;}
    break;

  case 54:
#line 133 "nvidl.y"
    { NvIdlEnumBegin( (yyvsp[(2) - (2)].id) ); ;}
    break;

  case 55:
#line 133 "nvidl.y"
    { NvIdlEnumEnd( (yyvsp[(2) - (6)].id) ); ;}
    break;

  case 58:
#line 142 "nvidl.y"
    { NvIdlEnumField( (yyvsp[(1) - (2)].id), NV_FALSE ); ;}
    break;

  case 59:
#line 143 "nvidl.y"
    { NvIdlEnumField( (yyvsp[(1) - (4)].id), NV_TRUE ); ;}
    break;

  case 61:
#line 147 "nvidl.y"
    { NvIdlFunctionBegin( (yyvsp[(3) - (3)].id) ); ;}
    break;

  case 62:
#line 147 "nvidl.y"
    { NvIdlFunctionEnd(); ;}
    break;

  case 63:
#line 151 "nvidl.y"
    { NvIdlSetFunctionDebug( NV_TRUE ); ;}
    break;

  case 66:
#line 157 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_Void ); ;}
    break;

  case 69:
#line 163 "nvidl.y"
    { NvIdlFunctionParamNext(); ;}
    break;

  case 71:
#line 167 "nvidl.y"
    { NvIdlFunctionParam( (yyvsp[(4) - (4)].id) ); ;}
    break;

  case 73:
#line 172 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_Semaphore ); ;}
    break;

  case 74:
#line 173 "nvidl.y"
    { NvIdlSetCurrentType( NvIdlType_VoidPtr ); ;}
    break;

  case 76:
#line 181 "nvidl.y"
    { NvIdlFunctionParamPass( NvIdlPassType_In ); ;}
    break;

  case 77:
#line 182 "nvidl.y"
    { NvIdlFunctionParamPass( NvIdlPassType_InOut ); ;}
    break;

  case 78:
#line 183 "nvidl.y"
    { NvIdlFunctionParamPass( NvIdlPassType_Out ); ;}
    break;

  case 80:
#line 187 "nvidl.y"
    { NvIdlParamSetRef( NvIdlRefType_Add ); ;}
    break;

  case 81:
#line 188 "nvidl.y"
    { NvIdlParamSetRef( NvIdlRefType_Del ); ;}
    break;

  case 83:
#line 193 "nvidl.y"
    { NvIdlParamAttrCount( (yyvsp[(3) - (4)].id) ); ;}
    break;

  case 84:
#line 197 "nvidl.y"
    { NvIdlSetConst(); ;}
    break;


/* Line 1267 of yacc.c.  */
#line 1797 "nvidl.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
    YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
    if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
      {
        YYSIZE_T yyalloc = 2 * yysize;
        if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
          yyalloc = YYSTACK_ALLOC_MAXIMUM;
        if (yymsg != yymsgbuf)
          YYSTACK_FREE (yymsg);
        yymsg = (char *) YYSTACK_ALLOC (yyalloc);
        if (yymsg)
          yymsg_alloc = yyalloc;
        else
          {
        yymsg = yymsgbuf;
        yymsg_alloc = sizeof yymsgbuf;
          }
      }

    if (0 < yysize && yysize <= yymsg_alloc)
      {
        (void) yysyntax_error (yymsg, yystate, yychar);
        yyerror (yymsg);
      }
    else
      {
        yyerror (YY_("syntax error"));
        if (yysize != 0)
          goto yyexhaustedlab;
      }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
     error, discard it.  */

      if (yychar <= YYEOF)
    {
      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        YYABORT;
    }
      else
    {
      yydestruct ("Error: discarding",
              yytoken, &yylval);
      yychar = YYEMPTY;
    }
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;    /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
    {
      yyn += YYTERROR;
      if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
        {
          yyn = yytable[yyn];
          if (0 < yyn)
        break;
        }
    }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
    YYABORT;


      yydestruct ("Error: popping",
          yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
         yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
          yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 201 "nvidl.y"


int main( int argc, char *argv[] )
{
    return NvIdlMain( argc, argv );
}

