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
 * There are three basic things one can find in a configuration file:
 * an option, a context and a binding.  For efficiency, global options
 * and bindings are filtered out at the beginning.
 * 
 * FIXME:  need to figure out some way to allow keybinding changes at
 * any time without binding/unbinding keys all the time.  May need to
 * make caller specify what has changed in some way.  sucks.
 * 
 * For now, only allow bindings to happen in the global context.
 * App-specific bindings added later.
 */

#include "config.h"

#include <X11/Xlib.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "compat.h"
#include "prefs.h"
#include "parser.h"
#include "debug.h"
#include "workspace.h"
#include "keyboard-mouse.h"
#include "focus.h"
#include "kill.h"
#include "move-resize.h"
#include "malloc.h"
#include "ewmh.h"
#include "paint.h"
#include "stacking.h"

#include "default-ahwmrc.h"
#include "default-message.h"

extern void yyparse();

#define CHECK_BOOL(x) ((x)->type_type == BOOLEAN ? True : False)
#define CHECK_STRING(x) ((x)->type_type == STRING ? True : False)
#define CHECK_INT(x) ((x)->type_type == INTEGER ? True : False)

/* ADDOPT 5: add to internal representation of option aggregations */
typedef struct _prefs {
    Bool titlebar;
    option_setting titlebar_set;
    Bool omnipresent;
    option_setting omnipresent_set;
    int workspace;
    option_setting workspace_set;
    int focus_policy;
    option_setting focus_policy_set;
    Bool always_on_top;
    option_setting always_on_top_set;
    Bool always_on_bottom;
    option_setting always_on_bottom_set;
    Bool pass_focus_click;
    option_setting pass_focus_click_set;
    int cycle_behaviour;
    option_setting cycle_behaviour_set;
    char *titlebar_color;
    option_setting titlebar_color_set;
    char *titlebar_focused_color;
    option_setting titlebar_focused_color_set;
    char *titlebar_text_color;
    option_setting titlebar_text_color_set;
    char *titlebar_text_focused_color;
    option_setting titlebar_text_focused_color_set;
    Bool dont_bind_mouse;
    option_setting dont_bind_mouse_set;
    Bool dont_bind_keys;
    option_setting dont_bind_keys_set;
    Bool sticky;
    option_setting sticky_set;
    int title_position;
    option_setting title_position_set;
    Bool keep_transients_on_top;
    option_setting keep_transients_on_top_set;
    int raise_delay;
    option_setting raise_delay_set;
    Bool use_net_wm_pid;
    option_setting use_net_wm_pid_set;
} prefs;

/* ADDOPT 6: set default value */
static prefs defaults = {
    True, UserSet,                /* titlebar */
    False, UserSet,               /* omnipresent */
    0, UserSet,                   /* workspace */
    TYPE_SLOPPY_FOCUS, UserSet,   /* focus_policy */
    False, UserSet,               /* always_on_top */
    False, UserSet,               /* always_on_bottom */
    True, UserSet,                /* pass_focus_click */
    TYPE_RAISE_IMMEDIATELY, UserSet, /* cycle_behaviour */
    NULL, UserSet,                /* titlebar_color */
    NULL, UserSet,                /* titlebar_focused_color */
    NULL, UserSet,                /* titlebar_text_color */
    NULL, UserSet,                /* titlebar_text_focused_color */
    False, UserSet,               /* dont_bind_mouse */
    False, UserSet,               /* dont_bind_keys */
    False, UserSet,               /* sticky */
    DisplayLeft, UserSet,         /* title_position */
    True, UserSet,                /* keep_transients_on_top */
    0, UserSet,                   /* raise_delay */
    True, UserSet,               /* use_net_wm_pid */
};

/* names of the types
 * used only for displaying to user
 * matches up with enumeration in struct _type in prefs.h
 */
static char *type_names[] = {
    "boolean", "integer", "string", "focus enumeration",
    "cycle enumeration", "position enumeration", "resize enumeration" 
};

static line *contexts;
static definition **definitions = NULL;
static int ndefinitions = 0;

static void make_definition(definition *def);
static void invocation_string_to_int(arglist *arg);
static void get_int(type *typ, int *val);
static void get_string(type *typ, char **val);
static void get_bool(type *typ, Bool *val);
static Bool context_applies(client_t *client, context *cntxt);
static void option_apply(client_t *client, option *opt, prefs *p);
static line *type_check(line *block);
static Bool type_check_context(context *cntxt);
static Bool type_check_option(option *opt);
static Bool type_check_keybinding(keybinding *kb);
static Bool type_check_mousebinding(mousebinding *mb);
static Bool type_check_function(function *fn);
static Bool type_check_definition(definition *def);
static char *get_type_name(int typ);
static Bool type_check_helper(type *typ, int expected,
                              char *tok, char *tok_type);
static void keybinding_apply(client_t *client, keybinding *kb);
static void keyunbinding_apply(client_t *client, keyunbinding *kb);
static void mousebinding_apply(client_t *client, mousebinding *kb);
static void mouseunbinding_apply(client_t *client, mouseunbinding *kb);
static void prefs_apply_internal(client_t *client, line *block, prefs *p);
static void globally_bind(line *lp);
static void globally_unbind(line *lp);
static int no_config(char *ahwmrc_path);
static void invoke(XEvent *e, arglist *args);
static void focus(XEvent *e, arglist *args);
static void crash(XEvent *e, arglist *args);
static void xmessage();

