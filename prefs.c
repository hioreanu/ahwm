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

#include "prefs.h"
#include "parser.h"
#include "debug.h"
#include "workspace.h"
#include "keyboard-mouse.h"
#include "focus.h"
#include "kill.h"
#include "move-resize.h"


#define CHECK_BOOL(x) ((x)->type_type == BOOLEAN ? True : False)
#define CHECK_STRING(x) ((x)->type_type == STRING ? True : False)
#define CHECK_INT(x) ((x)->type_type == INTEGER ? True : False)

typedef struct _prefs {
    Bool titlebar;
    Bool omnipresent;
    Bool skip_alt_tab;
    int workspace;
    int focus_policy;
} prefs;

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
static Bool type_check_keyunbinding(keyunbinding *kub);
static Bool type_check_mouseunbinding(mouseunbinding *mub);
static Bool type_check_function(function *fn);
static void keybinding_apply(client_t *client, keybinding *kb);
static void keyunbinding_apply(client_t *client, keyunbinding *kb);
static void mousebinding_apply(client_t *client, mousebinding *kb);
static void mouseunbinding_apply(client_t *client, mouseunbinding *kb);
static void prefs_apply_internal(client_t *client, line *block, prefs *p);
static void globally_bind(line *lp);
static void globally_unbind(line *lp);

static line *defaults, *contexts;
int pref_no_workspaces = 7;
Bool pref_display_titlebar = True;
Bool pref_omnipresent = False;
Bool pref_skip_alt_tab = False;
int pref_default_workspace = 0;
int pref_default_focus_policy = TYPE_SLOPPY_FOCUS;

void prefs_init()
{
    char buf[PATH_MAX];
    char *home;
    line *lp, *prev, *first_context, *last_context, *last_line, *first_line;
    extern FILE *yyin;

    home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "XWM: Could not get home directory; "
                "configuration file not found.\n");
        return;
    }
    snprintf(buf, PATH_MAX, "%s/.xwmrc", home);
    yyin = fopen(buf, "r");
    if (yyin == NULL) {
        fprintf(stderr, "XWM: Could not open configuration file '%s': %s\n",
                buf, strerror(errno));
        return;
    }
    debug("Start parsing\n");
    yyparse();
    debug("Done parsing\n");

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
                switch (lp->line_value.option->option_name) {
                    case DISPLAYTITLEBAR:
                        get_bool(lp->line_value.option->option_value,
                                 &pref_display_titlebar);
                        break;
                    case OMNIPRESENT:
                        get_bool(lp->line_value.option->option_value,
                                 &pref_omnipresent);
                        break;
                    case SKIPALTTAB:
                        get_bool(lp->line_value.option->option_value,
                                 &pref_skip_alt_tab);
                        break;
                    case DEFAULTWORKSPACE:
                        get_int(lp->line_value.option->option_value,
                                &pref_default_workspace);
                        break;
                    case NUMBEROFWORKSPACES:
                        get_int(lp->line_value.option->option_value,
                                &pref_no_workspaces);
                        break;
                    case FOCUSPOLICY:
                        get_int(lp->line_value.option->option_value,
                                &pref_default_focus_policy);
                        break;
                }
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
    defaults = first_line;
    contexts = first_context;
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
                if (type_check_context(lp->line_value.context)) {
                    lp->line_value.context->context_lines =
                        type_check(lp->line_value.context->context_lines);
                } else {
                    lp->line_value.context->context_selector = 0;
                }
                line_ok = True;
                break;
            case OPTION:
                line_ok = type_check_option(lp->line_value.option);
                break;
            case KEYBINDING:
                line_ok = type_check_keybinding(lp->line_value.keybinding);
                break;
            case KEYUNBINDING:
                line_ok = type_check_keyunbinding(lp->line_value.keyunbinding);
                break;
            case MOUSEBINDING:
                line_ok = type_check_mousebinding(lp->line_value.mousebinding);
                break;
            case MOUSEUNBINDING:
                line_ok = type_check_mouseunbinding(lp->line_value.mouseunbinding);
                break;
        }
        if (line_ok == True) {
            if (first_ok == NULL)
                first_ok = lp;
            prev = lp;
        } else {
            if (prev != NULL) {
                prev->line_next = lp->line_next;
            }
        }
    }
    return first_ok;
}

