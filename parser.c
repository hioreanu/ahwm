
/*  A Bison parser, made from parser.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	TOK_DISPLAYTITLEBAR	257
#define	TOK_OMNIPRESENT	258
#define	TOK_DEFAULTWORKSPACE	259
#define	TOK_FOCUS_POLICY	260
#define	TOK_ALWAYSONTOP	261
#define	TOK_ALWAYSONBOTTOM	262
#define	TOK_PASSFOCUSCLICK	263
#define	TOK_CYCLEBEHAVIOUR	264
#define	TOK_COLORTITLEBAR	265
#define	TOK_COLORTITLEBARFOCUSED	266
#define	TOK_COLORTEXT	267
#define	TOK_COLORTEXTFOCUSED	268
#define	TOK_NWORKSPACES	269
#define	TOK_SLOPPY_FOCUS	270
#define	TOK_CLICK_TO_FOCUS	271
#define	TOK_DONT_FOCUS	272
#define	TOK_SKIPCYCLE	273
#define	TOK_RAISEIMMEDIATELY	274
#define	TOK_RAISEONCYCLEFINISH	275
#define	TOK_DONTRAISE	276
#define	TOK_TRUE	277
#define	TOK_FALSE	278
#define	TOK_TRANSIENTFOR	279
#define	TOK_HASTRANSIENT	280
#define	TOK_NOT	281
#define	TOK_ISSHAPED	282
#define	TOK_INWORKSPACE	283
#define	TOK_WINDOWCLASS	284
#define	TOK_WINDOWNAME	285
#define	TOK_WINDOWINSTANCE	286
#define	TOK_FUNCTION	287
#define	TOK_MENU	288
#define	TOK_BINDKEY	289
#define	TOK_BINDBUTTON	290
#define	TOK_BINDDRAG	291
#define	TOK_BINDKEYRELEASE	292
#define	TOK_UNBINDKEY	293
#define	TOK_UNBINDBUTTON	294
#define	TOK_UNBINDDRAG	295
#define	TOK_UNBINDKEYRELEASE	296
#define	TOK_ROOT	297
#define	TOK_FRAME	298
#define	TOK_TITLEBAR	299
#define	TOK_SENDTOWORKSPACE	300
#define	TOK_GOTOWORKSPACE	301
#define	TOK_ALTTAB	302
#define	TOK_KILLNICELY	303
#define	TOK_KILLWITHEXTREMEPREJUDICE	304
#define	TOK_LAUNCH	305
#define	TOK_FOCUS	306
#define	TOK_MAXIMIZE	307
#define	TOK_MAXIMIZE_HORIZONTALLY	308
#define	TOK_MAXIMIZE_VERTICALLY	309
#define	TOK_NOP	310
#define	TOK_QUOTE	311
#define	TOK_MOVEINTERACTIVELY	312
#define	TOK_RESIZEINTERACTIVELY	313
#define	TOK_MOVERESIZE	314
#define	TOK_QUIT	315
#define	TOK_BEEP	316
#define	TOK_INVOKE	317
#define	TOK_SHOWMENU	318
#define	TOK_REFRESH	319
#define	TOK_SEMI	320
#define	TOK_EQUALS	321
#define	TOK_SET_UNCONDITIONALLY	322
#define	TOK_LBRACE	323
#define	TOK_RBRACE	324
#define	TOK_COMMA	325
#define	TOK_LPAREN	326
#define	TOK_RPAREN	327
#define	TOK_STRING	328
#define	TOK_INTEGER	329
#define	TOK_FLOAT	330

#line 1 "parser.y"
                                                      /* -*-Text-*- */
/* $Id$ */
/* Copyright (c) 2001 Alex Hioreanu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Parser definition file for use with bison (not tested with yacc).
 * Should have no s/r or r/r conflicts and doesn't use right recursion
 * anywhere.  I opted for clarity in lieu of brevity because this
 * stuff is complex enough without overloading rules, structures or
 * names.  A simple little parser, no "gotchas".
 */

#include "config.h"

#include "keyboard-mouse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef PARSER_DEBUG
#define parse_debug(x) printf(x)
#else
#define parse_debug(x) /* */
#endif

/* ADDOPT 2: define token */
#line 135 "parser.y"