/* must follow declarations */
static key_fn fn_table[] = {
/* 0  */    workspace_client_moveto_bindable,
/* 1  */    workspace_goto_bindable,
/* 2  */    focus_cycle_next,
/* 3  */    focus_cycle_prev,
/* 4  */    kill_nicely,
/* 5  */    kill_with_extreme_prejudice,
/* 6  */    run_program,
/* 7  */    focus,
/* 8  */    resize_maximize,
/* 9  */    resize_maximize_vertically,
/* 10 */    resize_maximize_horizontally,
/* 11 */    keyboard_ignore,
/* 12 */    keyboard_quote,
/* 13 */    move_client,
/* 14 */    resize_client,
/* 15 */    move_resize, /* non-interactive move/resize, must implement */
/* 16 */    ahwm_quit,
/* 17 */    NULL, /* beep */
/* 18 */    invoke,
/* 19 */    NULL, /* expansion for menu system */
/* 20 */    NULL, /* refresh/reset */
/* 21 */    ahwm_restart,
/* 22 */    crash,
};

static char *fn_names[] = {
/* 0  */    "SendToWorkspace",
/* 1  */    "GotoWorkspace",
/* 2  */    "CycleNext",
/* 3  */    "CyclePrevious",
/* 4  */    "KillNicely",
/* 5  */    "KillWithExtremePrejudice",
/* 6  */    "Launch",
/* 7  */    "Focus",
/* 8  */    "Maxmize",
/* 9  */    "MaximizeVertically",
/* 10 */    "MaximizeHorizontally",
/* 11 */    "Nop",
/* 12 */    "Quote",
/* 13 */    "MoveInteractively",
/* 14 */    "ResizeInteractively",
/* 15 */    "MoveResize",
/* 16 */    "Quit",
/* 17 */    "Beep",
/* 18 */    "Invoke",
/* 19 */    "ShowMenu",
/* 20 */    "Refresh",
/* 21 */    "Restart",
/* 22 */    "Crash",
};

void prefs_init()
{
    char buf[PATH_MAX];
    char *home;
    line *lp, *first_context, *last_context, *last_line, *first_line;
    extern FILE *yyin;
    int i;

    home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "AHWM: Could not get home directory; "
                "configuration file not found.\n");
        return;
    }
    snprintf(buf, PATH_MAX, "%s/.ahwmrc", home);
    yyin = fopen(buf, "r");
    if (yyin == NULL) {
        fprintf(stderr, "AHWM: Could not open configuration file '%s': %s\n",
                buf, strerror(errno));
        if (no_config(buf) == 0) {
            return;
        }
    }
    debug(("Start parsing\n"));
    yyparse();
    debug(("Done parsing\n"));
    fclose(yyin);

    preferences = type_check(preferences);

/* separate contexts and non-contexts in order to supply global defaults */
    last_context = last_line = first_line = first_context = NULL;
    for (lp = preferences; lp != NULL; lp = lp->line_next) {
        switch (lp->line_type) {
            case CONTEXT:
                if (first_context == NULL) {
                    first_context = lp;
                }
                if (last_context != NULL) {
                    last_context->line_next = lp;
                }
                last_context = lp;
                break;
            case KEYBINDING:
            case MOUSEBINDING:
                globally_bind(lp);
                break;
            case KEYUNBINDING:
            case MOUSEUNBINDING:
                globally_unbind(lp);
                break;
            case OPTION:
                if (lp->line_value.option->option_name == NWORKSPACES) {
                    /* special case: option only applies globally */
                    get_int(lp->line_value.option->option_value, &i);
                    if (i < 1) {
                        fprintf(stderr,
                                "AHWM: NumberOfWorkspaces must be at least one\n");
                    } else {
                        nworkspaces = i;
                    }
                } else if (lp->line_value.option->option_name == TITLEBARFONT) {
                    /* another global-only option */
                    get_string(lp->line_value.option->option_value,
                               &ahwm_fontname);
                } else {
                    option_apply(NULL, lp->line_value.option, &defaults);
                }
                break;
            case DEFINITION:
                break;
            default:
                if (first_line == NULL) {
                    first_line = lp;
                }
                if (last_line != NULL) {
                    last_line->line_next = lp;
                }
                last_line = lp;
                break;
        }
    }
    contexts = first_context;
}

static void make_definition(definition *def)
{
    definition **tmp;
    
    if (definitions == NULL) {
        definitions = Malloc(sizeof(definition));
        if (definitions == NULL) {
            fprintf(stderr,
                    "AHWM: parsing definition of %s: malloc: %s\n",
                    def->identifier, strerror(errno));
            return;
        }
        ndefinitions = 1;
    } else {
        ndefinitions++;
        tmp = Realloc(definitions, sizeof(definition) * ndefinitions);
        if (tmp == NULL) {
            ndefinitions--;
            fprintf(stderr,
                    "AHWM: parsing definitions of %s: realloc: %s\n",
                    def->identifier, strerror(errno));
            return;
        }
        definitions = tmp;
    }
    definitions[ndefinitions - 1] = def;
}

