/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "config.h"

#include <X11/Xlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "prefs.h"
#include "parser.h"

#define CHECK_BOOL(x) ((x)->type_type == BOOLEAN ? True : False)
#define CHECK_STRING(x) ((x)->type_type == STRING ? True : False)
#define CHECK_INT(x) ((x)->type_type == INTEGER ? True : False)

/* FIXMENOW: need is_shaped as attribute of client_t */

/*
 * Need to define two types of bindings:  global and local.  Global
 * bindings are applied to all clients.  Each client may also have a
 * set of local bindings, which can override the global keybindings.
 * 
 * Global option and binding lines are kept in an array; global
 * contexts are kept in another array.  Menus and functions are
 * independent of context and are filtered out after parsing.
 * 
 * Need to create default preferences if none specified; create some
 * bindings, nothing more.
 */

static Bool get_int(type *typ, int *val);
static Bool get_string(type *typ, char **val);
static Bool get_bool(type *typ, Bool *val);
static Bool context_applies(client_t *client, context *cntxt);
static void option_apply(client_t *client, option *opt);
static void type_check(line *block);
static Bool type_check_context(context *cntxt);
static Bool type_check_option(option *opt);
static Bool type_check_keybinding(keybinding *kb);
static Bool type_check_mousebinding(mousebinding *mb);
static Bool type_check_keyunbinding(keybinding *kb);
static Bool type_check_mouseunbinding(mousebinding *mb);
static Bool type_check_function(function *fn);

int pref_no_workspaces = 7;
Bool pref_display_titlebar = True;
int pref_default_workspace = 0;

void prefs_init()
{
    char buf[PATH_MAX];
    char *home;
    line *lp, *prev, *first_context;

    home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "XWM: Could not get home directory; "
                "configuration file not found.\n");
        return;
    }
    snprintf(buf, "%s/.xwmrc", home);
    yyin = fopen(buf, "r");
    if (yyin == NULL) {
        fprintf(stderr, "XWM: Could not open configuration file '%s': %s\n",
                buf, strerror(errno));
        return;
    }
    yyparse();

    preferences = type_check(preferences);

    ncontexts = 0;
    for (lp = preferences; lp != NULL; lp = lp->line_next) {
        switch (lp->line_type) {
            case CONTEXT:
                break;
            case OPTION:
                break;
            case KEYBINDING:
                break;
            case KEYUNBINDING:
                break;
            case MOUSEBINDING:
                break;
            case MOUSEUNBINDING:
                break;
        }
    }

    if (ncontexts != 0) {
        first_context = prev = NULL;
        for (lp = preferences; lp != NULL; lp = lp->line_next) {
            if (lp->line_type == CONTEXT) {
                if (first_context == NULL) {
                    first_context = lp;
                }
                if (prev != NULL) {
                    prev->line_next = lp;
                }
                prev = lp;
            }
            /* FIXME:  should free top-level non-contexts */
        }
        preferences = first_context;
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
        case INWORKSPACE:
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

static Bool type_check_keyunbinding(keybinding *kb)
{
    return True;
}

static Bool type_check_mouseunbinding(mousebinding *mb)
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
            if (CHECK_INT(fn->function_args) == False) {
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
            if (CHECK_STRING(fn->function_args) == False) {
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
            if (CHECK_STRING(fn->function_args) == True) {
                return True;
            }
            break;
        default:
            return False;
    }
}

void prefs_apply_internal(client_t *client, line *block)
{
    line *lp;

    for (lp = block; lp != NULL; lp = lp->line_next) {
        switch (lp->line_type) {
            case CONTEXT:
                if (context_applies(client, lp->line_value.context)) {
                    prefs_apply(client, lp->line_value.context->context_lines);
                }
                break;
            case OPTION:
                option_apply(client, lp->line_value.option);
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
        *val = (int)typ->type_value;
    }
}

static void get_string(type *typ, char **val)
{
    *val = NULL;
    if (typ->type_type == STRING) {
        *val = (char *)typ->type_value;
    }
}

static void get_bool(type *typ, Bool *val)
{
    *val = False;
    if (typ->type_type == BOOLEAN) {
        *val = (Bool)typ->type_value;
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
        retval = client->is_shaped == type_bool;
    } else if (cntxt->context_selector & SEL_INWORKSPACE) {
        get_int(cntxt->context_value, &type_int);
        retval = client->workspace == (unsigned)type_int;
    } else if (cntxt->context_selector & SEL_WINDOWNAME) {
        get_string(cntxt->context_value, &type_string);
        retval = !(strcmp(client->name, type_string));
    } else if (cntxt->context_selector & SEL_WINDOWCLASS) {
        get_string(cntxt->context_value, &type_string);
        retval = !(strcmp(client->class, type_string));
    } else if (cntxt->context_selector & SEL_WINDOWINSTANCE) {
        get_string(cntxt->context_value, &type_string);
        retval = !(strcmp(client->instance, type_string));
    }
    
    if (cntxt->context_selector & SEL_NOT) return !retval;
    else return retval;
}

/*
 * Applies given option to given client.
 */

static void option_apply(client_t *client, option *opt)
{
    int type_int;
    Bool type_bool;
    
    switch (opt->option_name) {
        case DISPLAYTITLEBAR:
            get_bool(opt->option_value, &type_bool);
            if (type_bool == True) {
                if (!client->has_titlebar) {
                    client_add_titlebar(client);
                }
            } else {
                if (client->has_titlebar) {
                    client_remove_titlebar(client);
                }
            }
            break;
        case DEFAULTWORKSPACE:
            if (client->state == Withdrawn) {
                get_int(opt->option_value, &type_int);
                client->workspace = type_int;
            }
            break;
    }
}