#include "prefs.h"
line *make_line(int type, void *dollar_one);
char *make_string(char *s);

#line 141 "parser.y"
typedef union {
    int value_int;
    float value_float;
    char *value_string;
    line *value_line;
    option *value_option;
    context *value_context;
    type *value_type;
    keybinding *value_keybinding;
    keyunbinding *value_keyunbinding;
    mousebinding *value_mousebinding;
    mouseunbinding *value_mouseunbinding;
    function *value_function;
    arglist *value_arglist;
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		120
#define	YYFLAG		-32768
#define	YYNTBASE	77

#define YYTRANSLATE(x) ((unsigned)(x) <= 330 ? yytranslate[x] : 97)

static const char yytranslate[] = {     0,
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
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
    67,    68,    69,    70,    71,    72,    73,    74,    75,    76
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     3,     6,     9,    11,    14,    17,    20,    23,
    25,    28,    32,    36,    38,    40,    42,    44,    46,    48,
    50,    52,    54,    56,    58,    60,    62,    64,    66,    68,
    70,    72,    74,    76,    78,    80,    82,    84,    86,    88,
    90,    97,    99,   101,   103,   105,   107,   108,   110,   112,
   114,   118,   122,   127,   132,   135,   138,   142,   146,   148,
   150,   152,   157,   161,   163,   165,   167,   169,   171,   173,
   175,   177,   179,   181,   183,   185,   187,   189,   191,   193,
   195,   197,   199,   201,   205
};

static const short yyrhs[] = {    78,
     0,     0,    78,    79,     0,    80,    66,     0,    86,     0,
    89,    66,     0,    91,    66,     0,    90,    66,     0,    92,
    66,     0,    66,     0,     1,    66,     0,    81,    67,    82,
     0,    81,    68,    82,     0,     3,     0,     4,     0,     5,
     0,     6,     0,     7,     0,     8,     0,     9,     0,    10,
     0,    11,     0,    12,     0,    13,     0,    14,     0,    15,
     0,    85,     0,    74,     0,    75,     0,    83,     0,    84,
     0,    16,     0,    17,     0,    18,     0,    19,     0,    20,
     0,    21,     0,    22,     0,    23,     0,    24,     0,    88,
    87,    82,    69,    78,    70,     0,    28,     0,    29,     0,
    31,     0,    30,     0,    32,     0,     0,    27,     0,    25,
     0,    26,     0,    35,    74,    94,     0,    38,    74,    94,
     0,    36,    93,    74,    94,     0,    37,    93,    74,    94,
     0,    39,    74,     0,    42,    74,     0,    40,    93,    74,
     0,    41,    93,    74,     0,    43,     0,    44,     0,    45,
     0,    95,    72,    96,    73,     0,    95,    72,    73,     0,
    46,     0,    47,     0,    48,     0,    49,     0,    50,     0,
    51,     0,    52,     0,    53,     0,    54,     0,    55,     0,
    56,     0,    57,     0,    58,     0,    59,     0,    60,     0,
    61,     0,    62,     0,    63,     0,    64,     0,    65,     0,
    96,    71,    82,     0,    82,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   177,   179,   180,   194,   195,   196,   197,   198,   199,   200,
   201,   208,   220,   234,   235,   236,   237,   238,   239,   240,
   241,   242,   243,   244,   245,   246,   248,   258,   268,   278,
   288,   299,   300,   301,   303,   304,   305,   306,   308,   309,
   311,   324,   325,   326,   327,   328,   330,   331,   332,   333,
   335,   346,   358,   370,   383,   393,   404,   415,   427,   428,
   429,   431,   440,   450,   451,   452,   453,   454,   455,   456,
   457,   458,   459,   460,   461,   462,   463,   464,   465,   466,
   467,   468,   469,   471,   488
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","TOK_DISPLAYTITLEBAR",
"TOK_OMNIPRESENT","TOK_DEFAULTWORKSPACE","TOK_FOCUS_POLICY","TOK_ALWAYSONTOP",
"TOK_ALWAYSONBOTTOM","TOK_PASSFOCUSCLICK","TOK_CYCLEBEHAVIOUR","TOK_COLORTITLEBAR",
"TOK_COLORTITLEBARFOCUSED","TOK_COLORTEXT","TOK_COLORTEXTFOCUSED","TOK_NWORKSPACES",
"TOK_SLOPPY_FOCUS","TOK_CLICK_TO_FOCUS","TOK_DONT_FOCUS","TOK_SKIPCYCLE","TOK_RAISEIMMEDIATELY",
"TOK_RAISEONCYCLEFINISH","TOK_DONTRAISE","TOK_TRUE","TOK_FALSE","TOK_TRANSIENTFOR",
"TOK_HASTRANSIENT","TOK_NOT","TOK_ISSHAPED","TOK_INWORKSPACE","TOK_WINDOWCLASS",
"TOK_WINDOWNAME","TOK_WINDOWINSTANCE","TOK_FUNCTION","TOK_MENU","TOK_BINDKEY",
"TOK_BINDBUTTON","TOK_BINDDRAG","TOK_BINDKEYRELEASE","TOK_UNBINDKEY","TOK_UNBINDBUTTON",
"TOK_UNBINDDRAG","TOK_UNBINDKEYRELEASE","TOK_ROOT","TOK_FRAME","TOK_TITLEBAR",
"TOK_SENDTOWORKSPACE","TOK_GOTOWORKSPACE","TOK_ALTTAB","TOK_KILLNICELY","TOK_KILLWITHEXTREMEPREJUDICE",
"TOK_LAUNCH","TOK_FOCUS","TOK_MAXIMIZE","TOK_MAXIMIZE_HORIZONTALLY","TOK_MAXIMIZE_VERTICALLY",
"TOK_NOP","TOK_QUOTE","TOK_MOVEINTERACTIVELY","TOK_RESIZEINTERACTIVELY","TOK_MOVERESIZE",
"TOK_QUIT","TOK_BEEP","TOK_INVOKE","TOK_SHOWMENU","TOK_REFRESH","TOK_SEMI","TOK_EQUALS",
"TOK_SET_UNCONDITIONALLY","TOK_LBRACE","TOK_RBRACE","TOK_COMMA","TOK_LPAREN",
"TOK_RPAREN","TOK_STRING","TOK_INTEGER","TOK_FLOAT","config_file","config","line",
"option","option_name","type","focus_enumeration","cycle_enumeration","boolean",
"context","context_name","context_option","keybinding","mousebinding","keyunbinding",
"mouseunbinding","location","function","function_name","arglist", NULL
};
#endif

static const short yyr1[] = {     0,
    77,    78,    78,    79,    79,    79,    79,    79,    79,    79,
    79,    80,    80,    81,    81,    81,    81,    81,    81,    81,
    81,    81,    81,    81,    81,    81,    82,    82,    82,    82,
    82,    83,    83,    83,    84,    84,    84,    84,    85,    85,
    86,    87,    87,    87,    87,    87,    88,    88,    88,    88,
    89,    89,    90,    90,    91,    91,    92,    92,    93,    93,
    93,    94,    94,    95,    95,    95,    95,    95,    95,    95,
    95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
    95,    95,    95,    96,    96
};

static const short yyr2[] = {     0,
     1,     0,     2,     2,     1,     2,     2,     2,     2,     1,
     2,     3,     3,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     6,     1,     1,     1,     1,     1,     0,     1,     1,     1,
     3,     3,     4,     4,     2,     2,     3,     3,     1,     1,
     1,     4,     3,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     3,     1
};

static const short yydefact[] = {     2,
     0,     0,    14,    15,    16,    17,    18,    19,    20,    21,
    22,    23,    24,    25,    26,    49,    50,    48,     0,     0,
     0,     0,     0,     0,     0,     0,    10,     3,     0,     0,
     5,     0,     0,     0,     0,     0,    11,     0,    59,    60,
    61,     0,     0,     0,    55,     0,     0,    56,     4,     0,
     0,    42,    43,    45,    44,    46,     0,     6,     8,     7,
     9,    64,    65,    66,    67,    68,    69,    70,    71,    72,
    73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
    83,    51,     0,     0,     0,    52,    57,    58,    32,    33,
    34,    35,    36,    37,    38,    39,    40,    28,    29,    12,
    30,    31,    27,    13,     0,     0,    53,    54,     2,    63,
    85,     0,     0,     0,    62,    41,    84,     0,     0,     0
};

static const short yydefgoto[] = {   118,
     1,    28,    29,    30,   100,   101,   102,   103,    31,    57,
    32,    33,    34,    35,    36,    42,    82,    83,   112
};

static const short yypact[] = {-32768,
    45,   -65,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   -54,    69,
    69,   -32,   -31,    69,    69,   -30,-32768,-32768,   -19,   -35,
-32768,   -13,    -2,     0,     2,    12,-32768,    79,-32768,-32768,
-32768,     5,    33,    79,-32768,    34,    41,-32768,-32768,    81,
    81,-32768,-32768,-32768,-32768,-32768,    81,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,    46,    79,    79,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,    48,    72,-32768,-32768,-32768,-32768,
-32768,   -50,    -1,    81,-32768,-32768,-32768,   119,   120,-32768
};

static const short yypgoto[] = {-32768,
    13,-32768,-32768,-32768,    10,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,    85,   -22,-32768,-32768
};


#define	YYLAST		156


static const short yytable[] = {     2,
    37,     3,     4,     5,     6,     7,     8,     9,    10,    11,
    12,    13,    14,    15,    52,    53,    54,    55,    56,    38,
   114,    86,   115,    16,    17,    18,   -47,   -47,   -47,   -47,
   -47,    50,    51,    19,    20,    21,    22,    23,    24,    25,
    26,    44,    45,    48,    -1,     2,    49,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
   104,   107,   108,    58,    27,    59,   105,    60,   116,    16,
    17,    18,   -47,   -47,   -47,   -47,   -47,    61,    84,    19,
    20,    21,    22,    23,    24,    25,    26,    89,    90,    91,
    92,    93,    94,    95,    96,    97,    89,    90,    91,    92,
    93,    94,    95,    96,    97,    43,    85,    87,    46,    47,
    27,    39,    40,    41,    88,   111,   109,   106,   119,   120,
     0,   113,     0,   117,    62,    63,    64,    65,    66,    67,
    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
    78,    79,    80,    81,   110,    98,    99,     0,     0,     0,
     0,     0,     0,     0,    98,    99
};

static const short yycheck[] = {     1,
    66,     3,     4,     5,     6,     7,     8,     9,    10,    11,
    12,    13,    14,    15,    28,    29,    30,    31,    32,    74,
    71,    44,    73,    25,    26,    27,    28,    29,    30,    31,
    32,    67,    68,    35,    36,    37,    38,    39,    40,    41,
    42,    74,    74,    74,     0,     1,    66,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    51,    84,    85,    66,    66,    66,    57,    66,    70,    25,
    26,    27,    28,    29,    30,    31,    32,    66,    74,    35,
    36,    37,    38,    39,    40,    41,    42,    16,    17,    18,
    19,    20,    21,    22,    23,    24,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    21,    74,    74,    24,    25,
    66,    43,    44,    45,    74,   106,    69,    72,     0,     0,
    -1,   109,    -1,   114,    46,    47,    48,    49,    50,    51,
    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
    62,    63,    64,    65,    73,    74,    75,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    74,    75
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/share/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 177 "parser.y"
{ preferences = yyvsp[0].value_line; ;
    break;}
case 2:
#line 179 "parser.y"
{ yyval.value_line = NULL; ;
    break;}
case 3:
#line 181 "parser.y"
{
           line *tmp;
           parse_debug("CONFIG\n");
           if (yyvsp[-1].value_line != NULL) {
               tmp = yyvsp[-1].value_line;
               while (tmp->line_next != NULL) tmp = tmp->line_next;
               tmp->line_next = yyvsp[0].value_line;
               yyval.value_line = yyvsp[-1].value_line;
           } else {
               yyval.value_line = yyvsp[0].value_line;
           }
       ;
    break;}
case 4:
#line 194 "parser.y"
{ yyval.value_line = make_line(OPTION, yyvsp[-1].value_option); ;
    break;}
case 5:
#line 195 "parser.y"
{ yyval.value_line = make_line(CONTEXT, yyvsp[0].value_context); ;
    break;}
case 6:
#line 196 "parser.y"
{ yyval.value_line = make_line(KEYBINDING, yyvsp[-1].value_keybinding); ;
    break;}
case 7:
#line 197 "parser.y"
{ yyval.value_line = make_line(KEYUNBINDING, yyvsp[-1].value_keyunbinding); ;
    break;}
case 8:
#line 198 "parser.y"
{ yyval.value_line = make_line(MOUSEBINDING, yyvsp[-1].value_mousebinding); ;
    break;}
case 9:
#line 199 "parser.y"
{ yyval.value_line = make_line(MOUSEUNBINDING, yyvsp[-1].value_mouseunbinding); ;
    break;}
case 10:
#line 200 "parser.y"
{ yyval.value_line = make_line(INVALID_LINE, NULL); ;
    break;}
case 11:
#line 202 "parser.y"
{
          extern int line_number;
          fprintf(stderr, "XWM: parse error on line %d.  Ignoring statement.\n",
                  line_number);
          yyval.value_line = make_line(INVALID_LINE, NULL);
      ;
    break;}
case 12:
#line 209 "parser.y"
{
            option *opt;
            parse_debug("OPTION\n");
            opt = malloc(sizeof(option));
            if (opt != NULL) {
                opt->option_name = yyvsp[-2].value_int;
                opt->option_value = yyvsp[0].value_type;
                opt->option_setting = UserSet;
            }
            yyval.value_option = opt;
        ;
    break;}
case 13:
#line 221 "parser.y"
{
            option *opt;
            parse_debug("OPTION\n");
            opt = malloc(sizeof(option));
            if (opt != NULL) {
                opt->option_name = yyvsp[-2].value_int;
                opt->option_value = yyvsp[0].value_type;
                opt->option_setting = UserOverridden;
            }
            yyval.value_option = opt;
        ;
    break;}
case 14:
#line 234 "parser.y"
{ yyval.value_int = DISPLAYTITLEBAR; ;
    break;}
case 15:
#line 235 "parser.y"
{ yyval.value_int = OMNIPRESENT; ;
    break;}
case 16:
#line 236 "parser.y"
{ yyval.value_int = DEFAULTWORKSPACE; ;
    break;}
case 17:
#line 237 "parser.y"
{ yyval.value_int = FOCUSPOLICY; ;
    break;}
case 18:
#line 238 "parser.y"
{ yyval.value_int = ALWAYSONTOP; ;
    break;}
case 19:
#line 239 "parser.y"
{ yyval.value_int = ALWAYSONBOTTOM; ;
    break;}
case 20:
#line 240 "parser.y"
{ yyval.value_int = PASSFOCUSCLICK; ;
    break;}
case 21:
#line 241 "parser.y"
{ yyval.value_int = CYCLEBEHAVIOUR; ;
    break;}
case 22:
#line 242 "parser.y"
{ yyval.value_int = COLORTITLEBAR; ;
    break;}
case 23:
#line 243 "parser.y"
{ yyval.value_int = COLORTITLEBARFOCUSED; ;
    break;}
case 24:
#line 244 "parser.y"
{ yyval.value_int = COLORTEXT; ;
    break;}
case 25:
#line 245 "parser.y"
{ yyval.value_int = COLORTEXTFOCUSED; ;
    break;}
case 26:
#line 246 "parser.y"
{ yyval.value_int = NWORKSPACES; ;
    break;}
case 27:
#line 249 "parser.y"
{
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = BOOLEAN;
              typ->type_value.intval = yyvsp[0].value_int;
          }
          yyval.value_type = typ;
      ;
    break;}
case 28:
#line 259 "parser.y"
{
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = STRING;
              typ->type_value.stringval = make_string(yyvsp[0].value_string);
          }
          yyval.value_type = typ;
      ;
    break;}