static line *type_check(line *block)
{
    line *lp, *prev, *first_ok;
    Bool line_ok;

    first_ok = NULL;
    prev = NULL;
    for (lp = block; lp != NULL; lp = lp->line_next) {
        switch (lp->line_type) {
            case CONTEXT:
                line_ok = type_check_context(lp->line_value.context);
                if (line_ok) {
                    lp->line_value.context->context_lines =
                        type_check(lp->line_value.context->context_lines);
                } else {
                    lp->line_value.context->context_selector = 0;
                }
                break;
            case OPTION:
                line_ok = type_check_option(lp->line_value.option);
                break;
            case KEYBINDING:
                line_ok = type_check_keybinding(lp->line_value.keybinding);
                break;
            case KEYUNBINDING:
                line_ok = True;
                break;
            case MOUSEBINDING:
                line_ok = type_check_mousebinding(lp->line_value.mousebinding);
                break;
            case MOUSEUNBINDING:
                line_ok = True;
                break;
            case DEFINITION:
                /* we fill the "definitions" table on type checking */
                line_ok = type_check_definition(lp->line_value.definition);
                if (line_ok) {
                    make_definition(lp->line_value.definition);
                }
                break;
            case INVALID_LINE:
                line_ok = False;
        }
        if (line_ok == True) {
            if (first_ok == NULL)
                first_ok = lp;
            prev = lp;
        } else {
            if (prev != NULL) {
                prev->line_next = lp->line_next;
            }
            fprintf(stderr,
                    "AHWM: error on statement ending on line %d, "
                    "ignoring statement\n", lp->line_number);
        }
    }
    return first_ok;
}

static Bool type_check_context(context *cntxt)
{
    Bool retval;
    
    if (cntxt->context_selector & SEL_HASTRANSIENT
        && cntxt->context_selector & SEL_TRANSIENTFOR) {
        /* FIXME: can't happen? */
        fprintf(stderr,
                "AHWM: HasTransient and TransientFor "
                "are mutually exclusive\n");
        retval = False;
    } else if (cntxt->context_selector & SEL_ISSHAPED) {
        retval = type_check_helper(cntxt->context_value, BOOLEAN,
                                   "IsShaped", "selector");
    } else if (cntxt->context_selector & SEL_INWORKSPACE) {
        retval = type_check_helper(cntxt->context_value, INTEGER,
                                   "InWorkspace", "selector");
    } else if (cntxt->context_selector & SEL_WINDOWNAME) {
        retval = type_check_helper(cntxt->context_value, STRING,
                                   "WindowName", "selector");
    } else if (cntxt->context_selector & SEL_WINDOWCLASS) {
        retval = type_check_helper(cntxt->context_value, STRING,
                                   "WindowClass", "selector");
    } else if (cntxt->context_selector & SEL_WINDOWINSTANCE) {
        retval = type_check_helper(cntxt->context_value, STRING,
                                   "WindowInstance", "selector");
    } else {
        fprintf(stderr, "AHWM: unknown selector\n");
        retval = False;
    }
    return retval;
}

static char *get_type_name(int typ)
{
    if (typ < sizeof(type_names) / sizeof(char *)) {
        return type_names[typ];
    } else {
        return "unknown type";  /* shouldn't ever be seen */
    }
}

static Bool type_check_helper(type *typ, int expected,
                              char *tok, char *tok_type)
{
    char *s_given;
    
    if (typ != NULL && typ->type_type == expected) {
        return True;
    } else {
        if (typ == NULL) {
            s_given = "null argument";
        } else {
            s_given = get_type_name(typ->type_type);
        }
        fprintf(stderr, "AHWM: type error: %s '%s' takes %s, not %s\n",
                tok_type, tok, get_type_name(expected), s_given);
        return False;
    }
}