/* FIXME:  error messages, need line number */
static Bool type_check_context(context *cntxt)
{
    Bool retval;
    
    if (cntxt->context_selector & SEL_HASTRANSIENT
        && cntxt->context_selector & SEL_TRANSIENTFOR) {
        fprintf(stderr, "XWM: error\n");
        retval = False;
    } else if (cntxt->context_selector & SEL_ISSHAPED) {
        if ( (retval = CHECK_BOOL(cntxt->context_value)) == False) {
            fprintf(stderr, "XWM: error\n");
        }
    } else if (cntxt->context_selector & SEL_INWORKSPACE) {
        if ( (retval = CHECK_INT(cntxt->context_value)) == False) {
            fprintf(stderr, "XWM: error\n");
        }
    } else if (cntxt->context_selector & SEL_WINDOWNAME) {
        if ( (retval = CHECK_STRING(cntxt->context_value)) == False) {
            fprintf(stderr, "XWM: error\n");
        }
    } else if (cntxt->context_selector & SEL_WINDOWCLASS) {
        if ( (retval = CHECK_STRING(cntxt->context_value)) == False) {
            fprintf(stderr, "XWM: error\n");
        }
    } else if (cntxt->context_selector & SEL_WINDOWINSTANCE) {
        if ( (retval = CHECK_STRING(cntxt->context_value)) == False) {
            fprintf(stderr, "XWM: error\n");
        }
    } else {
        retval = False;
    }
    return retval;
}