case 29:
#line 269 "parser.y"
{
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = INTEGER;
              typ->type_value.intval = yyvsp[0].value_int;
          }
          yyval.value_type = typ;
      ;
    break;}
case 30:
#line 279 "parser.y"
{
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = FOCUS_ENUM;
              typ->type_value.focus_enum = yyvsp[0].value_int;
          }
          yyval.value_type = typ;
      ;
    break;}
case 31:
#line 289 "parser.y"
{
          type *typ;
          typ = malloc(sizeof(type));
          if (typ != NULL) {
              typ->type_type = CYCLE_ENUM;
              typ->type_value.cycle_enum = yyvsp[0].value_int;
          }
          yyval.value_type = typ;
      ;
    break;}
case 32:
#line 299 "parser.y"
{ yyval.value_int = TYPE_SLOPPY_FOCUS; ;
    break;}
case 33:
#line 300 "parser.y"
{ yyval.value_int = TYPE_CLICK_TO_FOCUS; ;
    break;}
case 34:
#line 301 "parser.y"
{ yyval.value_int = TYPE_DONT_FOCUS; ;
    break;}
case 35:
#line 303 "parser.y"
{ yyval.value_int = TYPE_SKIP_CYCLE ;
    break;}
case 36:
#line 304 "parser.y"
{ yyval.value_int = TYPE_RAISE_IMMEDIATELY ;
    break;}