static Bool type_check_option(option *opt)
{
    Bool retval;
    
    switch (opt->option_name) {
        /* ADDOPT 7: define option's type */
        case NWORKSPACES:
            retval = type_check_helper(opt->option_value, INTEGER,
                                       "NumberOfWorkspaces", "option");
            break;
        case TITLEBARFONT:
            retval = type_check_helper(opt->option_value, STRING,
                                       "TitlebarFont", "option");
            break;
        case DISPLAYTITLEBAR:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "DisplayTitlebar", "option");
            break;
        case OMNIPRESENT:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "Omnipresent", "option");
            break;
        case DEFAULTWORKSPACE:
            retval = type_check_helper(opt->option_value, INTEGER,
                                       "DefaultWorkspace", "option");
            break;
        case FOCUSPOLICY:
            if (opt->option_value->type_type == FOCUS_ENUM) {
                retval = True;
            } else {
                fprintf(stderr, "AHWM:  Unknown value for FocusPolicy\n");
                retval = False;
            }
            break;
        case ALWAYSONTOP:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "AlwaysOnTop", "option");
            break;
        case ALWAYSONBOTTOM:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "AlwaysOnBottom", "option");
            break;
        case PASSFOCUSCLICK:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "PassFocusClick", "option");
            break;
        case CYCLEBEHAVIOUR:
            if (opt->option_value->type_type != CYCLE_ENUM) {
                fprintf(stderr, "AHWM:  Unknown value for CycleBehaviour\n");
                retval = False;
            } else {
                retval = True;
            }
            break;
        case COLORTITLEBAR:
            retval = type_check_helper(opt->option_value, STRING,
                                       "ColorTitlebar", "option");
            break;
        case COLORTITLEBARFOCUSED:
            retval = type_check_helper(opt->option_value, STRING,
                                       "ColorTitlebarFocused", "option");
            break;
        case COLORTEXT:
            retval = type_check_helper(opt->option_value, STRING,
                                       "ColorTitlebarText", "option");
            break;
        case COLORTEXTFOCUSED:
            retval = type_check_helper(opt->option_value, STRING,
                                       "ColorTitlebarTextFocused", "option");
            break;
        case DONTBINDMOUSE:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "DontBindMouse", "option");
            break;
        case DONTBINDKEYS:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "DontBindKeys", "option");
            break;
        case STICKY:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "Sticky", "option");
            break;
        case TITLEPOSITION:
            if (opt->option_value->type_type != POSITION_ENUM) {
                fprintf(stderr, "AHWM:  Unknown value for TitlePosition\n");
                retval = False;
            } else {
                retval = True;
            }
            break;
        case KEEPTRANSIENTSONTOP:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "KeepTransientsOnTop", "option");
            break;
        case RAISEDELAY:
            retval = type_check_helper(opt->option_value, INTEGER,
                                       "RaiseDelay", "option");
            break;
        case USENETWMPID:
            retval = type_check_helper(opt->option_value, BOOLEAN,
                                       "UseNetWmPid", "option");
            break;
        default:
            fprintf(stderr, "AHWM: unknown option type found...\n");
            retval = False;
            break;
    }
    return retval;
}

/* FIXME:  also check binding string */
static Bool type_check_keybinding(keybinding *kb)
{
    return type_check_function(kb->keybinding_function);
}

static Bool type_check_mousebinding(mousebinding *mb)
{
    return type_check_function(mb->mousebinding_function);
}

static Bool type_check_definition(definition *def)
{
    funclist *fl;

    for (fl = def->funclist; fl != NULL; fl = fl->next) {
        if (type_check_function(fl->func) == False) {
            return False;
        }
    }
    return True;
}

static Bool type_check_function(function *fn)
{
    arglist *al;
    char *fn_name = "(Unknown function)"; /* shouldn't display */
    Bool retval;

    if (fn->function_type < sizeof(fn_names) / sizeof(char *)) {
        fn_name = fn_names[fn->function_type];
    }
    switch (fn->function_type) {
        case CYCLENEXT:
        case CYCLEPREV:
        case KILLNICELY:
        case KILLWITHEXTREMEPREJUDICE:
        case FOCUS:
        case MAXIMIZE:
        case MAXIMIZE_H:
        case MAXIMIZE_V:
        case NOP:
        case QUOTE:
        case MOVEINTERACTIVELY:
        case RESIZEINTERACTIVELY:
        case QUIT:
        case BEEP:
        case REFRESH:
        case RESTART:
        case CRASH:
            if (fn->function_args == NULL) {
                retval = True;
            } else {
                fprintf(stderr,
                        "AHWM: type error: %s '%s' takes %s, not %s\n",
                        "function", fn_name, "no arguments",
                        fn->function_args->arglist_arg == NULL ?
                        "unknown argument" :
                        get_type_name(fn->function_args->arglist_arg->type_type));
                retval = False;
            }
            break;              /* aesthetic */
        case SENDTOWORKSPACE:
        case GOTOWORKSPACE:
            if (fn->function_args == NULL) {
                fprintf(stderr, "AHWM: type error: %s '%s' takes %s, not %s\n",
                        "function", fn_name,
                        type_names[INTEGER], "null argument");
                retval = False;
            } else if (fn->function_args->arglist_next != NULL) {
                fprintf(stderr, "AHWM: type error: %s '%s' takes %s, not %s\n",
                        "function", fn_name, "a single integer argument",
                        "multiple arguments");
                retval = False;
            } else {
                retval = type_check_helper(fn->function_args->arglist_arg,
                                           INTEGER, fn_name, "function");
            }
            break;
        case LAUNCH:
        case MOVERESIZE:
        case SHOWMENU:
            if (fn->function_args == NULL) {
                fprintf(stderr, "AHWM: type error: %s '%s' takes %s, not %s\n",
                        "function", fn_name,
                        type_names[STRING], "null argument");
                retval = False;
            } else if (fn->function_args->arglist_next != NULL) {
                fprintf(stderr, "AHWM: type error: %s '%s' takes %s, not %s\n",
                        "function", fn_name, "a single string argument",
                        "multiple arguments");
                retval = False;
            } else {
                retval = type_check_helper(fn->function_args->arglist_arg,
                                           STRING, fn_name, "function");
            }
            break;
        case INVOKE:
            if (fn->function_args == NULL) {
                fprintf(stderr, "AHWM: type error: %s '%s' takes %s, not %s\n",
                        "function", fn_name, type_names[STRING],
                        "null argument");
                retval = False;
            } else {
                for (al = fn->function_args; al != NULL; al = al->arglist_next) {
                    if (CHECK_STRING(al->arglist_arg) == False) {
                        fprintf(stderr,
                                "AHWM: type error: %s '%s' takes %s, not %s\n",
                                "function", fn_name, type_names[STRING],
                                get_type_name(al->arglist_arg->type_type));
                        retval = False;
                        break;
                    }
                }
                retval = True;
            }
            break;
        default:
            fprintf(stderr, "AHWM: unknown function %s\n", fn_name);
            retval = False;
    }
    return retval;
}

