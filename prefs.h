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

#ifndef PREFS_H
#define PREFS_H

#include "config.h"

#include "client.h"

/* Internal configuration file datatypes; these must be exported so
 * that the parser can build the parse tree.  Search for INTERFACE to
 * see the exported interface. */

struct _line;
typedef struct _line line;
struct _context;
typedef struct _context context;
struct _option;
typedef struct _option option;
struct _type;
typedef struct _type type;
struct _keybinding;
typedef struct _keybinding keybinding;
struct _mousebinding;
typedef struct _mousebinding mousebinding;
struct _keybinding;
typedef struct _keyunbinding keyunbinding;
struct _mouseunbinding;
typedef struct _mouseunbinding mouseunbinding;
struct _definition;
typedef struct _definition definition;
struct _function;
typedef struct _function function;
struct _arglist;
typedef struct _arglist arglist;
struct _funclist;
typedef struct _funclist funclist;

struct _line {
    enum { INVALID_LINE, CONTEXT, OPTION, KEYBINDING, MOUSEBINDING,
           KEYUNBINDING, MOUSEUNBINDING, DEFINITION } line_type;
    union {
        context *context;
        option *option;
        keybinding *keybinding;
        keyunbinding *keyunbinding;
        mousebinding *mousebinding;
        mouseunbinding *mouseunbinding;
        definition *definition;
    } line_value;
    int line_number;
    line *line_next;
};

struct _type {
    enum { BOOLEAN = 0, INTEGER, STRING, FOCUS_ENUM,
           CYCLE_ENUM, POSITION_ENUM, RESIZE_ENUM } type_type;
    union {
        int intval;
        char *stringval;
        enum { TYPE_SLOPPY_FOCUS, TYPE_CLICK_TO_FOCUS,
               TYPE_DONT_FOCUS } focus_enum;
        enum { TYPE_SKIP_CYCLE, TYPE_RAISE_IMMEDIATELY,
               TYPE_RAISE_ON_CYCLE_FINISH, TYPE_DONT_RAISE } cycle_enum;
        enum { TYPE_DISPLAY_LEFT, TYPE_DISPLAY_RIGHT,
               TYPE_DISPLAY_CENTERED, TYPE_DONT_DISPLAY } position_enum;
        enum { TYPE_TOPLEFT = 0, TYPE_TOP, TYPE_TOPRIGHT, TYPE_RIGHT,
               TYPE_BOTTOMRIGHT, TYPE_BOTTOM, TYPE_BOTTOMLEFT,
               TYPE_LEFT } resize_enum;
    } type_value;
};

struct _context {
    int context_selector;
    type *context_value;
    line *context_lines;
};

/* values for context_selector */
#define SEL_ISSHAPED 01
#define SEL_INWORKSPACE 02
#define SEL_WINDOWNAME 04
#define SEL_WINDOWCLASS 010
#define SEL_WINDOWINSTANCE 020
#define SEL_NOT 040             /* may be ORed with others */
#define SEL_TRANSIENTFOR 0100   /* may be ORed with others */
#define SEL_HASTRANSIENT 0200   /* may be ORed with others */

/* To add an option, follow the ADDOPT comments through *.l *.y *.h *.c */
/* ADDOPT 4: internal representation of parsed file */
struct _option {
    enum { DISPLAYTITLEBAR,
           TITLEBARFONT,
           OMNIPRESENT,
           DEFAULTWORKSPACE,
           FOCUSPOLICY,
           ALWAYSONTOP,
           ALWAYSONBOTTOM,
           PASSFOCUSCLICK,
           CYCLEBEHAVIOUR,
           COLORTITLEBAR,
           COLORTITLEBARFOCUSED,
           COLORTEXT,
           COLORTEXTFOCUSED,
           NWORKSPACES,
           DONTBINDMOUSE,
           DONTBINDKEYS,
           STICKY,
           TITLEPOSITION,
           KEEPTRANSIENTSONTOP,
           RAISEDELAY } option_name;
    option_setting option_setting;
    type *option_value;
};

struct _keybinding {
    char *keybinding_string;
    int keybinding_depress;
    function *keybinding_function;
};

struct _mousebinding {
    char *mousebinding_string;
    int mousebinding_depress;
    int mousebinding_location;
    function *mousebinding_function;
};

struct _keyunbinding {
    char *keyunbinding_string;
    int keyunbinding_depress;
};

struct _mouseunbinding {
    char *mouseunbinding_string;
    int mouseunbinding_location;
    int mouseunbinding_depress;
};

struct _function {
    enum { SENDTOWORKSPACE = 0,
           GOTOWORKSPACE = 1,
           CYCLENEXT = 2,
           CYCLEPREV = 3,
           KILLNICELY = 4,
           KILLWITHEXTREMEPREJUDICE = 5,
           LAUNCH = 6,
           FOCUS = 7,
           MAXIMIZE = 8,
           MAXIMIZE_V = 9,
           MAXIMIZE_H = 10,
           NOP = 11,
           QUOTE = 12,
           MOVEINTERACTIVELY = 13,
           RESIZEINTERACTIVELY = 14,
           MOVERESIZE = 15,
           QUIT = 16,
           BEEP = 17,
           INVOKE = 18,
           SHOWMENU = 19,
           REFRESH = 20,
           RESTART = 21,
           CRASH = 22,
    } function_type;
    arglist *function_args;
};

struct _definition {
    char *identifier;
    funclist *funclist;
};

struct _funclist {
    function *func;
    funclist *next;
};

struct _arglist {
    type *arglist_arg;
    arglist *arglist_next;
};

/* The first line of the configuration file */
/* only exported because parser needs this */
extern line *preferences;

/* INTERFACE */

/*
 * for main()
 * no dependencies
 */

void prefs_init();

/*
 * There are only two functions to interface to this module.
 * 
 * This function should be called:
 * 1. when the client is created
 * 2. after anything that can be used as a context selector changes
 *    (such as the client's window name or workspace)
 */

void prefs_apply(client_t *client);

#endif /* PREFS_H */