case 37:
#line 305 "parser.y"
{ yyval.value_int = TYPE_RAISE_ON_CYCLE_FINISH ;
    break;}
case 38:
#line 306 "parser.y"
{ yyval.value_int = TYPE_DONT_RAISE ;
    break;}
case 39:
#line 308 "parser.y"
{ yyval.value_int = 1; ;
    break;}
case 40:
#line 309 "parser.y"
{ yyval.value_int = 0; ;
    break;}
case 41:
#line 312 "parser.y"
{
             context *cntxt;
             parse_debug("CONTEXT\n");
             cntxt = malloc(sizeof(context));
             if (cntxt != NULL) {
                 cntxt->context_selector = yyvsp[-5].value_int | yyvsp[-4].value_int;
                 cntxt->context_value = yyvsp[-3].value_type;
                 cntxt->context_lines = yyvsp[-1].value_line;
             }
             yyval.value_context = cntxt;
         ;
    break;}
case 42:
#line 324 "parser.y"
{ yyval.value_int = SEL_ISSHAPED; ;
    break;}
case 43:
#line 325 "parser.y"
{ yyval.value_int = SEL_INWORKSPACE; ;
    break;}
case 44:
#line 326 "parser.y"
{ yyval.value_int = SEL_WINDOWNAME; ;
    break;}