static Bool type_check_option(option *opt)
{
    Bool retval;
    
    switch (opt->option_name) {
        case DISPLAYTITLEBAR:
            if ( (retval = CHECK_BOOL(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM:  DisplayTitlebar not given boolean value\n");
            }
            break;
        case OMNIPRESENT:
            if ( (retval = CHECK_BOOL(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM: Omnipresent not given boolean value\n");
            }
            break;
        case SKIPALTTAB:
            if ( (retval = CHECK_BOOL(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM: SkipAltTab not given boolean value\n");
            }
            break;
        case DEFAULTWORKSPACE:
            if ( (retval = CHECK_INT(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM: DefaultWorkspace not given integer value\n");
            }
            break;
        case FOCUSPOLICY:
            if (opt->option_value->type_type != FOCUS_ENUM) {
                fprintf(stderr, "XWM:  Unknown type for FocusPolicy\n");
                retval = False;
            } else {
                retval = True;
            }
            break;
        default:
            fprintf(stderr, "XWM: unknown option type found...\n");
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

static Bool type_check_keyunbinding(keyunbinding *kub)
{
    return True;
}

static Bool type_check_mouseunbinding(mouseunbinding *mub)
{
    return True;
}

static Bool type_check_function(function *fn)
{
    switch (fn->function_type) {
        case ALTTAB:
        case KILLNICELY:
        case KILLWITHEXTREMEPREJUDICE:
        case FOCUS:
        case MAXIMIZE:
        case NOP:
        case QUOTE:
        case MOVEINTERACTIVELY:
        case RESIZEINTERACTIVELY:
        case QUIT:
        case BEEP:
        case REFRESH:
            if (fn->function_args == NULL) {
                return True;
            } else {
                return False;
            }
            break;
        case MOVETOWORKSPACE:
        case GOTOWORKSPACE:
            if (fn->function_args == NULL) {
                return False;
            }
            if (CHECK_INT(fn->function_args->arglist_arg) == False) {
                return False;
            }
            if (fn->function_args->arglist_next != NULL) {
                return False;
            }
            return True;
            break;
        case LAUNCH:
        case MOVERESIZE:
        case SHOWMENU:
            if (fn->function_args == NULL) {
                return False;
            }
            if (CHECK_STRING(fn->function_args->arglist_arg) == False) {
                return False;
            }
            if (fn->function_args->arglist_next != NULL) {
                return False;
            }
            return True;
            break;
        case INVOKE:
            if (fn->function_args == NULL) {
                return False;
            }
            if (CHECK_STRING(fn->function_args->arglist_arg) == True) {
                return True;
            }
            break;
        default:
            return False;
    }
}

void prefs_apply(client_t *client)
{
    prefs p;

    p.titlebar = pref_display_titlebar;
    p.omnipresent = pref_omnipresent;
    p.skip_alt_tab = pref_skip_alt_tab;
    p.workspace = pref_default_workspace;
    p.focus_policy = pref_default_focus_policy;

    prefs_apply_internal(client, contexts, &p);

    if (client->state == WithdrawnState) {
        client->workspace = p.workspace;
    }
    debug(("client '%s' (%s,%s) %s a titlebar\n",
           client->name, client->class, client->instance,
           p.titlebar ? "HAS" : "DOES NOT HAVE"));
    if (p.titlebar == True) {
        if (!client->has_titlebar) {
            client->has_titlebar = 1;
            if (client->frame != None) client_add_titlebar(client);
        }
    } else {
        if (client->has_titlebar) {
            client->has_titlebar = 0;
            if (client->frame != None) client_remove_titlebar(client);
        }
    }
    if (p.skip_alt_tab) {
        client->skip_alt_tab = 1;
    } else {
        client->skip_alt_tab = 0;
    }
    if (p.omnipresent) {
        client->omnipresent = 1;
    } else {
        client->omnipresent = 0;
    }
    switch (p.focus_policy) {
        case TYPE_SLOPPY_FOCUS:
            client->focus_policy = SloppyFocus;
            break;
        case TYPE_CLICK_TO_FOCUS:
            client->focus_policy = ClickToFocus;
            break;
        case TYPE_DONT_FOCUS:
            client->focus_policy = DontFocus;
            break;
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
    if (typ->type_type == INTEGER || typ->type_type == FOCUS_ENUM) {
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
        } else if (client->omnipresent == 1) {
            retval = True;
        } else {
            retval = client->workspace == (unsigned)type_int;
        }
    } else if (cntxt->context_selector & SEL_WINDOWNAME) {
        get_string(cntxt->context_value, &type_string);
        retval = !(strcmp(client->name, type_string));
    } else if (cntxt->context_selector & SEL_WINDOWCLASS) {
        if (client->class == NULL) {
            retval = False;
        } else {
            get_string(cntxt->context_value, &type_string);
            retval = !(strcmp(client->class, type_string));
            debug(("Using window class (%s, %s), returning %d\n",
                   client->class, type_string, retval));
        }
    } else if (cntxt->context_selector & SEL_WINDOWINSTANCE) {
        if (client->instance == NULL) {
            retval = False;
        } else {
            get_string(cntxt->context_value, &type_string);
            retval = !(strcmp(client->instance, type_string));
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
    int type_int;
    Bool type_bool;
    
    switch (opt->option_name) {
        case DISPLAYTITLEBAR:
            get_bool(opt->option_value, &p->titlebar);
            break;
        case OMNIPRESENT:
            get_bool(opt->option_value, &p->omnipresent);
            break;
        case SKIPALTTAB:
            get_bool(opt->option_value, &p->skip_alt_tab);
            break;
        case DEFAULTWORKSPACE:
            if (client->state == WithdrawnState) {
                get_int(opt->option_value, &p->workspace);
            }
            break;
        case FOCUSPOLICY:
            get_int(opt->option_value, &p->focus_policy);
            break;
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

/* FIXME:  this should probably get its own module */
/* in addition, this is pretty ugly */
/* could prolly just move all this into the parser */
key_fn fn_table[] = {
/* 1  */    workspace_client_moveto_bindable,
/* 2  */    workspace_goto_bindable,
/* 3  */    focus_alt_tab,
/* 4  */    kill_nicely,
/* 5  */    kill_with_extreme_prejudice,
/* 6  */    run_program,
/* 7  */    NULL, /* focus function, only for mouse binding */
/* 8  */    resize_maximize,
/* 9  */    xwm_nop,
/* 10 */    keyboard_quote,
/* 11 */    move_client,
/* 12 */    resize_client,
/* 13 */    NULL, /* non-interactive move/resize, must implement */
/* 14 */    xwm_quit,
/* 15 */    NULL, /* beep */
/* 16 */    NULL, /* invoke, different from launch? */
/* 17 */    NULL, /* expansion for menu system */
/* 18 */    NULL, /* refresh */
};

/* FIXE: consolidate into next function */
static void get_binding_vals(line *lp, char **ks, key_fn *fn,
                             void **arg, int *location)
{
    keybinding *kb;
    mousebinding *mb;
    
    if (lp->line_type == KEYBINDING) {
        kb = lp->line_value.keybinding;
        *ks = kb->keybinding_string;
        printf("function_type = %d\n", kb->keybinding_function->function_type);
        *fn = fn_table[kb->keybinding_function->function_type];
        *arg = NULL; /* FIXME: punt for now */
        *location = MOUSE_NOWHERE;
    } else if (lp->line_type == MOUSEBINDING) {
        mb = lp->line_value.mousebinding;
        *ks = mb->mousebinding_string;
        *fn = fn_table[mb->mousebinding_function->function_type];
        *location = mb->mousebinding_location;
        *arg = NULL; /* FIXME */
    }
}

/*
 * FIXME:  bindable functions as written take only one argument;
 * multiple args packed into struct.  Need to write conversion
 * routines to take parser-gened 'arglist' and put that into some
 * structure.  Gets ugly.
 */
static void globally_bind(line *lp)
{
    char *keystring;
    key_fn fn;
    void *arg;
    int location;

    get_binding_vals(lp, &keystring, &fn, &arg, &location);
    if (lp->line_type == KEYBINDING && fn != NULL) {
        printf("Binding '%s'\n", keystring);
        keyboard_bind(keystring, KEYBOARD_RELEASE, fn, arg);
    } else if (lp->line_type == MOUSEBINDING && fn != NULL) {
        mouse_bind(keystring, MOUSE_RELEASE, location, fn, arg);
    }
}

static void globally_unbind(line *lp)
{
    if (lp->line_type == KEYUNBINDING) {
        keyboard_unbind(lp->line_value.keyunbinding->keyunbinding_string,
                        KEYBOARD_RELEASE);
    } else if (lp->line_type == MOUSEUNBINDING) {
        mouse_unbind(lp->line_value.mouseunbinding->mouseunbinding_string,
                     MOUSE_RELEASE,
                     lp->line_value.mouseunbinding->mouseunbinding_location);
    }
}