/*
 * We don't want to do list walking and string comparisons whenever
 * we apply an invocation, so we do them on startup.  This changes
 * the invocation's argument from a string representing the name
 * of the definition to invoke into an integer index into the
 * "definitions" table.  Works kind of like a symbol table.
 */
static void invocation_string_to_int(arglist *arglist)
{
    int i;

    for (i = 0; i < ndefinitions; i++) {
        if (strcmp(definitions[i]->identifier,
                   arglist->arglist_arg->type_value.stringval) == 0) {
            arglist->arglist_arg->type_type = INTEGER;
            arglist->arglist_arg->type_value.intval = i;
            return;
        }
    }
}

static void prefs_apply_internal(client_t *client, line *block, prefs *p)
{
    line *lp;

    for (lp = block; lp != NULL; lp = lp->line_next) {
        switch (lp->line_type) {
            case CONTEXT:
                if (context_applies(client, lp->line_value.context)) {
                    prefs_apply_internal(client,
                                         lp->line_value.context->context_lines,
                                         p);
                }
                break;
            case OPTION:
                option_apply(client, lp->line_value.option, p);
                break;
            case KEYBINDING:
                keybinding_apply(client, lp->line_value.keybinding);
                break;
            case KEYUNBINDING:
                keyunbinding_apply(client, lp->line_value.keyunbinding);
                break;
            case MOUSEBINDING:
                mousebinding_apply(client, lp->line_value.mousebinding);
                break;
            case MOUSEUNBINDING:
                mouseunbinding_apply(client, lp->line_value.mouseunbinding);
                break;
            default:
                /* nothing */
        }
    }
}

/*
 * Following three utility functions set the pointer argument and
 * return True if the type is correct.
 */

static void get_int(type *typ, int *val)
{
    *val = 0;
    
    if (typ->type_type != STRING && typ->type_type != BOOLEAN) {
        *val = typ->type_value.intval;
    }
}

static void get_string(type *typ, char **val)
{
    *val = NULL;
    if (typ->type_type == STRING) {
        *val = typ->type_value.stringval;
    }
}

static void get_bool(type *typ, Bool *val)
{
    *val = False;
    if (typ->type_type == BOOLEAN) {
        *val = typ->type_value.intval;
    }
}

/*
 * Returns True if given context applies to given client.
 */

static Bool context_applies(client_t *client, context *cntxt)
{
    Bool retval;
    int orig_selector;
    Bool type_bool;
    char *type_string;
    int type_int;
    client_t *c;
    
    if (cntxt->context_selector & SEL_TRANSIENTFOR) {
        c = client_find(client->transient_for);
        if (c == NULL) {
            retval = False;
        } else {
            orig_selector = cntxt->context_selector;
            cntxt->context_selector &=
                ~(SEL_NOT | SEL_TRANSIENTFOR | SEL_HASTRANSIENT);
            retval = context_applies(c, cntxt);
            cntxt->context_selector = orig_selector;
        }
    } else if (cntxt->context_selector & SEL_HASTRANSIENT) {
        orig_selector = cntxt->context_selector;
        cntxt->context_selector &=
            ~(SEL_NOT | SEL_TRANSIENTFOR | SEL_HASTRANSIENT);
        retval = False;
        for (c = client->transients;
             c != NULL && retval == False;
             c = c->next_transient) {
            retval = context_applies(c, cntxt);
        }
        cntxt->context_selector = orig_selector;
    } else if (cntxt->context_selector & SEL_ISSHAPED) {
        get_bool(cntxt->context_value, &type_bool);
        retval = client->is_shaped == (type_bool == True ? 1 : 0);
    } else if (cntxt->context_selector & SEL_INWORKSPACE) {
        get_int(cntxt->context_value, &type_int);
        if (client->workspace == 0) {
            retval = workspace_current == (unsigned)type_int;
        } else {
            retval = client->workspace == (unsigned)type_int;
        }
    } else if (cntxt->context_selector & SEL_WINDOWNAME) {
        get_string(cntxt->context_value, &type_string);
        if (strcmp(type_string, "*") == 0) {
            retval = True;
        } else {
            retval = !(strcmp(client->name, type_string));
        }
    } else if (cntxt->context_selector & SEL_WINDOWCLASS) {
        get_string(cntxt->context_value, &type_string);
        if (strcmp(type_string, "*") == 0) {
            retval = True;
        } else {
            if (client->class == NULL) {
                retval = False;
            } else {
                retval = !(strcmp(client->class, type_string));
                debug(("Using window class (%s, %s), returning %d\n",
                       client->class, type_string, retval));
            }
        }
    } else if (cntxt->context_selector & SEL_WINDOWINSTANCE) {
        get_string(cntxt->context_value, &type_string);
        if (strcmp(type_string, "*") == 0) {
            retval = True;
        } else {
            if (client->instance == NULL) {
                retval = False;
            } else {
                retval = !(strcmp(client->instance, type_string));
            }
        }
    }
    
    if (cntxt->context_selector & SEL_NOT) return !retval;
    else return retval;
}