case 45:
#line 327 "parser.y"
{ yyval.value_int = SEL_WINDOWCLASS; ;
    break;}
case 46:
#line 328 "parser.y"
{ yyval.value_int = SEL_WINDOWINSTANCE; ;
    break;}
case 47:
#line 330 "parser.y"
{ yyval.value_int = 0; ;
    break;}
case 48:
#line 331 "parser.y"
{ yyval.value_int = SEL_NOT; ;
    break;}
case 49:
#line 332 "parser.y"
{ yyval.value_int = SEL_TRANSIENTFOR; ;
    break;}
case 50:
#line 333 "parser.y"
{ yyval.value_int = SEL_HASTRANSIENT; ;
    break;}
case 51:
#line 336 "parser.y"
{
                keybinding *kb;
                kb = malloc(sizeof(keybinding));
                if (kb != NULL) {
                    kb->keybinding_string = make_string(yyvsp[-1].value_string);
                    kb->keybinding_function = yyvsp[0].value_function;
                    kb->keybinding_depress = KEYBOARD_DEPRESS;
                }
                yyval.value_keybinding = kb;
            ;
    break;}
case 52:
#line 347 "parser.y"
{
                keybinding *kb;
                kb = malloc(sizeof(keybinding));
                if (kb != NULL) {
                    kb->keybinding_string = make_string(yyvsp[-1].value_string);
                    kb->keybinding_function = yyvsp[0].value_function;
                    kb->keybinding_depress = KEYBOARD_RELEASE;
                }
                yyval.value_keybinding = kb;
            ;
    break;}
