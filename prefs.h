/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
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
    enum { BOOLEAN, INTEGER, STRING } type_type;
    union {
        int intval;
        char *stringval;
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
    enum { DISPLAYTITLEBAR, DEFAULTWORKSPACE } option_name;
    type *option_value;
};

struct _keybinding {
    char *keybinding_string;
    function *keybinding_function;
};

enum location_enum { TITLEBAR, FRAME, ROOT };

struct _mousebinding {
    char *mousebinding_string;
    enum location_enum mousebinding_location;
    function *mousebinding_function;
};

struct _keyunbinding {
    char *keyunbinding_string;
};

struct _mouseunbinding {
    char *mouseunbinding_string;
    enum location_enum mouseunbinding_location;
};

struct _function {
    enum { MOVETOWORKSPACE,
           GOTOWORKSPACE,
           ALTTAB,
           KILLNICELY,
           KILLWITHEXTREMEPREJUDICE,
           LAUNCH,
           FOCUS,
           MAXIMIZE,
           NOP,
           QUOTE,
           MOVEINTERACTIVELY,
           RESIZEINTERACTIVELY,
           MOVERESIZE,
           QUIT,
           BEEP,
           INVOKE,
           SHOWMENU,
           REFRESH,
    } function_type;
    arglist *function_args;
};

struct _arglist {
    type *arglist_arg;
    arglist *arglist_next;
};

/* INTERFACE */

/* The first line of the configuration file */
extern line *preferences;

/* The defaults from the configuration file */
extern int pref_no_workspaces;
extern Bool pref_display_titlebar;
extern int pref_default_workspace;

void prefs_apply(client_t *client, line *prefs);

#endif /* PREFS_H */
