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

#define CHECK_BOOL(x) ((x)->type_type == BOOLEAN ? True : False)
#define CHECK_STRING(x) ((x)->type_type == STRING ? True : False)
#define CHECK_INT(x) ((x)->type_type == INTEGER ? True : False)

/* FIXMENOW: need is_shaped as attribute of client_t */

typedef struct _prefs {
    Bool titlebar;
    int workspace;
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

static line *defaults, *contexts;
int pref_no_workspaces = 7;
Bool pref_display_titlebar = True;
int pref_default_workspace = 0;

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
    yyparse();

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
            case OPTION:
                switch (lp->line_value.option->option_name) {
                    case TITLEBAR:
                        get_bool(lp->line_value.option->option_value,
                                 &pref_display_titlebar);
                        break;
                    case DEFAULTWORKSPACE:
                        get_int(lp->line_value.option->option_value,
                                &pref_default_workspace);
                        break;
                    case NUMBEROFWORKSPACES:
                        get_int(lp->line_value.option->option_value,
                                &pref_no_workspaces);
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
                fprintf(stderr, "XWM: error\n");
            }
            break;
        case DEFAULTWORKSPACE:
            if ( (retval = CHECK_INT(opt->option_value)) == False) {
                fprintf(stderr, "XWM: error\n");
            }
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

    printf("prefs_apply\n");
    p.titlebar = pref_display_titlebar;
    p.workspace = pref_default_workspace;

    prefs_apply_internal(client, contexts, &p);

    if (client->state == WithdrawnState) {
        client->workspace = p.workspace;
    }
    printf("client '%s' (%s,%s) %s a titlebar\n",
           client->name, client->class, client->instance,
           p.titlebar ? "HAS" : "DOES NOT HAVE");
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
    if (typ->type_type == STRING) {
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
        /* FIXMENOW */
        retval = client->is_shaped == (type_bool == True ? 1 : 0);
    } else if (cntxt->context_selector & SEL_INWORKSPACE) {
        get_int(cntxt->context_value, &type_int);
        retval = client->workspace == (unsigned)type_int;
    } else if (cntxt->context_selector & SEL_WINDOWNAME) {
        get_string(cntxt->context_value, &type_string);
        retval = !(strcmp(client->name, type_string));
    } else if (cntxt->context_selector & SEL_WINDOWCLASS) {
        if (client->class == NULL) {
            retval = False;
        } else {
            get_string(cntxt->context_value, &type_string);
            retval = !(strcmp(client->class, type_string));
            printf("Using window class (%s, %s), returning %d\n",
                   client->class, type_string, retval);
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
        case DEFAULTWORKSPACE:
            if (client->state == WithdrawnState) {
                get_int(opt->option_value, &p->workspace);
            }
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