case 53:
#line 359 "parser.y"
{
                  mousebinding *mb;
                  mb = malloc(sizeof(mousebinding));
                  if (mb != NULL) {
                      mb->mousebinding_string = make_string(yyvsp[-1].value_string);
                      mb->mousebinding_location = yyvsp[-2].value_int;
                      mb->mousebinding_function = yyvsp[0].value_function;
                      mb->mousebinding_depress = MOUSE_RELEASE;
                  }
                  yyval.value_mousebinding = mb;
              ;
    break;}
case 54:
#line 371 "parser.y"
{
                  mousebinding *mb;
                  mb = malloc(sizeof(mousebinding));
                  if (mb != NULL) {
                      mb->mousebinding_string = make_string(yyvsp[-1].value_string);
                      mb->mousebinding_location = yyvsp[-2].value_int;
                      mb->mousebinding_function = yyvsp[0].value_function;
                      mb->mousebinding_depress = MOUSE_DEPRESS;
                  }
                  yyval.value_mousebinding = mb;
              ;
    break;}
case 55:
#line 384 "parser.y"
{
                  keyunbinding *kub;
                  kub = malloc(sizeof(keyunbinding));
                  if (kub != NULL) {
                      kub->keyunbinding_string = make_string(yyvsp[0].value_string);
                      kub->keyunbinding_depress = 1;
                  }
                  yyval.value_keyunbinding = kub;
              ;
    break;}
