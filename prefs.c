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

#include "default-xwmrc.h"

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
} prefs;

/* ADDOPT 6: set default value */
static prefs defaults = {
    True, UnSet,                /* titlebar */
    False, UnSet,               /* omnipresent */
    0, UnSet,                   /* workspace */
    TYPE_SLOPPY_FOCUS, UnSet,   /* focus_policy */
    False, UnSet,               /* always_on_top */
    False, UnSet,               /* always_on_bottom */
    True, UnSet,                /* pass_focus_click */
    TYPE_RAISE_IMMEDIATELY, UnSet, /* cycle_behaviour */
    NULL, UnSet,                /* titlebar_color */
    NULL, UnSet,                /* titlebar_focused_color */
    NULL, UnSet,                /* titlebar_text_color */
    NULL, UnSet,                /* titlebar_text_focused_color */
};

static line *contexts;

static void set_default(option *opt);
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
static int no_config(char *xwmrc_path);

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
                set_default(lp->line_value.option);
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

static void set_default(option *opt)
{
    int i;
    
    /* ADDOPT 7: set default if in global context */
    switch (opt->option_name) {
        case NWORKSPACES:
            /* special case: option only applies globally */
            get_int(opt->option_value, &i);
            if (i < 1) {
                fprintf(stderr,
                        "XWM: NumberOfWorkspaces must be at least one\n");
            } else {
                nworkspaces = i;
            }
            break;
        case DISPLAYTITLEBAR:
            get_bool(opt->option_value, &defaults.titlebar);
            defaults.titlebar_set = opt->option_setting;
            break;
        case OMNIPRESENT:
            get_bool(opt->option_value, &defaults.omnipresent);
            defaults.omnipresent_set = opt->option_setting;
            break;
        case DEFAULTWORKSPACE:
            get_int(opt->option_value, &defaults.workspace);
            defaults.workspace_set = opt->option_setting;
            break;
        case FOCUSPOLICY:
            get_int(opt->option_value, &defaults.focus_policy);
            defaults.focus_policy_set = opt->option_setting;
            break;
        case ALWAYSONTOP:
            get_bool(opt->option_value, &defaults.always_on_top);
            defaults.always_on_top_set = opt->option_setting;
            break;
        case ALWAYSONBOTTOM:
            get_bool(opt->option_value, &defaults.always_on_bottom);
            defaults.always_on_bottom_set = opt->option_setting;
            break;
        case PASSFOCUSCLICK:
            get_bool(opt->option_value, &defaults.pass_focus_click);
            defaults.pass_focus_click_set = opt->option_setting;
            break;
        case CYCLEBEHAVIOUR:
            get_int(opt->option_value, &defaults.cycle_behaviour);
            defaults.cycle_behaviour_set = opt->option_setting;
            break;
        case COLORTITLEBAR:
            get_string(opt->option_value, &defaults.titlebar_color);
            defaults.titlebar_color_set = opt->option_setting;
            break;
        case COLORTITLEBARFOCUSED:
            get_string(opt->option_value, &defaults.titlebar_focused_color);
            defaults.titlebar_focused_color_set = opt->option_setting;
            break;
        case COLORTEXT:
            get_string(opt->option_value, &defaults.titlebar_text_color);
            defaults.titlebar_text_color_set = opt->option_setting;
            break;
        case COLORTEXTFOCUSED:
            get_string(opt->option_value,
                       &defaults.titlebar_text_focused_color);
            defaults.titlebar_text_focused_color_set = opt->option_setting;
            break;
    }
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
        /* ADDOPT 8: define option's type */
        case NWORKSPACES:
            if ( (retval = CHECK_INT(opt->option_value)) == False) {
                fprintf(stderr,
                        "XWM: NumberOfWorkspaces not given integer value\n");
            }
            break;
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

    memcpy(&p, &defaults, sizeof(prefs));

    prefs_apply_internal(client, contexts, &p);

    if (client->state == WithdrawnState) {
        if (client->workspace_set <= p.workspace_set) {
            client->workspace = p.workspace;
            client->workspace_set = p.workspace_set;
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
                client->cycle_behaviour_set = p.cycle_behaviour_set;
                break;
            case TYPE_RAISE_IMMEDIATELY:
                client->cycle_behaviour = RaiseImmediately;
                client->cycle_behaviour_set = p.cycle_behaviour_set;
                break;
            case TYPE_RAISE_ON_CYCLE_FINISH:
                client->cycle_behaviour = RaiseOnCycleFinish;
                client->cycle_behaviour_set = p.cycle_behaviour_set;
                break;
            case TYPE_DONT_RAISE:
                client->cycle_behaviour = DontRaise;
                client->cycle_behaviour_set = p.cycle_behaviour_set;
                break;
        }
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
                /* moves to top of always-on-top windows: */
                stacking_remove(client);
                stacking_add(client);
            }
        } else {
            if (client->always_on_top == 1) {
                client->always_on_top = 0;
                client->always_on_top_set = p.always_on_top_set;
                /* moves to top of not always-on-top windows: */
                stacking_remove(client);
                stacking_add(client);
            }
        }
    }
    if (client->always_on_bottom_set <= p.always_on_bottom_set) {
        if (p.always_on_bottom) {
            if (client->always_on_bottom == 0) {
                client->always_on_bottom = 1;
                client->always_on_bottom_set = p.always_on_bottom_set;
                /* moves to top of always-on-bottom windows: */
                stacking_remove(client);
                stacking_add(client);
            }
        } else {
            if (client->always_on_bottom == 1) {
                client->always_on_bottom = 0;
                client->always_on_bottom_set = p.always_on_bottom_set;
                /* moves to top of not always-on-bottom windows: */
                stacking_remove(client);
                stacking_add(client);
            }
        }
    }
    if (client->pass_focus_click <= p.pass_focus_click_set) {
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

    /* ADDOPT 10: apply the option to the client */
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
        /* ADDOPT 9: set option if found within a context */
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
/* 0  */    workspace_client_moveto_bindable,
/* 1  */    workspace_goto_bindable,
/* 2  */    focus_cycle_next,
/* 3  */    focus_cycle_prev,
/* 4  */    kill_nicely,
/* 5  */    kill_with_extreme_prejudice,
/* 6  */    run_program,
/* 7  */    NULL, /* focus function, only for mouse binding */
/* 8  */    resize_maximize,
/* 9  */    resize_maximize_vertically,
/* 10 */    resize_maximize_horizontally,
/* 11 */    keyboard_ignore,
/* 12 */    keyboard_quote,
/* 13 */    move_client,
/* 14 */    resize_client,
/* 15 */    NULL, /* non-interactive move/resize, must implement */
/* 16 */    xwm_quit,
/* 17 */    NULL, /* beep */
/* 18 */    NULL, /* invoke composed function */
/* 19 */    NULL, /* expansion for menu system */
/* 20 */    NULL, /* refresh/reset */
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

/*
 * When the user has no .xwmrc, we try to create one and pop up an
 * explanatory message.
 */

static int no_config(char *xwmrc_path)
{
    extern FILE *yyin;
    int i;

    fprintf(stderr, "XWM: Creating default configuration file\n");
    yyin = fopen(xwmrc_path, "r+b");
    if (yyin == NULL) {
        fprintf(stderr,
                "XWM: Could not create default configuration file: %s\n",
                strerror(errno));
        return 0;
    }
    for (i = 0; i < sizeof(default_xwmrc); i++) {
        fprintf(yyin, "%s\n", default_xwmrc[i]);
    }
    fseek(yyin, 0, SEEK_SET);
    return 1;
}
