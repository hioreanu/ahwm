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

/* ADDOPT 4 */
typedef struct _prefs {
    Bool titlebar;
    Bool omnipresent;
    int workspace;
    int focus_policy;
    Bool always_on_top;
    Bool always_on_bottom;
    Bool pass_focus_click;
    int cycle_behaviour;
    char *titlebar_color;
    char *titlebar_focused_color;
    char *titlebar_text_color;
    char *titlebar_text_focused_color;
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

/* ADDOPT 5 */
int pref_no_workspaces = 7;
Bool pref_display_titlebar = True;
Bool pref_omnipresent = False;
int pref_default_workspace = 0;
int pref_default_focus_policy = TYPE_SLOPPY_FOCUS;
Bool pref_default_on_top = False;
Bool pref_default_on_bottom = False;
Bool pref_pass_focus_click = False;
int pref_default_cycle_behaviour = TYPE_RAISE_IMMEDIATELY;
char *pref_default_titlebar_color = NULL;
char *pref_default_titlebar_focused_color = NULL;
char *pref_default_titlebar_text_color = NULL;
char *pref_default_titlebar_text_focused_color = NULL;

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
                /* ADDOPT 6 */
                switch (lp->line_value.option->option_name) {
                    case DISPLAYTITLEBAR:
                        get_bool(lp->line_value.option->option_value,
                                 &pref_display_titlebar);
                        break;
                    case OMNIPRESENT:
                        get_bool(lp->line_value.option->option_value,
                                 &pref_omnipresent);
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
                    case ALWAYSONTOP:
                        get_bool(lp->line_value.option->option_value,
                                &pref_default_on_top);
                        break;
                    case ALWAYSONBOTTOM:
                        get_bool(lp->line_value.option->option_value,
                                &pref_default_on_bottom);
                        break;
                    case PASSFOCUSCLICK:
                        get_bool(lp->line_value.option->option_value,
                                 &pref_pass_focus_click);
                        break;
                    case CYCLEBEHAVIOUR:
                        get_int(lp->line_value.option->option_value,
                                &pref_default_cycle_behaviour);
                        break;
                    case COLORTITLEBAR:
                        get_string(lp->line_value.option->option_value,
                                   &pref_default_titlebar_color);
                        break;
                    case COLORTITLEBARFOCUSED:
                        get_string(lp->line_value.option->option_value,
                                   &pref_default_titlebar_focused_color);
                        break;
                    case COLORTEXT:
                        get_string(lp->line_value.option->option_value,
                                   &pref_default_titlebar_text_color);
                        break;
                    case COLORTEXTFOCUSED:
                        get_string(lp->line_value.option->option_value,
                                   &pref_default_titlebar_text_focused_color);
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
                line_ok =
                    type_check_mouseunbinding(lp->line_value.mouseunbinding);
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
        /* ADDOPT 7 */
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
        case DEFAULTWORKSPACE:
            if ( (retval = CHECK_INT(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM: DefaultWorkspace not given integer value\n");
            }
            break;
        case FOCUSPOLICY:
            if (opt->option_value->type_type != FOCUS_ENUM) {
                fprintf(stderr, "XWM:  Unknown value for FocusPolicy\n");
                retval = False;
            } else {
                retval = True;
            }
            break;
        case ALWAYSONTOP:
            if ( (retval = CHECK_BOOL(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM: AlwaysOnTop not given boolean value\n");
            }
            break;
        case ALWAYSONBOTTOM:
            if ( (retval = CHECK_BOOL(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM: AlwaysOnBottom not given boolean value\n");
            }
            break;
        case PASSFOCUSCLICK:
            if ( (retval = CHECK_BOOL(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM: PassFocusClick not given boolean value\n");
            }
            break;
        case CYCLEBEHAVIOUR:
            if (opt->option_value->type_type != CYCLE_ENUM) {
                fprintf(stderr, "XWM:  Unknown value for CycleBehaviour\n");
                retval = False;
            } else {
                retval = True;
            }
            break;
        case COLORTITLEBAR:
            if ( (retval = CHECK_STRING(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM:  Unknown value for ColorTitlebar\n");
            }
            break;
        case COLORTITLEBARFOCUSED:
            if ( (retval = CHECK_STRING(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM:  Unknown value for ColorTitlebarFocused\n");
            }
            break;
        case COLORTEXT:
            if ( (retval = CHECK_STRING(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM:  Unknown value for ColorTitlebarText\n");
            }
            break;
        case COLORTEXTFOCUSED:
            if ( (retval = CHECK_STRING(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM:  Unknown value for ColorTitlebarTextFocused\n");
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
        case SENDTOWORKSPACE:
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

    /* ADDOPT 9 */
    p.titlebar = pref_display_titlebar;
    p.omnipresent = pref_omnipresent;
    p.workspace = pref_default_workspace;
    p.focus_policy = pref_default_focus_policy;
    p.always_on_top = pref_default_on_top;
    p.always_on_bottom = pref_default_on_bottom;
    p.pass_focus_click = pref_pass_focus_click;
    p.cycle_behaviour = pref_default_cycle_behaviour;
    p.titlebar_color = pref_default_titlebar_color;
    p.titlebar_focused_color = pref_default_titlebar_focused_color;
    p.titlebar_text_color = pref_default_titlebar_text_color;
    p.titlebar_text_focused_color = pref_default_titlebar_text_focused_color;

    prefs_apply_internal(client, contexts, &p);

    if (client->state == WithdrawnState) {
        if (client->workspace_set <= UserSet) {
            client->workspace = p.workspace;
            client->workspace_set = UserSet;
        }
    }
    debug(("client '%s' (%s,%s) %s a titlebar\n",
           client->name, client->class, client->instance,
           p.titlebar ? "HAS" : "DOES NOT HAVE"));
    if (client->has_titlebar_set <= UserSet) {
        if (p.titlebar == True) {
            if (!client->has_titlebar) {
                client->has_titlebar = 1;
                if (client->frame != None) {
                    client_add_titlebar(client);
                    client->has_titlebar_set = UserSet;
                }
            }
        } else {
            if (client->has_titlebar) {
                client->has_titlebar = 0;
                if (client->frame != None) {
                    client_remove_titlebar(client);
                    client->has_titlebar_set = UserSet;
                }
            }
        }
    }
    if (client->cycle_behaviour_set <= UserSet) {
        switch (p.cycle_behaviour) {
            case TYPE_SKIP_CYCLE:
                client->cycle_behaviour = SkipCycle;
                client->cycle_behaviour_set = UserSet;
                break;
            case TYPE_RAISE_IMMEDIATELY:
                client->cycle_behaviour = RaiseImmediately;
                client->cycle_behaviour_set = UserSet;
                break;
            case TYPE_RAISE_ON_CYCLE_FINISH:
                client->cycle_behaviour = RaiseOnCycleFinish;
                client->cycle_behaviour_set = UserSet;
                break;
            case TYPE_DONT_RAISE:
                client->cycle_behaviour = DontRaise;
                client->cycle_behaviour_set = UserSet;
                break;
        }
    }
    if (client->omnipresent_set <= UserSet) {
        if (p.omnipresent) {
            client->omnipresent = 1;
            client->omnipresent_set = UserSet;
        } else {
            client->omnipresent = 0;
            client->omnipresent_set = UserSet;
        }
    }
    if (client->focus_policy_set <= UserSet) {
        switch (p.focus_policy) {
            case TYPE_SLOPPY_FOCUS:
                if (client->focus_policy == ClickToFocus) {
                    focus_policy_from_click(client);
                }
                client->focus_policy = SloppyFocus;
                client->focus_policy_set = UserSet;
                break;
            case TYPE_CLICK_TO_FOCUS:
                if (client->focus_policy != ClickToFocus) {
                    focus_policy_to_click(client);
                }
                client->focus_policy = ClickToFocus;
                client->focus_policy_set = UserSet;
                break;
            case TYPE_DONT_FOCUS:
                if (client->focus_policy == ClickToFocus) {
                    focus_policy_from_click(client);
                }
                client->focus_policy = DontFocus;
                client->focus_policy_set = UserSet;
                break;
        }
    }
    if (client->always_on_top_set <= UserSet) {
        if (p.always_on_top) {
            if (client->always_on_top == 0) {
                client->always_on_top = 1;
                client->always_on_top_set = UserSet;
                /* moves to top of always-on-top windows: */
                stacking_remove(client);
                stacking_add(client);
            }
        } else {
            if (client->always_on_top == 1) {
                client->always_on_top = 0;
                client->always_on_top_set = UserSet;
                /* moves to top of not always-on-top windows: */
                stacking_remove(client);
                stacking_add(client);
            }
        }
    }
    if (client->always_on_bottom_set <= UserSet) {
        if (p.always_on_bottom) {
            if (client->always_on_bottom == 0) {
                client->always_on_bottom = 1;
                client->always_on_bottom_set = UserSet;
                /* moves to top of always-on-bottom windows: */
                stacking_remove(client);
                stacking_add(client);
            }
        } else {
            if (client->always_on_bottom == 1) {
                client->always_on_bottom = 0;
                client->always_on_bottom_set = UserSet;
                /* moves to top of not always-on-bottom windows: */
                stacking_remove(client);
                stacking_add(client);
            }
        }
    }
    if (client->pass_focus_click <= UserSet) {
        if (p.pass_focus_click) {
            client->pass_focus_click = 1;
            client->pass_focus_click_set = UserSet;
        } else {
            client->pass_focus_click = 0;
            client->pass_focus_click_set = UserSet;
        }
    }

    paint_calculate_colors(client, p.titlebar_color,
                           p.titlebar_focused_color,
                           p.titlebar_text_color,
                           p.titlebar_text_focused_color);
    
    /* ADDOPT 10 */
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
        } else if (client->omnipresent == 1) {
            retval = True;
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
        /* ADDOPT 8 */
        case DISPLAYTITLEBAR:
            get_bool(opt->option_value, &p->titlebar);
            break;
        case OMNIPRESENT:
            get_bool(opt->option_value, &p->omnipresent);
            break;
        case DEFAULTWORKSPACE:
            if (client->state == WithdrawnState) {
                get_int(opt->option_value, &p->workspace);
            }
            break;
        case FOCUSPOLICY:
            get_int(opt->option_value, &p->focus_policy);
            break;
        case CYCLEBEHAVIOUR:
            get_int(opt->option_value, &p->cycle_behaviour);
            break;
        case ALWAYSONTOP:
            get_bool(opt->option_value, &p->always_on_top);
            break;
        case ALWAYSONBOTTOM:
            get_bool(opt->option_value, &p->always_on_bottom);
            break;
        case PASSFOCUSCLICK:
            get_bool(opt->option_value, &p->pass_focus_click);
            break;
        case COLORTITLEBAR:
            get_string(opt->option_value, &p->titlebar_color);
            break;
        case COLORTITLEBARFOCUSED:
            get_string(opt->option_value, &p->titlebar_focused_color);
            break;
        case COLORTEXT:
            get_string(opt->option_value, &p->titlebar_text_color);
            break;
        case COLORTEXTFOCUSED:
            get_string(opt->option_value, &p->titlebar_text_focused_color);
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
/* 9  */    keyboard_ignore,
/* 10 */    keyboard_quote,
/* 11 */    move_client,
/* 12 */    resize_client,
/* 13 */    NULL, /* non-interactive move/resize, must implement */
/* 14 */    xwm_quit,
/* 15 */    NULL, /* beep */
/* 16 */    NULL, /* invoke composed function */
/* 17 */    NULL, /* expansion for menu system */
/* 18 */    NULL, /* refresh/reset */
};

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