/*
 * Applies given option to given client.
 */
static void option_apply(client_t *client, option *opt, prefs *p)
{
    switch (opt->option_name) {
        /* ADDOPT 8: set option if found within a context */
        case DISPLAYTITLEBAR:
            get_bool(opt->option_value, &p->titlebar);
            p->titlebar_set = opt->option_setting;
            break;
        case OMNIPRESENT:
            get_bool(opt->option_value, &p->omnipresent);
            p->omnipresent_set = opt->option_setting;
            break;
        case DEFAULTWORKSPACE:
            get_int(opt->option_value, &p->workspace);
            p->workspace_set = opt->option_setting;
            break;
        case FOCUSPOLICY:
            get_int(opt->option_value, &p->focus_policy);
            p->focus_policy_set = opt->option_setting;
            break;
        case CYCLEBEHAVIOUR:
            get_int(opt->option_value, &p->cycle_behaviour);
            p->cycle_behaviour_set = opt->option_setting;
            break;
        case ALWAYSONTOP:
            get_bool(opt->option_value, &p->always_on_top);
            p->always_on_top_set = opt->option_setting;
            break;
        case ALWAYSONBOTTOM:
            get_bool(opt->option_value, &p->always_on_bottom);
            p->always_on_bottom_set = opt->option_setting;
            break;
        case PASSFOCUSCLICK:
            get_bool(opt->option_value, &p->pass_focus_click);
            p->pass_focus_click_set = opt->option_setting;
            break;
        case COLORTITLEBAR:
            get_string(opt->option_value, &p->titlebar_color);
            p->titlebar_color_set = opt->option_setting;
            break;
        case COLORTITLEBARFOCUSED:
            get_string(opt->option_value, &p->titlebar_focused_color);
            p->titlebar_focused_color_set = opt->option_setting;
            break;
        case COLORTEXT:
            get_string(opt->option_value, &p->titlebar_text_color);
            p->titlebar_text_color_set = opt->option_setting;
            break;
        case COLORTEXTFOCUSED:
            get_string(opt->option_value, &p->titlebar_text_focused_color);
            p->titlebar_text_focused_color_set = opt->option_setting;
            break;
        case DONTBINDMOUSE:
            get_bool(opt->option_value, &p->dont_bind_mouse);
            p->dont_bind_mouse_set = opt->option_setting;
            break;
        case DONTBINDKEYS:
            get_bool(opt->option_value, &p->dont_bind_keys);
            p->dont_bind_mouse_set = opt->option_setting;
            break;
        case STICKY:
            get_bool(opt->option_value, &p->sticky);
            p->sticky_set = opt->option_setting;
            break;
        case TITLEPOSITION:
            get_int(opt->option_value, &p->title_position);
            p->title_position_set = opt->option_setting;
            break;
        case KEEPTRANSIENTSONTOP:
            get_bool(opt->option_value, &p->keep_transients_on_top);
            p->keep_transients_on_top_set = opt->option_setting;
            break;
        case RAISEDELAY:
            get_int(opt->option_value, &p->raise_delay);
            p->raise_delay_set = opt->option_setting;
            break;
        case USENETWMPID:
            get_bool(opt->option_value, &p->use_net_wm_pid);
            p->use_net_wm_pid_set = opt->option_setting;
            break;
        default:
            /* nothing */
    }
}

static void keybinding_apply(client_t *client, keybinding *kb)
{
    /* FIXME */
}

static void keyunbinding_apply(client_t *client, keyunbinding *kb)
{
    /* FIXME */
}

static void mousebinding_apply(client_t *client, mousebinding *kb)
{
    /* FIXME */
}

static void mouseunbinding_apply(client_t *client, mouseunbinding *kb)
{
    /* FIXME */
}

/*
 * this function has two stages:
 * first, we get the preferences for the client; eg, we see if the
 * client needs a titlebar.
 * second, we apply those preferences; eg, we add or remove the
 * titlebar.
 * This function may be called at any time during the client's
 * lifetime, so we can't assume anything about the client's state
 * (ie, whether the client has been reparented, mapped, etc.).
 * We break it up into two stages so we don't, for example, add
 * and remove a titlebar multiple times on each call.
 */