case 56:
#line 394 "parser.y"
{
                  keyunbinding *kub;
                  kub = malloc(sizeof(keyunbinding));
                  if (kub != NULL) {
                      kub->keyunbinding_string = make_string(yyvsp[0].value_string);
                      kub->keyunbinding_depress = 0;
                  }
                  yyval.value_keyunbinding = kub;
              ;
    break;}
case 57:
#line 405 "parser.y"
{
                    mouseunbinding *mub;
                    mub = malloc(sizeof(mouseunbinding));
                    if (mub != NULL) {
                        mub->mouseunbinding_string = make_string(yyvsp[0].value_string);
                        mub->mouseunbinding_location = yyvsp[-1].value_int;
                        mub->mouseunbinding_depress = 0;
                    }
                    yyval.value_mouseunbinding = mub;
                ;
    break;}
case 58:
#line 416 "parser.y"
{
                    mouseunbinding *mub;
                    mub = malloc(sizeof(mouseunbinding));
                    if (mub != NULL) {
                        mub->mouseunbinding_string = make_string(yyvsp[0].value_string);
                        mub->mouseunbinding_location = yyvsp[-1].value_int;
                        mub->mouseunbinding_depress = 1;
                    }
                    yyval.value_mouseunbinding = mub;
                ;
    break;}
case 59:
#line 427 "parser.y"
{ yyval.value_int = MOUSE_ROOT; ;
    break;}
case 60:
#line 428 "parser.y"
{ yyval.value_int = MOUSE_FRAME; ;
    break;}
case 61:
#line 429 "parser.y"
{ yyval.value_int = MOUSE_TITLEBAR; ;
    break;}
case 62:
#line 432 "parser.y"
{
              function *f = malloc(sizeof(function));
              if (f != NULL) {
                  f->function_type = yyvsp[-3].value_int;
                  f->function_args = yyvsp[-1].value_arglist;
              }
              yyval.value_function = f;
          ;
    break;}
case 63:
#line 441 "parser.y"
{
              function *f = malloc(sizeof(function));
              if (f != NULL) {
                  f->function_type = yyvsp[-2].value_int;
                  f->function_args = NULL;
              }
              yyval.value_function = f;
          ;
    break;}
case 64:
#line 450 "parser.y"
{ yyval.value_int = SENDTOWORKSPACE; ;
    break;}
case 65:
#line 451 "parser.y"
{ yyval.value_int = GOTOWORKSPACE; ;
    break;}
case 66:
#line 452 "parser.y"
{ yyval.value_int = ALTTAB; ;
    break;}
case 67:
#line 453 "parser.y"
{ yyval.value_int = KILLNICELY; ;
    break;}
case 68:
#line 454 "parser.y"
{ yyval.value_int = KILLWITHEXTREMEPREJUDICE; ;
    break;}
case 69:
#line 455 "parser.y"
{ yyval.value_int = LAUNCH; ;
    break;}
case 70:
#line 456 "parser.y"
{ yyval.value_int = FOCUS; ;
    break;}
case 71:
#line 457 "parser.y"
{ yyval.value_int = MAXIMIZE; ;
    break;}
