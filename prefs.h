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
struct _function;
typedef struct _function function;
struct _arglist;
typedef struct _arglist arglist;

struct _line {
    enum { INVALID_LINE, CONTEXT, OPTION, KEYBINDING, MOUSEBINDING,
           KEYUNBINDING, MOUSEUNBINDING } line_type;
    union {
        context *context;
        option *option;
        keybinding *keybinding;
        keyunbinding *keyunbinding;
        mousebinding *mousebinding;
        mouseunbinding *mouseunbinding;
    } line_value;
    line *line_next;
};

struct _type {
    enum { BOOLEAN, INTEGER, STRING, FOCUS_ENUM } type_type;
    union {
        int intval;
        char *stringval;
        enum { TYPE_SLOPPY_FOCUS, TYPE_CLICK_TO_FOCUS,
               TYPE_DONT_FOCUS } focus_enum;
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

struct _option {
    enum { DISPLAYTITLEBAR,
           OMNIPRESENT,
           SKIPALTTAB,
           DEFAULTWORKSPACE,
           FOCUSPOLICY,
           NUMBEROFWORKSPACES } option_name;
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
    enum { MOVETOWORKSPACE = 0,
           GOTOWORKSPACE = 1,
           ALTTAB = 2,
           KILLNICELY = 3,
           KILLWITHEXTREMEPREJUDICE = 4,
           LAUNCH = 5,
           FOCUS = 6,
           MAXIMIZE = 7,
           NOP = 8,
           QUOTE = 9,
           MOVEINTERACTIVELY = 10,
           RESIZEINTERACTIVELY = 11,
           MOVERESIZE = 12,
           QUIT = 13,
           BEEP = 14,
           INVOKE = 15,
           SHOWMENU = 16,
           REFRESH = 17,
    } function_type;
    arglist *function_args;
};

struct _arglist {
    type *arglist_arg;
    arglist *arglist_next;
};

/* The first line of the configuration file */
/* only exported because parser needs this */
extern line *preferences;

/* INTERFACE */

/* The defaults from the configuration file */
extern int pref_no_workspaces;
extern Bool pref_display_titlebar;
extern int pref_default_workspace;

void prefs_apply(client_t *client);

#endif /* PREFS_H */