void prefs_apply(client_t *client)
{
    prefs p;

    memcpy(&p, &defaults, sizeof(prefs));

    prefs_apply_internal(client, contexts, &p);

    if (client->state == WithdrawnState) {
        if (client->workspace_set <= p.workspace_set) {
            client->workspace = p.workspace;
            client->workspace_set = p.workspace_set;
            ewmh_desktop_update(client);
        }
    }

    if (client->has_titlebar_set <= p.titlebar_set) {
        if (p.titlebar == True) {
            if (!client->has_titlebar) {
                client->has_titlebar = 1;
                if (client->frame != None) {
                    client_add_titlebar(client);
                    client->has_titlebar_set = p.titlebar_set;
                }
            }
        } else {
            if (client->has_titlebar) {
                client->has_titlebar = 0;
                if (client->frame != None) {
                    client_remove_titlebar(client);
                    client->has_titlebar_set = p.titlebar_set;
                }
            }
        }
    }
    if (client->cycle_behaviour_set <= p.cycle_behaviour_set) {
        switch (p.cycle_behaviour) {
            case TYPE_SKIP_CYCLE:
                client->cycle_behaviour = SkipCycle;
                break;
            case TYPE_RAISE_IMMEDIATELY:
                client->cycle_behaviour = RaiseImmediately;
                break;
            case TYPE_RAISE_ON_CYCLE_FINISH:
                client->cycle_behaviour = RaiseOnCycleFinish;
                break;
            case TYPE_DONT_RAISE:
                client->cycle_behaviour = DontRaise;
                break;
        }
        client->cycle_behaviour_set = p.cycle_behaviour_set;
    }
    if (client->omnipresent_set <= p.omnipresent_set) {
        if (p.omnipresent) {
            client->omnipresent = 1;
            client->omnipresent_set = p.omnipresent_set;
        } else {
            client->omnipresent = 0;
            client->omnipresent_set = p.omnipresent_set;
        }
    }
    if (client->focus_policy_set <= p.focus_policy_set) {
        switch (p.focus_policy) {
            case TYPE_SLOPPY_FOCUS:
                if (client->focus_policy == ClickToFocus) {
                    focus_policy_from_click(client);
                }
                client->focus_policy = SloppyFocus;
                client->focus_policy_set = p.focus_policy_set;
                break;
            case TYPE_CLICK_TO_FOCUS:
                if (client->focus_policy != ClickToFocus) {
                    focus_policy_to_click(client);
                }
                client->focus_policy = ClickToFocus;
                client->focus_policy_set = p.focus_policy_set;
                break;
            case TYPE_DONT_FOCUS:
                if (client->focus_policy == ClickToFocus) {
                    focus_policy_from_click(client);
                }
                client->focus_policy = DontFocus;
                client->focus_policy_set = p.focus_policy_set;
                break;
        }
    }
    if (client->always_on_top_set <= p.always_on_top_set) {
        if (p.always_on_top) {
            if (client->always_on_top == 0) {
                client->always_on_top = 1;
                client->always_on_top_set = p.always_on_top_set;
                stacking_restack(client);
            }
        } else {
            if (client->always_on_top == 1) {
                client->always_on_top = 0;
                client->always_on_top_set = p.always_on_top_set;
                stacking_restack(client);
            }
        }
    }
    if (client->always_on_bottom_set <= p.always_on_bottom_set) {
        if (p.always_on_bottom) {
            if (client->always_on_bottom == 0) {
                client->always_on_bottom = 1;
                client->always_on_bottom_set = p.always_on_bottom_set;
                stacking_restack(client);
            }
        } else {
            if (client->always_on_bottom == 1) {
                client->always_on_bottom = 0;
                client->always_on_bottom_set = p.always_on_bottom_set;
                stacking_restack(client);
            }
        }
    }
    if (client->pass_focus_click_set <= p.pass_focus_click_set) {
        if (p.pass_focus_click) {
            client->pass_focus_click = 1;
            client->pass_focus_click_set = p.pass_focus_click_set;
        } else {
            client->pass_focus_click = 0;
            client->pass_focus_click_set = p.pass_focus_click_set;
        }
    }

    paint_calculate_colors(client, p.titlebar_color,
                           p.titlebar_focused_color,
                           p.titlebar_text_color,
                           p.titlebar_text_focused_color);

    if (client->dont_bind_mouse_set <= p.dont_bind_mouse_set) {
        if (client->dont_bind_mouse == 1 &&
            p.dont_bind_mouse == 0) {
            mouse_grab_buttons(client);
        } else if (client->dont_bind_mouse == 0 &&
                   p.dont_bind_mouse == 1) {
            mouse_ungrab_buttons(client);
        }
        client->dont_bind_mouse = p.dont_bind_mouse;
        client->dont_bind_mouse_set = p.dont_bind_mouse_set;
    }
    if (client->dont_bind_keys_set <= p.dont_bind_keys_set) {
        if (client->dont_bind_keys == 1 &&
            p.dont_bind_keys == 0) {
            keyboard_grab_keys(client->frame);
        } else if (client->dont_bind_keys == 0 &&
                   p.dont_bind_keys == 1) {
            keyboard_ungrab_keys(client->frame);
        }
        client->dont_bind_keys = p.dont_bind_keys;
        client->dont_bind_keys_set = p.dont_bind_keys_set;
    }
    if (client->sticky_set <= p.sticky_set) {
        client->sticky = p.sticky;
        client->sticky_set = p.sticky_set;
    }
    if (client->title_position_set <= p.title_position_set) {
        switch (p.title_position) {
            case TYPE_DISPLAY_LEFT:
                client->title_position = DisplayLeft;
                break;
            case TYPE_DISPLAY_RIGHT:
                client->title_position = DisplayRight;
                break;
            case TYPE_DISPLAY_CENTERED:
                client->title_position = DisplayCentered;
                break;
            case TYPE_DONT_DISPLAY:
                client->title_position = DontDisplay;
                break;
        }
        client->title_position_set = p.title_position_set;
    }
    if (client->keep_transients_on_top_set <= p.keep_transients_on_top_set) {
        client->keep_transients_on_top = p.keep_transients_on_top;
        client->keep_transients_on_top_set = p.keep_transients_on_top_set;
    }
    if (client->raise_delay_set <= p.raise_delay_set) {
        client->raise_delay = p.raise_delay;
        client->raise_delay_set = p.raise_delay_set;
    }
    if (client->use_net_wm_pid_set <= p.use_net_wm_pid_set) {
        client->use_net_wm_pid = p.use_net_wm_pid;
        client->use_net_wm_pid_set = p.use_net_wm_pid_set;
    }
    
    /* ADDOPT 9: apply the option to the client */
    
    /* ADDOPT 10: document in doc/options.yo */
}