case 72:
#line 458 "parser.y"
{ yyval.value_int = MAXIMIZE_H; ;
    break;}
case 73:
#line 459 "parser.y"
{ yyval.value_int = MAXIMIZE_V; ;
    break;}
case 74:
#line 460 "parser.y"
{ yyval.value_int = NOP; ;
    break;}
case 75:
#line 461 "parser.y"
{ yyval.value_int = QUOTE; ;
    break;}
case 76:
#line 462 "parser.y"
{ yyval.value_int = MOVEINTERACTIVELY; ;
    break;}
case 77:
#line 463 "parser.y"
{ yyval.value_int = RESIZEINTERACTIVELY; ;
    break;}
case 78:
#line 464 "parser.y"
{ yyval.value_int = MOVERESIZE; ;
    break;}
case 79:
#line 465 "parser.y"
{ yyval.value_int = QUIT; ;
    break;}
case 80:
#line 466 "parser.y"
{ yyval.value_int = BEEP; ;
    break;}
case 81:
#line 467 "parser.y"
{ yyval.value_int = INVOKE; ;
    break;}
case 82:
#line 468 "parser.y"
{ yyval.value_int = SHOWMENU; ;
    break;}
case 83:
#line 469 "parser.y"
{ yyval.value_int = REFRESH; ;
    break;}
case 84:
#line 472 "parser.y"
{
             arglist *tmp;
             arglist *al;
             if (yyvsp[-2].value_arglist != NULL) {
                 al = malloc(sizeof(arglist));
                 if (al != NULL) {
                     al->arglist_arg = yyvsp[0].value_type;
                     al->arglist_next = NULL;
                     tmp = yyvsp[-2].value_arglist;
                     while (tmp->arglist_next != NULL)
                         tmp = tmp->arglist_next;
                     tmp->arglist_next = al;
                 }
             }
             yyval.value_arglist = yyvsp[-2].value_arglist;
         ;
    break;}
case 85:
#line 489 "parser.y"
{
             arglist *al = malloc(sizeof(arglist));
             if (al != NULL) {
                 al->arglist_arg = yyvsp[0].value_type;
                 al->arglist_next = NULL;
             }
             yyval.value_arglist = al;
         ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/share/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 498 "parser.y"


/*
 * Changes:
 * "blah \"blah" foo qux
 * To:
 * blah "blah
 * Returns newly-malloced string (which is perhaps over-malloced)
 */
char *make_string(char *s)
{
    char *n, *np, *sp, c;

    parse_debug(("String is '%s'\n", s));
    assert(s[0] == '"');
    n = malloc(strlen(s) - 2 + 1); /* minus quotes, plus NUL */
    if (n == NULL) return NULL;
    c = '\0';                   /* c is previous char examined */
    np = n;
    for (sp = s+1; *sp != '\0'; sp++) {
        if (c == '\\') {
            /* deal with escape characters */
            switch (*sp) {
                case 'n':
                    *np++ = '\n';
                    break;
                case 'a':
                    *np++ = '\a';
                    break;
                case 'b':
                    *np++ = '\b';
                    break;
                case 'r':
                    *np++ = '\r';
                    break;
                case 't':
                    *np++ = '\t';
                    break;
                case 'v':
                    *np++ = '\v';
                    break;
                case '"':
                    *np++ = '"';
                    break;
                default:
                    *np++ = '\\';
                    *np++ = *sp;
            }
        } else if (*sp == '"') {
            break;
        } else if (*sp == '\\') {
            ;
        } else {
            *np++ = *sp;
        }
        c = *sp;
    }
    *np = '\0';
    parse_debug(("String is now '%s'\n", n));
    return n;
}
       
line *make_line(int type, void *dollar_one)
{
    line *ln;
    parse_debug("LINE\n");
    ln = malloc(sizeof(line));
    if (ln != NULL) {
        ln->line_type = type;
        ln->line_value.option = dollar_one;
        ln->line_next = NULL;
    }
    return ln;
}
              
int yyerror(char *s)
{
    fprintf(stderr, "%s\n", s);
}

line *preferences = NULL;

#ifdef PARSER_DEBUG
int main(int argc, char **argv)
{
    yyparse();
    printf("preferences = 0x%lx\n", preferences);
}
#endif