static void globally_bind(line *lp)
{
    keybinding *kb;
    mousebinding *mb;
    key_fn fn;

    if (lp->line_type == KEYBINDING) {
        kb = lp->line_value.keybinding;
        fn = fn_table[kb->keybinding_function->function_type];
        if (fn != NULL) {
            keyboard_bind(kb->keybinding_string,
                          kb->keybinding_depress,
                          fn,
                          kb->keybinding_function->function_args);
        }
    } else if (lp->line_type == MOUSEBINDING) {
        mb = lp->line_value.mousebinding;
        fn = fn_table[mb->mousebinding_function->function_type];
        if (fn != NULL) {
            mouse_bind(mb->mousebinding_string,
                       mb->mousebinding_depress,
                       mb->mousebinding_location,
                       fn,
                       mb->mousebinding_function->function_args);
        }
    }
}

static void globally_unbind(line *lp)
{
    if (lp->line_type == KEYUNBINDING) {
        keyboard_unbind(lp->line_value.keyunbinding->keyunbinding_string,
                        lp->line_value.keyunbinding->keyunbinding_depress);
    } else if (lp->line_type == MOUSEUNBINDING) {
        mouse_unbind(lp->line_value.mouseunbinding->mouseunbinding_string,
                     lp->line_value.mouseunbinding->mouseunbinding_depress,
                     lp->line_value.mouseunbinding->mouseunbinding_location);
    }
}

/*
 * When the user has no .ahwmrc, we try to create one and pop up an
 * explanatory message.
 */

static int no_config(char *ahwmrc_path)
{
    extern FILE *yyin;
    int i;

    fprintf(stderr, "AHWM: Creating default configuration file\n");
    yyin = fopen(ahwmrc_path, "w+b");
    if (yyin == NULL) {
        fprintf(stderr,
                "AHWM: Could not create default configuration file: %s\n",
                strerror(errno));
        return 0;
    }
    for (i = 0; i < sizeof(default_ahwmrc) / sizeof(char *); i++) {
        fprintf(yyin, "%s\n", default_ahwmrc[i]);
    }
    fseek(yyin, 0, SEEK_SET);
    xmessage();
    return 1;
}

/*
 * We just use xmessage to display our welcome message.  Hopefully
 * user's system has xmessage.  Hopefully user's xmessage works the
 * same as my version.  Hopefully we don't run into system limit on
 * size or number of arguments.  Hopefully user isn't completely
 * disgusted with nasty athena-based xmessage.
 */

static void xmessage()
{
    pid_t pid;
    
    /* standard double fork trick, no zombies */
    pid = fork();
    if (pid == 0) {
        pid = fork();
        if (pid == 0) {
            sleep(1);
            execvp("xmessage", default_message);
            perror("AHWM: exec(xmessage)");
            fprintf(stderr, "This is a bug in AHWM.  "
                    "Please contact hioreanu+ahwm@uchicago.edu");
            exit(1);
        } else if (pid < 0) {
            perror("AHWM: xmessage: fork");
            exit(1);
        }
        exit(0);
    } else if (pid < 0) {
        perror("AHWM: xmessage: fork");
        return;                 /* not really fatal */
    }
    wait(NULL);
}

/*
 * The first time a definition is invoked, the arguments to the
 * invocation (and all subsequent invocation) are changed from strings
 * to integers.  The integer arguments denote indices into the
 * "definitions" table.  This does not break type checking as type
 * checking has already happenned by the time the user can invoke any
 * bindable functions.  We do this so we don't have to do any
 * searching or string comparisons whenever the user invokes a
 * function composition.
 */

static void invoke(XEvent *e, arglist *args)
{
    arglist *al;
    funclist *fl;
    int i;
    key_fn fn;
    
    for (al = args; al != NULL; al = al->arglist_next) {
        if (al->arglist_arg->type_type != INTEGER) {
            invocation_string_to_int(args);
        }
        i = al->arglist_arg->type_value.intval;
        if (al->arglist_arg->type_type != INTEGER ||
            i < 0 || i >= ndefinitions) {
            continue;
        }
        for (fl = definitions[i]->funclist; fl != NULL; fl = fl->next) {
            fn = fn_table[fl->func->function_type];
            if (fn != NULL) {
                (*fn)(e, fl->func->function_args);
            }
        }
    }
}

static void focus(XEvent *e, arglist *args)
{
    client_t *client;
    
    if (e->type == ButtonPress || e->type == ButtonRelease) {
        client = client_find(e->xbutton.window);
        if (client != NULL) focus_set(client, CurrentTime);
    }
}

static void crash(XEvent *e, arglist *args)
{
    int *p;
    
    fprintf(stderr, "AHWM:  Crashing...happy now?\n");

    p = NULL;
    p[0] = 0xDECAF;
    /* is */
    p[1] = 0xBAD;
    p[2] = 0xC0FFEE;
}
